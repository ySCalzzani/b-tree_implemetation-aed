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
    BTree(const std::string &path, int root);
    ~BTree();

    void writeNode(int record, const Node &p);
    SearchResult mSearch(int x);
};

#endif
