#!/usr/bin/env python3
"""Auditoria das linhas vazias nos arquivos esperados do golden (T5).

Classifica cada linha vazia dos .c e .h esperados na primeira categoria que
vale, nesta ordem:

  TOP-AFTER-SYNTH  logo depois do ultimo include sintetico do topo (sub 'after'),
                   ou entre includes sinteticos do topo (sub 'interior');
  SECTION-3        imediatamente antes ou depois de um bloco de includes de .h
                   que os corpos chamam (secao 3 de um .h);
  BEFORE-ENDIF     a linha seguinte e o #endif da guarda;
  MAPPED-SOURCE    sob um #line, e a linha do .k que o contador atribui e vazia
                   ou so comentario;
  MAPPED-OTHER     sob um #line, e a linha do .k tem codigo;
  UNMAPPED         qualquer outra (antes do primeiro #line, ou sem #line).

O contador de #line e o de golden/line-check.py. Tambem mede o topo de cada
arquivo (top-blank / top-noblank) e as formas da secao 3 de cada .h.

Com --simulate DEST aplica a regra (b) numa copia da arvore golden + base em
DEST (que nao pode existir) e roda line-check.py --check sobre a copia. O golden
original nao e tocado. Nunca roda make, cgen, verify.sh ou run.sh.

Uso, a partir da raiz do repositorio:

    python3 golden/blank-check.py [--verbose]
    python3 golden/blank-check.py --simulate /tmp/blank-sim [--rules top,sec3]
"""
import collections
import glob
import importlib.util
import os
import re
import shutil
import subprocess
import sys

sys.dont_write_bytecode = True

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
GOLDEN = os.path.join(ROOT, 'golden')
BASE = os.path.join(ROOT, 'base')
LINE_CHECK = os.path.join(HERE, 'line-check.py')

_spec = importlib.util.spec_from_file_location('line_check', LINE_CHECK)
lc = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(lc)

PROFS = ('c11', 'c23')
CATS = ['TOP-AFTER-SYNTH', 'SECTION-3', 'BEFORE-ENDIF', 'MAPPED-SOURCE',
        'MAPPED-OTHER', 'UNMAPPED', 'MAPPED-PAST-END', 'MAPPED-NO-K']
INC_RE = re.compile(r'#\s*include\s*(["<])([^">]+)[">]')
LINE_RE = lc.LINE_RE


def jobs():
    """Mesmos arquivos que line-check.py: cases/*/expected e golden/{c11,c23}."""
    out = []
    for case in sorted(glob.glob(os.path.join(GOLDEN, 'cases', '*') + os.sep)):
        case = case.rstrip(os.sep)
        for prof in PROFS:
            for p in sorted(glob.glob(os.path.join(case, 'expected', prof, '**', '*.[ch]'), recursive=True)):
                out.append((p, case, prof))
    for prof in PROFS:
        for p in sorted(glob.glob(os.path.join(GOLDEN, prof, '**', '*.[ch]'), recursive=True)):
            out.append((p, None, prof))
    return out


def file_info(path, lines):
    """ext: 'type.h', 'h' ou 'c'. origin: 'prelude' (keel.k), 'module' (o nome
    do arquivo e o do .k mangled), 'instance' (outro nome), 'unknown'."""
    base = os.path.basename(path)
    if base.endswith('.type.h'):
        ext, stem = 'type.h', base[:-len('.type.h')]
    elif base.endswith('.h'):
        ext, stem = 'h', base[:-2]
    else:
        ext, stem = 'c', base[:-2]
    if lines and 'entry unit' in lines[0]:
        return ext, 'entry'
    m = re.search(r'generated from (\S+\.k)', lines[0]) if lines else None
    if not m:
        return ext, 'unknown'
    src = m.group(1)
    if src == 'keel.k':
        return ext, 'prelude'
    mangled = src[:-2].replace('/', '_').replace('.', '_')
    return ext, ('module' if stem == mangled else 'instance')


def is_sec3_inc(s):
    m = INC_RE.match(s)
    return bool(m and m.group(1) == '"' and m.group(2).endswith('.h')
                and not m.group(2).endswith('.type.h'))


def describe(line):
    if line is None:
        return 'start'
    s = line.strip()
    if not s:
        return 'blank'
    if LINE_RE.match(s):
        return '#line'
    if s.startswith('#endif'):
        return '#endif'
    if INC_RE.match(s):
        return 'include'
    return 'code'


