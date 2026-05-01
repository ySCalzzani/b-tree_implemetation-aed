#ifndef BTREE_H
#define BTREE_H

#include <fstream>
#include <string>

#include "node.h"

class BTree {
private:
    std::fstream file;
    int root;

    Node readNode(int record);

public:
    // One node per text line: (2m+3) ints, single-space separated, padded
    // with trailing spaces to a fixed RECORD_SIZE and terminated by '\n'.
    // Fixed width lets us seek by record number.
    static constexpr int NUM_INTS = 2 * (m + 1) + 1;
    static constexpr int RECORD_SIZE = 7 * NUM_INTS;

    BTree(const std::string &path, int root);
    ~BTree();

    void loadFromFile(const std::string &path);
    
    void writeNode(int record, const Node &p);
    void printNode(int record, const Node &p);

    void mSearch(int x);
};

#endif
