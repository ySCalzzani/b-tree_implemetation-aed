#include <iostream>
#include <fstream>
#include <string>

#include "node.h"
#include "printer.h"

using namespace std;

class BTree {
private:
    fstream file;
    int root;

    Node readNode(int record) {
        Node p;
        file.seekg(record * sizeof(Node));
        file.read((char *) &p, sizeof(Node));
        return p;
    }

public:
    BTree(const string &path, int root) : root(root) {
        file.open(path, ios::in | ios::out | ios::trunc | ios::binary);
    }

    ~BTree() {
        file.close();
    }

    void writeNode(int record, const Node &p) {
        file.seekp(record * sizeof(Node));
        file.write((const char *) &p, sizeof(Node));
    }

    SearchResult mSearch(int x) {
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
};

int main() {
    BTree tree("tmp/btree.dat", 1);
    Node p;

    // record 0 (unused slot)
    tree.writeNode(0, p);

    // record 1
    p.n = 2; p.A[0] = 2; p.K[1] = 20; p.A[1] = 3; p.K[2] = 40; p.A[2] = 4;
    tree.writeNode(1, p);

    // record 2
    p.n = 2; p.A[0] = 0; p.K[1] = 10; p.A[1] = 0; p.K[2] = 15; p.A[2] = 0;
    tree.writeNode(2, p);

    // record 3
    p.n = 2; p.A[0] = 0; p.K[1] = 25; p.A[1] = 0; p.K[2] = 30; p.A[2] = 5;
    tree.writeNode(3, p);

    // record 4
    p.n = 2; p.A[0] = 0; p.K[1] = 45; p.A[1] = 0; p.K[2] = 50; p.A[2] = 0;
    tree.writeNode(4, p);

    // record 5
    p.n = 1; p.A[0] = 0; p.K[1] = 35; p.A[1] = 0;
    tree.writeNode(5, p);

    int input;
    while (true) {
        cout << "\nEnter a number to search: ";
        cin >> input;

        SearchResult result = tree.mSearch(input);
        printResult(result);
    }

    return 0;
}
