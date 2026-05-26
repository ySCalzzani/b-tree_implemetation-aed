# b-tree_implementation-aed

B-Tree Implementation — PPGCA DCM, Algorithms and Data Structures.

by Caio Uehara Martins
#TODO: Add your names here

## Overview

A disk-oriented B-Tree implemented in C++. Nodes are stored as fixed-size records inside a binary file (`tmp/btree.dat`), and the tree operates by seeking to `record * sizeof(Node)` offsets, mimicking real-world page-based I/O.

- **Programming Language:** C++17
- **Description**: Language use for comments, descriptions and coding is English.

## Project layout

```
.
├── src/       # all C++ sources and headers (main.cpp lives here)
├── tests/     # mock-data test harness + text fixtures
├── Makefile
└── tmp/       # created at runtime; holds btree.dat and test data files
```

## Build and run

Requires `g++` (C++17) and `make`.

```bash
make          # build main.exe
make run      # build (if needed) and run
make test     # build and run the mock-data test harness
make clean    # remove binary, objects, and tmp/
```

`make run` creates `tmp/` automatically.

The program populates a sample B-Tree and enters an interactive prompt; type a number to search, or Ctrl-C to exit.

## Tests

`make test` builds and runs `tests/test_btree.cpp`, which exercises `insert()`
and `search()` against mock data.

Each test describes a tree in a readable text fixture (`tests/fixtures/*.txt`)
using the format:

```
root <id>
<id> <numKeys> <A[0]> <K[1]> <A[1]> ... <K[numKeys]> <A[numKeys]>
```

`BTree::loadFromFile()` builds a tree straight from such a file. An insert test
loads a *sample* tree, runs `insert()`, then compares the result **record by
record** against the expected *final* tree (also a fixture). Each case runs in a
forked child, so a crashing stub is reported as a single `FAIL` rather than
aborting the suite.

Because `allocateNode()` and `split()` are still stubs, the empty-tree and
split cases are expected to **fail** — their diffs are the spec for that work.