#ifndef BTREE_H
#define BTREE_H

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include "node.h"
#include "diskmanager.h"

/* ----------------------------------------------------------------------------
* Btree: Implementação de uma árvore B.
* ------------------------------------------------------------------------- */
template <int M>
class BTree {
private:
    std::string filename;
    // fstream: classe para manipulação de arquivos, permite leitura e escrita.
    std::fstream file;
    int rootID; 
    DiskManager disk;

    void writeNode(int id, BTreeNode<M>& node);
    void readNode(int id, BTreeNode<M>& node);
    int allocateNode();
    void split(std::vector<int>& path);
    int minKeys() const;
    bool isLeaf(const BTreeNode<M>& node) const;
    void removeFromLeaf(BTreeNode<M>& node, int keyIndex);
    int getPredecessorKey(int nodeID);
    int getSuccessorKey(int nodeID);
    void borrowFromLeft(BTreeNode<M>& parent, int parentID, int childIndex);
    void borrowFromRight(BTreeNode<M>& parent, int parentID, int childIndex);
    void mergeChildren(BTreeNode<M>& parent, int parentID, int leftChildIndex);
    bool removeRecursive(int nodeID, int key);

public:
    BTree(const std::string& fname);
    ~BTree();

    void insert(int key);
    void remove(int key);
    bool search(int key);
    void loadFromFile(const std::string& path);
    DiskManager& getDiskManager() { return disk; }

    // Acessores de leitura para o módulo de visualização (src/display/).
    // A BTree não imprime nada; apenas expõe a estrutura para percorrê-la.
    int getRoot() const { return rootID; }
    BTreeNode<M> getNode(int id) { BTreeNode<M> n; readNode(id, n); return n; }
};

/* ----------------------------------------------------------------------------
* Constructor
* - Se o arquivo não existir, cria um novo arquivo vazio.
* - Se o arquivo existir, lê o registro 0 para carregar o ID da raiz.
* ------------------------------------------------------------------------- */
template <int M>
BTree<M>::BTree(const std::string& fname) : filename(fname), disk(fname) {
    
    // Manda abrir o arquivo para 
    file.open(filename, std::ios::in | std::ios::out | std::ios::binary);


    if (!file.is_open()) {
        file.clear();
        file.open(filename, std::ios::out | std::ios::binary);
        file.close();
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

        BTreeNode<M> headerNode; 
        headerNode.A[0] = 0;
        writeNode(0, headerNode);

        rootID = 0; 
    } else {
        BTreeNode<M> headerNode;
        readNode(0, headerNode); 
        rootID = headerNode.A[0];
    }
}

template <int M>
BTree<M>::~BTree() {
    if (file.is_open()) {
        file.close();
    }
}

template <int M>
void BTree<M>::writeNode(int id, BTreeNode<M>& node) {
    file.clear();
    // seekp: manipula o ponteiro de escrita para começo da informação no local do nó de id 'id'.
    file.seekp(id * sizeof(BTreeNode<M>), std::ios::beg);
    // write: escreve o nó no arquivo a partir do endereço de memória do node
    file.write((const char*)(&node), sizeof(BTreeNode<M>));
    file.flush();
    disk.incrementWrite(); 
}

template <int M>
void BTree<M>::readNode(int id, BTreeNode<M>& node) {
    file.clear();
    // seekg: manipula o ponteiro de leitura do arquivo,
    // move o ponteiro para o id * sizeof(BTreeNode<M>) bytes a partir do início (ios::beg) do arquivo.
    file.seekg(id * sizeof(BTreeNode<M>), std::ios::beg);
    // le o btrenode do arquivo que se encontra no endereço de memória do node "onde os dados lidos serão guardados"
    // (char* -byte puro)(&node)
    file.read((char*)(&node), sizeof(BTreeNode<M>));
    disk.incrementRead(); 
}

template <int M>
int BTree<M>::allocateNode() {
    file.clear();
    // índice do último registro (adicionar no próximo registro disponível)
    // Reaproveitamento de nós
    // seekp : manipulação de arquivos, move o ponteiro de escrita 0 bytes 
    // a partir do final (ios::end) do arquivo.
    file.seekp(0, std::ios::end);

    // Retorna inteiro so tipo streampos - posição atual do ponteiro de escrita (end position)
    long endPos = file.tellp();


    return (int)(endPos / sizeof(BTreeNode<M>));
}

template <int M>
bool BTree<M>::search(int key) {
    int currentID = rootID; 

    while (currentID != 0) {
        BTreeNode<M> currentNode;
        readNode(currentID, currentNode); 

        int i = currentNode.findIndex(key);

        if (i > 0 && currentNode.K[i] == key) {
            return true; 
        }

        if (currentNode.A[0] == 0) {
            return false;
        }

        currentID = currentNode.A[i];
    }
    return false;
}

