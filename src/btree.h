#ifndef BTREE_H
#define BTREE_H

#include <string>
#include <fstream>
#include <vector>
#include "node.h"

/* ----------------------------------------------------------------------------
* DiskManager: Classe para as estatísticas de I/O (ex: número de leituras e escritas no disco).
* ------------------------------------------------------------------------- */
class DiskManager {
private:
    int readCount = 0;
    int writeCount = 0;
    std::string filename;

public:
    DiskManager(const std::string& fname) : filename(fname) {}

    void incrementRead() { readCount++; }
    void incrementWrite() { writeCount++; }
    
    int getReadCount() const { return readCount; }
    int getWriteCount() const { return writeCount; }
    void resetCounters() { readCount = 0; writeCount = 0; }

    long getFileSize() {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return 0;
        return file.tellg();
    }
};

/* ----------------------------------------------------------------------------
* BTree: Classe principal da Árvore B.
* - M: Ordem da árvore.
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
    void print();
    DiskManager& getDiskManager() { return disk; }
};

#endif