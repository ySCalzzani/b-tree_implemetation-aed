"""Live B-Tree dashboard.

UI layer: matplotlib widgets drive operations on the C++ binary (via
BTreeClient) and re-render the current tree state (via BTreeReader, reading
the on-disk tmp/btree.dat directly).

Run from the project root:  .venv/bin/python src/python/dashboard.py
"""

from __future__ import annotations

import sys
import textwrap
from collections import deque
from pathlib import Path

import matplotlib.patches as patches
import matplotlib.pyplot as plt
from matplotlib.widgets import Button, TextBox

sys.path.insert(0, str(Path(__file__).resolve().parent))
from btree_client import BTreeClient, SearchResult
from btree_reader import BTreeReader, TreeNode


DAT_PATH = Path("tmp/btree.dat")
BINARY = Path("./main.exe")
DEFAULT_DATA = "data/tree_sample.txt"


def snapshot(reader: BTreeReader):
    """BFS the tree, return (positions, nodes_by_record, parent_to_children, height)."""
    nodes: dict[int, TreeNode] = {}
    children_of: dict[int, list[int]] = {}
    depth_of: dict[int, int] = {}
    height = 0

    if reader.meta.root != 0:
        q = deque([(reader.meta.root, 0)])
        while q:
            rec, d = q.popleft()
            node = reader.read_node(rec)
            nodes[rec] = node
            depth_of[rec] = d
            height = max(height, d)
            kids = [c for c in node.children if c != 0]
            children_of[rec] = kids
            for c in kids:
                q.append((c, d + 1))

    positions: dict[int, tuple[float, float]] = {}
    leaf_x = [0.0]

    def assign(rec: int) -> float:
        kids = children_of.get(rec, [])
        if not kids:
            x = leaf_x[0]
            leaf_x[0] += 1.0
        else:
            x = sum(assign(c) for c in kids) / len(kids)
        positions[rec] = (x, float(height - depth_of[rec]))
        return x

    if reader.meta.root != 0:
        assign(reader.meta.root)
    return positions, nodes, children_of, height


def draw_tree(ax, positions, nodes, children_of, height, *, highlight_path=None,
              highlight_target=None):
    ax.clear()
    ax.set_title("Tree (highlighted = last search path)", loc="left")
    ax.axis("off")

    if not positions:
        ax.text(0.5, 0.5, "(empty tree)", ha="center", va="center")
        return

    highlight = set(highlight_path or [])

    for parent, kids in children_of.items():
        px, py = positions[parent]
        for child in kids:
            cx, cy = positions[child]
            on_path = parent in highlight and child in highlight
            ax.plot(
                [px, cx], [py, cy],
                color=("crimson" if on_path else "gray"),
                linewidth=(2.2 if on_path else 1.0),
                alpha=(0.9 if on_path else 0.5),
                zorder=1,
            )

    for rec, (x, y) in positions.items():
        node = nodes[rec]
        keys_str = " | ".join(str(k) for k in node.keys) if node.keys else "·"
        is_target = rec == highlight_target
        in_path = rec in highlight
        if is_target:
            facecolor = "#ffd166"
            edge = "crimson"
        elif in_path:
            facecolor = "#ffe9c4"
            edge = "crimson"
        else:
            facecolor = "white"
            edge = "black"
        width = 0.35 + 0.25 * max(node.n, 1)
        box = patches.FancyBboxPatch(
            (x - width / 2, y - 0.18), width, 0.36,
            boxstyle="round,pad=0.03",
            facecolor=facecolor, edgecolor=edge,
            linewidth=(1.8 if (in_path or is_target) else 1.0),
            zorder=2,
        )
        ax.add_patch(box)
        ax.text(x, y + 0.02, keys_str, ha="center", va="center",
                fontsize=9, zorder=3, family="monospace")
        ax.text(x, y - 0.30, f"rec {rec}", ha="center", va="center",
                fontsize=7, color="gray", zorder=3)

    xs = [p[0] for p in positions.values()]
    ys = [p[1] for p in positions.values()]
    pad_x = 0.6
    ax.set_xlim(min(xs) - pad_x, max(xs) + pad_x)
    ax.set_ylim(min(ys) - 0.6, max(ys) + 0.6)


