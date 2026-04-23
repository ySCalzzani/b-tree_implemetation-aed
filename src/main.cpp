#include <iostream>

#include "btree.h"
#include "node.h"
#include "printer.h"

using namespace std;

int main() {
    BTree tree("tmp/btree.dat", 0);
    tree.loadFromFile("data/tree_sample.txt");

    int input;
    while (true) {
        cout << "\nEnter a number to search: ";
        cin >> input;

        SearchResult result = tree.mSearch(input);
        printResult(result);
    }

    return 0;
}
