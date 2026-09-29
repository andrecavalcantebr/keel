"""Failure cases (diag-design §7, [G3]): sources written to be refused.

A case is a directory under tools/cgen/test/diag/ that is a source root, and a
line of diag/cases.txt names it and the file to translate:

    <case dir> <source relative to it>

The expectation is the source itself: a line that must draw a diagnostic ends
with `/* DIAG: <identifier> */` (or `// DIAG: <identifier>`), once per
diagnostic; `DIAG-NEXT:` on the line above stands for a line that cannot hold a
comment (an unterminated literal swallows it). The test runs `cgen --stop-after=parse` and compares the set of
(file, line, identifier) it printed with the set the markers ask for, so moving
a line breaks nothing and the test never repeats what the source says. Exit
code 1 when an `error` was expected, 0 otherwise.

    python3 tools/cgen/test/diag.py            # run every case
    python3 tools/cgen/test/diag.py --coverage # what the catalog still lacks
"""
import os
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path

repo = Path.cwd()
binary = repo / 'tools/cgen/cgen'
base = repo / 'base'
cases_dir = repo / 'tools/cgen/test/diag'
MARK = re.compile(r'(?://|/\*)\s*DIAG(-NEXT)?:\s*([a-z0-9]+(?:-[a-z0-9]+)*)')
LINE = re.compile(r'^(.+?):(\d+):(\d+): (error|warning|info): .* \[([a-z0-9-]+)\]$')


def expected(root):
    want = Counter()
    for k in sorted(root.rglob('*.k')):
        for n, text in enumerate(k.read_text(encoding='utf-8').splitlines(), 1):
            for nxt, ident in MARK.findall(text):
                want[(str(k.relative_to(root)), n + bool(nxt), ident)] += 1
    return want


def severities():
    out = subprocess.run([sys.executable, str(repo / 'tools/cgen/gen-diags.py'), '--list'],
                         capture_output=True, text=True, check=True).stdout
    return dict(line.split() for line in out.splitlines())


def run_case(name, source):
    root = cases_dir / name
    result = subprocess.run([str(binary), '--base-dir', str(base), '-I', '.', '--stop-after=parse', source],
                            cwd=root, capture_output=True, text=True, timeout=20)
    got = Counter()
    kinds = severities()
    errors = 0
    for line in result.stderr.splitlines():
        m = LINE.match(line)
        if not m:
            return f'{name}: a line of stderr is not a diagnostic: {line!r}'
        path = os.path.relpath(os.path.join(root, m.group(1)), root) if not os.path.isabs(m.group(1)) \
            else os.path.relpath(m.group(1), root)
        got[(path, int(m.group(2)), m.group(5))] += 1
        errors += m.group(4) == 'error'
    want = expected(root)
    if got != want:
        lines = [f'{name}: diagnostics differ from the markers']
        for key in sorted((want - got)):
            lines.append('  missing  %s:%d [%s]' % key)
        for key in sorted((got - want)):
            lines.append('  unwanted %s:%d [%s]' % key)
        return '\n'.join(lines)
    wants_error = any(kinds.get(ident) == 'error' for (_, _, ident) in want)
    if (result.returncode == 1) != wants_error or result.returncode not in (0, 1):
        return f'{name}: exit code {result.returncode}, expected {1 if wants_error else 0}'
    if result.returncode == 1 and result.stdout:
        return f'{name}: a translation with errors wrote to stdout'
    return None


def covered():
    seen = set()
    for k in cases_dir.rglob('*.k'):
        seen.update(ident for _, ident in MARK.findall(k.read_text(encoding='utf-8')))
    return seen


def coverage():
    kinds = severities()
    seen = covered()
    catalog = [i for i, s in kinds.items() if s != 'debug']
    missing = [i for i in catalog if i not in seen]
    unknown = sorted(seen - set(kinds))
    return len(catalog) - len(missing), len(catalog), missing, unknown


def main():
    if '--coverage' in sys.argv:
        done, total, missing, unknown = coverage()
        print(f'diag coverage: {done}/{total} identifiers of the catalog have a failure case')
        for ident in unknown:
            print(f'  marker names an identifier that is not in the catalog: {ident}')
        if '-v' in sys.argv:
            for ident in missing:
                print('  missing', ident)
        return 1 if unknown else 0
    problems = []
    listed = 0
    for line in (cases_dir / 'cases.txt').read_text().splitlines():
        if not line.strip() or line.startswith('#'):
            continue
        name, source = line.split()
        listed += 1
        problem = run_case(name, source)
        if problem:
            problems.append(problem)
    for problem in problems:
        print(problem)
    done, total, _, unknown = coverage()
    for ident in unknown:
        print(f'marker names an identifier that is not in the catalog: {ident}')
    if problems or unknown:
        return 1
    print(f'{listed} failure cases; the catalog is covered {done}/{total}')
    return 0


sys.exit(main())
