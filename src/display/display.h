#ifndef DISPLAY_H
#define DISPLAY_H

#include "btree.h"
#include "node.h"

void clearScreen();

void printNode(int record, const Node &p);

void displaySearchResult(int record, int index, bool found);

// Indented hierarchical view: each child is printed under its parent with
// extra indentation. Best for understanding parent/child relationships.
void displayTree(BTree &tree);

// Level-order (BFS) view: all nodes at the same depth printed on one line.
// Best for reading the shape of the tree by level.
void displayLevelOrder(BTree &tree);

#endif