def analyze(path, case_dir, prof):
    lines = lc.read_lines(path)
    n = len(lines)
    kinds, _ = lc.classify(lines)
    ext, origin = file_info(path, lines)

    # Mapa de linha fisica -> (arquivo .k, linha .k), como o contador do
    # compilador (line-check.simulate): a linha seguinte a #line N recebe N.
    mapping = {}
    cur, last = None, None
    for i, (kind, data) in enumerate(kinds):
        if kind == 'line':
            cur, last = data[1], data[0] - 1
        elif cur is not None:
            last += 1
            mapping[i] = (cur, last)

    # Topo: depois da guarda (ou do inicio, sem guarda), os includes sinteticos.
    # Num .h, um include de .h que nao seja .type.h ja e a secao 3: o topo para.
    top_start = 0
    for i in range(min(3, n) - 1):
        if lines[i].startswith('#ifndef') and lines[i + 1].startswith('#define'):
            top_start = i + 2
            break
    tops = []
    i = top_start
    while i < n:
        s = lines[i].strip()
        if not s:
            i += 1
            continue
        if s.startswith('/*') and not tops:
            i += 1
            continue
        m = INC_RE.match(s)
        if m and (ext == 'c' or m.group(1) == '<' or m.group(2).endswith('.type.h')):
            tops.append(i)
            i += 1
            continue
        break
    top_last = tops[-1] if tops else None

    # Secao 3 de um .h: corridas de includes de .h (nao .type.h), fora do topo.
    runs = []
    if ext == 'h':
        i = (top_last + 1) if top_last is not None else top_start
        while i < n:
            if is_sec3_inc(lines[i].strip()):
                j = i
                while j + 1 < n and is_sec3_inc(lines[j + 1].strip()):
                    j += 1
                runs.append((i, j))
                i = j + 1
            else:
                i += 1

    top_blank = bool(tops) and top_last + 1 < n and lines[top_last + 1].strip() == ''
    after = None
    if tops:
        k = top_last + 1
        while k < n and lines[k].strip() == '':
            k += 1
        if k >= n:
            after = 'EOF'
        else:
            after = describe(lines[k]).replace('include', 'include-block').replace('code', 'codigo')

    cats = {}
    for j in range(n):
        if lines[j].strip():
            continue
        if tops and j == top_last + 1:
            cats[j] = dict(cat='TOP-AFTER-SYNTH', sub='after')
        elif tops and top_start <= j <= top_last:
            cats[j] = dict(cat='TOP-AFTER-SYNTH', sub='interior')
        else:
            sec = None
            for s0, e0 in runs:
                if j == s0 - 1:
                    sec = 'before'
                elif j == e0 + 1:
                    sec = 'after'
            if sec:
                cats[j] = dict(cat='SECTION-3', sub=sec)
            elif j + 1 < n and lines[j + 1].strip().startswith('#endif'):
                cats[j] = dict(cat='BEFORE-ENDIF', sub=None)
            elif j in mapping:
                kf, kl = mapping[j]
                kp = lc.resolve(case_dir, kf)
                if kp is None:
                    cats[j] = dict(cat='MAPPED-NO-K', sub=None, kfile=kf, kline=kl, ktext='')
                    continue
                klines, kcodes = lc.k_source(kp)
                if 0 < kl <= len(klines):
                    kc = kcodes[kl - 1].strip()
                    cat = 'MAPPED-SOURCE' if kc == '' else 'MAPPED-OTHER'
                else:
                    kc, cat = '<past end of .k>', 'MAPPED-PAST-END'
                cats[j] = dict(cat=cat, sub=None, kfile=kf, kline=kl, ktext=kc)
            else:
                cats[j] = dict(cat='UNMAPPED', sub=None)

    return dict(path=path, rel=os.path.relpath(path, ROOT), case=case_dir, prof=prof,
                lines=lines, n=n, ext=ext, origin=origin, mapping=mapping,
                tops=tops, top_start=top_start, top_last=top_last, top_blank=top_blank,
                after=after, runs=runs, cats=cats)


def guard_only(ctx):
    """None se o arquivo tem mais que includes na guarda; senao, se ha linha
    vazia antes do #endif."""
    lines, ts = ctx['lines'], ctx['top_start']
    ends = [k for k, l in enumerate(lines) if l.startswith('#endif')]
    if not ends:
        return None
    e = ends[-1]
    body = [lines[k] for k in range(ts, e)
            if lines[k].strip() and not INC_RE.match(lines[k].strip())]
    if body or not ctx['tops']:
        return None
    return e - 1 >= 0 and lines[e - 1].strip() == ''


