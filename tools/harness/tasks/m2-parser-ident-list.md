---
id: m2-parser-ident-list
output: tools/cgen/src/engine/parser_ident_list.c
acceptance:
  - "sh tools/cgen/test/unit/parser_ident_list.sh"
max_attempts: 5
---

Implement one function of `cgen`'s parser. First line:

```c
#include "engine/parser.h"
```

That header declares the prototype below and everything you call
(`KLexer`, `KToken`, `TKPpKind`, `k_lexer_next`, `k_token_is_punct`). Do not
add any other `#include`.

Here is an already-accepted function in this same directory,
`engine/parser_scan_qualified_name.c`, copied verbatim, showing exactly how
`k_lexer_next` and `k_token_is_punct` are called — **follow this pattern**:

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

`k_lexer_next(lexer, next_pp_kind_out)` **returns** the next token — never
passed a `KToken*` to fill in. `k_token_is_punct(t, spelling)` returns
`bool` and does not consume a token.

## `k_scan_ident_list` — `IDENT { ',' IDENT }`

```c
bool k_scan_ident_list(KLexer *lexer, KToken first, KToken *out, size_t cap,
                        size_t *count_out, KToken *next_out, TKPpKind *next_pp_kind_out);
```

`first` is already the first `IDENT` (given to you). Store tokens into
`out[0]`, `out[1]`, ... as you find them, exactly like `k_scan_qualified_name`
tracks `last` above, except here every identifier is kept, not merged into
one slice:

1. `size_t count = 0;` then `out[count++] = first;` (there is always room:
   `cap` is never 0 in practice).
2. Loop, same shape as the quoted function's `while (true)`:
   - `KToken tok = k_lexer_next(lexer, next_pp_kind_out);`.
   - If `!k_token_is_punct(tok, ",")`: `*next_out = tok;` and stop the loop
     (this is the token after the list).
   - Otherwise: read one more token,
     `KToken ident = k_lexer_next(lexer, next_pp_kind_out);` (assume it is
     an `IDENT`, well-formed input). If `count == cap` at this point,
     set `*count_out = count;` and `return false;` immediately, without
     writing to `out[count]` (it would be out of bounds). Otherwise
     `out[count++] = ident;` and continue the loop.
3. After the loop ends normally (not through the `cap` check above):
   `*count_out = count;` and `return true;`.

Assume well-formed input otherwise (a `,` here is always followed by an
`IDENT`). No diagnostics, no recovery, in this task.

## Out of scope

- Where the list's caller (`dim`/`tags`/`type` on `module`) reads its own
  keyword — this function starts after that keyword, at the first `IDENT`.
