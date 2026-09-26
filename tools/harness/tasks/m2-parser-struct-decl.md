---
id: m2-parser-struct-decl
output: tools/cgen/src/engine/parser_struct_decl.c
acceptance:
  - "sh tools/cgen/test/unit/parser_struct_decl.sh"
max_attempts: 5
---

Implement one function of `cgen`'s parser: a third one that registers a
symbol, reusing an already-accepted recognizer. First line:

```c
#include "engine/parser.h"
```

That header declares the prototype below, `KSymbolTable`, and everything
you call (`KLexer`, `KToken`, `TKPpKind`, `k_lexer_next`, `k_token_is_ident`,
`k_token_is_c_word_named`, `k_scan_braced_opaque`, `k_symtab_insert`). Do
not add any other `#include`.

Here is an already-accepted function, `engine/parser_extern_c.c`, copied
verbatim, showing exactly how `k_scan_braced_opaque` is called — you call
it the same way:

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

`k_scan_braced_opaque(lexer, open, next_out, next_pp_kind_out)` takes the
already-read `'{'` token as `open`, and on its own sets `*next_out` to the
token right after the matching `'}'`. `k_lexer_next(lexer,
next_pp_kind_out)` **returns** the next token — never an out-pointer.
`k_token_is_ident(t)` and `k_token_is_c_word_named(t, name)` both return
`bool` and do not consume a token.

Assume well-formed input throughout. No diagnostics, no recovery, in this
task.

## `k_scan_struct_decl` — `('struct'|'union') [IDENT] '{' <opaque> '}' ';'`

```c
typedef struct {
    keel_slice_char tag_name;   /* .len == 0 when anonymous */
    bool is_union;
    keel_slice_char body;
} KStructDecl;

bool k_scan_struct_decl(KLexer *lexer, KToken struct_or_union_kw, KSymbolTable *symtab,
                         KStructDecl *out, KToken *next_out, TKPpKind *next_pp_kind_out);
```

`struct_or_union_kw` is the already-consumed `'struct'` or `'union'` token
(any `pub`/`priv` before it, too — given to you, unused otherwise). Body:

1. `out->is_union = k_token_is_c_word_named(struct_or_union_kw, "union");`
2. `KToken tok = k_lexer_next(lexer, next_pp_kind_out);`.
3. If `k_token_is_ident(tok)` (there is a tag name): `out->tag_name = tok;`,
   then `tok = k_lexer_next(lexer, next_pp_kind_out);` — `tok` is now the
   `'{'`. Otherwise (anonymous): `out->tag_name = (keel_slice_char){0};` —
   `tok` was already the `'{'`.
4. `out->body = k_scan_braced_opaque(lexer, tok, next_out, next_pp_kind_out);`
5. **This grammar's `'}'` is followed by `';'`, unlike `k_scan_extern_c`'s
   above (which has no `';'` at all).** So after step 4, `*next_out` is
   already sitting on that `';'` (read, but the lexer has not advanced past
   it) — one more read lands past it:
   `*next_out = k_lexer_next(lexer, next_pp_kind_out);`.
6. If `out->tag_name.len != 0`:
   `return k_symtab_insert(symtab, out->tag_name, K_SYM_TYPE, 0);` —
   otherwise (anonymous, nothing to register) `return true;` — an anonymous
   struct is not a failure.

Worked example, source `"struct Person { i32 age; }; ok"` (`'struct'`
already consumed): step 2 reads `Person`; since it is an `IDENT`, step 3
sets `tag_name = Person` and reads `{`; step 4's `k_scan_braced_opaque`
scans `i32 age;` as content and stops at the matching `}`, its own
`*next_out` landing on `;` (not past it — this decl's own terminator); step
5 reads past that `;`, landing on `ok`. `symtab` gains one entry: `Person`,
`K_SYM_TYPE`.

For `"struct { i32 x; }; ok2"` (anonymous): step 2 reads `{` directly (not
an `IDENT`), so `tag_name.len == 0` and nothing is registered; the rest is
the same shape, ending with `*next_out = ok2`.

## Out of scope

- Interpreting keel fields inside the body (`array`, typed declarations) —
  the body stays fully opaque, a later task.
- Any diagnostic for a name already in `symtab`.