def transform(ctx, rules):
    """Regra (b) sobre um arquivo. Devolve (linhas novas ou None, estatisticas).
    Nunca apaga nem insere perto de linha MAPPED; so ajusta o topo e a secao 3."""
    lines, n, mapping = ctx['lines'], ctx['n'], ctx['mapping']
    ins, dele, st = set(), set(), collections.Counter()
    if 'top' in rules and ctx['tops']:
        a = ctx['top_last'] + 1
        g = 0
        while a + g < n and lines[a + g].strip() == '':
            g += 1
        if a + g < n:
            if g == 0:
                ins.add(a)
                st['top +1'] += 1
                if lines[a].strip().startswith('#endif'):
                    st['top +1 before #endif'] += 1
            elif g > 1:
                for k in range(a + 1, a + g):
                    if k in mapping:
                        st['top kept (mapped)'] += 1
                    else:
                        dele.add(k)
                        st['top -1'] += 1
    if 'sec3' in rules:
        for s0, e0 in ctx['runs']:
            b = 0
            while s0 - 1 - b >= 0 and lines[s0 - 1 - b].strip() == '':
                b += 1
            if b == 0:
                if s0 not in ins:
                    ins.add(s0)
                    st['sec3 before +1'] += 1
            elif b > 1:
                for k in range(s0 - b, s0 - 1):
                    if k in mapping:
                        st['sec3 kept (mapped)'] += 1
                    else:
                        dele.add(k)
                        st['sec3 -1'] += 1
            a2 = 0
            while e0 + 1 + a2 < n and lines[e0 + 1 + a2].strip() == '':
                a2 += 1
            if a2 == 0:
                if e0 + 1 not in ins:
                    ins.add(e0 + 1)
                    st['sec3 after +1'] += 1
            elif a2 > 1:
                for k in range(e0 + 2, e0 + 1 + a2):
                    if k in mapping:
                        st['sec3 kept (mapped)'] += 1
                    else:
                        dele.add(k)
                        st['sec3 -1'] += 1
    if not ins and not dele:
        return None, st
    out = []
    for k in range(n + 1):
        if k in ins:
            out.append('')
        if k < n and k not in dele:
            out.append(lines[k])
    return out, st


def simulate(dest, rules):
    if os.path.exists(dest):
        sys.exit(f"blank-check: {dest} already exists; give a new directory under /tmp")
    os.makedirs(dest)
    ignore = shutil.ignore_patterns('__pycache__')
    shutil.copytree(GOLDEN, os.path.join(dest, 'golden'), ignore=ignore)
    shutil.copytree(BASE, os.path.join(dest, 'base'), ignore=ignore)
    total = collections.Counter()
    changed = 0
    for p, case, prof in jobs():
        ctx = analyze(p, case, prof)
        new, st = transform(ctx, rules)
        total.update(st)
        if new is not None:
            changed += 1
            target = os.path.join(dest, 'golden', os.path.relpath(p, GOLDEN))
            with open(target, 'w') as f:
                f.write('\n'.join(new) + '\n')
    print(f"== simulation in {dest} (rules: {','.join(sorted(rules)) or 'none'}) ==")
    print(f"files changed: {changed}")
    for k in sorted(total):
        print(f"  {k}: {total[k]}")
    r = subprocess.run([sys.executable, os.path.join(dest, 'golden', 'line-check.py'), '--check'],
                       capture_output=True, text=True)
    print(f"line-check.py --check on the copy: exit {r.returncode}")
    out = (r.stdout + r.stderr).rstrip()
    print(out if out else '(no output)')
    return r.returncode


def short(s, width=90):
    s = ' '.join(s.split())
    return s if len(s) <= width else s[:width - 3] + '...'


