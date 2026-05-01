// REPL driven by the Python dashboard. Reads commands from stdin, writes
// structured responses to stdout. One command per line; one response per
// line. See src/python/btree_client.py for the consumer side.
//
// Protocol:
//   <-- ready                                   (printed once on startup)
//   --> search <int>
//   <-- result rec=<int> idx=<int> found=<0|1> path=<csv-of-records>
//   --> load <path>
//   <-- ok loaded=<path> root=<int>
//   --> quit
//   <-- bye

#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "btree.h"

int main() {
    BTree tree("tmp/btree.dat", 0);
    tree.loadFromFile("data/tree_sample.txt");

    std::cout << "ready" << std::endl;

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "search") {
            int x;
            if (!(iss >> x)) {
                std::cout << "error usage=search <int>" << std::endl;
                continue;
            }
            auto [rec, idx, found, path] = tree.mSearch(x);
            std::cout << "result rec=" << rec
                      << " idx=" << idx
                      << " found=" << (found ? 1 : 0)
                      << " path=";
            for (size_t i = 0; i < path.size(); i++) {
                if (i) std::cout << ",";
                std::cout << path[i];
            }
            std::cout << std::endl;
        } else if (cmd == "load") {
            std::string path;
            if (!(iss >> path)) {
                std::cout << "error usage=load <path>" << std::endl;
                continue;
            }
            tree.loadFromFile(path);
            std::cout << "ok loaded=" << path
                      << " root=" << tree.getRoot() << std::endl;
        } else if (cmd == "quit" || cmd == "exit") {
            std::cout << "bye" << std::endl;
            break;
        } else if (cmd.empty()) {
            continue;
        } else {
            std::cout << "error unknown=" << cmd << std::endl;
        }
    }

    return 0;
}
