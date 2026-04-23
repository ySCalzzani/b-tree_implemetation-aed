#include "btree.h"

#include <fstream>
#include <sstream>
#include <string>

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

// Expected format (blank lines and lines starting with '#' are ignored):
//   root <record>
//   <record> <n> <A[0]> <K[1]> <A[1]> ... <K[n]> <A[n]>
void BTree::loadFromFile(const std::string &path) {
    std::ifstream in(path);

    // Record 0 is reserved (pointer value 0 means "no child"), so we still
    // write a zeroed Node there to make the first real record land at offset
    // sizeof(Node) rather than 0.
    Node empty{};
    writeNode(0, empty);

    std::string line;
    while (std::getline(in, line)) {
        // Skip blank lines and comments. find_first_not_of locates the first
        // non-whitespace character; npos means the line was fully whitespace.
        std::size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos || line[start] == '#') continue;

        // Parse the first whitespace-delimited token to dispatch on line type.
        std::istringstream iss(line);
        std::string first;
        iss >> first;

        // "root <N>" sets which record number is the tree's root.
        if (first == "root") {
            iss >> root;
            continue;
        }

        // Otherwise this is a record line. The first token is the record
        // number; the rest describes the Node: n, then A[0], then n pairs of
        // (K[i], A[i]).
        int record = std::stoi(first);
        Node p{};
        iss >> p.n >> p.A[0];
        for (int k = 1; k <= p.n; k++) {
            iss >> p.K[k] >> p.A[k];
        }
        writeNode(record, p);
    }
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
