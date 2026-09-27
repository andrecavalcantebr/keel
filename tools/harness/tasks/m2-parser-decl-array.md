---
id: m2-parser-decl-array
output: tools/cgen/src/engine/parser_decl_array.c
acceptance:
  - "make -C tools/cgen"
  - "sh tools/cgen/test/unit/parser_decl_array.sh"
max_attempts: 5
---

# `k_scan_decl_array` — one grammar production of the keel parser

Write the whole of `{output}`. It implements one production of the keel
grammar, `decl-array`, as a recognizer over a live lexer. The file must
begin with exactly this include and nothing else:

```c
#include "engine/parser.h"
```

## The production

Verbatim from the grammar. `IDENT` is an identifier token; everything in
quotes is a literal token; `<opaque>` is any run of tokens whose inner
brackets are balanced.

```ebnf
decl-array   ::= { spec-c } 'array' argument decl-array-1
                 { ',' decl-array-1 } ';'
decl-array-1 ::= { '*' } IDENT dimensions [ '=' <opaque> ]
dimensions   ::= '[' [ <opaque> { ',' <opaque> } ] ']'
              | '[' [ <opaque> ] ']' { '[' <opaque> ']' }
```

`dimensions` has two spellings for the same thing — `[2,3,4]` and
`[2][3][4]` are both rank 3. Your job is only to capture the text; a
later pass decides what it means.

Real examples from the golden cases:

```c
array i32 v[2,3,4];
array u8 r[H, W];
array Person people[4];
array i32 v[4] = {0, 1};
array i32 a[2], b[3];
```

The `{ spec-c }` in front (`static`, `alignas(64)`, ...) is **already
consumed by the caller** — it is not your concern.

## The signature and the struct you fill

Both are already declared in `engine/parser.h`. Do not redeclare them;
define the function. Copied from that header:

```c
#define K_ARRAY_MAX_NAMES 8

typedef struct {
    keel_slice_char element;
    keel_slice_char names[K_ARRAY_MAX_NAMES];
    int pointer_depth[K_ARRAY_MAX_NAMES];
    keel_slice_char dims[K_ARRAY_MAX_NAMES];
    size_t name_count;
} KArrayDecl;

bool k_scan_decl_array(KLexer *lexer, KToken array_kw, KSymbolTable *symtab,
                        KArrayDecl *out, KToken *next_out,
                        TKPpKind *next_pp_kind_out);
```

`array_kw` is the `'array'` token, **already read** before your function
is called. The first token you read is the one after it.

`KToken` and `keel_slice_char` are the same type — a view into the
source, `{ size_t len; char *ptr; }`. A token with `.len == 0` is
end-of-file.

`dims[k]` is that name's whole dimension part **exactly as written,
brackets included**: `[2,3,4]` is seven bytes, `[2][3]` is six, `[]` is
two. Build it as one slice from the first `'['` to the last `']'`.

Register every name into `symtab` as `K_SYM_VARIABLE` with arity `0`, in
the order written.

Return `false`, without writing to `*next_out`, when: the element is not
an argument; a name has no dimensions (the production requires them);
there are more than `K_ARRAY_MAX_NAMES` names; anything but `','` or
`';'` follows a name; or the source ends early. Otherwise consume
through the `';'`, put the token **after** it in `*next_out`, and
return true.

## The functions you call — real declarations and real call sites

Every one already exists and compiles in `tools/cgen/src/engine/`.

### `k_lexer_next` — read the next token

```c
KToken k_lexer_next(KLexer *lexer, TKPpKind *pp_kind);
```

It **returns** the token and advances the lexer. The second parameter is
not the token; it is an unrelated out-parameter you pass through. Real
call site, from `engine/parser_scan_qualified_name.c`:

```c
tok = k_lexer_next(lexer, next_pp_kind_out);
```

### `k_scan_argument` — read the element type

```c
bool k_scan_argument(KLexer *lexer, KToken first, const KSymbolTable *symtab,
                      keel_slice_char *out, KToken *next_out,
                      TKPpKind *next_pp_kind_out);
```

This reads the one `argument` after the `array` marker. **An argument is
often several tokens**: `i32` is one, but `const char`, `struct Person`
and `slice i32` are each a single argument too. `first` is the
argument's first token, already read. `*out` comes back as the whole
argument as written, and `*next_out` is the token after it. It returns
false when what follows is not an argument at all. Real call site, from
`tools/cgen/test/unit/parser_known_type_main.c`:

