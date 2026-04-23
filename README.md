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
├── Makefile
└── tmp/       # created at runtime; holds btree.dat
```

## Build and run

Requires `g++` (C++17) and `make`.

```bash
make          # build main.exe
make run      # build (if needed) and run
make clean    # remove binary, objects, and tmp/
```

`make run` creates `tmp/` automatically. 

The program populates a sample B-Tree and enters an interactive prompt; type a number to search, or Ctrl-C to exit.