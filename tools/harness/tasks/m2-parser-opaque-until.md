---
id: m2-parser-opaque-until
output: tools/cgen/src/engine/parser_opaque_until.c
acceptance:
  - "sh tools/cgen/test/unit/parser_opaque_until.sh"
max_attempts: 5
---

Implement one function of `cgen`'s parser: a general utility, not tied to
one specific grammar production. First line:

```c
#include "engine/parser.h"
```

That header declares the prototype below and everything you call
(`KLexer`, `KToken`, `TKPpKind`, `k_lexer_next`, `k_token_is_punct`). Do not
add any other `#include`.

`k_lexer_next(lexer, next_pp_kind_out)` **returns** the next token — never
an out-pointer. `k_token_is_punct(t, spelling)` returns `bool` and does not
consume a token. Assume well-formed input otherwise. No diagnostics, no
recovery, in this task.

## `k_scan_opaque_until`

```c
void k_scan_opaque_until(KLexer *lexer, KToken first, const char *const *terminators,
                          size_t terminator_count, size_t *terminator_index_out,
                          KToken *next_out, TKPpKind *next_pp_kind_out);
```

Skips tokens until it finds one that is both (a) equal, by
`k_token_is_punct`, to one of the strings in `terminators[0..terminator_count)`,
and (b) at "depth 0" — not inside any `(`, `[` or `{` this function has
seen opened and not yet closed. `first` is already read (given to you) and
may itself already be the match.

Algorithm — track one `int depth = 0;` shared across all three delimiter
kinds (a keel/C token stream nests them consistently, so one counter is
enough to know when you're back at the top level):

1. `KToken tok = first;`.
2. Loop:
   a. **If `depth == 0`**, check `tok` against every entry of
      `terminators`: for `i` from `0` to `terminator_count - 1`, if
      `k_token_is_punct(tok, terminators[i])`, then set
      `*terminator_index_out = i;`, `*next_out = tok;`, and **return** —
      done.
   b. **EOF safety**: if `tok.len == 0` (end of file — this is what an
      empty token from `k_lexer_next` means), treat it the same as a match
      on `terminators[0]`: set `*terminator_index_out = 0;`,
      `*next_out = tok;`, and return. (This never happens with
      well-formed input reaching a real terminator first, but the
      function must not loop forever calling `k_lexer_next` past the end
      of the file if it somehow did — `tools/harness/README.md`, "um
      oráculo tem que sobreviver a um travamento".)
   c. Otherwise, adjust `depth`: if `k_token_is_punct(tok, "(")`,
      `k_token_is_punct(tok, "[")`, or `k_token_is_punct(tok, "{")`,
      increment `depth`; if `k_token_is_punct(tok, ")")`,
      `k_token_is_punct(tok, "]")`, or `k_token_is_punct(tok, "}")`,
      decrement `depth`.
   d. `tok = k_lexer_next(lexer, next_pp_kind_out);` and go back to step
      2a.

## Worked example (this is exactly why the depth counter exists)

Source `"f(a,b) ; ok"`, `terminators = {",", ";"}`, `first = "f"`:

| token read | depth (before adjusting for it) | terminator check | action |
| --- | --- | --- | --- |
| `f` (=`first`) | 0 | no match | not a delimiter, depth stays 0 |
| `(` | 0 | no match (`(` isn't `,` or `;`) | opens, depth → 1 |
| `a` | 1 | **skipped**, depth ≠ 0 | not a delimiter |
| `,` | 1 | **skipped** — this comma is inside the parens, must not stop here | not a delimiter |
| `b` | 1 | skipped | not a delimiter |
| `)` | 1 | skipped | closes, depth → 0 |
| `;` | 0 | **matches** `terminators[1]` | stop: `*terminator_index_out = 1`, `*next_out = ";"` |

The function returns there — it never reads `ok`. `*next_out` holds the
`;` token itself (already read from `lexer`); the caller reads the next
token after it later, on its own.

A simpler case: source `", rest"`, same `terminators`, `first = ","` — the
very first check (step 2a, depth 0) already matches `terminators[0]`, so
the function returns immediately with `*terminator_index_out = 0`,
`*next_out = ","`, having read nothing beyond `first`.

## Out of scope

- Any diagnostic for an unmatched delimiter or for reaching EOF — the EOF
  case above is a silent, safe stop, not an error, in this task.
