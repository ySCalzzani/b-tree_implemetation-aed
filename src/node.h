#ifndef NODE_H
#define NODE_H

constexpr int m = 3;

extern const int POSITIVE_INFINITY;
extern const int NEGATIVE_INFINITY;

class Node {
public:
    int n;
    int K[m + 1];
    int A[m + 1];
};

class SearchResult {
public:
    int p;
    int i;
    bool found;
};

int findIndex(int x, const Node &p);

#endif
