#include "display.h"

#include <iostream>
#include <queue>
#include <utility>

// ANSI escape: \033[2J clears the screen, \033[H moves cursor to top-left.
void clearScreen() {
    std::cout << "\033[2J\033[H";
}

void printNode(int record, const Node &p) {
    std::cout << "Node " << record << " (n=" << p.n << "): ";
    std::cout << "A[0]=" << p.A[0];
    for (int i = 1; i <= p.n; i++) {
        std::cout << " K[" << i << "]=" << p.K[i];
        std::cout << " A[" << i << "]=" << p.A[i];
    }
    std::cout << std::endl;
}

void displaySearchResult(int record, int index, bool found) {
    std::cout << "Value found: " << (found ? "Yes" : "No") << std::endl;
    std::cout << "Node " << record << " - K[" << index << "]" << std::endl;
}

static void printKeys(int record, const Node &p) {
    std::cout << "[";
    for (int k = 1; k <= p.n; k++) {
        if (k > 1) std::cout << " ";
        std::cout << p.K[k];
    }
    std::cout << "] (rec " << record << ")";
}

static void displayHelper(BTree &tree, int record, int depth) {
    if (record == 0) return;
    Node p = tree.readNode(record);

    for (int i = 0; i < depth; i++) std::cout << "  ";
    printKeys(record, p);
    std::cout << std::endl;

    for (int k = 0; k <= p.n; k++) {
        displayHelper(tree, p.A[k], depth + 1);
    }
}

void displayTree(BTree &tree) {
    int root = tree.getRoot();
    if (root == 0) {
        std::cout << "(empty tree)" << std::endl;
        return;
    }
    displayHelper(tree, root, 0);
}

void displayLevelOrder(BTree &tree) {
    int root = tree.getRoot();
    if (root == 0) {
        std::cout << "(empty tree)" << std::endl;
        return;
    }

    std::queue<std::pair<int, int>> q;
    q.push({root, 0});
    int currentDepth = -1;

    while (!q.empty()) {
        auto [record, depth] = q.front();
        q.pop();

        if (depth != currentDepth) {
            if (currentDepth >= 0) std::cout << std::endl;
            std::cout << "Level " << depth << ": ";
            currentDepth = depth;
        }

        Node p = tree.readNode(record);
        printKeys(record, p);
        std::cout << "  ";

        for (int k = 0; k <= p.n; k++) {
            if (p.A[k] != 0) q.push({p.A[k], depth + 1});
        }
    }
    std::cout << std::endl;
}
