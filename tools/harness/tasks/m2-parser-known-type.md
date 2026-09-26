---
id: m2-parser-known-type
output: tools/cgen/src/engine/parser_known_type.c
acceptance:
  - "sh tools/cgen/test/unit/parser_known_type.sh"
max_attempts: 5
---

Implement one function of `cgen`'s parser: the first that **dispatches** on
a symbol table lookup instead of just recognizing fixed keywords. First
line:

```c
#include "engine/parser.h"
```

That header declares the prototype below, `KSpecifier`, `KSymbolTable`, and
everything you call (`KLexer`, `KToken`, `TKPpKind`, `k_lexer_next`,
`k_symtab_lookup`). Do not add any other `#include`.

`k_lexer_next(lexer, next_pp_kind_out)` **returns** the next token — never
an out-pointer. `k_symtab_lookup`, exactly as declared in
`engine/symtab.h`:

```c
/* Returns the first match in insertion order, or NULL. */
const KSymbol *k_symtab_lookup(const KSymbolTable *t, keel_slice_char name);
```

`KSymbol` has three fields: `keel_slice_char name`, `KSymKind kind`, and
`int arity`. The two kinds this task cares about are `K_SYM_MODIFIER` and
`K_SYM_TYPE`. Assume well-formed input throughout — no diagnostics, no
recovery, in this task.

## `k_scan_known_type`

```c
typedef enum { K_SPEC_NONE, K_SPEC_MODIFIER, K_SPEC_NAMED_TYPE } KSpecifierKind;

typedef struct {
    KSpecifierKind kind;
    keel_slice_char modifier_name;   /* meaningful when kind == K_SPEC_MODIFIER */
    KToken args[4];  size_t arg_count;
    keel_slice_char type_name;       /* meaningful when kind == K_SPEC_NAMED_TYPE */
} KSpecifier;

bool k_scan_known_type(KLexer *lexer, KToken first, const KSymbolTable *symtab,
                        KSpecifier *out, KToken *next_out, TKPpKind *next_pp_kind_out);
```

`first` is already read (an `IDENT`) — you do not call `k_lexer_next` to
obtain it, it is a parameter. Look it up:

```c
const KSymbol *sym = k_symtab_lookup(symtab, first);
```

Then, in this exact order:

1. **`sym == NULL`** (not a known symbol at all — also treat any `kind`
   other than the two below the same way, as not recognized): set
   `out->kind = K_SPEC_NONE;` and **return `true` immediately, without
   reading anything else from `lexer` and without writing to `*next_out`
   at all.** This function has not consumed any token beyond `first`, so
   there is nothing to hand back — the caller already knows exactly where
   the lexer is (right after `first`) and continues from there itself.
   `*next_out` must be left exactly as the caller passed it in.

2. **`sym->kind == K_SYM_MODIFIER`**: set `out->kind = K_SPEC_MODIFIER;`
   and `out->modifier_name = first;`. Read `sym->arity` further tokens,
   one per argument, storing each into `out->args[0]`, `out->args[1]`, ...
   as you go (set `out->arg_count` to how many you stored). If
   `sym->arity` is more than 4, stop after storing the 4th and `return
   false;` immediately (do not read further, do not write `*next_out` —
   this never happens with any module registered so far, but must not
   write out of bounds if it somehow did). Otherwise, after reading all
   `sym->arity` arguments, read one more token into `*next_out` (the token
   right after the last argument), and `return true;`.

3. **`sym->kind == K_SYM_TYPE`**: set `out->kind = K_SPEC_NAMED_TYPE;` and
   `out->type_name = first;`. Read exactly one more token into
   `*next_out`, and `return true;`.

## Worked examples

`symtab` has `"buffer"` registered as `K_SYM_MODIFIER` with `arity = 1`,
and `"arena"` as `K_SYM_TYPE`.

- Source `"xyz rest_token"`, `first = "xyz"` (not in `symtab`): `out->kind
  = K_SPEC_NONE`, and nothing else happens — `*next_out` untouched.
- Source `"buffer i32 xs"`, `first = "buffer"`: found as `K_SYM_MODIFIER`,
  `arity = 1`. Read one argument: `i32` → `out->args[0] = i32`,
  `out->arg_count = 1`. Read one more token: `xs` → `*next_out = xs`.
- Source `"arena x"`, `first = "arena"`: found as `K_SYM_TYPE`. Read one
  more token: `x` → `*next_out = x`.

## Out of scope

- Multi-token arguments, nested modifiers, qualifiers (`const`/`volatile`)
  on an argument, `tagged-type` (`struct`/`union`/`enum` + name) — all
  later tasks.
- What the caller does with `K_SPEC_NONE` (falls through to opaque C,
  elsewhere).
