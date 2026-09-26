---
id: m2-parser-extern-c
output: tools/cgen/src/engine/parser_extern_c.c
acceptance:
  - "sh tools/cgen/test/unit/parser_extern_c.sh"
max_attempts: 5
---

Implement two functions of `cgen`'s parser, in this one file, where the
second calls the first. The first line must be:

```c
#include "engine/parser.h"
```

That header declares both prototypes below and everything you call
(`KLexer`, `KToken`, `TKPpKind`, `keel_slice_char`, `k_lexer_next`,
`k_token_is_punct`). Do not add any other `#include`.

Here is a function already accepted in this same directory,
`engine/parser_scan_qualified_name.c`, copied verbatim, to show exactly how
`k_lexer_next` and `k_token_is_punct` are actually called and used —
**follow this pattern, do not invent a different one**:

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

**`k_lexer_next(lexer, next_pp_kind_out)` returns the next token — it is
never passed a `KToken*` to fill in.** `KToken` is `keel_slice_char` (a
plain `{ size_t len; char *ptr; }`), not a pointer, and has no field named
`slice`. `k_token_is_punct(t, spelling)` returns `bool`, does not consume a
token, and is safe on any token including EOF.

Assume well-formed input throughout — the grammar shapes below, nothing
malformed. No diagnostics, no recovery, in this task.

## 1. `k_scan_braced_opaque` — a balanced `'{' ... '}'` region

Declared as:

```c
keel_slice_char k_scan_braced_opaque(KLexer *lexer, KToken open,
                                      KToken *next_out, TKPpKind *next_pp_kind_out);
```

`open` is the already-consumed `'{'` token (given to you, not read by you).
Only `'{'` and `'}'` tokens matter for finding where this region ends —
track an `int depth`, starting at `1` (for `open`). Algorithm, one token at
a time, no lookahead beyond the current token:

1. Read a token: `KToken tok = k_lexer_next(lexer, next_pp_kind_out);`.
2. If `k_token_is_punct(tok, "{")`: increment `depth` (this token is still
   part of the content — do not treat it specially beyond that).
3. Else if `k_token_is_punct(tok, "}")`: decrement `depth`. **If `depth` is
   now `0`, this token is the match for `open` — stop the loop right here,
   without adding this token to the content.**
4. Otherwise (any other token — including if you just did step 2, or did
   step 3 but `depth` is still above `0`): this token is content. Remember
   it as `last`; if this is the first content token seen, also remember it
   as `first`.
5. If the loop did not stop, go back to step 1.

After the loop stops: `*next_out = k_lexer_next(lexer, next_pp_kind_out);`
— the token right after the closing `'}'`.

**Return value:** if no content token was ever seen (the body was empty,
`{}`), return `(keel_slice_char){ .ptr = open.ptr + open.len, .len = 0 }`.
Otherwise return
`(keel_slice_char){ .ptr = first.ptr, .len = (size_t)((last.ptr + last.len) - first.ptr) }`
— `first`/`last` are the `first`/`last` remembered in step 4, exactly the
same expression shape the quoted function above uses for its own return.

Worked example, source `"{ a { b } c } ok"` (space-separated for clarity;
`open` is the very first `{`, already consumed): tokens read by this
function's own loop, in order, are `a`, `{`, `b`, `}`, `c`, `}`, `ok`.
- `a`: not a brace → content, `first = last = a`.
- `{`: `depth` → 2 → content too (step 2 falls through to step 4) →
  `last = {`.
- `b`: content → `last = b`.
- `}`: `depth` → 1 (not 0) → content too → `last = }`.
- `c`: content → `last = c`.
- `}`: `depth` → 0 → **stop, this token is not content**.
- Then one more `k_lexer_next` reads `ok` into `*next_out`.

Result: the slice from `first` (`a`) to `last` (`c`) — the text
`"a { b } c"` — and `*next_out` is `ok`.

## 2. `k_scan_extern_c` — `[ 'pub' | 'priv' ] 'extern_c' [ '[' 'type_h' ']' ] '{' <opaque> '}'`

Declared as:

```c
typedef struct {
    bool has_type_h;
    keel_slice_char body;
} KExternCDecl;
void k_scan_extern_c(KLexer *lexer, KToken extern_c_kw, KExternCDecl *out,
                      KToken *next_out, TKPpKind *next_pp_kind_out);
```

Any `pub`/`priv` and the `extern_c` keyword itself are already consumed by
the caller — `extern_c_kw` is given to you, unused. Body:

1. `KToken tok = k_lexer_next(lexer, next_pp_kind_out);`; set
   `out->has_type_h = false;`.
2. If `k_token_is_punct(tok, "[")`: set `out->has_type_h = true;`, then read
   and discard two more tokens (`type_h`, then `]` — well-formed input
   means you do not need to check their spelling), then
   `tok = k_lexer_next(lexer, next_pp_kind_out);` — now `tok` is the `'{'`.
3. `out->body = k_scan_braced_opaque(lexer, tok, next_out, next_pp_kind_out);`
   — this one call also takes care of `*next_out` for you; you do not call
   `k_lexer_next` again after it.

## Out of scope

- Anything about the names declared inside the `extern_c` body (a later
  task — the body is opaque C for now, per keel-spec §2.3).
- The `pub`/`priv` prefix itself — a different function's job.