template <int M>
void BTree<M>::insert(int key) {
    // Adicionar comentários de compreensão do código
    if (rootID == 0) {
        rootID = allocateNode();
        
        BTreeNode<M> root;
        root.numKeys = 1;
        root.K[1] = key; 
        
        writeNode(rootID, root);

        BTreeNode<M> headerNode;
        readNode(0, headerNode);
        headerNode.A[0] = rootID;
        writeNode(0, headerNode);
        
        return; 
    }

    std::vector<int> path; 
    int currentID = rootID;
    BTreeNode<M> currentNode;

    while (currentID != 0) {
        readNode(currentID, currentNode);
        
        int i = currentNode.findIndex(key);

        if (i > 0 && currentNode.K[i] == key) {
            return;
        }

        path.push_back(currentID); 

        if (currentNode.A[0] == 0) {
            break;
        }

        currentID = currentNode.A[i]; 
    }

    int i = currentNode.numKeys;
    while (i > 0 && currentNode.K[i] > key) {
        currentNode.K[i + 1] = currentNode.K[i];
        currentNode.A[i + 1] = currentNode.A[i];
        i--;
    }

    currentNode.K[i + 1] = key;
    currentNode.A[i + 1] = 0; 
    currentNode.numKeys++;

    writeNode(currentID, currentNode); 

    if (currentNode.numKeys == M) {
        split(path);
    }
}

template <int M>
void BTree<M>::split(std::vector<int>& path) {
    // Adicionar comentários de compreensão do código
    // Lógica de particionamento do nó cheio
    // Encontra o meio do nó
    int mid = (M + 1) / 2;          // para M=3: mid=2 → K[2] é a mediana

    // Nó que transbordou é o último do path
    int leftID = path.back();
    // ADICIONAR COBERTURA PARA GARANTIR PATH NÃO VAZIO ANTES DE ACESSAR BACK
    path.pop_back();

    BTreeNode<M> leftNode;
    readNode(leftID, leftNode);

    // Cria o nó irmão direito
    int rightID = allocateNode();
    BTreeNode<M> rightNode;

    int promotedKey = leftNode.K[mid];

    // Preenche o irmão direito: K[mid+1..M] e A[mid..M]
    rightNode.A[0] = leftNode.A[mid];
    rightNode.numKeys = M - mid;
    for (int k = 1; k <= M - mid; k++) {
        rightNode.K[k] = leftNode.K[mid + k];
        rightNode.A[k] = leftNode.A[mid + k];
    }

    // Trunca o nó esquerdo: mantém K[1..mid-1] e A[0..mid-1]
    leftNode.numKeys = mid - 1;
    for (int k = mid; k <= M; k++) {
        leftNode.K[k] = 0;
        leftNode.A[k] = 0;
    }

    writeNode(leftID, leftNode);
    writeNode(rightID, rightNode);

    if (path.empty()) {
        // A raiz transbordou — cria uma nova raiz
        int newRootID = allocateNode();
        BTreeNode<M> newRoot;
        newRoot.numKeys = 1;
        newRoot.K[1] = promotedKey;
        newRoot.A[0] = leftID;
        newRoot.A[1] = rightID;
        writeNode(newRootID, newRoot);

        BTreeNode<M> header;
        readNode(0, header);
        header.A[0] = newRootID;
        writeNode(0, header);
        rootID = newRootID;

    } else {
        // Insere a mediana promovida no pai
        int parentID = path.back();
        BTreeNode<M> parentNode;
        readNode(parentID, parentNode);

        int i = parentNode.numKeys;
        while (i > 0 && parentNode.K[i] > promotedKey) {
            parentNode.K[i + 1] = parentNode.K[i];
            parentNode.A[i + 1] = parentNode.A[i];
            i--;
        }
        parentNode.K[i + 1] = promotedKey;
        parentNode.A[i + 1] = rightID;   // filho direito da mediana
        parentNode.numKeys++;

        writeNode(parentID, parentNode);

        if (parentNode.numKeys == M) {
            split(path);                  // propaga overflow para cima
        }
    }
}

template <int M>
int BTree<M>::minKeys() const {
    return ((M + 1) / 2) - 1;
}

template <int M>
bool BTree<M>::isLeaf(const BTreeNode<M>& node) const {
    return node.A[0] == 0;
}

template <int M>
void BTree<M>::removeFromLeaf(BTreeNode<M>& node, int keyIndex) {
    for (int i = keyIndex; i < node.numKeys; i++) {
        node.K[i] = node.K[i + 1];
        node.A[i] = node.A[i + 1];
    }
    node.K[node.numKeys] = 0;
    node.A[node.numKeys] = 0;
    node.numKeys--;
}

