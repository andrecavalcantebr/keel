---
id: m2-parser-extent-decl
output: tools/cgen/src/engine/parser_extent_decl.c
acceptance:
  - "make -C tools/cgen"
  - "sh tools/cgen/test/unit/parser_extent_decl.sh"
max_attempts: 5
---

# `k_scan_extent_decl` — one grammar production of the keel parser

Write the whole of `{output}`. It implements one production of the keel
grammar, `decl-extent`, as a recognizer over a live lexer. The file must
begin with exactly this include and nothing else:

```c
#include "engine/parser.h"
```

## The production

This is the grammar, verbatim. `IDENT` is an identifier token, `NUM` a
number token; everything in quotes is a literal token.

```ebnf
decl-extent   ::= 'extent' 'struct' IDENT extent-dim { extent-dim }
                  '{' { extent-field } '}' ';'
extent-dim    ::= '[' IDENT ',' ( qualified-name | NUM ) ']'
qualified-name ::= IDENT { '.' IDENT }
```

There is always **at least one** `extent-dim`; `{ extent-dim }` means
"zero or more *more* of them" after the first. `{ extent-field }` is the
body, which this function does **not** interpret — it is kept as an
opaque slice for a later task.

Three real examples, from `golden/cases/020-extent/app/pos.k`:

```c
pub extent struct position [len, cap] { array f32 *x; bool active; };
pub extent struct grid [rows, rcap] [cols, ccap] { array i32 *v; };
pub extent struct image [h, H] [w, W] { array u8 r[H, W]; };
```

## The signature and the struct you fill

Both are already declared in `engine/parser.h`. Do not redeclare them;
just define the function. This is the declaration, copied from that
header:

```c
#define K_EXTENT_MAX_DIMS 4

typedef struct {
    keel_slice_char name;                         /* the IDENT after 'struct' */
    KToken dim_names[K_EXTENT_MAX_DIMS];          /* each dimension's binder */
    keel_slice_char dim_caps[K_EXTENT_MAX_DIMS];  /* its capacity, as written */
    size_t dim_count;
    keel_slice_char body;                         /* between '{' and '}' */
} KExtentDecl;

bool k_scan_extent_decl(KLexer *lexer, KToken extent_kw, KSymbolTable *symtab,
                         KExtentDecl *out, KToken *next_out,
                         TKPpKind *next_pp_kind_out);
```

`extent_kw` is the `'extent'` token, **already read** from the lexer
before your function is called. Any `pub`/`priv` before it was already
read too. So the first thing your function reads is the token after
`'extent'`, which must be `'struct'`.

`KToken` and `keel_slice_char` are the same type — a view into the
source, `{ size_t len; char *ptr; }`. A token with `.len == 0` is
end-of-file.

Return `false`, **without writing to `*next_out`**, on anything that does
not fit the production: no `'struct'` after `'extent'`, no dimension at
all, more than `K_EXTENT_MAX_DIMS` dimensions, no `';'` after the body,
or the source ending early. Otherwise consume through the `';'`, put the
token *after* it in `*next_out`, and return what `k_symtab_insert`
returns.

## The functions you call — real declarations and real call sites

Every one of these already exists, compiles, and is already used by
accepted code in `tools/cgen/src/engine/`. Use them exactly as shown.

### `k_lexer_next` — read the next token

```c
KToken k_lexer_next(KLexer *lexer, TKPpKind *pp_kind);
```

It **returns** the token and advances the lexer. The second parameter is
not the token; it is an unrelated out-parameter you only need to pass
through. A real, accepted call site, from
`engine/parser_scan_qualified_name.c`:

```c
tok = k_lexer_next(lexer, next_pp_kind_out);
```

### `k_scan_braced_opaque` — consume a `{ ... }` body

```c
keel_slice_char k_scan_braced_opaque(KLexer *lexer, KToken open,
                                      KToken *next_out, TKPpKind *next_pp_kind_out);
```

`open` is the already-read `'{'`. It counts brace depth, so inner `{`/`}`
inside the body do not end it early. It returns the slice of everything
strictly between the braces — the closing `'}'` is consumed but is not
part of the slice. An empty body `{}` returns a slice with `.len == 0`
whose `.ptr` is still valid. It puts the token right after the closing
`'}'` into `*next_out`. A real, accepted call site, from
`engine/parser_struct_decl.c`:

```c
out->body = k_scan_braced_opaque(lexer, tok, next_out, next_pp_kind_out);
```

