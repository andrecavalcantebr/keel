---
id: m1-token-predicates-words
output: tools/cgen/src/engine/token_predicates_words.c
acceptance:
  - "sh tools/harness/oracles/m1-token-predicates-words.sh {output}"
max_attempts: 5
---

Implement word classification from `lexer-design.md §4-§5`: is a token an
exact match for one of the fixed C keyword spellings, or a valid C
identifier that is not one of them. This is `engine/`, not `tool/` — no
`#include` of `stdio.h`, `stdlib.h`, `unistd.h`, `fcntl.h` or anything
under `sys/`. `<stdbool.h>` and `<string.h>` are both fine and expected —
`<string.h>` is not I/O, it is pure memory comparison (`memcmp`), and you
will want it.

## Type

```c
#include "keel/keel_slice_char.type.h"

typedef keel_slice_char KToken;
```

`t.ptr` is **not NUL-terminated** — never call `strcmp`/`strlen` on it
directly. Compare `t.len` against the candidate's length first, then
`memcmp` the bytes.

## Table

```c
static const char *const k_c_words[] = {
    "_Alignas", "_Alignof", "_Atomic", "_BitInt", "_Bool", "_Complex",
    "_Decimal128", "_Decimal32", "_Decimal64", "_Generic", "_Imaginary",
    "_Noreturn", "_Static_assert", "_Thread_local",
    "alignas", "alignof", "auto", "bool", "break", "case", "char", "const",
    "constexpr", "continue", "default", "do", "double", "else", "enum",
    "extern", "false", "float", "for", "goto", "if", "inline", "int", "long",
    "nullptr", "register", "restrict", "return", "short", "signed", "sizeof",
    "static", "static_assert", "struct", "switch", "thread_local", "true",
    "typedef", "typeof", "typeof_unqual", "union", "unsigned", "void",
    "volatile", "while"
};
```

**Use a plain linear scan over this table (`for` loop comparing each
entry) — not a binary search.** The table has under 70 entries; there is
nothing to gain from a sorted-array binary search here, and it is a
needless place to introduce an ordering bug (comparing lengths before
comparing bytes silently breaks the sort assumption a binary search
depends on). A linear scan has no such assumption to get wrong.

## Functions

```c
#include <stdbool.h>

bool k_token_is_c_word(KToken t);
/* true iff t's spelling exactly matches an entry of k_c_words. */

bool k_token_is_ident(KToken t);
/* true iff t is a valid C identifier spelling (ASCII letters, digits, '_',
   not starting with a digit; plus \uXXXX and \UXXXXXXXX universal
   character names using exactly 4 / 8 hex digits, as C allows inside
   identifiers) AND it does NOT match any entry of k_c_words. An
   identifier and a C keyword are mutually exclusive by definition here. */

bool k_token_is_ident_named(KToken t, const char *name);
/* true iff k_token_is_ident(t) and t's spelling equals `name` exactly. */

bool k_token_is_c_word_named(KToken t, const char *name);
/* true iff k_token_is_c_word(t) and t's spelling equals `name` exactly. */
```

## What "correct" means here

- No dynamic allocation, no global mutable state beyond the `static const`
  table above.
- A token identical in bytes to a `k_c_words` entry is a C word, never an
  identifier — the two predicates must never both return true for the same
  token.
