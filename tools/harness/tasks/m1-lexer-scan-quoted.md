---
id: m1-lexer-scan-quoted
output: tools/cgen/src/engine/lexer_scan_quoted.c
acceptance:
  - "sh tools/harness/oracles/m1-lexer-scan-quoted.sh {output}"
max_attempts: 5
---

Implement string/char literal scanning, from `lexer-design.md §6`:
"`scan_quoted` reconhece prefixos `u8`, `u`, `U`, `L`, aspas e escapes, mas
não decodifica. Newline não emendada antes do fechamento... a recuperação
termina a token antes da newline." This is `engine/` — no `#include` of
`stdio.h`, `stdlib.h`, `unistd.h`, `fcntl.h`, or anything under `sys/`.

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
own value (`0`-`255` as an `int`), or `-1` for end of input — a quote
character is a normal, nonzero, successful result. `*width_out` is how
many physical bytes to advance by, already accounting for splices.

## Function

```c
size_t k_lexer_scan_quoted(keel_slice_char source, size_t pos, bool *unterminated_out);
```

**Precondition, guaranteed by the caller:** the logical byte at `pos`
starts a string or char literal — it is the optional prefix (`u8`, `u`,
`U`, or `L`) followed immediately by `"` or `'`, or directly a `"` or `'`
with no prefix. You do not need to re-detect this; scan past it.

Algorithm:

1. **Find and skip the prefix, if any, then the opening quote.** Peek the
   logical byte at `pos`:
   - If it is `"` or `'`: there is **no** prefix. That byte is the
     opener; consume it and move on to step 2.
   - If it is `u`: peek the *next* logical byte. If that one is `8`,
     you have the 2-byte prefix `u8` — consume both, then the opener
     (`"` — `u8` is never followed by `'`, this project has no `u8` char
     prefix) right after. If that one is `"` instead, you have the
     1-byte prefix `u` — consume it, then the opener right after.
   - If it is `U` or `L`: same as the 1-byte `u` case — consume the
     prefix byte, then the opener right after.

   Whichever case applied, remember the opener's character (`"` or `'`)
   — that is the one you are looking for as the closer in step 2.
2. **Consume logical bytes** one at a time until one of:
   - **The closing quote** (the same character as the opener) is found:
     consume it too, and you are done — set `*unterminated_out = false`
     (when the pointer is not NULL) and return the position right after
     it.
   - **A `\` (backslash)**: this is an escape — consume the backslash
     **and** the one logical byte right after it, whatever it is
     (including if that byte happens to be the quote character — an
     escaped quote does **not** close the literal; including if it is
     itself a `\`, which does **not** start a second escape on top of the
     first). Do not try to interpret what the escape means — "não
     decodifica" — just consume both bytes as a pair and keep going.
   - **A raw, non-spliced newline (`\n` or `\r`)**: the literal is
     unterminated. Stop **without** consuming the newline. Set
     `*unterminated_out = true` (when the pointer is not NULL) and return
     the current position — this is the recovery `lexer-design.md`
     describes: "a recuperação termina a token antes da newline". (A
     newline reached *through* a splice never triggers this — a splice
     makes the newline invisible, per `k_lexer_peek_at`'s own contract;
     only a newline `k_lexer_peek_at` actually returns as the next
     logical byte counts.)
   - **End of input**: also unterminated, same as the newline case, but
     there is no newline byte to avoid consuming — just stop. Set
     `*unterminated_out = true` and return the current position (which is
     `source.len`).
3. Otherwise (any other byte): consume it, continue.

`unterminated_out` may be `NULL` — do not write through it if so.

## Worked examples

`source` holding the 4 bytes `"`, `a`, `b`, `"` (the literal `"ab"`),
`pos = 0`: opener `"` at 0, closer character to look for is `"`. Consume
`a`, `b` (ordinary bytes). At offset 3, `"` matches the opener — consume
it too. Returns **4**, `*unterminated_out = false`.

`source = "'a\\'b'"` (`'`, `a`, `\`, `'`, `b`, `'` — 6 bytes), `pos = 0`:
opener `'`. Consume `a`. At offset 2, `\` — this is an escape: consume it
**and** the next byte (`'` at offset 3) as the pair, landing at offset 4.
Consume `b` (offset 4 → 5). At offset 5, `'` matches the opener and is
not part of a prior escape (the previous escape already finished) —
consume it, done. Returns **6**, `*unterminated_out = false`. (Without
the escape rule, a naive scanner would have stopped at offset 3, treating
the escaped quote as the closer — that is the bug this worked example
exists to rule out.)

`source = "\"ab\ncd\""` (`"`,`a`,`b`,`\n`,`c`,`d`,`"` — 7 bytes), `pos =
0`: opener `"`. Consume `a`, `b`. At offset 3, a raw newline — stop
without consuming it. Returns **3**, `*unterminated_out = true`.

## What "correct" means here

- No dynamic allocation, no global state.
- Never index `source.ptr` directly.
- The escape rule applies even at the very end of input: a lone trailing
  `\` with nothing after it consumes just that `\` (its own width) and
  then the next loop iteration finds end of input, which is the
  unterminated case — it does not read past `source.len`.
