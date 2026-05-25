#include "btree.h"
#include <iostream>

/* ----------------------------------------------------------------------------
* Constructor
* - Se o arquivo não existir, cria um novo arquivo vazio.
* - Se o arquivo existir, lê o registro 0 para carregar o ID da raiz.
* ------------------------------------------------------------------------- */
template <int M>
BTree<M>::BTree(const std::string& fname) : filename(fname), disk(fname) {
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
    file.seekp(id * sizeof(BTreeNode<M>), std::ios::beg);
    file.write((const char*)(&node), sizeof(BTreeNode<M>));
    disk.incrementWrite();
}

template <int M>
void BTree<M>::readNode(int id, BTreeNode<M>& node) {
    file.seekg(id * sizeof(BTreeNode<M>), std::ios::beg);
    file.read((char*)(&node), sizeof(BTreeNode<M>));
    disk.incrementRead();
}

template <int M>
int BTree<M>::allocateNode() {
    // No futuro, se houver nós removidos (Free List), eles serão reaproveitados aqui.
    // Por enquanto, aloca um novo ID no final do arquivo.
    file.seekp(0, std::ios::end);
    return file.tellp() / sizeof(BTreeNode<M>);
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

// INSERÇÃO

template <int M>
void BTree<M>::insert(int key) {
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
    // A MÁGICA DA MATEMÁTICA VAI AQUI!
}

template <int M>
void BTree<M>::remove(int key) {
    // Remoção
}

template <int M>
void BTree<M>::print() {
    // Impressão
}

template class BTree<3>;
template class BTree<4>;
template class BTree<100>;
template class BTree<1000>;