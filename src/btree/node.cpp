#include "node.h"
#include <limits>

const int POSITIVE_INFINITY = std::numeric_limits<int>::max();
const int NEGATIVE_INFINITY = std::numeric_limits<int>::min();

int findIndex(int x, const Node &p) {
    int i = 0;
    while (i < p.n && x >= p.K[i + 1]) {
        i++;
    }
    return i;
}
