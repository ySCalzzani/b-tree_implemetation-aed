#include <iostream>

#include "btree.h"
#include "node.h"
#include "printer.h"

using namespace std;

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