### `k_scan_qualified_name` — read `IDENT { '.' IDENT }`

```c
keel_slice_char k_scan_qualified_name(KLexer *lexer, KToken first,
                                       KToken *next_out, TKPpKind *next_pp_kind_out);
```

`first` is the already-read first `IDENT`. It returns the slice spanning
the whole dotted name, and puts the token after the name into
`*next_out`. For a name with no dot, the slice is just that one token and
`*next_out` is the token after it. A real, accepted call site, from
`engine/parser_tags_decl.c`:

```c
keel_slice_char name = k_scan_qualified_name(lexer, tok, &tok, next_pp_kind_out);
```

### Token predicates

```c
bool k_token_is_ident(KToken t);        /* an identifier, and NOT a C keyword */
bool k_token_is_number(KToken t);       /* a number literal */
bool k_token_is_punct(KToken t, const char *spelling);
bool k_token_is_c_word_named(KToken t, const char *name);
```

`k_token_is_ident` returns **false** for C keywords, and `struct` is a C
keyword — so test for it with `k_token_is_c_word_named(tok, "struct")`,
not with `k_token_is_ident`. Punctuation is tested by spelling:
`k_token_is_punct(tok, "[")`.

### `k_symtab_insert` — register the declared name

```c
bool k_symtab_insert(KSymbolTable *t, keel_slice_char name, KSymKind kind, int arity);
```

It returns `false` only when the table is full. Register the extent's
name with kind `K_SYM_TYPE` and arity `0`. A real, accepted call site,
from `engine/parser_modifier_decl.c` (that one uses a different kind):

```c
return k_symtab_insert(symtab, out->name, K_SYM_MODIFIER, module_arity);
```

## A worked example, byte by byte

Source, with `'extent'` already consumed by the caller:

```
extent struct pos [len, cap] { bool active; } ; REST
0         1         2         3         4         5
0123456789012345678901234567890123456789012345678901
```

Every field of the result, with exact offsets:

| field | value | `.ptr` at | `.len` |
| --- | --- | --- | --- |
| `out->name` | `pos` | 14 | 3 |
| `out->dim_count` | 1 | — | — |
| `out->dim_names[0]` | `len` | 19 | 3 |
| `out->dim_caps[0]` | `cap` | 24 | 3 |
| `out->body` | `"bool active;"` | 31 | 12 |
| `*next_out` | `REST` | 48 | 4 |

The body runs from the first byte of its first token to the last byte
of its last token: `bool` starts at 31 and the `;` ends at 42, so
`43 - 31 = 12`. The spaces against the braces (30, and 43) are **not**
in it; the space between `bool` and `active` is. This is exactly what
`k_scan_braced_opaque` returns, so use it and do not adjust its result.

Note that `*next_out` is `REST`, at 48: **past** the `';'` at 46, not on
it. Getting this one field wrong is the most common way this kind of
function fails, so check it against this example before you finish.

## What "correct" means beyond the oracle

- No global or `static` mutable state; the function is re-entrant.
- Never read past the end of the source. Every token you read can be
  end-of-file (`.len == 0`); when it is, return `false` rather than
  looping. A loop that keeps reading after end-of-file hangs forever,
  and the oracle kills it after 10 seconds and scores it a failure.
- Never write past `dim_names[K_EXTENT_MAX_DIMS - 1]` or
  `dim_caps[K_EXTENT_MAX_DIMS - 1]`. Return `false` instead. The oracle
  compiles with AddressSanitizer, so an overrun is a hard failure.
- Walk the dimensions with a plain loop over the tokens. Do not build any
  table or index; there are at most four.

## Out of scope

Do not write any of this — it does not exist, or it belongs to a later
task:

- Interpreting the body. `extent-field`, `extent-column`, `array` columns
  and `column-decl` are a later task; the body stays the opaque slice
  that `k_scan_braced_opaque` returns.
- Any diagnostic. Do not call anything from `engine/diag.h`, do not
  print, and do not build a message. Signalling a malformed declaration
  is `return false`, nothing more.
- Registering anything other than the extent's own name — not the
  dimension binders, not any field.
- Any file I/O, allocation, or `exit`. `engine/` may not include
  `<stdio.h>`, `<stdlib.h>`, `<unistd.h>` or any `<sys/*.h>`; the build
  has a check that fails if it does.
- Changing `engine/parser.h` or any other existing file. Write only
  `{output}`.
