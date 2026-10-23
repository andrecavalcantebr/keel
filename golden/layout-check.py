#!/usr/bin/env python3
"""Layout of the blank lines of every golden header (codegen-design §4.2, E6).

    python3 golden/layout-check.py          report the headers off the layout
    python3 golden/layout-check.py --fix    rewrite them into it

R0: one blank after the guard. R1: one after the synthetic include block at
the top. R2: none inside that block. R3: in a .h, one blank around the
includes of section 3. R5: one before the #endif. Synthetic blanks never add
up. .c files are not touched (R4: no synthetic blank).
"""
import glob, os, re, sys
R = os.path.dirname(os.path.dirname(os.path.abspath(__file__))) + '/'
files = sorted(set(glob.glob(R + 'golden/c1[1]/**/*.h', recursive=True) + glob.glob(R + 'golden/c23/**/*.h', recursive=True)
                   + glob.glob(R + 'golden/cases/*/expected/**/*.h', recursive=True)))
INC = re.compile(r'#include "[^"]+"$|#include <(stdbool|assert)\.h>$')
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
