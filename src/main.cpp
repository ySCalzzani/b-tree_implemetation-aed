#include <iostream>

#include "btree.h"
#include "node.h"

using namespace std;

int main() {
    BTree tree("tmp/btree.dat", 0);
    tree.loadFromFile("data/tree_sample.txt");

    int option;
    while (true) {
        cout << "\n=== B-Tree Menu ===" << endl;
        cout << "1. Search a number" << endl;
        cout << "0. Exit" << endl;
        cout << "Choose an option: ";
        cin >> option;

        if (option == 0) break;

        cout << endl;

        switch (option) {
            case 1: {
                int input;
                cout << "Enter a number to search: ";
                cin >> input;
                cout << "\n==== Result ====" << endl;
                tree.mSearch(input);
                break;
            }
            default:
                cout << "Invalid option." << endl;
        }
    }

    return 0;
}
