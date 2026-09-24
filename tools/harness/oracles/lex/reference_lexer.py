#!/usr/bin/env python3
"""Reference lexer for `cgen --stop-after=lex` (cgen design §5.1,
lexer-design §3-§6). Written independently of tools/cgen/src/engine, only to
produce and cross-check the expected token dumps. Not part of cgen.

Usage: reference_lexer.py <file.k> [display-name]
"""
import sys

C_WORDS = set("""_Alignas _Alignof _Atomic _BitInt _Bool _Complex _Decimal128 _Decimal32
_Decimal64 _Generic _Imaginary _Noreturn _Static_assert _Thread_local alignas alignof
auto bool break case char const constexpr continue default do double else enum extern
false float for goto if inline int long nullptr register restrict return short signed
sizeof static static_assert struct switch thread_local true typedef typeof
typeof_unqual union unsigned void volatile while""".split())
P3 = ["...", "<<=", ">>="]
P2 = ["->", "++", "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "*=", "/=",
      "%=", "+=", "-=", "&=", "^=", "|=", "##", ".."]
P1 = set("[](){}.&*+-~!/%<>^|?:;=,#")

def main():
    path = sys.argv[1]
    name = sys.argv[2] if len(sys.argv) > 2 else path
    src = open(path, "rb").read()

    # logical view: positions of the logical bytes, splices removed
    phys = []                      # physical index of each logical byte
    i = 0
    while i < len(src):
        if src[i] == 0x5c and i + 1 < len(src) and src[i+1] in (0x0a, 0x0d):
            i += 2
            if src[i-1] == 0x0d and i < len(src) and src[i] == 0x0a:
                i += 1
            continue
        phys.append(i)
        i += 1
    L = bytes(src[p] for p in phys)
    n = len(L)

    def at(k):
        return L[k] if k < n else -1

    def physical_end(s, k):       # just past the token's last logical byte:
        return phys[k-1] + 1 if k > s else phys[s]   # a trailing splice is not the token's

    out = []
    line_clean = True
    k = 0
    while True:
        # trivia
        while k < n:
            c = L[k]
            if c in (0x20, 0x09, 0x0b, 0x0c):
                k += 1
            elif c == 0x0a:
                k += 1; line_clean = True
            elif c == 0x0d:
                k += 1
                if at(k) == 0x0a: k += 1
                line_clean = True
            elif c == 0x2f and at(k+1) == 0x2f:
                k += 2
                while k < n and L[k] not in (0x0a, 0x0d): k += 1
            elif c == 0x2f and at(k+1) == 0x2a:
                k += 2
                while k < n and not (L[k] == 0x2a and at(k+1) == 0x2f): k += 1
                k = min(k + 2, n)
            else:
                break
        start_phys = phys[k] if k < n else len(src)
        if k >= n:
            out.append((start_phys, "eof", None)); break
        c = chr(L[k]); s = k
        cls = None
        if c == "#" and line_clean:
            while k < n and L[k] not in (0x0a, 0x0d): k += 1
            if k < n:
                if L[k] == 0x0d and at(k+1) == 0x0a: k += 2
                else: k += 1
            j = s + 1
            while j < n and L[j] in (0x20, 0x09): j += 1
            w = b""
            while j < n and chr(L[j]).isascii() and chr(L[j]).isalpha() and len(w) < 8:
                w += bytes([L[j]]); j += 1
            w = w.decode()
            cls = {"if": "pp-if", "ifdef": "pp-if", "ifndef": "pp-if",
                   "elif": "pp-else", "elifdef": "pp-else", "elifndef": "pp-else",
                   "else": "pp-else", "endif": "pp-endif"}.get(w, "pp-other")
            line_clean = True
        else:
            line_clean = False
            q = None
            if c in "\"'": q = k
            elif c in "uUL" and at(k+1) in (0x22, 0x27): q = k + 1
            elif c == "u" and at(k+1) == 0x38 and at(k+2) == 0x22: q = k + 2
            if q is not None:
                opener = L[q]; k = q + 1
                while k < n:
                    if L[k] == opener: k += 1; break
                    if L[k] == 0x5c: k = min(k + 2, n); continue
                    if L[k] in (0x0a, 0x0d): break
                    k += 1
            elif c.isascii() and (c.isalpha() or c == "_"):
                while k < n and (chr(L[k]).isascii() and (chr(L[k]).isalnum() or L[k] == 0x5f)):
                    k += 1
            elif c.isdigit() or (c == "." and at(k+1) != -1 and chr(at(k+1)).isdigit()):
                prev = None
                while k < n:
                    ch = chr(L[k])
                    if ch.isascii() and (ch.isalnum() or ch == "_"):
                        prev = ch; k += 1
                    elif ch == "." and at(k+1) != 0x2e:
                        prev = ch; k += 1
                    elif ch in "+-" and prev in ("e", "E", "p", "P"):
                        prev = ch; k += 1
                    else:
                        break
            else:
                t3 = bytes(L[k:k+3]).decode("latin-1")
                if t3 in P3: k += 3
                elif t3[:2] in P2: k += 2
                else: k += 1
        spell_phys = src[start_phys:physical_end(s, k)]
        logical = bytes(L[s:k]).decode("latin-1")
        if cls is None:
            if logical in C_WORDS: cls = "cword"
            elif (logical[0].isascii() and (logical[0].isalpha() or logical[0] == "_")
                  and all(ch.isascii() and (ch.isalnum() or ch == "_") for ch in logical)):
                cls = "ident"
            elif logical[0].isdigit() or (logical[0] == "." and len(logical) > 1 and logical[1].isdigit()):
                cls = "number"
            else:
                body = logical[2:] if logical.startswith("u8") else logical[1:] if logical[:1] in "uUL" else logical
                if body.startswith('"'): cls = "string"
                elif (logical[1:] if logical[:1] in "uUL" else logical).startswith("'"): cls = "char"
                elif logical[0] in P1: cls = "punct"
                else: cls = "other"
        out.append((start_phys, cls, spell_phys))

    # physical line/column
    lines = []
    line, col, p = 1, 1, 0
    for start, cls, spell in out:
        while p < start:
            ch = src[p]
            if ch == 0x0a or (ch == 0x0d and not (p + 1 < len(src) and src[p+1] == 0x0a)):
                line += 1; col = 1
            elif ch != 0x0d:
                col += 1
            p += 1
        if cls == "eof":
            lines.append(f"{name}:{line}:{col}: eof")
        else:
            esc = ""
            for b in spell:
                if b == 0x5c: esc += "\\\\"
                elif b == 0x22: esc += '\\"'
                elif b == 0x0a: esc += "\\n"
                elif b == 0x0d: esc += "\\r"
                elif b == 0x09: esc += "\\t"
                elif b < 0x20 or b == 0x7f: esc += "\\x%02x" % b
                else: esc += chr(b) if b < 0x80 else bytes([b]).decode("latin-1")
            lines.append(f'{name}:{line}:{col}: {cls} "{esc}"')
    sys.stdout.buffer.write(("\n".join(lines) + "\n").encode("latin-1"))

main()
