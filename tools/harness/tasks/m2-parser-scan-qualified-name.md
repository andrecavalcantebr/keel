---
id: m2-parser-scan-qualified-name
output: tools/cgen/src/engine/parser_scan_qualified_name.c
acceptance:
  - "sh tools/cgen/test/unit/parser_scan_qualified_name.sh"
max_attempts: 5
---

Implement `k_scan_qualified_name`, the first recognizer of `cgen`'s parser.
It scans the grammar production `qualified-name ::= IDENT { '.' IDENT }` —
the dotted name shared by a module's own name, the module path of `import`,
and every dotted type or verb reference in keel source.

The very first line of the file must be:

```c
#include "engine/parser.h"
```

This one header is enough. It already declares everything you need:
`KLexer`, `KToken`, `TKPpKind`, `keel_slice_char`, `k_lexer_next`,
`k_token_is_punct`, and this function's own exact prototype. Do not add any
other `#include`.

## The exact prototype you are implementing

```c
keel_slice_char k_scan_qualified_name(KLexer *lexer, KToken first,
                                       KToken *next_out, TKPpKind *next_pp_kind_out);
```

- `first` is already an `IDENT` token — the caller's own previous call to
  `k_lexer_next`, not yet consumed by you. You do not call `k_lexer_next` to
  obtain it; it is a parameter.
- `next_out` and `next_pp_kind_out` are never `NULL`.
- The input is always well-formed: a `.` immediately after an identifier in
  this position is always followed by another `IDENT`. You do not need to
  detect or handle the malformed case (a `.` followed by something else) —
  that has no diagnostic yet and is out of scope for this task.

## The return-value contracts you are building on

**`k_lexer_next(lexer, &pp_kind)`** — already implemented, in
`engine/lexer.c` — returns the next token from `lexer`, advancing it.
**An empty token (`.len == 0`) means end of file.** It is not an error and
you must not treat it as one; it is a perfectly normal value to receive and
to hand onward through `next_out`.

**`k_token_is_punct(token, spelling)`** — already implemented, in
`engine/token_predicates_shape.c` — returns `true` when `token`'s spelling is
exactly `spelling`. It is safe to call on an empty (EOF) token: it reads
nothing out of bounds and simply returns `false`. You do not need to check
`token.len` yourself before calling it.

## Algorithm

Because seeing `.` after an identifier always means another identifier
follows (input is well-formed, see above), you never need to look more than
one token ahead, and you never need to undo a read:

1. `last = first`.
2. Loop:
   a. `tok = k_lexer_next(lexer, next_pp_kind_out)`.
   b. If `tok` is **not** the punctuation `.` (`k_token_is_punct(tok, ".")`
      is `false`): this is the token after the name. Store it into
      `*next_out`, and stop the loop.
   c. Otherwise (`tok` **is** `.`): the name continues. Read the identifier
      that must follow it: `last = k_lexer_next(lexer, next_pp_kind_out)`.
      Go back to step 2a.
3. Return the slice from `first`'s first byte to `last`'s last byte:
   `{ .len = (last.ptr + last.len) - first.ptr, .ptr = first.ptr }`.

Note step 2c calls `k_lexer_next` a second time inside the same loop
iteration, writing over `*next_pp_kind_out` again — that is intentional and
correct: `*next_pp_kind_out` only needs to reflect the token that ends up in
`*next_out`, which is written last, in step 2b of whichever iteration
actually stops the loop.

**The returned slice includes any trivia (spaces, comments) between the
tokens of the name — it is not stripped.** Only trivia strictly *after* the
name (before the token that becomes `*next_out`) is excluded, because that
trivia is skipped by the `k_lexer_next` call that reads `*next_out`, not
included in any token's own span.

## Worked example (exact bytes)

Source: `"app . cfg ;"` (11 bytes, 0-indexed):

```
index:  0   1   2   3   4   5   6   7   8   9   10
byte:   a   p   p   _   .   _   c   f   g   _   ;
```

(`_` marks a space.) Caller already has `first = { .ptr = &src[0], .len = 3 }`
(`"app"`). Your function then does:

1. `tok = k_lexer_next(...)` → skips the space at index 3, reads `.` at index
   4 → `{ .ptr = &src[4], .len = 1 }`. It **is** `.`, so:
   `last = k_lexer_next(...)` → skips the space at index 5, reads `cfg` at
   index 6 → `last = { .ptr = &src[6], .len = 3 }`.
2. Loop again: `tok = k_lexer_next(...)` → skips the space at index 9, reads
   `;` at index 10 → `{ .ptr = &src[10], .len = 1 }`. It is **not** `.`, so
   `*next_out = tok` (the `;`), and the loop stops.
3. Return `{ .len = (&src[6] + 3) - &src[0], .ptr = &src[0] }` =
   `{ .len = 9, .ptr = &src[0] }` — the 9 bytes `"app . cfg"`, **not**
   including the space before `;`.

## Out of scope for this task

- Any recovery, diagnostic, or defined behavior for malformed input (a `.`
  not followed by an `IDENT`, or EOF in the middle of the name after a
  `.`). Assume it never happens.
- Anything about `module`, `import`, or any other grammar construction that
  *uses* this recognizer — this task only implements the recognizer itself.
- A `KParser`/lookahead-buffer type. This function takes `KLexer` directly
  and has no state of its own.
