"""Python reader for the on-disk B-Tree format produced by ./main.exe.

The .dat file is a sequence of fixed-width records. Record 0 is a META header
(written by BTree::writeMetadata) describing m, root, node count, and
record_size. Higher records each hold one Node serialized as space-separated
ints, padded with spaces and terminated by '\n'.

This module mirrors BTree::readNode in Python so the dashboard can walk the
tree the same way C++ does — one record at a time via seek — without ever
loading the whole file into memory.
"""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass
from pathlib import Path
from typing import Iterator


@dataclass
class TreeNode:
    record: int
    n: int
    keys: list[int]      # K[1..n], length n
    children: list[int]  # A[0..n], length n+1


@dataclass
class TreeMeta:
    m: int
    root: int
    record_size: int
    node_count: int


class BTreeReader:
    def __init__(self, path: Path | str):
        self.path = Path(path)
        self.f = self.path.open("rb")
        self.meta = self._read_meta()

    def __enter__(self) -> "BTreeReader":
        return self

    def __exit__(self, *exc) -> None:
        self.f.close()

    def _read_meta(self) -> TreeMeta:
        # We don't yet know record_size, so read enough to find the newline.
        # 1 KiB is far above any plausible RECORD_SIZE for this project.
        self.f.seek(0)
        chunk = self.f.read(1024)
        newline = chunk.find(b"\n")
        if newline == -1:
            raise ValueError(f"{self.path}: no newline in first 1KiB; not a valid .dat")
        line = chunk[:newline].decode().strip()
        if not line.startswith("META "):
            raise ValueError(f"{self.path}: expected META header, got {line[:40]!r}")

        kv = {}
        for token in line[5:].split():
            k, _, v = token.partition("=")
            kv[k] = int(v)

        required = {"m", "root", "nodes", "record_size"}
        missing = required - kv.keys()
        if missing:
            raise ValueError(f"{self.path}: META missing fields: {sorted(missing)}")

        return TreeMeta(
            m=kv["m"],
            root=kv["root"],
            record_size=kv["record_size"],
            node_count=kv["nodes"],
        )

    def read_node(self, record: int) -> TreeNode:
        if record <= 0:
            raise ValueError(f"record {record} is not a data record")
        self.f.seek(record * self.meta.record_size)
        raw = self.f.read(self.meta.record_size).decode()
        tokens = raw.split()

        # Layout written by BTree::writeNode: n K[0] K[1] .. K[m] A[0] A[1] .. A[m]
        m = self.meta.m
        n = int(tokens[0])
        K = [int(x) for x in tokens[1 : 1 + (m + 1)]]
        A = [int(x) for x in tokens[1 + (m + 1) : 1 + 2 * (m + 1)]]

        return TreeNode(
            record=record,
            n=n,
            keys=K[1 : n + 1],
            children=A[: n + 1],
        )

    def walk(self) -> Iterator[tuple[int, TreeNode]]:
        """BFS from the root yielding (depth, node)."""
        if self.meta.root == 0:
            return
        q: deque[tuple[int, int]] = deque([(self.meta.root, 0)])
        while q:
            rec, depth = q.popleft()
            node = self.read_node(rec)
            yield depth, node
            for child in node.children:
                if child != 0:
                    q.append((child, depth + 1))
