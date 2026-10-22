#!/usr/bin/env python3
"""Auditoria do #line nos arquivos esperados do golden (backend §6).

Simula o contador de linhas do compilador C (C11 §6.10.4) em cada .c/.h
esperado e classifica cada #line:

  REQUIRED first  primeiro #line do arquivo;
  REQUIRED file   muda o .k de origem;
  REQUIRED jump   o contador valeria M != N (salto N-M);
  REDUNDANT       mesmo .k, e o contador já valeria N sem a diretiva.

Depois confere cada linha de código mapeada: o texto C precisa corresponder
a alguma palavra da linha do .k para onde ela aponta (MISSING). Não roda
cgen, make, verify.sh nem golden/run.sh; só lê os arquivos.

Uso, a partir da raiz do repositório:

    python3 golden/line-check.py [--verbose]
"""
import glob
import os
import re
import sys
import collections

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GOLDEN = os.path.join(ROOT, 'golden')
BASE = os.path.join(ROOT, 'base')

KW = set("""if else for while do return break continue goto switch case default sizeof alignof _Alignof static inline const
volatile struct union enum typedef void int char bool true false size_t NULL extern unsigned signed long short float double
i8 i16 i32 i64 u8 u16 u32 u64 f32 f64 restrict _Atomic static_assert _Static_assert nullptr constexpr typeof include""".split())

LINE_RE = re.compile(r'#\s*line\s+(\d+)\s+"([^"]+)"')
INC_RE = re.compile(r'#\s*include\s*<([^>]+)>')


def code_only(lines):
    """Texto de código de cada linha: sem comentários (também os de várias
    linhas) e com literais de string/char trocados por aspas vazias."""
    out = []
    incom = False
    for line in lines:
        res = ''
        i = 0
        while i < len(line):
            if incom:
                j = line.find('*/', i)
                if j < 0:
                    i = len(line)
                    break
                incom = False
                i = j + 2
                continue
            if line.startswith('/*', i):
                incom = True
                i += 2
                continue
            if line.startswith('//', i):
                break
            c = line[i]
            if c in '"\'':
                j = i + 1
                while j < len(line) and line[j] != c:
                    if line[j] == '\\':
                        j += 1
                    j += 1
                res += '""'
                i = j + 1
                continue
            res += c
            i += 1
        out.append(res)
    return out


def idents(code):
    return [t for t in re.findall(r'[A-Za-z_]\w*', code) if t not in KW]


def c_idents(code):
    # Como golden/line-audit.py: o prefixo de mangling keel__ e a macro KEEL_
    # não contam para o casamento.
    return [t for t in idents(code) if not t.startswith(('keel__', 'KEEL_'))]


def match(ct, kt):
    return ct == kt or ct.endswith('_' + kt) or ct.startswith(kt + '_')


def classify(lines):
    """Uma entrada por linha física: line (#line N "f"), inc (#include <x>),
    dir (outra diretiva, inclusive as continuações com \\), code, comment,
    blank."""
    codes = code_only(lines)
    kinds = []
    cont = False
    for line, code in zip(lines, codes):
        s = line.strip()
        if cont:
            kinds.append(('dir', None))
            cont = s.endswith('\\')
            continue
        if s.startswith('#'):
            m = LINE_RE.match(s)
            if m:
                kinds.append(('line', (int(m.group(1)), m.group(2))))
            else:
                m = INC_RE.match(s)
                kinds.append(('inc', m.group(1)) if m else ('dir', None))
            cont = s.endswith('\\')
        elif code.strip():
            kinds.append(('code', None))
        elif s:
            kinds.append(('comment', None))
        else:
            kinds.append(('blank', None))
    return kinds, codes


def simulate(kinds):
    """Percorre as linhas como o compilador: a linha seguinte a um #line N
    recebe N; cada linha física depois soma 1. Devolve as classificações dos
    #line e o mapa índice -> (arquivo .k, linha .k) das linhas que ocupam
    posição na saída (código, include, outras diretivas, comentários)."""
    events = []
    mapped = {}
    cur_file = None
    last = None
    for i, (kind, data) in enumerate(kinds):
        if kind == 'line':
            n, f = data
            if cur_file is None:
                cls, motive, expected = 'REQUIRED', 'first', None
            elif cur_file != f:
                cls, motive, expected = 'REQUIRED', 'file', None
            elif last + 1 == n:
                cls, motive, expected = 'REDUNDANT', None, n
            else:
                cls, motive, expected = 'REQUIRED', 'jump', last + 1
            events.append(dict(idx=i, n=n, f=f, cls=cls, motive=motive,
                               expected=expected))
            cur_file, last = f, n - 1
        elif cur_file is not None and kind in ('code', 'inc', 'dir', 'comment', 'blank'):
            last += 1
            if kind in ('code', 'inc'):
                mapped[i] = (cur_file, last)
    return events, mapped


def resolve(case_dir, f):
    if case_dir:
        p = os.path.join(case_dir, f)
        if os.path.exists(p):
            return p
    p = os.path.join(BASE, f)
    return p if os.path.exists(p) else None


_kcache = {}


def read_lines(path):
    """Linhas do arquivo; a quebra final não abre uma linha nova."""
    text = open(path).read()
    lines = text.split('\n')
    if text.endswith('\n'):
        lines.pop()
    return lines


def k_source(path):
    if path not in _kcache:
        lines = read_lines(path)
        _kcache[path] = (lines, code_only(lines))
    return _kcache[path]


