#include "printer.h"
#include <iostream>

void printNode(int record, const Node &p) {
    std::cout << "Node " << record << " (n=" << p.n << "): ";
    std::cout << "A[0]=" << p.A[0];
    for (int i = 1; i <= p.n; i++) {
        std::cout << " K[" << i << "]=" << p.K[i];
        std::cout << " A[" << i << "]=" << p.A[i];
    }
    std::cout << std::endl;
}

void printResult(const SearchResult &result) {
    std::cout << "Value found: " << (result.found ? "Yes" : "No") << std::endl;
    std::cout << "Node " << result.p << " - K[" << result.i << "]" << std::endl;
}
