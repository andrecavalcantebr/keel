---
id: m1-lexer-scan-identifier
output: tools/cgen/src/engine/lexer_scan_identifier.c
acceptance:
  - "sh tools/harness/oracles/m1-lexer-scan-identifier.sh {output}"
max_attempts: 5
---

Implement identifier scanning, from `lexer-design.md §3` (the scanner
list) and the identifier shape already used by
`tools/cgen/src/engine/token_predicates_words.c` (`k_token_is_ident`).
This is `engine/` — no `#include` of `stdio.h`, `stdlib.h`, `unistd.h`,
`fcntl.h`, or anything under `sys/`.

## Includes your file needs, at the top

```c
#include <stddef.h>
#include <stdbool.h>
#include "keel/keel_slice_char.type.h"
```

## Building block already implemented

```c
int k_lexer_peek_at(keel_slice_char source, size_t pos, size_t *width_out);
```

From `tools/cgen/src/engine/lexer_peek.c` — declare it `extern`, use it
for every byte, never index `source.ptr` yourself.

**Its return value is not a status code.** It returns the logical byte's
own value (`0`-`255` as an `int`), or `-1` specifically for end of input.
A letter or digit is a normal, nonzero, successful result — there is no
separate "ok" flag to check. The only value that means "stop, nothing
more to read" is exactly `-1`.

`*width_out` (when the pointer given is not NULL) is how many physical
bytes to advance by to reach the *next* logical byte — this already
accounts for any line splice (`\` + newline) transparently, so a spliced
identifier like `my\<newline>_var` still scans as one continuous
identifier `my_var`, just spanning more physical bytes than logical ones.

## Function

```c
size_t k_lexer_scan_identifier(keel_slice_char source, size_t pos);
```

**Precondition, guaranteed by the caller (you do not need to check it):**
the logical byte at `pos` is already known to be a valid identifier start
— an ASCII letter, `_`, or the `\` of a `\uXXXX`/`\UXXXXXXXX` universal
character name. Your job is only to find **how far the identifier
extends**, not to validate that it's an identifier at all.

Starting at `pos`, keep consuming logical bytes that continue an
identifier, in this order of preference at each position:

1. A `\` immediately followed by `u` and exactly 4 hex digits (`0-9`,
   `a-f`, `A-F`), or by `U` and exactly 8 hex digits — a universal
   character name. Consume the whole thing (6 or 10 logical bytes) as one
   step.
2. An ASCII letter (`a-z`, `A-Z`), digit (`0-9`), or `_`.
3. Anything else: stop. Return the current position — **do not consume**
   the byte that stopped you.

Return the physical offset one past the end of the identifier (so
`source.ptr[pos .. returned)` — read through `k_lexer_peek_at`, not
`source.ptr` directly — is the identifier's physical span, splices
included).

A malformed universal character name (a `\u` or `\U` not followed by
enough hex digits) is **not** rule 1 — it does not match, so it falls to
rule 3 and stops the scan right there, before the `\`. This can only
happen after at least one earlier byte was already consumed (the caller
guarantees `pos` itself is valid), so the function never returns `pos`
unchanged.

## Worked examples

For `source = "my_var2 x"` (9 bytes) and `pos = 0`: consumes `m` `y` `_`
`v` `a` `r` `2` (all rule 2), stops at the space (rule 3, not consumed).
Returns **7**.

For `source = "my\\\n_x"` (`m`,`y`,`\`,`\n`,`_`,`x` — 6 bytes) and `pos =
0`: `m` and `y` consume 1 byte each (`pos` → 2). At `pos = 2` there is a
splice (`\` + `\n`, width 2) before the logical byte `_` — `k_lexer_peek_at`
already collapses this, returning `_` with `*width_out = 3`; that is one
step of rule 2, `pos` → 5. `x` consumes 1 more, `pos` → 6, end of input.
Returns **6** — the whole 6-byte source, because the splice is invisible
to what counts as "still the same identifier".

## What "correct" means here

- No dynamic allocation, no global state.
- Never index `source.ptr` directly.
