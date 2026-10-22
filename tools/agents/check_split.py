#!/usr/bin/env python3
"""Oracle of a file split (tools/agents/tasks/T3-islands-split.md).

    python3 tools/agents/check_split.py REV:OLD_PATH NEW_FILE...

Checks that splitting one C file into several moved code without changing it:

1. every line of the old file, stripped and with a leading `static ` removed,
   appears in the new files at least as many times as it did in the old one;
2. every line the new files add over that is glue: blank, a comment, a
   preprocessor line (#include, the guard), a prototype or `extern`
   declaration ending in `;` on a line that starts a declaration, or a
   continuation line of such a prototype.

It prints the lines that break 1 (lost) and 2 (unexplained), and exits 1 if
there is any. It does not parse C: it is a line multiset, which is exactly
what "moved, not changed" means.
"""
import collections
import re
import subprocess
import sys


def norm(line):
    s = line.strip()
    if s.startswith('static '):
        s = s[len('static '):]
    return s


def is_glue(s, in_proto):
    if not s:
        return True
    if s.startswith(('//', '/*', '*', '*/')):
        return True
    if s.startswith('#'):
        return True
    if in_proto:
        return True
    if s.startswith('extern '):
        return True
    return False


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    rev, _, old_path = sys.argv[1].partition(':')
    old = subprocess.run(['git', 'show', f'{rev}:{old_path}'], capture_output=True,
                         text=True, check=True).stdout.split('\n')
    new = []
    for path in sys.argv[2:]:
        with open(path) as f:
            new.extend(f.read().split('\n'))

    want = collections.Counter(norm(l) for l in old if norm(l))
    have = collections.Counter(norm(l) for l in new if norm(l))

    lost = want - have
    extra = have - want

    # the lines a prototype spans: a declaration line that ends a statement
    # with `);` (possibly after continuation lines), outside any body
    unexplained = []
    in_proto = False
    for line in new:
        s = norm(line)
        if not s or extra[s] == 0:
            in_proto = False if s.endswith(';') else in_proto
            continue
        starts_decl = (not line.startswith((' ', '\t'))) and '(' in s and not s.endswith('{')
        proto = in_proto or starts_decl
        if is_glue(s, proto) or (starts_decl and s.endswith(';')) or (in_proto and s.endswith(';')):
            extra[s] -= 1
            in_proto = proto and not s.endswith(';')
            continue
        if starts_decl and not s.endswith(';'):
            # first line of a multi-line prototype
            extra[s] -= 1
            in_proto = True
            continue
        unexplained.append(s)
        in_proto = False

    for s, n in sorted(lost.items()):
        print(f'LOST ({n}x): {s[:120]}')
    for s in unexplained:
        print(f'UNEXPLAINED: {s[:120]}')
    total = sum(want.values())
    if lost or unexplained:
        print(f'FAIL split: {sum(lost.values())} lost, {len(unexplained)} unexplained, of {total} lines')
        return 1
    print(f'ok   split: all {total} non-blank lines of {old_path} are in the new files, the rest is glue')
    return 0


if __name__ == '__main__':
    sys.exit(main())
