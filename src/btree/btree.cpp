#include "btree.h"

#include <fstream>
#include <sstream>
#include <string>

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

// Record 0 is reserved as a self-describing header. Writing it makes the .dat
// file readable by any client (including the Python dashboard) without
// out-of-band knowledge of m or RECORD_SIZE.
void BTree::writeMetadata(int nodeCount) {
    std::ostringstream oss;
    oss << "META m=" << m
        << " root=" << root
        << " nodes=" << nodeCount
        << " record_size=" << RECORD_SIZE;
    std::string line = oss.str();
    line.resize(RECORD_SIZE - 1, ' ');
    line.push_back('\n');

    file.seekp(0);
    file.write(line.data(), RECORD_SIZE);
    file.flush();
}

// Expected format (blank lines and lines starting with '#' are ignored):
//   root <record>
//   <record> <n> <A[0]> <K[1]> <A[1]> ... <K[n]> <A[n]>
void BTree::loadFromFile(const std::string &path) {
    std::ifstream in(path);

    // Placeholder at record 0 so seeks into higher records have valid file
    // structure beneath them; overwritten with real metadata at end of parse.
    Node empty{};
    writeNode(0, empty);

    int nodeCount = 0;
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
        nodeCount++;
    }

    writeMetadata(nodeCount);
}

std::tuple<int, int, bool, std::vector<int>> BTree::mSearch(int x) {
    int p = root;
    int q = 0;
    int i = 0;
    std::vector<int> path;

    while (p != 0) {
        path.push_back(p);
        Node current = readNode(p);

        current.K[0] = NEGATIVE_INFINITY;
        current.K[current.n + 1] = POSITIVE_INFINITY;

        i = findIndex(x, current);

        if (x == current.K[i]) {
            return {p, i, true, path};
        }

        q = p;
        p = current.A[i];
    }

    return {q, i, false, path};
}