def report(verbose):
    ctxs = [analyze(p, c, prof) for p, c, prof in jobs()]
    cats = {prof: collections.Counter() for prof in PROFS}
    examples = collections.defaultdict(list)
    interior = collections.Counter()
    blanks_total = collections.Counter()
    unmapped_reason = collections.Counter()
    for ctx in ctxs:
        prof = ctx['prof']
        for j in sorted(ctx['cats']):
            c = ctx['cats'][j]
            name = c['cat']
            blanks_total[prof] += 1
            if name == 'TOP-AFTER-SYNTH' and c['sub'] == 'interior':
                interior[prof] += 1
            cats[prof][name] += 1
            if len(examples[name]) < 2:
                examples[name].append(f"{ctx['rel']}:{j + 1}")
            if verbose:
                print(f"{prof} {ctx['rel']}:{j + 1} {name}"
                      f"{' ' + c['sub'] if c.get('sub') else ''}")
            if name == 'UNMAPPED':
                unmapped_reason['no #line in file' if not ctx['mapping'] else 'before first #line'] += 1

    print('== blank lines per category ==')
    print(f"{'category':<18}{'c11':>6}{'c23':>6}{'total':>7}   examples")
    for name in CATS:
        a, b = cats['c11'][name], cats['c23'][name]
        print(f"{name:<18}{a:>6}{b:>6}{a + b:>7}   {', '.join(examples[name])}")
    print(f"{'blank lines':<18}{blanks_total['c11']:>6}{blanks_total['c23']:>6}"
          f"{sum(blanks_total.values()):>7}")
    print(f"TOP-AFTER-SYNTH sub 'interior' (blank between two synthetic includes): "
          f"c11={interior['c11']} c23={interior['c23']}")
    print(f"UNMAPPED reasons: {dict(unmapped_reason)}")

    print('\n== top block: top-blank vs top-noblank (prof, ext, origin) ==')
    groups = collections.defaultdict(collections.Counter)
    for ctx in ctxs:
        g = groups[(ctx['prof'], ctx['ext'], ctx['origin'])]
        g['files'] += 1
        if not ctx['tops']:
            g['no-top'] += 1
        else:
            g['top-blank' if ctx['top_blank'] else 'top-noblank'] += 1
            g['after ' + ctx['after']] += 1
    for key in sorted(groups):
        g = groups[key]
        after = ', '.join(f"{k[6:]}={v}" for k, v in sorted(g.items()) if k.startswith('after '))
        print(f"{key[0]} {key[1]:<8} {key[2]:<9} files={g['files']:<4} top-blank={g['top-blank']:<4} "
              f"top-noblank={g['top-noblank']:<4} no-top={g['no-top']:<4} after: {after}")

    print('\n== guard-only files (only includes between the guard) ==')
    go = collections.Counter()
    for ctx in ctxs:
        r = guard_only(ctx)
        if r is None:
            continue
        go[(ctx['prof'], 'blank before #endif' if r else 'no blank before #endif')] += 1
    for key in sorted(go):
        print(f"{key[0]}: {key[1]}: {go[key]}")
    if not go:
        print('none')

    print('\n== section 3 of .h: blocks of .h includes after the top ==')
    forms = collections.Counter()
    nruns = collections.Counter()
    for ctx in ctxs:
        if ctx['ext'] != 'h':
            continue
        lines = ctx['lines']
        for s0, e0 in ctx['runs']:
            bef = s0 - 1 >= 0 and lines[s0 - 1].strip() == ''
            aft = e0 + 1 < ctx['n'] and lines[e0 + 1].strip() == ''
            form = ('blank before and after' if bef and aft else 'blank before only' if bef
                    else 'blank after only' if aft else 'no blank')
            prev = describe(lines[s0 - 1]) if s0 - 1 >= 0 else 'start'
            nxt = describe(lines[e0 + 1]) if e0 + 1 < ctx['n'] else 'EOF'
            forms[(ctx['prof'], form, 'prev=' + prev, 'next=' + nxt)] += 1
            nruns[ctx['prof']] += 1
    for key in sorted(forms):
        print(f"{key[0]}: {key[1]}; {key[2]}; {key[3]}: {forms[key]}")
    print(f"blocks per profile: c11={nruns['c11']} c23={nruns['c23']}")

    for name in ('MAPPED-OTHER', 'UNMAPPED'):
        print(f'\n== {name}, one by one ==')
        for ctx in ctxs:
            for j in sorted(ctx['cats']):
                c = ctx['cats'][j]
                if c['cat'] != name:
                    continue
                if name == 'UNMAPPED':
                    where = 'no #line in file' if not ctx['mapping'] else 'before first #line'
                    print(f"{ctx['prof']} {ctx['rel']}:{j + 1} [{where}] "
                          f"prev={describe(ctx['lines'][j - 1]) if j else 'start'} "
                          f"next={describe(ctx['lines'][j + 1]) if j + 1 < ctx['n'] else 'EOF'}")
                else:
                    print(f"{ctx['prof']} {ctx['rel']}:{j + 1} -> {c['kfile']}:{c['kline']} "
                          f"k: {short(c['ktext'], 70)}")


def main(argv):
    if '--simulate' in argv:
        dest = os.path.abspath(argv[argv.index('--simulate') + 1])
        rules = {'top', 'sec3'}
        if '--rules' in argv:
            rules = {x for x in argv[argv.index('--rules') + 1].split(',') if x}
        return simulate(dest, rules)
    report('--verbose' in argv)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
