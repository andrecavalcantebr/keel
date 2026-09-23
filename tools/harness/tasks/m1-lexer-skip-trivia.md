---
id: m1-lexer-skip-trivia
output: tools/cgen/src/engine/lexer_skip_trivia.c
acceptance:
  - "sh tools/harness/oracles/m1-lexer-skip-trivia.sh {output}"
max_attempts: 5
---

Implement trivia skipping, from `lexer-design.md §4`: "`skip_trivia`
consome espaço horizontal, `//` até newline lógica e `/* ... */` até o
primeiro `*/`." This is `engine/` — no `#include` of `stdio.h`,
`stdlib.h`, `unistd.h`, `fcntl.h`, or anything under `sys/`.

**Scope, precisely:** this function skips horizontal whitespace and both
comment forms — nothing else. It does **not** consume a bare newline
(newline handling and the `line_clean` flag are a different function's
job — not yours here). It stops the instant it finds a byte that is
neither horizontal whitespace nor the start of a comment, including when
that byte is a newline.

## Includes your file needs, at the top

```c
#include <stddef.h>
#include "keel/keel_slice_char.type.h"
```

## Building block already implemented

```c
int k_lexer_peek_at(keel_slice_char source, size_t pos, size_t *width_out);
```

From `tools/cgen/src/engine/lexer_peek.c` — declare it `extern` and use it
for **every** byte you look at.

**Its return value is not a status code — read this carefully, it is easy
to get backwards.** It returns the logical byte's own value, `0` to `255`
(cast to `int`), or `-1` specifically for end of input. There is no
separate "ok" flag: a space character (`' '`, value 32) is a completely
normal, successful result, and it is **not** zero. Checking `if
(k_lexer_peek_at(...) != 0)` to mean "failed" is wrong — that would treat
almost every real byte, including every letter and every space, as
failure. The only value that means "nothing more to read" is exactly
`-1`:

```c
size_t width;
int c = k_lexer_peek_at(source, pos, &width);
if (c < 0) {
    /* end of input */
} else if (c == ' ' || c == '\t') {
    /* an ordinary logical byte, here a space or tab */
}
```

It is splice-transparent: a comment can
itself contain a splice (`// x \<newline>module y;` keeps `module` inside
the logical comment line — this is a real case from `keel-spec.md §2.7`'s
own acceptance table), and if you read `source.ptr[pos]` directly instead
of going through `k_lexer_peek_at`, you will treat a spliced newline
inside a comment as ending it early. Never index `source.ptr` yourself in
this function.

## Function

```c
size_t k_lexer_skip_trivia(keel_slice_char source, size_t pos);
```

Returns the physical offset after skipping zero or more, in any mixture
and order, of:

1. **Horizontal whitespace** — one or more consecutive space (`' '`) or
   tab (`'\t'`) logical bytes.
2. **A `//` comment** — `/` immediately followed by another `/`, then
   every logical byte up to (**not including**) the next logical newline
   (`'\n'` or `'\r'`) or end of input. The comment's own two `/` bytes and
   everything after them up to that point are all consumed; the newline
   itself, if there is one, is left for the caller.
3. **A `/* */` comment** — `/` immediately followed by `*`, then every
   logical byte up to and including the next `*/` pair. If there is no
   closing `*/` before end of input, consume everything to the end —
   this is a real source error, but reporting it is out of scope here (no
   diagnostic call, no special return value, just stop at `source.len`).

Keep looping over these three cases until none of them match at the
current position — trivia can repeat and mix (` // a comment\n   /* b */`
is whitespace, then a line comment, stopping before the newline you must
NOT consume).

## Worked example

For `source = "  //x\n/**/y"` (2 spaces, `//x`, newline, `/**/`, `y` — 11
bytes) and `pos = 0`:

- Skip the 2 spaces: now at offset 2.
- `//` at offset 2: skip `/`, `/`, `x`, stopping right before the `\n` at
  offset 5 (not consuming it). `k_lexer_skip_trivia` does **not** return
  yet — a `\n` is not a byte this function recognizes as "still trivia",
  so this is where the loop's next iteration finds nothing more to skip
  and the function returns.
- Result: **5** (the offset of the `\n`, not consumed).

A second call starting from offset 6 (past the newline, which the caller
consumed itself) would skip the `/**/` (4 bytes, offsets 6-9) and return
**10**, the offset of `y`.

## What "correct" means here

- No dynamic allocation, no global state.
- Never index `source.ptr` directly — every byte is read through
  `k_lexer_peek_at`, so splices inside whitespace runs and comments are
  transparent.
- `pos == source.len` (already at end of input) returns `source.len`
  immediately — nothing to skip.
