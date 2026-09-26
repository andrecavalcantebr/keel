---
id: m2-parser-tags-decl
output: tools/cgen/src/engine/parser_tags_decl.c
acceptance:
  - "sh tools/cgen/test/unit/parser_tags_decl.sh"
max_attempts: 5
---

Implement one function of `cgen`'s parser: another one that registers a
symbol. First line:

```c
#include "engine/parser.h"
```

That header declares the prototype below, `KSymbolTable`, and everything
you call (`KLexer`, `KToken`, `TKPpKind`, `k_lexer_next`, `k_scan_ident_list`,
`k_symtab_insert`). Do not add any other `#include`.

Here is an already-accepted function, `engine/parser_ident_list.c`, copied
verbatim — you call this exact function, the same way:

```c
#include "engine/parser.h"

bool k_scan_ident_list(KLexer *lexer, KToken first, KToken *out, size_t cap,
                        size_t *count_out, KToken *next_out, TKPpKind *next_pp_kind_out) {
    size_t count = 0;
    out[count++] = first;

    while (true) {
        KToken tok = k_lexer_next(lexer, next_pp_kind_out);
        if (!k_token_is_punct(tok, ",")) {
            *next_out = tok;
            break;
        }

        KToken ident = k_lexer_next(lexer, next_pp_kind_out);
        if (count == cap) {
            *count_out = count;
            return false;
        }
        out[count++] = ident;
    }

    *count_out = count;
    return true;
}
```

It reads `IDENT { ',' IDENT }`, given the first `IDENT` already read; when
it returns, `*next_out` is the first token that was **not** part of the
list (a `']'`, in this task) — that token has already been read from
`lexer` (the cursor is past it), it is just handed to you instead of you
calling `k_lexer_next` again for it.

Also, exactly as declared in `engine/symtab.h`:

```c
/* Appends unconditionally. Returns false, without writing past
   items[cap-1], if the table is full. */
bool k_symtab_insert(KSymbolTable *t, keel_slice_char name, KSymKind kind, int arity);
```

`k_lexer_next(lexer, next_pp_kind_out)` **returns** the next token — never
an out-pointer. Assume well-formed input throughout. No diagnostics, no
recovery, in this task.

## `k_scan_tags_decl` — `'tags' IDENT '[' IDENT { ',' IDENT } ']' ';'`

```c
typedef struct {
    keel_slice_char name;
    KToken items[16];
    size_t item_count;
} KTagsDecl;

bool k_scan_tags_decl(KLexer *lexer, KToken tags_kw, KSymbolTable *symtab,
                       KTagsDecl *out, KToken *next_out, TKPpKind *next_pp_kind_out);
```

`tags_kw` is the already-consumed `'tags'` token (any `pub`/`priv` before
it, too — given to you, unused). Body:

1. `out->name = k_lexer_next(lexer, next_pp_kind_out);` — the tags set's
   own name.
2. `k_lexer_next(lexer, next_pp_kind_out);` — reads the `'['`, discard the
   result (well-formed input means it is always `'['`).
3. `KToken first_item = k_lexer_next(lexer, next_pp_kind_out);` — the first
   item's name.
4. `KToken tok; k_scan_ident_list(lexer, first_item, out->items, 16, &out->item_count, &tok, next_pp_kind_out);`
   — after this, `tok` holds `']'` (already read from `lexer`; ignore its
   return value here — this task does not need to handle the `cap`
   overflow case).
5. **The lexer has not yet read `';'`** — `tok` is `']'`, not the
   terminator. This is the exact same situation as `k_scan_import_c`
   (already accepted), which handles it with two separate reads in this
   order:
   ```c
   KToken semicolon = k_lexer_next(lexer, next_pp_kind_out);
   (void)semicolon;
   *next_out = k_lexer_next(lexer, next_pp_kind_out);
   ```
   Do exactly that here: the first call reads and discards `';'` itself;
   the second is what leaves `*next_out` holding the token after the whole
   declaration.
6. `return k_symtab_insert(symtab, out->name, K_SYM_TAGS, 0);` — arity is
   always `0` for this kind; the function's own return value is just
   whatever this call returns.

Worked example, source `"tags Cycle [ST1, ST2, ST3]; ok"` (`'tags'` already
consumed): step 1 reads `Cycle`; step 2 reads and discards `[`; step 3
reads `ST1`; step 4's `k_scan_ident_list` reads `,`, `ST2`, `,`, `ST3`, then
`]` (not a `,`, so it stops there) — `out->items` ends up `[ST1, ST2,
ST3]`, `out->item_count = 3`, and `tok = ]`; step 5 reads `;` then `ok` —
`*next_out` ends up `ok`.

## Out of scope

- Tag values (`IDENT '=' tag-value` per item) — a later task, the same way
  `module`'s `dim`/`tags`/`type` binders were added after its first version.
- Any diagnostic for a name already in `symtab`, or for more than 16 items.
