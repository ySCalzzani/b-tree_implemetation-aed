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
├── src/                 # all C++ sources and headers (main.cpp lives here)
├── tests/
│   ├── test_btree.cpp   # mock-data test harness (its own main(), built standalone)
│   └── fixtures/        # text fixtures, one per test case (see naming below)
├── Makefile
└── tmp/                 # created at runtime; holds btree.dat and test data files
```

Fixtures for the **state-changing** operations are named after the test case
that uses them — `insert_<letter>_before.txt` / `insert_<letter>_after.txt` and
`remove_<letter>_before.txt` / `remove_<letter>_after.txt`, where `<letter>` is
the case letter (A, B, C…). `search` is read-only and has no before/after state:
all of its cases query the same tree in a single shared `search_tree.txt`, so
its A–K letters label the individual queries, not files. See the
[test table](#test-cases-and-coverage) for the full list.

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
`insert()` and `remove()`. Most cases are the worked examples from the lecture
slides (`AED-03-Arvores-B.pdf`, order m=3); the rest are edge cases added to
cover paths the slides don't reach (borrow-from-left, internal-key removal,
cascading split, no-ops and order m≠3). All 27 cases pass.

### Test cases and coverage

Cases are lettered per suite (A, B, C…). For the state-changing operations
(`insert`, `remove`) each case has its own self-contained fixture pair sharing
that letter — `<op>_<letter>_before.txt` / `<op>_<letter>_after.txt` (no shared
chain files). `search` is read-only: every case queries the same shared
`search_tree.txt`, so its letters index queries rather than files. The
**Source** column marks whether a case is a worked example from the lecture
slides (`AED-03-Arvores-B.pdf`) or an extra edge case added for coverage.

**`search()` — slide-60 B-tree** (read-only; all cases query `search_tree.txt`)

| Case | Op / key | Source | What it covers |
|------|----------|--------|----------------|
| A | search 30 | slides | key present in the root |
| B | search 20 | slides | key present in an internal node |
| C | search 40 | slides | key present in an internal node |
| D | search 10 | slides | key present in a leaf |
| E | search 25 | slides | key present in a leaf |
| F | search 35 | slides | key present in a leaf |
| G | search 50 | slides | key present in a leaf |
| H | search 5 | slides | absent key below the minimum |
| I | search 33 | slides | absent key between existing keys |
| J | search 99 | slides | absent key above the maximum |
| K | search 42 | added | search against an empty tree (`rootID == 0`, built inline, no fixture) |

**`insert()`** (fixtures `insert_<letter>_before.txt` / `_after.txt`)

| Case | Op / key | Source | What it covers |
|------|----------|--------|----------------|
| A | insert 20 | added | insert into a leaf with spare room (sorted in place) |
| B | insert 10 | added | inserting an existing key is a no-op |
| C | insert 50 | added | first key allocates the root |
| D | insert 30 | added | full root leaf splits, median promoted to a new root |
| E | insert 55 | slides | leaf split + median promoted into an existing parent |
| F | insert 28 | added | leaf split overflows the parent, which splits too → new root (recursive `split()`) |
| G | insert 25 | added | split at order m=5 (mid = (M+1)/2), proving order generality |

**`remove()`** (fixtures `remove_<letter>_before.txt` / `_after.txt`)

| Case | Op / key | Source | What it covers |
|------|----------|--------|----------------|
| A | remove 58 | slides | simple leaf delete, no underflow |
| B | remove 65 | slides | underflow fixed by borrowing from the **right** sibling |
| C | remove 55 | slides | underflow fixed by **merging** with a sibling (parent key pulled down) |
| D | remove 40 | slides | cascading merge that empties and **collapses the root** |
| E | remove 50 | added | key in an internal node → replaced by in-order **successor** (`getSuccessorKey`) |
| F | remove 80 | added | underflow fixed by borrowing from the **left** sibling (`borrowFromLeft`) |
| G | remove 42 | added | removing the last key collapses the tree to **empty** |
| H | remove 99 | added | removing an absent key is a no-op |
| I | remove 10 | added | removing from an empty tree is a no-op |

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
regression that crashes is reported as a single `FAIL` rather than aborting the
suite.