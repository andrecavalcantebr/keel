#!/usr/bin/env python3
"""Line-mapping audit of the golden expected files (backend §6).

Follows the #line directives of every expected .c/.h under cases/, works out
which .k line each generated line claims to be, and flags the lines whose C
mentions no name from that .k line. Run from golden/:

    python3 line-audit.py

It is a heuristic, not an oracle: generated code that has no .k line of its
own is flagged by design — the dispatch of `match`, the manager of
`parallel`, the accessors of `extent`, a struct split over several lines, and
the `__chk` object of `constexpr` under C11. A new suspect in a copied body is
a real drift. Checked by hand on 2026-09-25: the remaining suspects are those.
"""
import re, os, sys, glob
ROOT='/home/andre/code/cprojects/keel/golden'
KW=set("""if else for while do return break continue goto switch case default sizeof alignof _Alignof static inline const
volatile struct union enum typedef void int char bool true false size_t NULL extern unsigned signed long short float double
i8 i16 i32 i64 u8 u16 u32 u64 f32 f64 restrict _Atomic static_assert _Static_assert nullptr constexpr typeof""".split())
def idents(line):
    line=re.sub(r'"([^"\\]|\\.)*"','',line); line=re.sub(r"'([^'\\]|\\.)*'",'',line)
    line=re.sub(r'/\*.*?\*/','',line); line=re.sub(r'//.*','',line)
    return [t for t in re.findall(r'[A-Za-z_]\w*',line) if t not in KW]
def match(ct, kt):
    return ct==kt or ct.endswith('_'+kt) or ct.startswith(kt+'_')
report=[]
for case in sorted(glob.glob(f'{ROOT}/cases/*/')):
    for prof in ('c23','c11'):
        for gen in glob.glob(f'{case}expected/{prof}/**/*.[ch]', recursive=True):
            lines=open(gen).read().split('\n')
            cur=None; kfile=None
            for idx,l in enumerate(lines):
                m=re.match(r'#line (\d+) "([^"]+)"',l)
                if m:
                    cur=int(m.group(1)); kfile=m.group(2); continue
                if cur is None: continue
                kpath=os.path.join(case,kfile)
                if not os.path.exists(kpath):
                    kpath=os.path.join(ROOT,'..','base',kfile)
                if os.path.exists(kpath):
                    kl=open(kpath).read().split('\n')
                    ktext=kl[cur-1] if 0<cur<=len(kl) else None
                    ct=[t for t in idents(l) if not t.startswith('keel__') and not t.startswith('KEEL_')]
                    if l.strip().startswith('#'):
                        pass
                    elif ktext is None:
                        if l.strip(): report.append((gen,idx+1,cur,l.strip(),'<past end of .k>'))
                    elif ct:
                        kt=idents(ktext)
                        kcode=re.sub(r'/\*.*?\*/|//.*','',ktext).strip()
                        kcomment=(not kcode) or ktext.strip().startswith(('/*','*','//')) or ktext.rstrip().endswith('*/') and not kt
                        if (not kcode or kcomment) or (kt and not any(match(c,k) for c in ct for k in kt)):
                            report.append((gen,idx+1,cur,l.strip(),ktext.strip()))
                cur+=1
import collections
by=collections.OrderedDict()
for r in report: by.setdefault(r[0],[]).append(r)
for gen,rs in by.items():
    print(f"{os.path.relpath(gen,ROOT)}: {len(rs)} suspect(s), first C:{rs[0][1]} -> .k:{rs[0][2]} | C: {rs[0][3][:60]} | k: {rs[0][4][:50]}")
for gen,cl,kl,c,k in []:
    rel=os.path.relpath(gen,ROOT)
    print(f"{rel}:{cl} -> .k:{kl}\n    C: {c[:110]}\n    k: {k[:110]}")
print(len(report),"suspect lines")
