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
* BTree: Implementação de uma árvore B.
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
    void freeNode(int id);
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
* Construtor
* - Se o arquivo não existir, cria um novo arquivo vazio.
* - Se o arquivo existir, lê o registro 0 para carregar o ID da raiz.
* ------------------------------------------------------------------------- */
template <int M>
BTree<M>::BTree(const std::string& fname) : filename(fname), disk(fname) {

    // Tenta abrir o arquivo existente para leitura e escrita (modo binário).
    file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

    if (!file.is_open()) {
        // Arquivo não existe: cria-o vazio e reabre para leitura/escrita.
        file.clear();
        file.open(filename, std::ios::out | std::ios::binary);
        file.close();
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

        // Registro 0 é o cabeçalho: A[0] = id da raiz, A[1] = topo da lixeira.
        BTreeNode<M> headerNode;
        headerNode.A[0] = 0;
        headerNode.A[1] = 0;
        writeNode(0, headerNode);

        rootID = 0;  // árvore vazia
    } else {
        // Arquivo já existe: lê o cabeçalho para recuperar o id da raiz.
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

// Escreve um nó no registro 'id' do arquivo (cada registro = 1 BTreeNode<M>).
template <int M>
void BTree<M>::writeNode(int id, BTreeNode<M>& node) {
    disk.startTimer();
    file.clear();
    // seekp: manipula o ponteiro de escrita para começo da informação no local do nó de id 'id'.
    file.seekp(id * sizeof(BTreeNode<M>), std::ios::beg);
    // write: escreve o nó no arquivo a partir do endereço de memória do node
    file.write((const char*)(&node), sizeof(BTreeNode<M>));
    file.flush();
    disk.stopTimer();
    disk.incrementWrite(); 
}

// Lê o nó do registro 'id' do arquivo para 'node'.
template <int M>
void BTree<M>::readNode(int id, BTreeNode<M>& node) {
    disk.startTimer();
    file.clear();
    // seekg: manipula o ponteiro de leitura do arquivo,
    // move o ponteiro para o id * sizeof(BTreeNode<M>) bytes a partir do início (ios::beg) do arquivo.
    file.seekg(id * sizeof(BTreeNode<M>), std::ios::beg);
    // le o btrenode do arquivo que se encontra no endereço de memória do node "onde os dados lidos serão guardados"
    // (char* -byte puro)(&node)
    file.read((char*)(&node), sizeof(BTreeNode<M>));
    disk.stopTimer();
    disk.incrementRead(); 
}

// Reserva um id de registro para um novo nó. Reaproveita nós livres (lixeira)
// quando houver; caso contrário, cresce o arquivo no fim.
template <int M>
int BTree<M>::allocateNode() {
    BTreeNode<M> header;
    readNode(0, header);

    // Verifica se tem lixo para reciclar
    if (header.A[1] != 0) {
        int reusedID = header.A[1];
        
        BTreeNode<M> reusedNode;
        readNode(reusedID, reusedNode);

        // A lixeira agora aponta para o próximo lixo da fila
        header.A[1] = reusedNode.A[0]; 
        writeNode(0, header);

        return reusedID; // Retorna o ID reciclado
    }

    // Se a lixeira estiver vazia, faz o comportamento normal
    file.clear();
    file.seekp(0, std::ios::end);
    long endPos = file.tellp();
    return (int)(endPos / sizeof(BTreeNode<M>));
}

// Marca o nó 'id' como livre, empilhando-o na lixeira (lista encadeada pelo A[0]
// dos nós livres, com o topo guardado no A[1] do cabeçalho).
template <int M>
void BTree<M>::freeNode(int id) {
    BTreeNode<M> header;
    readNode(0, header);

    BTreeNode<M> deletedNode;
    // O nó apagado vai apontar para o antigo "topo" da lixeira
    deletedNode.A[0] = header.A[1]; 
    writeNode(id, deletedNode);

    // O cabeçalho agora aponta para este nó que acabou de ser apagado
    header.A[1] = id; 
    writeNode(0, header);
}

// Busca uma chave descendo da raiz até uma folha. Cada nível custa um readNode.
template <int M>
bool BTree<M>::search(int key) {
    int currentID = rootID;

    while (currentID != 0) {
        BTreeNode<M> currentNode;
        readNode(currentID, currentNode);

        // Acha onde a chave estaria neste nó.
        int i = currentNode.findIndex(key);

        // Chave encontrada neste nó.
        if (i > 0 && currentNode.K[i] == key) {
            return true;
        }

        // Nó folha (sem filhos) e não achou => a chave não existe.
        if (currentNode.A[0] == 0) {
            return false;
        }

        // Desce para o filho apropriado.
        currentID = currentNode.A[i];
    }
    return false;
}

// Insere uma chave. Sempre insere numa folha; se a folha estourar, chama split().
template <int M>
void BTree<M>::insert(int key) {
    // Árvore vazia: cria a raiz com a única chave e atualiza o cabeçalho.
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

    // path guarda o caminho da raiz até a folha (usado pelo split para subir).
    std::vector<int> path;
    int currentID = rootID;
    BTreeNode<M> currentNode;

    // Desce até a folha, registrando o caminho.
    while (currentID != 0) {
        readNode(currentID, currentNode);

        int i = currentNode.findIndex(key);

        // Chave já existe: insert é no-op (sem duplicatas).
        if (i > 0 && currentNode.K[i] == key) {
            return;
        }

        path.push_back(currentID);

        // Chegou numa folha: para de descer.
        if (currentNode.A[0] == 0) {
            break;
        }

        currentID = currentNode.A[i];
    }

    // Abre espaço na folha empurrando as chaves maiores uma posição à direita.
    int i = currentNode.numKeys;
    while (i > 0 && currentNode.K[i] > key) {
        currentNode.K[i + 1] = currentNode.K[i];
        currentNode.A[i + 1] = currentNode.A[i];
        i--;
    }

    // Insere a chave na posição encontrada (filho à direita = 0, é folha).
    currentNode.K[i + 1] = key;
    currentNode.A[i + 1] = 0;
    currentNode.numKeys++;

    writeNode(currentID, currentNode);

    // Estouro (numKeys == M): precisa dividir o nó.
    if (currentNode.numKeys == M) {
        split(path);
    }
}

template <int M>
void BTree<M>::split(std::vector<int>& path) {
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

// Mínimo de chaves que um nó (exceto a raiz) deve ter: ceil(M/2) - 1.
template <int M>
int BTree<M>::minKeys() const {
    return ((M + 1) / 2) - 1;
}

// Um nó é folha quando não tem o primeiro filho (A[0] == 0).
template <int M>
bool BTree<M>::isLeaf(const BTreeNode<M>& node) const {
    return node.A[0] == 0;
}

// Remove a chave na posição keyIndex de uma folha, deslocando as seguintes
// para a esquerda e limpando o último slot.
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

// Predecessor em ordem: maior chave da subárvore — desce sempre pelo filho mais
// à direita até chegar numa folha.
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

// Sucessor em ordem: menor chave da subárvore — desce sempre pelo filho mais à
// esquerda até chegar numa folha.
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

// Empréstimo do irmão da esquerda: rotação à direita pelo pai.
// A chave do pai desce para o filho e a maior chave do irmão sobe para o pai.
template <int M>
void BTree<M>::borrowFromLeft(BTreeNode<M>& parent, int parentID, int childIndex) {
    int childID = parent.A[childIndex];
    int leftID = parent.A[childIndex - 1];

    BTreeNode<M> child;
    BTreeNode<M> left;
    readNode(childID, child);
    readNode(leftID, left);

    // Abre espaço no início do filho (chave + ponteiro entram à esquerda).
    for (int i = child.numKeys; i >= 1; i--) {
        child.K[i + 1] = child.K[i];
    }
    for (int i = child.numKeys; i >= 0; i--) {
        child.A[i + 1] = child.A[i];
    }

    // Chave do pai desce para o filho; ponteiro mais à direita do irmão vem junto.
    child.K[1] = parent.K[childIndex];
    child.A[0] = left.A[left.numKeys];
    child.numKeys++;

    // Maior chave do irmão esquerdo sobe para o pai; remove-a do irmão.
    parent.K[childIndex] = left.K[left.numKeys];
    left.K[left.numKeys] = 0;
    left.A[left.numKeys] = 0;
    left.numKeys--;

    writeNode(leftID, left);
    writeNode(childID, child);
    writeNode(parentID, parent);
}

// Empréstimo do irmão da direita: rotação à esquerda pelo pai.
// A chave do pai desce para o filho e a menor chave do irmão sobe para o pai.
template <int M>
void BTree<M>::borrowFromRight(BTreeNode<M>& parent, int parentID, int childIndex) {
    int childID = parent.A[childIndex];
    int rightID = parent.A[childIndex + 1];

    BTreeNode<M> child;
    BTreeNode<M> right;
    readNode(childID, child);
    readNode(rightID, right);

    // Chave do pai desce para o fim do filho; primeiro ponteiro do irmão vem junto.
    child.numKeys++;
    child.K[child.numKeys] = parent.K[childIndex + 1];
    child.A[child.numKeys] = right.A[0];

    // Menor chave do irmão direito sobe para o pai.
    parent.K[childIndex + 1] = right.K[1];

    // Desloca as chaves/ponteiros restantes do irmão uma posição à esquerda.
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

// Funde dois filhos vizinhos num só: o filho esquerdo recebe a chave separadora
// do pai e todo o conteúdo do filho direito. O filho direito é liberado.
template <int M>
void BTree<M>::mergeChildren(BTreeNode<M>& parent, int parentID, int leftChildIndex) {
    int leftID = parent.A[leftChildIndex];
    int rightID = parent.A[leftChildIndex + 1];

    BTreeNode<M> left;
    BTreeNode<M> right;
    readNode(leftID, left);
    readNode(rightID, right);

    // Chave separadora do pai desce para o fim do filho esquerdo.
    left.numKeys++;
    left.K[left.numKeys] = parent.K[leftChildIndex + 1];
    left.A[left.numKeys] = right.A[0];

    // Copia todas as chaves/ponteiros do filho direito para o esquerdo.
    for (int i = 1; i <= right.numKeys; i++) {
        left.numKeys++;
        left.K[left.numKeys] = right.K[i];
        left.A[left.numKeys] = right.A[i];
    }

    // Remove do pai a chave separadora e o ponteiro para o filho direito.
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
    freeNode(rightID);  // devolve o nó direito à lixeira para reuso
}

// Remoção recursiva de uma chave a partir do nó nodeID. Retorna true se removeu.
template <int M>
bool BTree<M>::removeRecursive(int nodeID, int key) {
    // Após mexer num filho, garante que ele ainda tenha o mínimo de chaves.
    // Se ficou com chaves de menos (underflow), tenta emprestar de um irmão;
    // se nenhum pode emprestar, funde com um irmão.
    auto rebalanceChildIfNeeded = [&](int parentID, int childIndex) {
        BTreeNode<M> parent;
        readNode(parentID, parent);

        // Pai virou folha (sem filhos): nada a rebalancear.
        if (parent.A[0] == 0) {
            return;
        }

        // Após um merge o índice pode ter saído do intervalo válido.
        if (childIndex > parent.numKeys) {
            childIndex = parent.numKeys;
        }

        int childID = parent.A[childIndex];
        BTreeNode<M> child;
        readNode(childID, child);

        // Filho está dentro do mínimo: nada a fazer.
        if (child.numKeys >= minKeys()) {
            return;
        }

        bool fixed = false;

        // 1ª opção: emprestar do irmão esquerdo, se ele tiver chave sobrando.
        if (childIndex > 0) {
            BTreeNode<M> leftSibling;
            readNode(parent.A[childIndex - 1], leftSibling);
            if (leftSibling.numKeys > minKeys()) {
                borrowFromLeft(parent, parentID, childIndex);
                fixed = true;
            }
        }

        // 2ª opção: emprestar do irmão direito.
        if (!fixed && childIndex < parent.numKeys) {
            BTreeNode<M> rightSibling;
            readNode(parent.A[childIndex + 1], rightSibling);
            if (rightSibling.numKeys > minKeys()) {
                borrowFromRight(parent, parentID, childIndex);
                fixed = true;
            }
        }

        // 3ª opção: nenhum irmão pode emprestar => funde com um deles.
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

    // Procura a chave neste nó.
    int i = node.findIndex(key);
    bool found = (i > 0 && node.K[i] == key);

    if (found) {
        // Caso 1: a chave está numa folha — remove diretamente.
        if (isLeaf(node)) {
            removeFromLeaf(node, i);
            writeNode(nodeID, node);
            return true;
        }

        // Caso 2: chave em nó interno — substitui pelo predecessor/sucessor e
        // remove esse substituto recursivamente da subárvore correspondente.
        int leftChildID = node.A[i - 1];
        int rightChildID = node.A[i];

        BTreeNode<M> leftChild;
        BTreeNode<M> rightChild;
        readNode(leftChildID, leftChild);
        readNode(rightChildID, rightChild);

        // Filho esquerdo tem folga: usa o predecessor (maior chave à esquerda).
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

        // Senão, filho direito tem folga: usa o sucessor (menor chave à direita).
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

        // Ambos no mínimo: funde os dois filhos e remove a chave do nó fundido.
        mergeChildren(node, nodeID, i - 1);
        return removeRecursive(node.A[i - 1], key);
    }

    // Chave não está aqui e isto é folha => a chave não existe.
    if (isLeaf(node)) {
        return false;
    }

    // Desce para o filho onde a chave estaria e rebalanceia ao voltar.
    int childIndex = i;
    int childID = node.A[childIndex];
    bool removed = removeRecursive(childID, key);
    if (removed) {
        rebalanceChildIfNeeded(nodeID, childIndex);
    }

    return removed;
}

// Remove uma chave da árvore e, se necessário, ajusta a altura (raiz vazia).
template <int M>
void BTree<M>::remove(int key) {
    // Árvore vazia: nada a remover.
    if (rootID == 0) {
        return;
    }

    removeRecursive(rootID, key);

    if (rootID == 0) {
        return;
    }

    BTreeNode<M> root;
    readNode(rootID, root);

    // Raiz ficou sem chaves após a remoção: a árvore encolhe um nível.
    if (root.numKeys == 0) {
        int oldRootID = rootID;

        // Se a raiz era folha, a árvore fica vazia; senão, o único filho vira raiz.
        if (root.A[0] == 0) {
            rootID = 0;
        } else {
            rootID = root.A[0];
        }

        // Atualiza o cabeçalho e libera a raiz antiga.
        BTreeNode<M> header;
        readNode(0, header);
        header.A[0] = rootID;
        writeNode(0, header);

        freeNode(oldRootID);
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