template <int M>
int BTree<M>::getPredecessorKey(int nodeID) {
    int currentID = nodeID;
    BTreeNode<M> node;

    while (true) {
        readNode(currentID, node);
        if (isLeaf(node)) {
            return node.K[node.numKeys];
        }
        currentID = node.A[node.numKeys];
    }
}

template <int M>
int BTree<M>::getSuccessorKey(int nodeID) {
    int currentID = nodeID;
    BTreeNode<M> node;

    while (true) {
        readNode(currentID, node);
        if (isLeaf(node)) {
            return node.K[1];
        }
        currentID = node.A[0];
    }
}

template <int M>
void BTree<M>::borrowFromLeft(BTreeNode<M>& parent, int parentID, int childIndex) {
    int childID = parent.A[childIndex];
    int leftID = parent.A[childIndex - 1];

    BTreeNode<M> child;
    BTreeNode<M> left;
    readNode(childID, child);
    readNode(leftID, left);

    for (int i = child.numKeys; i >= 1; i--) {
        child.K[i + 1] = child.K[i];
    }
    for (int i = child.numKeys; i >= 0; i--) {
        child.A[i + 1] = child.A[i];
    }

    child.K[1] = parent.K[childIndex];
    child.A[0] = left.A[left.numKeys];
    child.numKeys++;

    parent.K[childIndex] = left.K[left.numKeys];
    left.K[left.numKeys] = 0;
    left.A[left.numKeys] = 0;
    left.numKeys--;

    writeNode(leftID, left);
    writeNode(childID, child);
    writeNode(parentID, parent);
}

template <int M>
void BTree<M>::borrowFromRight(BTreeNode<M>& parent, int parentID, int childIndex) {
    int childID = parent.A[childIndex];
    int rightID = parent.A[childIndex + 1];

    BTreeNode<M> child;
    BTreeNode<M> right;
    readNode(childID, child);
    readNode(rightID, right);

    child.numKeys++;
    child.K[child.numKeys] = parent.K[childIndex + 1];
    child.A[child.numKeys] = right.A[0];

    parent.K[childIndex + 1] = right.K[1];

    for (int i = 1; i < right.numKeys; i++) {
        right.K[i] = right.K[i + 1];
    }
    for (int i = 0; i < right.numKeys; i++) {
        right.A[i] = right.A[i + 1];
    }

    right.K[right.numKeys] = 0;
    right.A[right.numKeys] = 0;
    right.numKeys--;

    writeNode(rightID, right);
    writeNode(childID, child);
    writeNode(parentID, parent);
}

template <int M>
void BTree<M>::mergeChildren(BTreeNode<M>& parent, int parentID, int leftChildIndex) {
    int leftID = parent.A[leftChildIndex];
    int rightID = parent.A[leftChildIndex + 1];

    BTreeNode<M> left;
    BTreeNode<M> right;
    readNode(leftID, left);
    readNode(rightID, right);

    left.numKeys++;
    left.K[left.numKeys] = parent.K[leftChildIndex + 1];
    left.A[left.numKeys] = right.A[0];

    for (int i = 1; i <= right.numKeys; i++) {
        left.numKeys++;
        left.K[left.numKeys] = right.K[i];
        left.A[left.numKeys] = right.A[i];
    }

    for (int i = leftChildIndex + 1; i < parent.numKeys; i++) {
        parent.K[i] = parent.K[i + 1];
    }
    for (int i = leftChildIndex + 1; i < parent.numKeys; i++) {
        parent.A[i] = parent.A[i + 1];
    }

    parent.K[parent.numKeys] = 0;
    parent.A[parent.numKeys] = 0;
    parent.numKeys--;

    writeNode(leftID, left);
    writeNode(parentID, parent);
}

