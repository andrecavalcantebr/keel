---
id: m2-parser-header
output: tools/cgen/src/engine/parser_header.c
acceptance:
  - "sh tools/cgen/test/unit/parser_header.sh"
max_attempts: 5
---

Implement three recognizers of `cgen`'s parser, all in this one file. The
first line must be:

```c
#include "engine/parser.h"
```

That header declares every type and prototype you need, including this
function, already implemented and already in the same directory:

```c
keel_slice_char k_scan_qualified_name(KLexer *lexer, KToken first,
                                       KToken *next_out, TKPpKind *next_pp_kind_out);
```

Here is its **entire real implementation**, `engine/parser_scan_qualified_name.c`,
copied verbatim — read it before writing anything, because every function
below calls the same two functions it calls, the same way:

```c
#include "engine/parser.h"

keel_slice_char k_scan_qualified_name(KLexer *lexer, KToken first,
                                       KToken *next_out, TKPpKind *next_pp_kind_out) {
    KToken last = first;
    KToken tok;

    while (true) {
        tok = k_lexer_next(lexer, next_pp_kind_out);
        if (!k_token_is_punct(tok, ".")) {
            *next_out = tok;
            break;
        }
        last = k_lexer_next(lexer, next_pp_kind_out);
    }

    return (keel_slice_char){ .ptr = first.ptr, .len = (last.ptr + last.len) - first.ptr };
}
```

Notice exactly how it is called: `tok = k_lexer_next(lexer, next_pp_kind_out);`
— **`k_lexer_next` returns the token; its second argument is only where the
directive classification goes, never the token itself.** `KToken` is
`keel_slice_char` (a plain `{ size_t len; char *ptr; }`), not a pointer, and
it has no field named `slice` — `tok` above *is* the token. Every
`k_lexer_next` call you write must have this same shape: a token-typed
variable on the left of `=`, `lexer` and a `TKPpKind *` as the two arguments.

The two other functions this file needs, exactly as declared in
`engine/lexer.h`:

```c
/* the logical spelling is exactly `s`, or begins with `prefix` */
bool k_token_is_ident_named(KToken t, const char *name);
bool k_token_is_string(KToken t);
bool k_token_is_punct(KToken t, const char *spelling);
```

All three return a plain `bool` about `t`'s spelling; none of them consume a
token or advance `lexer` — they only look at the token you already have,
the same way the quoted function above calls `k_token_is_punct(tok, ".")`.

Do not add any other `#include` beyond `"engine/parser.h"`. Assume
well-formed input throughout — the grammar shapes below, nothing malformed.
No diagnostics, no recovery, in this task.

## Shared calling convention

The `..._kw` parameter of every function below is the already-consumed
keyword token (`module`, `import`, or `import_c`) — given to you, not read
by you; you may ignore its value. Each function reads everything after that
keyword, **through and including the terminating `;`**, and hands back the
token that comes *after* the `;` via `*next_out`/`*next_pp_kind_out` — the
exact convention `k_scan_qualified_name` above already uses: it consumes one
token past its own production and returns it that way, so the caller never
calls `k_lexer_next` a second time for a token it already has.

## 1. `k_scan_module_decl` — `'module' module-name ';'`

Declared in `engine/parser.h` as:

```c
typedef struct { keel_slice_char module_name; } KModuleHeader;
void k_scan_module_decl(KLexer *lexer, KToken module_kw, KModuleHeader *out,
                         KToken *next_out, TKPpKind *next_pp_kind_out);
```

Body, following the quoted function above as the pattern:

1. `KToken name_first = k_lexer_next(lexer, next_pp_kind_out);` — the
   name's own first token.
2. `out->module_name = k_scan_qualified_name(lexer, name_first, next_out, next_pp_kind_out);`
   — after this call, `*next_out` holds the token right after the name
   (exactly as `k_scan_qualified_name`'s own doc-comment in `engine/parser.h`
   says), which must be `;`.
3. `*next_out = k_lexer_next(lexer, next_pp_kind_out);` — consumes that `;`
   and leaves `*next_out` holding the token after the whole declaration.

## 2. `k_scan_import` — `'import' module-name ['as' IDENT] ['types'] ';'`

Declared as:

```c
typedef struct {
    keel_slice_char module_name;
    keel_slice_char alias;    /* .len == 0 when no `as` clause */
    bool has_types;
} KImportDecl;
void k_scan_import(KLexer *lexer, KToken import_kw, KImportDecl *out,
                    KToken *next_out, TKPpKind *next_pp_kind_out);
```

Body:

1. Same first two steps as `k_scan_module_decl` above, writing into
   `out->module_name` instead. Initialize `out->alias = (keel_slice_char){0};`
   and `out->has_types = false;`.
2. You now have, in `*next_out`, the token right after the name. Check it
   with `k_token_is_ident_named(*next_out, "as")`:
   - If true: `out->alias = k_lexer_next(lexer, next_pp_kind_out);` (the
     alias itself), then `*next_out = k_lexer_next(lexer, next_pp_kind_out);`
     (the token after the alias).
3. Check `*next_out` (whatever it is now) with
   `k_token_is_ident_named(*next_out, "types")`:
   - If true: `out->has_types = true;`, then
     `*next_out = k_lexer_next(lexer, next_pp_kind_out);`.
4. `*next_out` must now be `;`. Consume it:
   `*next_out = k_lexer_next(lexer, next_pp_kind_out);`.

`as` then `types`, in that order, exactly as the grammar above shows — you do
not need to handle the other order.

## 3. `k_scan_import_c` — `'import_c' (system-header | STRING) ';'`

Declared as:

```c
typedef struct { keel_slice_char header; } KImportCDecl;
void k_scan_import_c(KLexer *lexer, KToken import_c_kw, KImportCDecl *out,
                      KToken *next_out, TKPpKind *next_pp_kind_out);
```

Body:

1. `KToken tok = k_lexer_next(lexer, next_pp_kind_out);`.
2. If `k_token_is_string(tok)`: `out->header = tok;` — a `STRING` token
   already covers its own quotes.
3. Otherwise (the system-header form — `<stdio.h>` tokenizes as five
   separate tokens: `<`, `stdio`, `.`, `h`, `>`): keep track of `first = tok`
   (the `<`) and `last = tok`; loop
   `last = k_lexer_next(lexer, next_pp_kind_out);` until
   `k_token_is_punct(last, ">")` is true for the token you just read; then
   `out->header = (keel_slice_char){ .ptr = first.ptr, .len = (size_t)((last.ptr + last.len) - first.ptr) };`.
4. **Unlike the two functions above, at this point the lexer has not yet
   read `;`** — step 2 stopped exactly at the `STRING` token, and step 3
   stopped exactly at `>`. So this needs two separate reads, in this exact
   order:
   ```c
   KToken semicolon = k_lexer_next(lexer, next_pp_kind_out);
   (void)semicolon;
   *next_out = k_lexer_next(lexer, next_pp_kind_out);
   ```
   The first call reads and discards `;` itself; the second is what leaves
   `*next_out` holding the real token after the whole declaration. Writing
   only one call here is the one bug this task's oracle specifically checks
   for — it asserts `next_out` against a marker token placed right after
   the `;` in the test source, for every one of the three functions.

## Out of scope

- Diagnostics, malformed input, and `dim`/`tags`/`type` binders on `module`
  (a later task).
- Anything about `extern_c`.
