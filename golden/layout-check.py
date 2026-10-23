#!/usr/bin/env python3
"""Layout of the blank lines of every golden header (codegen-design §4.2, E6).

    python3 golden/layout-check.py          report the headers off the layout
    python3 golden/layout-check.py --fix    rewrite them into it

R0: one blank after the guard. R1: one after the synthetic include block at
the top. R2: none inside that block. R3: in a .h, one blank between
non-empty sections (prototypes | includes of section 3 | bodies). R5: one before
the #endif. R6: no blank between two declarations of a header; the next one
takes the #line the counter asks for. Synthetic blanks never add
up. .c files are not touched (R4: no synthetic blank).
"""
import glob, os, re, sys
R = os.path.dirname(os.path.dirname(os.path.abspath(__file__))) + '/'
files = sorted(set(glob.glob(R + 'golden/c1[1]/**/*.h', recursive=True) + glob.glob(R + 'golden/c23/**/*.h', recursive=True)
                   + glob.glob(R + 'golden/cases/*/expected/**/*.h', recursive=True)))
INC = re.compile(r'#include "[^"]+"$|#include <(stdbool|assert)\.h>$')
LINE = re.compile(r'#line (\d+) "([^"]+)"$')
STR = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')


def r6(content, is_h):
    """R6: no blank between two declarations of a header. A blank survives
    only as R3 (next to a run of section-3 includes or, in a .h without that
    run, between the last prototype and the first body). A removed blank that
    was not followed by a #line costs one, so the next line keeps its place."""
    depth, at, mapped, top = 0, None, [], []
    for s in content:
        m = LINE.match(s)
        mapped.append(at)
        top.append(depth == 0)
        if m:
            at = (int(m.group(1)), m.group(2))
        elif at:
            at = (at[0] + 1, at[1])
        code = STR.sub('""', s)
        depth += code.count('{') - code.count('}')
    first_body = None
    if is_h and not any(s.startswith('#include "') for s in content):
        for k, s in enumerate(content):
            if top[k] and s and not s.startswith('#') and '{' in STR.sub('""', s):
                first_body = k
                break
    keep = set()
    if first_body is not None:
        j = first_body - 1
        while j >= 0 and content[j].startswith('#line '):
            j -= 1
        if j > 0 and content[j] == '':
            keep.add(j)
    out = []
    for k, s in enumerate(content):
        if s or not top[k] or k in keep or k == 0 or k + 1 == len(content):
            out.append(s)
            continue
        if content[k - 1].startswith('#include "') or content[k + 1].startswith('#include "'):
            out.append(s)
            continue
        nxt = content[k + 1]
        if not LINE.match(nxt) and mapped[k + 1]:
            out.append('#line %d "%s"' % mapped[k + 1])
    return out


changed = 0
for p in files:
    old = open(p).read()
    L = old.split('\n')
    assert L[-1] == '', p
    L = L[:-1]
    assert L[0].startswith('/*') and L[1].startswith('#ifndef ') and L[2].startswith('#define ') and L[-1].startswith('#endif'), p
    body = L[3:-1]
    i = 0
    synth = []
    while i < len(body) and (body[i] == '' or INC.match(body[i])):
        if body[i]:
            synth.append(body[i])
        i += 1
    content = body[i:]
    while content and content[-1] == '':
        content.pop()
    out = L[:3] + ['']
    if synth:
        out += synth + ['']
    # R3, section 3: one blank before and after a run of #include "…" inside the content
    c = []
    k = 0
    while k < len(content):
        if content[k].startswith('#include "'):
            j = k
            while j < len(content) and content[j].startswith('#include "'):
                j += 1
            if c and c[-1] != '':
                c.append('')
            c += content[k:j]
            if j < len(content) and content[j] != '':
                c.append('')
            k = j
        else:
            c.append(content[k]); k += 1
    content = c
    content = r6(content, p.endswith('.h') and not p.endswith('.type.h'))
    if content:
        out += content + ['']
    out += [L[-1]]
    new = '\n'.join(out) + '\n'
    if new != old:
        changed += 1
        if '--fix' in sys.argv[1:]:
            open(p, 'w').write(new)
        else:
            print(f'layout-check: {p[len(R):]}: blank lines off the layout (E6)')
print(f"{'fixed' if '--fix' in sys.argv[1:] else 'off the layout'}: {changed} of {len(files)} headers")
sys.exit(1 if changed and '--fix' not in sys.argv[1:] else 0)
