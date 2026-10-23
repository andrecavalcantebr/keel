#!/usr/bin/env python3
"""Generates src/engine/diag_catalog.def from the catalog of keel-spec.md §6.2
(diag-design [G1]): the catalog and the implementation cannot drift apart
without breaking the build. The messages come from tools/cgen/diag-messages.md,
which has a row for every identifier of keel-spec §6.2, of cgen-tool-spec §7.1
and of the implementation's own (K_DIAG_TOOL in src/engine/diag.h), and is
generated into src/engine/diag_messages.def.

    python3 tools/cgen/gen-diags.py            # rewrite the file
    python3 tools/cgen/gen-diags.py --check    # fail if it is out of date

The `debug` rows are checks the generated program makes when it runs, not
diagnostics of the translation: the tool never emits them, so they are not
in the table. --list prints every identifier of the catalog with its severity,
for the coverage script.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SPEC = ROOT / 'keel-spec.md'
TOOL_SPEC = ROOT / 'cgen-tool-spec.md'
MESSAGES = ROOT / 'tools/cgen/diag-messages.md'
DIAG_H = ROOT / 'tools/cgen/src/engine/diag.h'
OUT = ROOT / 'tools/cgen/src/engine/diag_catalog.def'
OUT_MESSAGES = ROOT / 'tools/cgen/src/engine/diag_messages.def'
SEVERITY = {'error': 'K_ERROR', 'warning': 'K_WARNING', 'info': 'K_INFO'}


def catalog():
    text = SPEC.read_text(encoding='utf-8')
    start = text.index('### 6.2 Cat')
    end = text.index('### 6.3', start)
    rows = []
    for line in text[start:end].splitlines():
        if not line.startswith('| `'):
            continue
        cells = line.strip().strip('|').split(' | ')
        ident = cells[0].strip().strip('`')
        # the last three cells are severity, owner and contract; the condition
        # is everything between (it may hold a `|` inside code)
        severity = cells[-3].strip().strip('`')
        assert re.fullmatch(r'[a-z0-9]+(-[a-z0-9]+)*', ident), line
        assert severity in ('error', 'warning', 'info', 'debug'), line
        rows.append((ident, severity))
    ids = [i for i, _ in rows]
    assert len(ids) == len(set(ids)), 'duplicate identifier in the catalog'
    return rows


def tool_ids():
    """The identifiers of cgen-tool-spec §7.1, in its order."""
    text = TOOL_SPEC.read_text(encoding='utf-8')
    start = text.index('### 7.1 Diag')
    end = text.index('\n## ', start)
    return [re.match(r'\| `([^`]+)`', line).group(1)
            for line in text[start:end].splitlines() if line.startswith('| `')]


def engine_tool_names():
    """K_DIAG_TOOL of diag.h: identifier -> enum name (not always the
    identifier in capitals: unexpected-token is PARSE_FAILED)."""
    text = DIAG_H.read_text(encoding='utf-8')
    start = text.index('#define K_DIAG_TOOL')
    end = text.index('#include', start)
    return {i: n for n, i in re.findall(r'X\((\w+), "([a-z0-9-]+)", K_\w+\)', text[start:end])}


def split_cells(line):
    """The cells of a table row; `\\|` is a `|` inside a cell."""
    cells, cur, k = [], '', 0
    body = line.strip()[1:-1]
    while k < len(body):
        if body.startswith('\\|', k):
            cur += '|'; k += 2
        elif body[k] == '|':
            cells.append(cur.strip()); cur = ''; k += 1
        else:
            cur += body[k]; k += 1
    cells.append(cur.strip())
    return cells


def message_sections():
    """diag-messages.md: section heading -> [(identifier, message, condition)]."""
    sections, current = {}, None
    for line in MESSAGES.read_text(encoding='utf-8').splitlines():
        if line.startswith('## '):
            current = sections.setdefault(line[3:].strip(), [])
        elif line.startswith('| `') and current is not None:
            cells = split_cells(line)
            assert len(cells) == 3, 'diag-messages.md: three cells per row: ' + line
            current.append((cells[0].strip('`'), cells[1], cells[2]))
    return sections


def messages(rows):
    """Checks diag-messages.md against the tables, and returns the messages
    of the identifiers of the KDiagId enum: [(enum name, message)]."""
    sections = message_sections()
    own = engine_tool_names()
    tool = tool_ids()
    want = {'keel-spec §6.2': [i for i, _ in rows],
            'cgen-tool-spec §7.1': tool,
            'cgen': [i for i in own if i not in tool]}
    errors = []
    for title, ids in want.items():
        got = [i for i, _, _ in sections.get(title, [])]
        if got != ids:
            missing = [i for i in ids if i not in got]
            extra = [i for i in got if i not in ids]
            errors.append('section "%s": missing %s, extra %s%s' % (
                title, missing, extra, '' if missing or extra else ', out of order'))
    for title in sections:
        if title not in want:
            errors.append('unknown section "%s"' % title)
    for title, entries in sections.items():
        for ident, _, condition in entries:
            if not condition:
                errors.append('%s: no Condition' % ident)
    if errors:
        raise SystemExit('diag-messages.md disagrees with the tables:\n  ' + '\n  '.join(errors))
    live = {i for i, s in rows if s != 'debug'}
    out = []
    for entries in sections.values():
        for ident, message, _ in entries:
            if not message:
                continue
            if ident in own:
                out.append((own[ident], message))
            elif ident in live:
                out.append((ident.upper().replace('-', '_'), message))
            else:
                raise SystemExit('diag-messages.md: %s has a Message and the translation never emits it' % ident)
    return out


def c_string(text):
    return '"' + text.replace('\\', '\\\\').replace('"', '\\"') + '"'


def render_messages(entries):
    out = ['/* Generated by tools/cgen/gen-diags.py from tools/cgen/diag-messages.md. Do not edit.',
           '   One X(NAME, "message") per diagnostic that has a message. */',
           '#define K_DIAG_MESSAGES(X) \\']
    for n, (name, message) in enumerate(entries):
        tail = ' \\' if n + 1 < len(entries) else ''
        out.append('    X(%s, %s)%s' % (name, c_string(message), tail))
    return '\n'.join(out) + '\n'


def render(rows):
    out = ['/* Generated by tools/cgen/gen-diags.py from keel-spec.md section 6.2. Do not edit.',
           '   One X(NAME, "identifier", severity) per diagnostic the translation can emit. */',
           '#define K_DIAG_CATALOG(X) \\']
    live = [(i, s) for i, s in rows if s != 'debug']
    for n, (ident, severity) in enumerate(live):
        name = ident.upper().replace('-', '_')
        tail = ' \\' if n + 1 < len(live) else ''
        out.append('    X(%s, "%s", %s)%s' % (name, ident, SEVERITY[severity], tail))
    return '\n'.join(out) + '\n'


def main():
    rows = catalog()
    if '--list' in sys.argv:
        for ident, severity in rows:
            print(ident, severity)
        return 0
    outputs = [(OUT, render(rows)), (OUT_MESSAGES, render_messages(messages(rows)))]
    if '--check' in sys.argv:
        stale = [p.name for p, text in outputs if not p.exists() or p.read_text() != text]
        if stale:
            print('%s out of date: run tools/cgen/gen-diags.py' % ', '.join(stale), file=sys.stderr)
            return 1
        return 0
    for path, text in outputs:
        path.write_text(text)
    return 0


sys.exit(main())
