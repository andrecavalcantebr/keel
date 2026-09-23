---
id: m1-lexer-scan-number
output: tools/cgen/src/engine/lexer_scan_number.c
acceptance:
  - "sh tools/harness/oracles/m1-lexer-scan-number.sh {output}"
max_attempts: 5
---

Implement pp-number scanning, from `lexer-design.md §6`. This is
`engine/` — no `#include` of `stdio.h`, `stdlib.h`, `unistd.h`, `fcntl.h`,
or anything under `sys/`.

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
own value (`0`-`255` as an `int`), or `-1` for end of input — a digit is a
normal, nonzero, successful result. `*width_out` is how many physical
bytes to advance by, already accounting for any line splice.

## Function

```c
size_t k_lexer_scan_number(keel_slice_char source, size_t pos);
```

**Precondition, guaranteed by the caller:** the logical byte at `pos` is
already known to start a number — a decimal digit, or `.` immediately
followed by a decimal digit. Your job is only to find where it ends.

Starting at `pos`, keep consuming logical bytes, checking each one **in
this order**:

1. **A decimal digit (`0`-`9`) or an ASCII letter (`a`-`z`, `A`-`z`) or
   `_`** — consume it, continue. (This is deliberately permissive: it is
   what lets `0x1fp3` and `1e6` and even nonsense like `1abc` all scan as
   one pp-number token — validating that it is a *real* number is not
   this function's job, nor keel's; the C compiler does that.)

2. **`.`** — look at the byte immediately after it first. If *that* byte
   is also `.`, **do not consume this `.`** — stop right here, before it.
   (This is what lets `2..7` scan as the number `2`, then keel's `..`
   range operator, then the number `7` — a run of two or more dots never
   belongs to a number.) Otherwise, consume the `.` and continue.

3. **`+` or `-`** — consume it **only if the immediately preceding
   consumed byte was `e`, `E`, `p`, or `P`** (case-sensitive, exactly
   those four) — this is what lets an exponent's sign (`1e-6`, `0x1p+3`)
   stay part of the number, while a bare `+`/`-` anywhere else stops the
   scan. You need to remember the last byte you consumed to check this.

4. **Anything else** (including end of input): stop. Return the current
   position — do not consume the byte that stopped you.

Return the physical offset one past the end of the number.

## Worked examples

`source = "2..7"` (4 bytes), `pos = 0`: `2` is a digit, consume (`pos` →
1). At `pos = 1` the byte is `.`; the byte right after it (`pos = 2`) is
also `.` — rule 2's exception fires, stop **without** consuming. Returns
**1** — just the `"2"`. (A separate call to the punctuator scanner, not
built here, would then take the `..`, and a third call to this function
starting at `pos = 3` would scan the `"7"`.)

`source = "1e-6"` (4 bytes), `pos = 0`: `1` (digit) → `pos` 1. `e`
(letter) → `pos` 2, last consumed byte is now `e`. `-`: last consumed was
`e`, so rule 3 applies — consume it → `pos` 3. `6` (digit) → `pos` 4, end
of input. Returns **4** — the whole string is one number.

`source = "0x1.fp3"` (7 bytes), `pos = 0`: `0`,`x`,`1` consume in turn
(`pos` → 3). At `pos = 3` the byte is `.`; the byte after it (`pos = 4`)
is `f`, not `.` — no exception, consume the `.` normally (`pos` → 4).
`f`, `p` consume as letters (`pos` → 6, last consumed is now `p`). `3`:
digit, rule 1 applies regardless of what came before — consume (`pos` →
7), end of input. Returns **7** — the whole string.

## What "correct" means here

- No dynamic allocation, no global state.
- Never index `source.ptr` directly.
- Checking rule 2's "byte after the dot" still goes through
  `k_lexer_peek_at`, at the physical offset the `.` itself ended at
  (`pos + width_of_the_dot`) — not `pos + 1` by assumption, in case a
  splice sits between them.
