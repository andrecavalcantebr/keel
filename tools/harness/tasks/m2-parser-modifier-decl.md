---
id: m2-parser-modifier-decl
output: tools/cgen/src/engine/parser_modifier_decl.c
acceptance:
  - "sh tools/cgen/test/unit/parser_modifier_decl.sh"
max_attempts: 5
---

Implement one function of `cgen`'s parser: the first one that **registers a
symbol**, not just recognizes syntax. First line:

```c
#include "engine/parser.h"
```

That header declares the prototype below, `KSymbolTable`, and everything you
call (`KLexer`, `KToken`, `TKPpKind`, `k_lexer_next`, `k_token_is_ident_named`,
`k_scan_braced_opaque`, `k_symtab_insert`). Do not add any other `#include`.

Here is an already-accepted function in this same directory,
`engine/parser_extern_c.c`, copied verbatim, showing the exact call pattern
for `k_lexer_next` and for calling another already-implemented recognizer
(`k_scan_braced_opaque`) — **follow this pattern**:

```c
#include "engine/parser.h"

void k_scan_extern_c(KLexer *lexer, KToken extern_c_kw, KExternCDecl *out,
                      KToken *next_out, TKPpKind *next_pp_kind_out) {
    KToken tok = k_lexer_next(lexer, next_pp_kind_out);
    out->has_type_h = false;

    if (k_token_is_punct(tok, "[")) {
        out->has_type_h = true;
        k_lexer_next(lexer, next_pp_kind_out); // type_h
        k_lexer_next(lexer, next_pp_kind_out); // ]
        tok = k_lexer_next(lexer, next_pp_kind_out);
    }

    out->body = k_scan_braced_opaque(lexer, tok, next_out, next_pp_kind_out);
}
```

Notice: `k_lexer_next(lexer, next_pp_kind_out)` **returns** the token (never
an out-pointer). `k_scan_braced_opaque(lexer, open, next_out, next_pp_kind_out)`
takes the already-read `'{'` token as `open`, and **on its own** takes care
of `*next_out`/`*next_pp_kind_out` for the whole `{ ... }` region — the
caller above does not call `k_lexer_next` again after it.

Its prototype, exactly as declared in `engine/parser.h`:

```c
keel_slice_char k_scan_braced_opaque(KLexer *lexer, KToken open,
                                      KToken *next_out, TKPpKind *next_pp_kind_out);
```

`k_symtab_insert`, exactly as declared in `engine/symtab.h`:

```c
/* Appends unconditionally. Returns false, without writing past
   items[cap-1], if the table is full. */
bool k_symtab_insert(KSymbolTable *t, keel_slice_char name, KSymKind kind, int arity);
```

Assume well-formed input throughout. No diagnostics, no recovery, in this
task.

## `k_scan_modifier_decl` — `'modifier' IDENT ['byref'] '{' <opaque> '}'`

```c
typedef struct {
    keel_slice_char name;
    bool byref;
    keel_slice_char body;
} KModifierDecl;

bool k_scan_modifier_decl(KLexer *lexer, KToken modifier_kw, int module_arity,
                           KSymbolTable *symtab, KModifierDecl *out,
                           KToken *next_out, TKPpKind *next_pp_kind_out);
```

`modifier_kw` is the already-consumed `'modifier'` token (any `pub`/`priv`
before it, too — given to you, unused). `module_arity` is a plain number,
already computed by the caller — you do not compute it, just pass it
through to `k_symtab_insert`.

Body:

1. `out->name = k_lexer_next(lexer, next_pp_kind_out);` — the modifier's
   own name.
2. `KToken tok = k_lexer_next(lexer, next_pp_kind_out);` and
   `out->byref = false;`.
3. If `k_token_is_ident_named(tok, "byref")`: set `out->byref = true;`, then
   `tok = k_lexer_next(lexer, next_pp_kind_out);` — `tok` is now the `'{'`
   either way (whether or not step 3 ran).
4. `out->body = k_scan_braced_opaque(lexer, tok, next_out, next_pp_kind_out);`
   — this single call also produces `*next_out` for you, exactly like in
   the quoted `k_scan_extern_c` above; you do not call `k_lexer_next` again
   after it.
5. `return k_symtab_insert(symtab, out->name, K_SYM_MODIFIER, module_arity);`
   — the function's own return value is just whatever this call returns.

## Out of scope

- Computing `module_arity` — always given to you, already correct.
- Interpreting `out->body` — kept opaque for a later task.
- Any diagnostic for a name already in `symtab`.
