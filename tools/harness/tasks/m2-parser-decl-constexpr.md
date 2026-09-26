---
id: m2-parser-decl-constexpr
output: tools/cgen/src/engine/parser_decl_constexpr.c
acceptance:
  - "sh tools/cgen/test/unit/parser_decl_constexpr.sh"
max_attempts: 5
---

Implement one function of `cgen`'s parser. First line:

```c
#include "engine/parser.h"
```

That header declares the prototype below and everything you call
(`KLexer`, `KToken`, `TKPpKind`, `KSymbolTable`, `k_lexer_next`,
`k_token_is_punct`, `k_token_is_ident`, `k_symtab_insert`,
`k_scan_opaque_until`). Do not add any other `#include`.

`k_lexer_next(lexer, next_pp_kind_out)` **returns** the next token — never
an out-pointer. `k_token_is_punct(t, spelling)` and `k_token_is_ident(t)`
return `bool` and do not consume a token.

```c
/* Appends unconditionally. Returns false, without writing past
   items[cap-1], if the table is full. */
bool k_symtab_insert(KSymbolTable *t, keel_slice_char name, KSymKind kind, int arity);

/* Skips tokens (balancing '(' ')' '[' ']' '{' '}') until it reads one that
   matches one of `terminators` at depth 0. `*next_out` gets that
   terminator token (already read). */
void k_scan_opaque_until(KLexer *lexer, KToken first, const char *const *terminators,
                          size_t terminator_count, size_t *terminator_index_out,
                          KToken *next_out, TKPpKind *next_pp_kind_out);
```

Assume well-formed input otherwise — no diagnostics beyond the one case
described below, no recovery.

## `k_scan_decl_constexpr` — `'constexpr' <opaque> [ IDENT '=' <opaque> ] ';'`

```c
typedef struct {
    bool has_name;
    keel_slice_char name;
} KConstexprDecl;

bool k_scan_decl_constexpr(KLexer *lexer, KToken constexpr_kw, KSymbolTable *symtab,
                            KConstexprDecl *out, KToken *next_out, TKPpKind *next_pp_kind_out);
```

`constexpr_kw` is the already-consumed `'constexpr'` token — given to you,
unused. **There is no pushback and no lookahead beyond one token**: keep
two variables as you scan, `prev` (the previous token, or "none yet") and
`cur` (the one you just read); every token is a `keel_slice_char`, so
keeping a copy of `prev` costs nothing.

Algorithm:

1. `bool has_prev = false; KToken prev = {0};`
2. `KToken cur = k_lexer_next(lexer, next_pp_kind_out); int depth = 0;`
3. Loop:
   a. If `depth == 0` and `k_token_is_punct(cur, "=")`:
      - If `!has_prev || !k_token_is_ident(prev)`: **`return false;`
        immediately** — do not write to `*out` or `*next_out` at all. This
        is the `constexpr-name-missing` case (keel-spec §4.2): a
        top-level `'='` with no valid name right before it.
      - Otherwise: `out->has_name = true; out->name = prev;`
        `k_symtab_insert(symtab, prev, K_SYM_CONSTANT, 0);`. Read the
        initializer's first token,
        `KToken init_first = k_lexer_next(lexer, next_pp_kind_out);`, skip
        it with
        `static const char *terms[] = { ";" }; size_t idx;
        k_scan_opaque_until(lexer, init_first, terms, 1, &idx, next_out, next_pp_kind_out);`
        (this leaves `*next_out` sitting on the `';'` itself), then
        `*next_out = k_lexer_next(lexer, next_pp_kind_out);` (one more
        read, past it). **`return true;`**
   b. Else if `depth == 0` and `k_token_is_punct(cur, ";")`: no `'='` was
      ever seen. `out->has_name = false; out->name = (keel_slice_char){0};`
      `*next_out = k_lexer_next(lexer, next_pp_kind_out);` (one more read,
      past this `';'`). **`return true;`**
   c. Otherwise: adjust `depth` — if `cur` is `'('`, `'['`, or `'{'`,
      increment it; if `')'`, `']'`, or `'}'`, decrement it. Then
      `prev = cur; has_prev = true; cur = k_lexer_next(lexer, next_pp_kind_out);`
      and go back to step 3a.

## Worked examples

- `"size_t MAX = 256; ok"` (`constexpr` already consumed): reads `size_t`
  (→ `prev`), `MAX` (→ `prev`, replacing `size_t`), then `=` at depth 0 —
  `prev` is `MAX`, an `IDENT` → `out->name = "MAX"`, registered. Reads
  `256` as the initializer's first token; `k_scan_opaque_until` walks to
  the `;` (nothing nested here); one more read gives `*next_out = "ok"`.
- `"unsigned long flag; ok2"`: reads `unsigned`, `long`, `flag` into
  `prev` one after another (`'='` never appears), then `;` at depth 0 →
  `out->has_name = false`; one more read gives `*next_out = "ok2"`.
  Nothing registered — not an error.
- `"= 5; oops"`: the very first token read is `'='` itself, at depth 0,
  and `has_prev` is still `false` (nothing was read before it) → `return
  false;` immediately.

## Out of scope

- Emitting the `constexpr-name-missing` diagnostic itself — no
  diagnostic sink is wired into these functions yet; returning `false` is
  enough for this task.
- `nonscalar-constexpr`, `constexpr-as-lvalue` — later, semantic checks.
