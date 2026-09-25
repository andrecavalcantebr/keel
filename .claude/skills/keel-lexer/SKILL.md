---
name: keel-lexer
description: Maintain the keel lexer (milestone M1, closed) in tools/cgen — the engine lexer, its predicates and diagnostics, and `cgen --stop-after=lex` — verified by the token-dump oracle. Use when working on tools/cgen/src/engine/lexer*.c, token_predicates*.c, diag.c, parser_keel.c or the lex dump.
---

# keel lexer — M1

`cgen --stop-after=lex file.k` works end to end: the tool reads the file,
calls the engine entry point `k_parser_keel(input, output, diagnostics)`,
prints one token per line on stdout and the lexical diagnostics on stderr,
and exits 1 on an `error`. Milestone M1 is closed: every `.k` of `/base` and
`golden/cases` lexes with no diagnostic. This skill is for changing the lexer
without breaking that.

## Where things are

| Path | What |
| --- | --- |
| `tools/cgen/src/engine/lexer.h` | `KToken`, `TKPpKind`, `KLexer`, every lexer prototype |
| `tools/cgen/src/engine/lexer.c` | `k_lexer_init`, `k_lexer_next` — one token per call |
| `tools/cgen/src/engine/lexer_*.c` | one recognizer per file: `peek`, `skip_trivia`, `scan_directive`, `scan_identifier`, `scan_number`, `scan_quoted`, `scan_punct` |
| `tools/cgen/src/engine/token_predicates_*.c` | `k_token_is_ident`, `_c_word`, `_number`, `_string`, `_char`, `_punct` |
| `tools/cgen/src/engine/diag.h`, `diag.c` | the diagnostics sink and table (diag-design §2, §3); storage is the caller's |
| `tools/cgen/src/engine/parser_keel.c` | `k_parser_keel`: the token dump, and later the parser |
| `tools/cgen/src/tool/stop_lex.c` | `--stop-after=lex`: reads the file, sizes the dump and the diagnostics, prefixes the file name |
| `tools/cgen/src/tool/report.c` | prints diagnostics as `<file>:<line>:<col>: <severity>: <message> [<name>]` |
| `tools/harness/oracles/m1-lex-dump.sh` | the oracle for everything here |
| `tools/harness/oracles/lex/` | fixed cases (`cases.txt`), their expected dumps (`*.tokens`), `edge.k`, `reference_lexer.py`, and the diagnostics cases `diag.k` → `diag.stderr`, `diag-base.k` |

## Read before changing anything

- `design/lexer-design.md` — the model (§1–§2), recognizers (§3), trivia and
  directives (§4), predicates (§5), numbers and literals (§6), acceptance cases (§8).
- `design/cgen-tool.md` §5.1 — the dump format; §2.6 — `--stop-after=lex` lexes
  only the given `.k`, no import, no base.
- `keel-spec.md` §2.1 (lexical elements) and §2.5 (the lexer's diagnostics).
- `design/diag-design.md` — the diagnostics sink and how a message is written.

## Rules you must not break

1. **`engine/` does no I/O.** No `#include` of `stdio.h`, `stdlib.h`,
   `unistd.h`, `fcntl.h` or `sys/*`. `make -C tools/cgen` runs the `boundary`
   check. The engine receives bytes and writes into buffers it is given.
2. **Never edit a `.tokens` file to make a test pass, and never regenerate one
   from `cgen`.** They are the expectation. A new expected dump comes from
   `reference_lexer.py` and is read line by line against the design before it
   is committed. If you believe an expected line is wrong, stop and report it,
   citing the design paragraph.
3. **Keep the signatures in `lexer.h`.** Each recognizer and predicate also has
   its own oracle (`tools/harness/oracles/m1-*.sh`) that compiles that one file
   in isolation. After touching one, run its oracle too.
4. **Never read past `source.len`, and every loop advances.** The oracle builds
   with `-fsanitize=address,undefined`; an out-of-bounds read fails it.
5. Code and comments in English, C23 (`-std=c2x`). Engine functions are
   `k_…`, tool functions `cgen_…`.

## The loop

```sh
make -C tools/cgen
sh tools/harness/oracles/m1-lex-dump.sh
# after touching a recognizer or predicate, its own oracle, e.g.:
sh tools/harness/oracles/m1-lexer-skip-trivia.sh tools/cgen/src/engine/lexer_skip_trivia.c
```

`m1-lex-dump.sh` checks four things: the build, the fixed cases against their
`.tokens` byte for byte, every `.k` of `golden/cases` and `base` against the
reference lexer with no diagnostic, and the diagnostics of `diag.k` against
`diag.stderr`. It prints `ok` at the end, or the first lines of each diff.

## Dump format (cgen design §5.1)

```
<file>:<line>:<col>: <class> "<spelling>"
...
<file>:<line>:<col>: eof
```

- `<line>` and `<col>` are physical, 1-based, columns in bytes; CRLF and a lone
  CR are one line break.
- `<class>`, first match wins: `pp-if`, `pp-else`, `pp-endif`, `pp-other` (a
  directive line), then `cword`, `ident`, `number`, `string`, `char`, `punct`,
  else `other`. It is computed on the **logical** spelling, splices removed [D7]:
  `ret\` + newline + `urn` is `cword`.
- `<spelling>` is the **physical** bytes, escaped as a C literal: `\\`, `\"`,
  `\n`, `\r`, `\t`, `\xHH` for other control bytes.
- A token spans from its first to its last logical byte: a splice right after
  the last byte is not part of it.

## Adding behaviour

For anything the lexer should now do differently: add a line to
`oracles/lex/edge.k` (before its last line, a `#` with no newline, which tests
a directive at EOF), regenerate **only** `edge.tokens` with
`python3 tools/harness/oracles/lex/reference_lexer.py tools/harness/oracles/lex/edge.k > tools/harness/oracles/lex/edge.tokens`,
and check the new lines by hand against the design. If the reference lexer
itself lacks the behaviour, extend it first, from the design, not from your C.
A new diagnostic goes into `diag.k`, and its expected line into `diag.stderr`,
written by hand.

## When stuck

Report the failing oracle line, the input bytes (`od -c`), the expected and the
actual dump line, and the design paragraph you read. Do not loosen the oracle.
