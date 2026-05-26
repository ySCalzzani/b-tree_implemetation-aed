#ifndef BTREE_H
#define BTREE_H

#include <string>
#include <fstream>
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
    std::fstream file;
    int rootID; 
    DiskManager disk;

    void writeNode(int id, BTreeNode<M>& node);
    void readNode(int id, BTreeNode<M>& node);
    int allocateNode();
    void split(std::vector<int>& path);

public:
    BTree(const std::string& fname);
    ~BTree();

    void insert(int key);
    void remove(int key);
    bool search(int key);
    DiskManager& getDiskManager() { return disk; }
};

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
    // Reaproveitamento de nós
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
    // Lógica de particionamento do nó cheio
}

template <int M>
void BTree<M>::remove(int key) {
    // Remoção de chaves
}

#endif