"""Subprocess wrapper around ./main.exe.

The C++ binary runs as a long-lived REPL: one command per line on stdin, one
response per line on stdout. This module wraps that as typed methods so the
dashboard can ignore the protocol details.
"""

from __future__ import annotations

import subprocess
from dataclasses import dataclass
from pathlib import Path


@dataclass
class SearchResult:
    record: int
    index: int
    found: bool
    path: list[int]


@dataclass
class LoadResult:
    loaded: str
    root: int


class BTreeClient:
    def __init__(self, binary: Path | str = "./main.exe"):
        # Resolve to an absolute path so subprocess doesn't PATH-search
        # (str(Path("./main.exe")) drops the "./" prefix on POSIX).
        binary_path = Path(binary).resolve()
        self.proc = subprocess.Popen(
            [str(binary_path)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        first = self._readline()
        if first != "ready":
            raise RuntimeError(f"expected 'ready', got: {first!r}")

    def __enter__(self) -> "BTreeClient":
        return self

    def __exit__(self, *exc) -> None:
        self.close()

    def _readline(self) -> str:
        line = self.proc.stdout.readline()
        if not line:
            err = self.proc.stderr.read()
            raise RuntimeError(f"main.exe exited unexpectedly. stderr: {err}")
        return line.rstrip("\n")

    def _send(self, command: str) -> str:
        self.proc.stdin.write(command + "\n")
        self.proc.stdin.flush()
        return self._readline()

    @staticmethod
    def _parse_kv(line: str, expected_prefix: str) -> dict[str, str]:
        if not line.startswith(expected_prefix + " ") and line != expected_prefix:
            raise RuntimeError(f"unexpected response: {line!r}")
        rest = line[len(expected_prefix):].strip()
        kv: dict[str, str] = {}
        for token in rest.split():
            k, _, v = token.partition("=")
            kv[k] = v
        return kv

    def search(self, key: int) -> SearchResult:
        line = self._send(f"search {key}")
        kv = self._parse_kv(line, "result")
        path_str = kv.get("path", "")
        return SearchResult(
            record=int(kv["rec"]),
            index=int(kv["idx"]),
            found=kv["found"] == "1",
            path=[int(x) for x in path_str.split(",")] if path_str else [],
        )

    def load(self, source: str) -> LoadResult:
        line = self._send(f"load {source}")
        kv = self._parse_kv(line, "ok")
        return LoadResult(loaded=kv["loaded"], root=int(kv["root"]))

    def close(self) -> None:
        if self.proc.poll() is not None:
            return
        try:
            self.proc.stdin.write("quit\n")
            self.proc.stdin.flush()
            self.proc.wait(timeout=2)
        except Exception:
            self.proc.kill()
            self.proc.wait()
