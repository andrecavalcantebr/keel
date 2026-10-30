#!/usr/bin/env python3
"""Checks that every marked code block of a document is a literal excerpt of
a golden file.

A marker is a line `<!-- golden: <path>:<first>-<last> -->` (path relative to
golden/, lines 1-based and inclusive) immediately before a fenced block. The
block body, minus an optional first label line (`//keel` or `//C gerado`),
must equal those lines of the file exactly.

Usage: docs.py <document.md>...    exit 1 if any block differs."""
import re
import sys
from pathlib import Path

MARK = re.compile(r"^<!-- golden: (\S+):(\d+)-(\d+) -->$")
LABEL = re.compile(r"^//(keel|C gerado)$")
GOLDEN = Path(__file__).resolve().parent.parent / "golden"


def check(doc):
    lines = Path(doc).read_text().split("\n")
    ok = bad = 0
    i = 0
    while i < len(lines):
        m = MARK.match(lines[i])
        i += 1
        if not m:
            continue
        where = f"{doc}:{i}"
        path, first, last = m.group(1), int(m.group(2)), int(m.group(3))
        if i >= len(lines) or not lines[i].startswith("```"):
            print(f"FAIL   {where} — the marker is not followed by a fence")
            bad += 1
            continue
        j = i + 1
        body = []
        while j < len(lines) and not lines[j].startswith("```"):
            body.append(lines[j])
            j += 1
        if body and LABEL.match(body[0]):
            body = body[1:]
        src = GOLDEN / path
        if not src.is_file():
            print(f"FAIL   {where} — {path} does not exist")
            bad += 1
            continue
        want = src.read_text().split("\n")[first - 1:last]
        if body == want:
            ok += 1
        else:
            bad += 1
            print(f"FAIL   {where} — differs from golden/{path}:{first}-{last}")
            for k in range(max(len(body), len(want))):
                a = body[k] if k < len(body) else "<missing>"
                b = want[k] if k < len(want) else "<missing>"
                if a != b:
                    print(f"         line {k + 1}: doc    {a!r}\n"
                          f"                  golden {b!r}")
                    break
        i = j + 1
    return ok, bad


ok = bad = 0
for d in sys.argv[1:]:
    o, b = check(d)
    ok += o
    bad += b
print(f"docs: {ok} ok, {bad} fail")
sys.exit(1 if bad else 0)
