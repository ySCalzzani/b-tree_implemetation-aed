#include "btree.h"

BTree::BTree(const std::string &path, int root) : root(root) {
    file.open(path, std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
}

BTree::~BTree() {
    file.close();
}

Node BTree::readNode(int record) {
    Node p;
    file.seekg(record * sizeof(Node));
    file.read((char *) &p, sizeof(Node));
    return p;
}

void BTree::writeNode(int record, const Node &p) {
    file.seekp(record * sizeof(Node));
    file.write((const char *) &p, sizeof(Node));
}

SearchResult BTree::mSearch(int x) {
    int p = root;
    int q = 0;
    int i = 0;

    while (p != 0) {
        Node current = readNode(p);

        current.K[0] = NEGATIVE_INFINITY;
        current.K[current.n + 1] = POSITIVE_INFINITY;

        i = findIndex(x, current);

        if (x == current.K[i]) {
            return SearchResult{p, i, true};
        }

        q = p;
        p = current.A[i];
    }

    return SearchResult{q, i, false};
}
