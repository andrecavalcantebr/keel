---
id: m1-lexer-peek
output: tools/cgen/src/engine/lexer_peek.c
acceptance:
  - "sh tools/harness/oracles/m1-lexer-peek.sh {output}"
max_attempts: 5
---

Implement line-splice-transparent byte peeking, from `lexer-design.md §2-§3`.
This is the primitive every other recognizer in the lexer is built on: a
`\` immediately followed by a line break is a **splice** — invisible to
every comparison the lexer makes (a spliced `ret\<newline>urn` still reads
as the keyword `return`), but the physical bytes are never dropped (the
backend copies them verbatim later). This is `engine/` — no `#include` of
`stdio.h`, `stdlib.h`, `unistd.h`, `fcntl.h`, or anything under `sys/`.
`<stddef.h>` and `<stdbool.h>` are fine.

## Type

```c
#include "keel/keel_slice_char.type.h"
```

`source.ptr[0 .. source.len)` is the whole file; a `size_t pos` is a
**physical** byte offset into it (0 to `source.len` inclusive — `pos ==
source.len` means "at the end").

## Splice recognition

A splice at physical position `pos` is `source.ptr[pos] == '\\'`
immediately followed by one of three line-break spellings:

| Bytes after `\` | Splice width (total, including the `\`) |
| --- | --- |
| `\r\n` | 3 |
| `\n` | 2 |
| `\r` | 2 |

(`\r\n` must be checked before plain `\r`, or you would recognize a 2-byte
splice and misread the `\n` that follows as the start of the *next* logical
character — an off-by-one that only shows up on Windows-style line
endings.) Anything else after the `\` (including end of input right after
it) is not a splice at all.

## Functions

```c
size_t k_lexer_splice_width(keel_slice_char source, size_t pos);
/* Returns the splice width (2 or 3) if a splice starts at `pos`, or 0 if
   there is no splice at `pos`. `pos >= source.len` is always 0 (nothing
   to check). Does not modify anything, does not advance any cursor —
   this is a pure query. */

int k_lexer_peek_at(keel_slice_char source, size_t pos, size_t *width_out);
/* The next LOGICAL byte starting at physical offset `pos`, after
   transparently skipping zero or more consecutive splices (a source can
   have `\` <newline> `\` <newline> `x` — skip both before returning the
   byte for 'x'). Returns the byte's value cast to `int` (0-255), or -1 if
   `pos` is already at or past `source.len` once every splice at or after
   `pos` has been skipped (end of input).

   `*width_out`, when `width_out` is not NULL, receives the number of
   PHYSICAL bytes from `pos` up to and including the returned logical
   byte — that is, `pos + *width_out` is the physical offset where the
   NEXT logical byte would start. On the -1 (EOF) return, `*width_out` is
   set to 0. */
```

**Worked example, because this is the part every prior attempt at this
exact function got wrong:** for `source = "\\\nx"` (backslash, LF, `x` —
3 physical bytes) and `pos = 0`:

- The splice at `pos` is 2 bytes wide (`k_lexer_splice_width` returns 2).
- Skipping those 2 bytes lands on `x` at physical offset 2, which is the
  logical byte to return: `k_lexer_peek_at` returns `'x'` (120).
- `*width_out` must be **3**, not 1: it is the *total* physical distance
  from `pos` (0) to one past the returned byte (3) — the splice's 2 bytes
  **plus** the 1 byte for `x` itself. A `*width_out` that only ever
  counts the final byte and ignores how many bytes the splice-skipping
  loop advanced through is wrong for every case with a splice, even
  though it looks right for the common case with no splice at all (where
  the two happen to coincide, both being 1).

`k_lexer_peek_at` is how a caller advances: read the logical byte, then
move its own cursor forward by `*width_out` — this function never mutates
`source` or any cursor itself, it only computes.

## What "correct" means here

- No dynamic allocation, no global state.
- Never read `source.ptr[i]` for any `i >= source.len`.
- A splice at the very end of the file (`\` as the last byte, nothing
  after it) is not a splice — `k_lexer_splice_width` returns 0, and
  `k_lexer_peek_at` returns the `\` itself as an ordinary logical byte.
