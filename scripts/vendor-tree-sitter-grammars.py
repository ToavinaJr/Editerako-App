#!/usr/bin/env python3
"""Download pinned Tree-sitter grammars into tree-sitter/ (src + LICENSE only)."""

from __future__ import annotations

import io
import json
import sys
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / "tree-sitter"

# LANGUAGE_VERSION 14 grammars are compatible with the vendored runtime (15 / min 13).
GRAMMARS = [
    {
        "repo": "tree-sitter/tree-sitter-c",
        "tag": "v0.23.6",
        "dest": "tree-sitter-c",
        "keep": ("src/", "LICENSE"),
    },
    {
        "repo": "tree-sitter/tree-sitter-python",
        "tag": "v0.23.6",
        "dest": "tree-sitter-python",
        "keep": ("src/", "LICENSE"),
    },
    {
        "repo": "tree-sitter/tree-sitter-javascript",
        "tag": "v0.23.1",
        "dest": "tree-sitter-javascript",
        "keep": ("src/", "LICENSE"),
    },
    {
        "repo": "tree-sitter/tree-sitter-typescript",
        "tag": "v0.23.2",
        "dest": "tree-sitter-typescript",
        "keep": ("common/", "typescript/src/", "tsx/src/", "LICENSE"),
    },
    {
        "repo": "tree-sitter/tree-sitter-json",
        "tag": "v0.24.8",
        "dest": "tree-sitter-json",
        "keep": ("src/", "LICENSE"),
    },
    {
        "repo": "tree-sitter/tree-sitter-css",
        "tag": "v0.23.2",
        "dest": "tree-sitter-css",
        "keep": ("src/", "LICENSE"),
    },
    {
        "repo": "tree-sitter/tree-sitter-bash",
        "tag": "v0.23.3",
        "dest": "tree-sitter-bash",
        "keep": ("src/", "LICENSE"),
    },
    {
        "repo": "uyha/tree-sitter-cmake",
        "tag": "v0.7.1",
        "dest": "tree-sitter-cmake",
        "keep": ("src/", "LICENSE"),
    },
    {
        "repo": "tree-sitter-grammars/tree-sitter-yaml",
        "tag": "v0.7.1",
        "dest": "tree-sitter-yaml",
        "keep": ("src/", "LICENSE"),
    },
    {
        "repo": "tree-sitter-grammars/tree-sitter-markdown",
        "tag": "v0.3.2",
        "dest": "tree-sitter-markdown",
        "keep": ("tree-sitter-markdown/src/", "LICENSE"),
        "flatten_prefix": "tree-sitter-markdown/",
    },
    {
        "repo": "DerekStride/tree-sitter-sql",
        "tag": "gh-pages",
        "dest": "tree-sitter-sql",
        "keep": ("src/", "LICENSE"),
        "zip_url": "https://github.com/DerekStride/tree-sitter-sql/archive/refs/heads/gh-pages.zip",
    },
]


def keep_member(rel: str, prefixes: tuple[str, ...]) -> bool:
    name = rel.replace("\\", "/")
    if name.endswith("/"):
        return False
    for prefix in prefixes:
        if name == prefix.rstrip("/") or name.startswith(prefix):
            return True
    return False


def download(url: str) -> bytes:
    req = urllib.request.Request(url, headers={"User-Agent": "Editerako-vendor"})
    with urllib.request.urlopen(req, timeout=120) as resp:
        return resp.read()


def extract_grammar(spec: dict) -> None:
    repo = spec["repo"]
    tag = spec["tag"]
    dest = VENDOR / spec["dest"]
    url = spec.get("zip_url") or f"https://github.com/{repo}/archive/refs/tags/{tag}.zip"
    print(f"Fetching {url} ...")
    try:
        data = download(url)
    except Exception as exc:  # noqa: BLE001
        branch_url = f"https://github.com/{repo}/archive/refs/heads/master.zip"
        print(f"  tag failed ({exc}); trying {branch_url}")
        data = download(branch_url)

    flatten = spec.get("flatten_prefix", "")
    with zipfile.ZipFile(io.BytesIO(data)) as zf:
        names = zf.namelist()
        if not names:
            raise RuntimeError(f"empty zip for {repo}")
        root = names[0].split("/")[0] + "/"
        dest.mkdir(parents=True, exist_ok=True)
        count = 0
        for info in zf.infolist():
            if info.is_dir():
                continue
            rel = info.filename[len(root) :] if info.filename.startswith(root) else info.filename
            rel = rel.replace("\\", "/")
            if not keep_member(rel, spec["keep"]):
                continue
            if flatten and rel.startswith(flatten):
                rel = rel[len(flatten) :]
            out = dest / rel
            out.parent.mkdir(parents=True, exist_ok=True)
            out.write_bytes(zf.read(info.filename))
            count += 1
    print(f"  -> {dest} ({count} files)")


def main() -> int:
    VENDOR.mkdir(parents=True, exist_ok=True)
    for spec in GRAMMARS:
        extract_grammar(spec)
    (VENDOR / "grammars.json").write_text(json.dumps(GRAMMARS, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