template <int M>
bool BTree<M>::removeRecursive(int nodeID, int key) {
    auto rebalanceChildIfNeeded = [&](int parentID, int childIndex) {
        BTreeNode<M> parent;
        readNode(parentID, parent);

        if (parent.A[0] == 0) {
            return;
        }

        if (childIndex > parent.numKeys) {
            childIndex = parent.numKeys;
        }

        int childID = parent.A[childIndex];
        BTreeNode<M> child;
        readNode(childID, child);

        if (child.numKeys >= minKeys()) {
            return;
        }

        bool fixed = false;

        if (childIndex > 0) {
            BTreeNode<M> leftSibling;
            readNode(parent.A[childIndex - 1], leftSibling);
            if (leftSibling.numKeys > minKeys()) {
                borrowFromLeft(parent, parentID, childIndex);
                fixed = true;
            }
        }

        if (!fixed && childIndex < parent.numKeys) {
            BTreeNode<M> rightSibling;
            readNode(parent.A[childIndex + 1], rightSibling);
            if (rightSibling.numKeys > minKeys()) {
                borrowFromRight(parent, parentID, childIndex);
                fixed = true;
            }
        }

        if (!fixed) {
            if (childIndex < parent.numKeys) {
                mergeChildren(parent, parentID, childIndex);
            } else if (childIndex > 0) {
                mergeChildren(parent, parentID, childIndex - 1);
            }
        }
    };

    BTreeNode<M> node;
    readNode(nodeID, node);

    int i = node.findIndex(key);
    bool found = (i > 0 && node.K[i] == key);

    if (found) {
        if (isLeaf(node)) {
            removeFromLeaf(node, i);
            writeNode(nodeID, node);
            return true;
        }

        int leftChildID = node.A[i - 1];
        int rightChildID = node.A[i];

        BTreeNode<M> leftChild;
        BTreeNode<M> rightChild;
        readNode(leftChildID, leftChild);
        readNode(rightChildID, rightChild);

        if (leftChild.numKeys > minKeys()) {
            int pred = getPredecessorKey(leftChildID);
            node.K[i] = pred;
            writeNode(nodeID, node);
            bool removed = removeRecursive(leftChildID, pred);
            if (removed) {
                rebalanceChildIfNeeded(nodeID, i - 1);
            }
            return removed;
        }

        if (rightChild.numKeys > minKeys()) {
            int succ = getSuccessorKey(rightChildID);
            node.K[i] = succ;
            writeNode(nodeID, node);
            bool removed = removeRecursive(rightChildID, succ);
            if (removed) {
                rebalanceChildIfNeeded(nodeID, i);
            }
            return removed;
        }

        mergeChildren(node, nodeID, i - 1);
        return removeRecursive(node.A[i - 1], key);
    }

    if (isLeaf(node)) {
        return false;
    }

    int childIndex = i;
    int childID = node.A[childIndex];
    bool removed = removeRecursive(childID, key);
    if (removed) {
        rebalanceChildIfNeeded(nodeID, childIndex);
    }

    return removed;
}

template <int M>
void BTree<M>::remove(int key) {
    if (rootID == 0) {
        return;
    }

    removeRecursive(rootID, key);

    if (rootID == 0) {
        return;
    }

    BTreeNode<M> root;
    readNode(rootID, root);

    if (root.numKeys == 0) {
        if (root.A[0] == 0) {
            rootID = 0;
        } else {
            rootID = root.A[0];
        }

        BTreeNode<M> header;
        readNode(0, header);
        header.A[0] = rootID;
        writeNode(0, header);
    }
}

/* ----------------------------------------------------------------------------
* loadFromFile
* Carrega uma árvore B já montada a partir de um arquivo de texto (mock).
* Útil para testes: descreve a árvore esperada de forma legível em vez de
* construí-la por sucessivas inserções.
*
* Formato (linhas em branco e linhas iniciadas por '#' são ignoradas):
*   root <id>
*   <id> <numKeys> <A[0]> <K[1]> <A[1]> ... <K[numKeys]> <A[numKeys]>
*
* O registro 0 é o cabeçalho: A[0] guarda o id da raiz (ver construtor).
* ------------------------------------------------------------------------- */
template <int M>
void BTree<M>::loadFromFile(const std::string& path) {
    std::ifstream in(path);

    int newRoot = 0;
    std::string line;
    while (std::getline(in, line)) {
        // Ignora linhas em branco e comentários. find_first_not_of localiza o
        // primeiro caractere não-branco; npos => linha só com espaços.
        std::size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos || line[start] == '#') continue;

        std::istringstream iss(line);
        std::string first;
        iss >> first;

        // "root <id>" define qual registro é a raiz da árvore.
        if (first == "root") {
            iss >> newRoot;
            continue;
        }

        // Caso contrário, é uma linha de registro: id, numKeys, A[0] e então
        // numKeys pares de (K[i], A[i]).
        int id = std::stoi(first);
        BTreeNode<M> node;  // zera campos via construtor
        iss >> node.numKeys >> node.A[0];
        for (int k = 1; k <= node.numKeys; k++) {
            iss >> node.K[k] >> node.A[k];
        }
        writeNode(id, node);
    }

    // Cabeçalho (registro 0): A[0] aponta para a raiz.
    BTreeNode<M> header;
    header.A[0] = newRoot;
    writeNode(0, header);
    rootID = newRoot;
}

#endif