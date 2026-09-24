---
name: keel-lexer
description: Finish the keel lexer (milestone M1) in tools/cgen — the engine lexer, its predicates and diagnostics, and `cgen --stop-after=lex` — verified by the token-dump oracle. Use when working on tools/cgen/src/engine/lexer*.c, token_predicates*.c, parser_keel.c or the lex dump.
---

# keel lexer — finishing M1

`cgen --stop-after=lex file.k` already works end to end: the tool reads the
file, calls the engine entry point `k_parser_keel(slice char input, slice char
output)`, and prints one token per line. What is left is listed below, in
order. Work one item at a time, and keep the oracle green after each.

## Where things are

| Path | What |
| --- | --- |
| `tools/cgen/src/engine/lexer.h` | `KToken`, `TKPpKind`, `KLexer`, every lexer prototype |
| `tools/cgen/src/engine/lexer.c` | `k_lexer_init`, `k_lexer_next` — one token per call |
| `tools/cgen/src/engine/lexer_*.c` | one recognizer per file: `peek`, `skip_trivia`, `scan_directive`, `scan_identifier`, `scan_number`, `scan_quoted`, `scan_punct` |
| `tools/cgen/src/engine/token_predicates_*.c` | `k_token_is_ident`, `_c_word`, `_number`, `_string`, `_char`, `_punct` |
| `tools/cgen/src/engine/parser_keel.c` | `k_parser_keel`: the token dump, and later the parser |
| `tools/cgen/src/tool/stop_lex.c` | `--stop-after=lex`: reads the file, sizes the buffer, prefixes the file name |
| `tools/harness/oracles/m1-lex-dump.sh` | the oracle for everything here |
| `tools/harness/oracles/lex/` | fixed cases (`cases.txt`), their expected dumps (`*.tokens`), `edge.k`, and `reference_lexer.py` |

## Read before changing anything

- `design/lexer-design.md` — the model (§1–§2), recognizers (§3), trivia and
  directives (§4), predicates (§5), numbers and literals (§6), acceptance cases (§8).
- `design/cgen-tool.md` §5.1 — the dump format; §2.6 — `--stop-after=lex` lexes
  only the given `.k`, no import, no base.
- `keel-spec.md` §2.1 (lexical elements) and §2.5 (the lexer's diagnostics).
- `design/diag-design.md` — the diagnostics sink, for item 5.

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

`m1-lex-dump.sh` checks three things: the build, the fixed cases against their
`.tokens` byte for byte, and every `.k` of `golden/cases` and `base` against
the reference lexer. It prints `ok` at the end, or the first lines of each diff.

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

## What is left, in order

Each item ends with the oracle green. For items that add behaviour, add a line
to `oracles/lex/edge.k`, regenerate **only** `edge.tokens` with
`python3 tools/harness/oracles/lex/reference_lexer.py tools/harness/oracles/lex/edge.k > tools/harness/oracles/lex/edge.tokens`,
and check the new lines by hand against the design. If the reference lexer
itself lacks the behaviour, extend it first, from the design, not from your C.

1. **`skip_trivia` and splices.** After the second `peek`, `width` holds the
   width of the second byte, and `pos += width + 1` assumes the first one was a
   single byte. A comment opener or closer split by a splice (`/` + splice +
   `/`, or `*` + splice + `/`) is then mis-skipped. Keep one width variable per
   byte you peek. Accept: `edge.k` gains both forms, and the oracle passes.
2. **Predicates on the logical spelling** (lexer-design §5, [D7]). Today they
   compare physical bytes, and `parser_keel.c` works around it with a local
   `logical()` copy. Make each predicate skip splices (use `k_lexer_peek_at`
   over the token), then drop the copy from `token_class`. Accept: the
   `m1-token-predicates-*` oracles and the dump oracle unchanged.
3. **Identifiers with universal character names.** `scan_identifier` accepts
   `\uXXXX` and `\UXXXXXXXX`; `k_token_is_ident` must too (lexer-design §5), so
   `café` is `ident`, not `other`.
4. **`u8` character literals** (C23 `u8'x'`). `scan_quoted` and
   `starts_quoted` in `lexer.c` only take `u8` before `"`.
5. **Lexer diagnostics**, with the sink of `design/diag-design.md` (`diag.c` in
   `engine/`, reporting in `tool/`):
   - `literal-with-newline` — an unescaped newline before the closing quote;
     the token already ends before the newline (`unterminated` from
     `k_lexer_scan_quoted`);
   - `define-over-keel-name` — `#define` or `#undef` whose target starts with
     `keel_` or `KEEL_`, or is one of the contextual words (lexer-design §4,
     including `type_h`).
   Diagnostics go to stderr, in the format of diag-design, and the exit code
   follows cgen design §4. Accept: the diagnostic rows of lexer-design §8.
6. **M1 acceptance** (cgen design §9): every `.k` of `/base` and
   `golden/cases` lexes with no diagnostic — step 3 of the oracle, plus an
   empty stderr.

## When stuck

Report the failing oracle line, the input bytes (`od -c`), the expected and the
actual dump line, and the design paragraph you read. Do not loosen the oracle.