def draw_stats(ax, reader: BTreeReader, positions, nodes, last_event: str):
    ax.clear()
    ax.axis("off")
    ax.set_title("Stats", loc="left")
    if not positions:
        ax.text(0.05, 0.5, "(empty tree)", va="center", family="monospace")
        return
    leaves = sum(1 for r, n in nodes.items() if all(c == 0 for c in n.children))
    total_keys = sum(n.n for n in nodes.values())
    avg_fill = (total_keys / len(nodes)) / reader.meta.m if nodes else 0
    height_pixels = (
        max(positions[r][1] for r in positions)
        - min(positions[r][1] for r in positions)
        + 1
    )
    wrapped_event = textwrap.fill(last_event, width=28,
                                  subsequent_indent="  ")
    text = (
        f"order m         = {reader.meta.m}\n"
        f"root record     = {reader.meta.root}\n"
        f"height          = {height_pixels:.0f}\n"
        f"total nodes     = {len(nodes)}\n"
        f"total keys      = {total_keys}\n"
        f"leaves          = {leaves}\n"
        f"internal        = {len(nodes) - leaves}\n"
        f"avg fill ratio  = {avg_fill:.2f}\n"
        f"\n"
        f"Last event:\n{wrapped_event}"
    )
    ax.text(0.02, 0.98, text, va="top", ha="left",
            family="monospace", fontsize=9)


class Dashboard:
    def __init__(self, client: BTreeClient):
        self.client = client
        self.last_result: SearchResult | None = None
        self.last_event: str = "(none)"

        self.fig = plt.figure(figsize=(14, 8))
        self.fig.suptitle("B-Tree live dashboard", fontsize=14)

        # 2x2 layout: tree (top-left, large), stats (right column),
        # widget row (bottom-left).
        gs = self.fig.add_gridspec(
            nrows=3, ncols=2,
            width_ratios=[3, 1],
            height_ratios=[8, 0.6, 0.4],
            hspace=0.35, wspace=0.18,
            left=0.10, right=0.97, top=0.92, bottom=0.06,
        )
        self.ax_tree = self.fig.add_subplot(gs[0, 0])
        self.ax_stats = self.fig.add_subplot(gs[:, 1])

        # Widget row: TextBox + Search + Reload
        ax_textbox = self.fig.add_subplot(gs[1, 0])
        ax_search_btn = self.fig.add_axes([0.55, 0.13, 0.10, 0.05])
        ax_reload_btn = self.fig.add_axes([0.66, 0.13, 0.10, 0.05])

        self.text_box = TextBox(ax_textbox, "Search key  ", initial="")
        self.text_box.on_submit(self._on_submit)
        self.search_btn = Button(ax_search_btn, "Search")
        self.search_btn.on_clicked(self._on_search)
        self.reload_btn = Button(ax_reload_btn, "Reload")
        self.reload_btn.on_clicked(self._on_reload)

        # Status line
        ax_status = self.fig.add_axes([0.05, 0.02, 0.92, 0.04])
        ax_status.axis("off")
        self.status_text = ax_status.text(
            0.0, 0.5, "ready", va="center", ha="left",
            family="monospace", fontsize=10, color="dimgray",
        )

        self.refresh()

    def _set_status(self, msg: str, *, error: bool = False) -> None:
        self.status_text.set_text(msg)
        self.status_text.set_color("crimson" if error else "dimgray")
        self.last_event = msg

    def _on_submit(self, _text: str) -> None:
        self._on_search(None)

    def _on_search(self, _event) -> None:
        raw = self.text_box.text.strip()
        if not raw:
            self._set_status("enter a key first", error=True)
            self.fig.canvas.draw_idle()
            return
        try:
            key = int(raw)
        except ValueError:
            self._set_status(f"invalid key: {raw!r}", error=True)
            self.fig.canvas.draw_idle()
            return
        try:
            result = self.client.search(key)
        except Exception as e:
            self._set_status(f"search failed: {e}", error=True)
            self.fig.canvas.draw_idle()
            return
        self.last_result = result
        self._set_status(
            f"search {key} -> rec={result.record} idx={result.index} "
            f"found={result.found} path={result.path}"
        )
        self.refresh()

    def _on_reload(self, _event) -> None:
        try:
            res = self.client.load(DEFAULT_DATA)
        except Exception as e:
            self._set_status(f"reload failed: {e}", error=True)
            self.fig.canvas.draw_idle()
            return
        self.last_result = None
        self._set_status(f"reloaded {res.loaded} (root={res.root})")
        self.refresh()

    def refresh(self) -> None:
        with BTreeReader(DAT_PATH) as reader:
            positions, nodes, children_of, height = snapshot(reader)
            highlight_path = self.last_result.path if self.last_result else None
            target = (
                self.last_result.record
                if self.last_result and self.last_result.found
                else None
            )
            draw_tree(
                self.ax_tree, positions, nodes, children_of, height,
                highlight_path=highlight_path, highlight_target=target,
            )
            draw_stats(self.ax_stats, reader, positions, nodes, self.last_event)
        self.fig.canvas.draw_idle()


def main() -> None:
    if not BINARY.exists():
        sys.exit(f"missing {BINARY}; run `make` first")
    if not DAT_PATH.exists():
        sys.exit(f"missing {DAT_PATH}; the binary should populate it on launch")

    with BTreeClient(BINARY) as client:
        Dashboard(client)
        plt.show()


if __name__ == "__main__":
    main()