def audit(path, case_dir, profile, rep):
    rel = os.path.relpath(path, ROOT)
    lines = read_lines(path)
    kinds, codes = classify(lines)
    events, mapped = simulate(kinds)
    rep['files'][profile] += 1
    for e in events:
        rep['events'].append((profile, rel, e))
        rep['counts'][profile][e['cls'] if e['cls'] == 'REDUNDANT' else e['motive']] += 1
    for i in sorted(mapped):
        kind, data = kinds[i]
        f, kl = mapped[i]
        kpath = resolve(case_dir, f)
        if kpath is None:
            rep['missing_k'].append((profile, rel, i + 1, f))
            continue
        klines, kcodes = k_source(kpath)
        kcode = kcodes[kl - 1].strip() if 0 < kl <= len(klines) else None
        item = dict(line=i + 1, text=lines[i].strip(), kfile=f, kline=kl,
                    ktext=(kcode if kcode is not None else '<past end of .k>'))
        if kcode is None:
            rep['missing'].append((profile, rel, item, 'past-end'))
            rep['counts'][profile]['MISSING'] += 1
            continue
        if kind == 'inc':
            # import_c <x.h>; no .k, or raw C #include <x.h> (keel.k, bloco de C cru)
            ok = re.search(r'(import_c\s*|#\s*include\s*)<' + re.escape(data) + r'>', kcode)
            if not ok:
                rep['missing'].append((profile, rel, item, 'include'))
                rep['counts'][profile]['MISSING'] += 1
            continue
        if kind != 'code':
            continue
        if not kcode:
            rep['missing'].append((profile, rel, item, 'k-vazio'))
            rep['counts'][profile]['MISSING'] += 1
            continue
        ct = c_idents(codes[i])
        kt = idents(kcode)
        if not ct or not kt:
            rep['counts'][profile]['sem-id'] += 1
            continue
        if not any(match(c, k) for c in ct for k in kt):
            rep['missing'].append((profile, rel, item, 'nome'))
            rep['counts'][profile]['MISSING'] += 1


ALLOW = os.path.join(GOLDEN, 'line-check.allow')


def allow_key(rel, text):
    return f"{rel}\t{' '.join(text.split())}"


def check(rep):
    """--check (golden/run.sh): no REDUNDANT #line, and every MISSING line is
    one reviewed by hand and listed in line-check.allow (code that has no .k
    line of its own: extent accessors, lowerings, instance structs). The key is
    the file and the C text, so the list survives a #line moving."""
    allowed = set()
    if os.path.exists(ALLOW):
        with open(ALLOW) as f:
            allowed = {l.rstrip('\n') for l in f if l.strip() and not l.startswith('#')}
    bad = [(prof, rel, e) for prof, rel, e in rep['events'] if e['cls'] == 'REDUNDANT']
    for prof, rel, e in bad:
        print(f"line-check: {rel}:{e['idx'] + 1}: redundant #line {e['n']}")
    new = [(rel, it) for prof, rel, it, why in rep['missing'] if allow_key(rel, it['text']) not in allowed]
    for rel, it in new:
        print(f"line-check: {rel}:{it['line']}: maps to {it['kfile']}:{it['kline']}, "
              f"which does not match (not in line-check.allow)")
    return 1 if bad or new or rep['missing_k'] else 0


def main():
    verbose = '--verbose' in sys.argv[1:]
    rep = dict(files=collections.Counter(), events=[], missing=[], missing_k=[],
               counts=collections.defaultdict(collections.Counter))
    jobs = []
    for case in sorted(glob.glob(os.path.join(GOLDEN, 'cases', '*') + os.sep)):
        for prof in ('c11', 'c23'):
            for p in sorted(glob.glob(os.path.join(case, 'expected', prof, '**', '*.[ch]'), recursive=True)):
                jobs.append((p, case.rstrip(os.sep), prof))
    for prof in ('c11', 'c23'):
        for p in sorted(glob.glob(os.path.join(GOLDEN, prof, '**', '*.[ch]'), recursive=True)):
            jobs.append((p, None, prof))
    for p, case_dir, prof in jobs:
        audit(p, case_dir, prof, rep)

    if '--check' in sys.argv[1:]:
        return check(rep)
    if '--allow' in sys.argv[1:]:
        for prof, rel, it, why in rep['missing']:
            print(allow_key(rel, it['text']))
        return 0

    print('== totals per profile ==')
    for prof in ('c11', 'c23'):
        c = rep['counts'][prof]
        print(f"{prof}: {rep['files'][prof]} files; "
              f"REQUIRED first={c['first']} file={c['file']} jump={c['jump']}; "
              f"REDUNDANT={c['REDUNDANT']}; MISSING={c['MISSING']}; "
              f"no-id (not checkable)={c['sem-id']}")
    total = sum(rep['files'].values())
    print(f"total: {total} files, {len(rep['events'])} #line, "
          f"{sum(1 for e in rep['events'] if e[2]['cls'] == 'REDUNDANT')} REDUNDANT, "
          f"{len(rep['missing'])} MISSING, {len(rep['missing_k'])} without .k")
    if verbose:
        print('\n== #line ==')
        for prof, rel, e in rep['events']:
            tag = e['cls'] if e['cls'] == 'REDUNDANT' else f"REQUIRED {e['motive']}"
            extra = f" (M={e['expected']}, jump={e['n'] - e['expected']})" if e['motive'] == 'jump' else ''
            print(f"{prof} {rel}:{e['idx'] + 1} #line {e['n']} \"{e['f']}\": {tag}{extra}")
        print('\n== MISSING ==')
        for prof, rel, it, why in rep['missing']:
            print(f"{prof} {rel}:{it['line']} [{why}] -> {it['kfile']}:{it['kline']}\n"
                  f"    C: {it['text'][:110]}\n    k: {it['ktext'][:110]}")
        print('\n== .k not found ==')
        for prof, rel, ln, f in rep['missing_k']:
            print(f"{prof} {rel}:{ln} -> {f}")


if __name__ == '__main__':
    sys.exit(main())
