#include <iostream>
#include <limits>

#include "btree.h"
#include "display.h"
#include "node.h"

using namespace std;

int main() {
    BTree tree("tmp/btree.dat", 0);
    tree.loadFromFile("data/tree_sample.txt");

    int option;
    while (true) {
        clearScreen();
        cout << "=== B-Tree Menu ===" << endl;
        cout << "1. Search a number" << endl;
        cout << "2. Display tree (indented)" << endl;
        cout << "3. Display tree (level order)" << endl;
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
                auto [record, index, found] = tree.mSearch(input);
                cout << "\n==== Result ====" << endl;
                displaySearchResult(record, index, found);
                break;
            }
            case 2:
                cout << "==== Tree ====" << endl;
                displayTree(tree);
                break;
            case 3:
                cout << "==== Tree (level order) ====" << endl;
                displayLevelOrder(tree);
                break;
            default:
                cout << "Invalid option." << endl;
        }

        // Pause so the user can read the result before the screen is cleared
        // for the next menu draw. Discard the trailing newline left by `>>`
        // first, otherwise get() returns immediately.
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "\nPress Enter to continue...";
        cin.get();
    }

    return 0;
}