```c
keel_slice_char arg; KToken next = { 0, NULL };
if (!k_scan_argument(&lexer, first, &symtab, &arg, &next, &pp)) { /* … */ }
```

### `k_scan_opaque_until` — skip an initializer

```c
void k_scan_opaque_until(KLexer *lexer, KToken first,
                          const char *const *terminators, size_t terminator_count,
                          size_t *terminator_index_out,
                          KToken *next_out, TKPpKind *next_pp_kind_out);
```

It skips tokens, counting `(`/`[`/`{` up and `)`/`]`/`}` down, until it
reads one of `terminators` **at depth 0**. Real call site, from
`engine/parser_decl_keel.c`, skipping exactly this kind of initializer:

```c
KToken init_first = k_lexer_next(lexer, next_pp_kind_out);
static const char *const terms[] = { ",", ";" };
size_t idx;
k_scan_opaque_until(lexer, init_first, terms, 2, &idx, next_out, next_pp_kind_out);
```

**`*next_out` comes back holding the terminator itself, not the token
after it.** Measured on `{0, 1}, b[3]; REST` with those two terminators:
`*next_out` is `,` and `idx` is 0 — and the comma *inside* the braces
was skipped, because it was at depth 1. So after this call you are
standing **on** the `,` or the `;`, and you must not read it again.

### Token predicates and the symbol table

```c
bool k_token_is_ident(KToken t);        /* an identifier, and NOT a C keyword */
bool k_token_is_punct(KToken t, const char *spelling);
bool k_symtab_insert(KSymbolTable *t, keel_slice_char name, KSymKind kind, int arity);
```

`k_symtab_insert` returns false only when the table is full. Real call
site, from `engine/parser_modifier_decl.c` (a different kind):

```c
return k_symtab_insert(symtab, out->name, K_SYM_MODIFIER, module_arity);
```

## A worked example, byte by byte

Source, with `'array'` already consumed by the caller:

```
array i32 v[2,3,4]; REST
0         1         2
0123456789012345678901234
```

| field | value | `.ptr` at | `.len` |
| --- | --- | --- | --- |
| `out->element` | `i32` | 6 | 3 |
| `out->name_count` | 1 | — | — |
| `out->names[0]` | `v` | 10 | 1 |
| `out->pointer_depth[0]` | 0 | — | — |
| `out->dims[0]` | `[2,3,4]` | 11 | 7 |
| `*next_out` | `REST` | 20 | 4 |

`dims[0]` starts at 11 because that is the `'['`, and its length is 7
because the `']'` is at 17 and is one byte long: `18 - 11 = 7`. The
brackets are **inside** the slice.

Note `*next_out` is `REST` at 20 — **past** the `';'` at 18, not on it.
Getting this field wrong is the most common way this kind of function
fails; check it against this table before you finish.

A second example, with a pointer and an initializer:

```
array const char *p[3] = {0}, q[]; REST
```

gives `element` = `const char`, two names: `p` with
`pointer_depth` 1 and `dims` `[3]`, then `q` with `pointer_depth` 0 and
`dims` `[]`. `*next_out` is `REST`.

## What "correct" means beyond the oracle

- No global or `static` mutable state; the function is re-entrant.
  (A `static const char *const` array of terminator strings, as in the
  call site above, is fine — it is not mutable.)
- Never read past the end of the source. Every token you read can be
  end-of-file (`.len == 0`); when it is, return `false` rather than
  looping. A loop that keeps reading after end-of-file hangs forever,
  and the oracle kills it after 10 seconds and scores it a failure.
- Never write past index `K_ARRAY_MAX_NAMES - 1` in any of the three
  arrays. Return `false` instead. The oracle compiles with
  AddressSanitizer, so an overrun is a hard failure.
- Walk the names and the bracket groups with plain loops. Do not build
  any table or index; there are at most eight names.

## Out of scope

Do not write any of this — it does not exist, or it belongs elsewhere:

- The `{ spec-c }` prefix. The caller consumed it.
- Interpreting the dimensions. You capture their text; deciding rank,
  evaluating `H` or `W`, and checking bounds are later passes.
- Interpreting the initializer. It is skipped whole.
- Any diagnostic. Do not call anything from `engine/diag.h`, do not
  print, do not build a message. Signalling a malformed declaration is
  `return false`, nothing more.
- Any file I/O, allocation, or `exit`. `engine/` may not include
  `<stdio.h>`, `<stdlib.h>`, `<unistd.h>` or any `<sys/*.h>`; the build
  has a check that fails if it does.
- Changing `engine/parser.h` or any other existing file. Write only
  `{output}`.
