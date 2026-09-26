---
id: m2-parser-declarator-head
output: tools/cgen/src/engine/parser_declarator_head.c
acceptance:
  - "sh tools/cgen/test/unit/parser_declarator_head.sh"
max_attempts: 5
---

Implement one function of `cgen`'s parser. First line:

```c
#include "engine/parser.h"
```

That header declares the prototype below and everything you call
(`KLexer`, `KToken`, `TKPpKind`, `k_lexer_next`, `k_token_is_punct`). Do not
add any other `#include`.

`k_lexer_next(lexer, next_pp_kind_out)` **returns** the next token — never
an out-pointer. `k_token_is_punct(t, spelling)` returns `bool` and does not
consume a token. Assume well-formed input throughout. No diagnostics, no
recovery, in this task.

## `k_scan_declarator_head` — `{ '*' } IDENT`

```c
typedef struct {
    int pointer_depth;
    keel_slice_char name;
} KDeclaratorHead;

bool k_scan_declarator_head(KLexer *lexer, KToken first, KDeclaratorHead *out,
                             KToken *next_out, TKPpKind *next_pp_kind_out);
```

`first` is already read (given to you as a parameter) — it is either a
`'*'` token or the `IDENT` itself, if there is no `'*'` at all.

Algorithm:

1. `KToken tok = first; out->pointer_depth = 0;`.
2. While `k_token_is_punct(tok, "*")` is true: increment
   `out->pointer_depth`, then `tok = k_lexer_next(lexer, next_pp_kind_out);`
   and check again (loop).
3. Once the loop stops, `tok` is the `IDENT` — `out->name = tok;`.
4. `*next_out = k_lexer_next(lexer, next_pp_kind_out);` — the token right
   after the name.
5. `return true;`.

## Worked examples

- Source `"p rest1"`, `first = "p"` (no `'*'` at all): the `while` loop
  never runs (`first` is not `'*'`), so `out->pointer_depth = 0`,
  `out->name = "p"`, and reading one more token gives `*next_out =
  "rest1"`.
- Source `"* * name rest2"`, `first = "*"`: loop runs twice
  (`out->pointer_depth` goes `0` → `1` → `2`), reading a `'*'` each time,
  then reads `"name"` — which is not `'*'`, so the loop stops with `tok =
  "name"` → `out->name = "name"`. One more read gives `*next_out =
  "rest2"`.

## Out of scope

- Qualifiers between stars (`const`, `volatile`, `restrict`, `_Atomic`,
  `ref`) — a later task.
- Trailing suffixes on the name (`[...]`, `(...)`) — a later task.
