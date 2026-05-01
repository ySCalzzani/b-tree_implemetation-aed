#include "btree.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// TO ADD: Add line 0 with metadata
BTree::BTree(const std::string &path, int root) : root(root) {
    file.open(path, std::ios::in | std::ios::out | std::ios::trunc);
}

BTree::~BTree() {
    file.close();
}

Node BTree::readNode(int record) {

    // Seek record
    std::string line(RECORD_SIZE, ' ');
    file.seekg(record * RECORD_SIZE);
    file.read(&line[0], RECORD_SIZE);

    std::istringstream iss(line);
    Node p{};
    iss >> p.n;

    for (int i = 0; i <= m; i++) iss >> p.K[i];
    for (int i = 0; i <= m; i++) iss >> p.A[i];
    return p;
}

void BTree::writeNode(int record, const Node &p) {
    std::ostringstream oss;
    oss << p.n;
    for (int i = 0; i <= m; i++) oss << " " << p.K[i];
    for (int i = 0; i <= m; i++) oss << " " << p.A[i];
    std::string line = oss.str();

    // Pad or truncate to RECORD_SIZE - 1 so every line ends with '\n' at the
    // same offset — required for seek-by-record-number to land on a record
    // boundary.
    line.resize(RECORD_SIZE - 1, ' ');
    line.push_back('\n');

    file.seekp(record * RECORD_SIZE);
    file.write(line.data(), RECORD_SIZE);
    file.flush();
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

void BTree::mSearch(int x) {
    int p = root;
    int q = 0;
    int i = 0;

    while (p != 0) {
        Node current = readNode(p);

        current.K[0] = NEGATIVE_INFINITY;
        current.K[current.n + 1] = POSITIVE_INFINITY;

        i = findIndex(x, current);

        if (x == current.K[i]) {
            std::cout << "Value found: Yes" << std::endl;
            std::cout << "Node " << p << " - K[" << i << "]" << std::endl;
            return;
        }

        q = p;
        p = current.A[i];
    }

    std::cout << "Value found: No" << std::endl;
    std::cout << "Node " << q << " - K[" << i << "]" << std::endl;
}

void BTree::printNode(int record, const Node &p) {
    std::cout << "Node " << record << " (n=" << p.n << "): ";
    std::cout << "A[0]=" << p.A[0];
    for (int i = 1; i <= p.n; i++) {
        std::cout << " K[" << i << "]=" << p.K[i];
        std::cout << " A[" << i << "]=" << p.A[i];
    }
    std::cout << std::endl;
}
