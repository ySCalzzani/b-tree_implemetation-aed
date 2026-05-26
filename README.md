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

`make test` builds and runs `tests/test_btree.cpp`, which exercises `search()`,
`insert()` and `remove()`. The cases are the worked examples from the lecture
slides (`AED-03-Arvores-B.pdf`, order m=3):

- **search** — the slide-60 B-tree, hits and misses at every level;
- **insert** — insert X=55 (leaf split + median promoted into the parent);
- **remove** — the running sequence 58, 65, 55, 40, which covers a simple leaf
  delete, borrowing from a sibling, merging, and a cascading merge that
  collapses the root.

Each test describes a tree in a readable text fixture (`tests/fixtures/*.txt`):

```
root <id>
<id> <numKeys> <A[0]> <K[1]> <A[1]> ... <K[numKeys]> <A[numKeys]>
```

`BTree::loadFromFile()` builds a tree straight from such a file. A case loads a
*before* tree, runs the operation, then compares the result against the
expected *final* tree. The comparison is **logical**: it walks both trees from
the root, comparing each node's keys and the subtree shape, ignoring physical
record ids and nodes abandoned on disk. Each case runs in a forked child, so a
crashing stub is reported as a single `FAIL` rather than aborting the suite.

Because `allocateNode()`/`split()` are still stubs and `remove()` is
unimplemented, the split, empty-tree and all remove cases are expected to
**fail** — their diffs are the spec for that work.