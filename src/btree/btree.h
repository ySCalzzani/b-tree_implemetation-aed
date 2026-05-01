#ifndef BTREE_H
#define BTREE_H

#include <fstream>
#include <string>
#include <tuple>

#include "node.h"

class BTree {
private:
    std::fstream file;
    int root;

public:
    // One node per text line: (2m+3) ints, single-space separated, padded
    // with trailing spaces to a fixed RECORD_SIZE and terminated by '\n'.
    // Fixed width lets us seek by record number.
    static constexpr int NUM_INTS = 2 * (m + 1) + 1;
    static constexpr int RECORD_SIZE = 7 * NUM_INTS;

    BTree(const std::string &path, int root);
    ~BTree();

    void loadFromFile(const std::string &path);

    Node readNode(int record);
    void writeNode(int record, const Node &p);

    int getRoot() const { return root; }

    // Returns (record, index, found). When found, the key sits at K[index] of
    // the node at `record`. When not found, `record` is the leaf where the key
    // would have been inserted and `index` is the slot it would occupy.
    std::tuple<int, int, bool> mSearch(int x);
};

#endif
