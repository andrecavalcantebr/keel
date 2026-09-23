---
id: m1-token-predicates-shape
output: tools/cgen/src/engine/token_predicates_shape.c
acceptance:
  - "sh tools/harness/oracles/m1-token-predicates-shape.sh {output}"
max_attempts: 5
---

Implement the by-shape classification predicates from `lexer-design.md
§5-§6`: recognizing a number, string, char, or exact punctuator by
inspecting the first byte(s) of an already-scanned token's spelling. This
is `engine/`, not `tool/` — no `#include` of `stdio.h`, `stdlib.h`,
`unistd.h`, `fcntl.h` or anything under `sys/`. `<stdbool.h>` and
`<string.h>` are both fine and expected.

## Type

```c
#include "keel/keel_slice_char.type.h"

typedef keel_slice_char KToken;
```

`t.ptr` is **not NUL-terminated** — never call `strcmp`/`strlen` on it
directly; use `t.len` and `memcmp`/manual byte comparison.

## Functions

```c
#include <stdbool.h>

bool k_token_is_number(KToken t);
/* true iff t's spelling starts with a decimal digit, or with '.'
   immediately followed by a decimal digit — the pp-number shape from
   lexer-design.md §6. Do not attempt to validate the rest of the number
   (exponents, hex floats, etc.) — the lexer's own scanner already only
   ever produces tokens with a valid shape; this predicate only needs to
   recognize that shape from the first byte(s). */

bool k_token_is_string(KToken t);
/* true iff t's spelling is a string literal: optionally starts with a
   prefix (`u8`, `u`, `U`, or `L`) immediately followed by `"`. */

bool k_token_is_char(KToken t);
/* true iff t's spelling is a char literal: optionally starts with a
   prefix (`u`, `U`, or `L` — NOT `u8`, C has no u8 char prefix) followed
   by `'`. */

bool k_token_is_punct(KToken t, const char *spelling);
/* true iff t's spelling equals `spelling` exactly (byte for byte —
   `spelling` is a NUL-terminated C string, `t` is not). */
```

## What "correct" means here

- No dynamic allocation, no global mutable state.
- `k_token_is_string`/`k_token_is_char` only look at the prefix and the
  opening quote character — they do not scan for the closing quote or
  validate escapes; that already happened when the token was scanned.
- An empty token (`t.len == 0`) is none of these — always `false`, never
  read `t.ptr[0]` when `t.len == 0`.
