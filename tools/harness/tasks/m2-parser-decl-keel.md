---
id: m2-parser-decl-keel
output: tools/cgen/src/engine/parser_decl_keel.c
acceptance:
  - "sh tools/cgen/test/unit/parser_decl_keel.sh"
max_attempts: 5
---

Implement one function of `cgen`'s parser: the first that composes three
already-accepted recognizers into a full declaration. First line:

```c
#include "engine/parser.h"
```

That header declares the prototype below, `KSpecifier`, `KDeclaratorHead`,
`KSymbolTable`, and every function you call. Do not add any other
`#include`.

**The three functions you call, and how each one's own contract works —
read all three before writing anything:**

```c
bool k_scan_known_type(KLexer *lexer, KToken first, const KSymbolTable *symtab,
                        KSpecifier *out, KToken *next_out, TKPpKind *next_pp_kind_out);
```
Already accepted. Looks up `first` in `symtab`. If not found (or found as
neither a modifier nor a type), sets `out->kind = K_SPEC_NONE` and returns
`true` — **without touching `*next_out` at all** in that case (it has not
read anything beyond `first`). Otherwise it reads the specifier's
argument(s), if it is a modifier, and always leaves `*next_out` holding
the token right after the whole specifier.

```c
bool k_scan_declarator_head(KLexer *lexer, KToken first, KDeclaratorHead *out,
                             KToken *next_out, TKPpKind *next_pp_kind_out);
```
Reads `{ '*' } IDENT` starting from `first` (already read), puts the name
into `out->name`, and leaves `*next_out` holding the token right after the
name.

```c
void k_scan_opaque_until(KLexer *lexer, KToken first, const char *const *terminators,
                          size_t terminator_count, size_t *terminator_index_out,
                          KToken *next_out, TKPpKind *next_pp_kind_out);
```
Skips a balanced region (respecting `()[]{}` nesting) starting from `first`
(already read) until it finds one of `terminators` at the top level, and
leaves `*next_out` holding that terminator token, with
`*terminator_index_out` saying which one matched.

All three follow the same convention as everything else in this file: they
**return** tokens from `k_lexer_next`, never through an out-pointer, and
they hand back "the token after their own production" already read from
`lexer` — the caller never calls `k_lexer_next` again for that same token.

Assume well-formed input throughout. No diagnostics, no recovery, in this
task.

## `k_scan_decl_keel` — `specifier init-decl { ',' init-decl } ';'`, `init-decl ::= declarator ['=' <opaque>]`

```c
typedef struct { keel_slice_char name; } KKeelDeclName;

typedef struct {
    KSpecifier spec;
    KKeelDeclName names[8];
    size_t name_count;
} KKeelDecl;

bool k_scan_decl_keel(KLexer *lexer, KToken first, KSymbolTable *symtab,
                       KKeelDecl *out, KToken *next_out, TKPpKind *next_pp_kind_out);
```

`first` is already read (the specifier's own first token). Body:

1. `bool ok = k_scan_known_type(lexer, first, symtab, &out->spec, next_out, next_pp_kind_out);`
2. **If `out->spec.kind == K_SPEC_NONE`: `return false;` immediately** —
   this declaration is not applicable, and (matching `k_scan_known_type`'s
   own contract) nothing beyond `first` has been read, so `*next_out` must
   not be touched either.
3. Otherwise, `*next_out` now holds the token right after the specifier —
   the first init-decl's declarator starts there. `out->name_count = 0;`.
   Loop:
   a. `KDeclaratorHead dh; k_scan_declarator_head(lexer, *next_out, &dh, next_out, next_pp_kind_out);`
      — note `*next_out` is both the input (`first`, i.e. the declarator's
      own first token) and, after the call, overwritten with the token
      after the name.
   b. If `out->name_count == 8`, `return false;` immediately (cap safety —
      never happens with real input). Otherwise store:
      `out->names[out->name_count++].name = dh.name;`, and register it:
      `k_symtab_insert(symtab, dh.name, K_SYM_VARIABLE, 0);`.
   c. If `k_token_is_punct(*next_out, "=")` (declared with `=` in
      `engine/lexer.h`, already used by earlier tasks): read the
      initializer's first token,
      `KToken init_first = k_lexer_next(lexer, next_pp_kind_out);`, then
      skip it with
      `static const char *terms[] = { ",", ";" }; size_t idx;
      k_scan_opaque_until(lexer, init_first, terms, 2, &idx, next_out, next_pp_kind_out);`
      — `*next_out` now holds whichever of `,`/`;` stopped the skip.
   d. If `k_token_is_punct(*next_out, ",")`: read the next init-decl's
      first token, `*next_out = k_lexer_next(lexer, next_pp_kind_out);`,
      and go back to step 3a. Otherwise (`*next_out` is `;`): stop the
      loop.
4. `*next_out = k_lexer_next(lexer, next_pp_kind_out);` — one more read,
   past the `;`.
5. `return true;`.

## Worked example (the one with a nested comma, which is why step 3c uses `k_scan_opaque_until` instead of scanning to the next `,` by hand)

Source `"arena a = f(1,2), b; ok2"`, `symtab` has `"arena"` as a
`K_SYM_TYPE`, `first = "arena"`:

1. `k_scan_known_type` → `K_SPEC_NAMED_TYPE`, `*next_out = "a"`.
2. Not `K_SPEC_NONE` — continue.
3. Loop, iteration 1: `k_scan_declarator_head(..., "a", ...)` →
   `dh.name = "a"`, `*next_out = "="`. Register `"a"`. `*next_out` is `"="`
   → read `"f"` as the initializer's first token, then
   `k_scan_opaque_until` walks `f ( 1 , 2 ) ,` — the comma inside `(1,2)`
   is at depth 1 (skipped), the closing `)` brings depth back to 0, and
   the *next* comma (depth 0) is the match: `*next_out = ","`,
   `idx = 0`. `*next_out` is `","` → read `"b"`, go back to 3a.
   Iteration 2: `k_scan_declarator_head(..., "b", ...)` → `dh.name = "b"`,
   `*next_out = ";"`. Register `"b"`. Not `"="`. Not `","` (it's `";"`) —
   stop the loop.
4. Read one more token: `*next_out = "ok2"`.
5. `out->name_count == 2`, `out->names = ["a", "b"]`, `symtab` gained both.

## Out of scope

- `spec-c` prefixes (`inline`, `static`, ...), `array`/`constexpr`
  alternative forms, the `else`-tail form — all later tasks.
- Distinguishing `constexpr` (which would register `K_SYM_CONSTANT`
  instead) — always `K_SYM_VARIABLE` in this task.
- Any diagnostic.
