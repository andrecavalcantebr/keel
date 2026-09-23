---
id: m1-lexer-scan-punct
output: tools/cgen/src/engine/lexer_scan_punct.c
acceptance:
  - "sh tools/harness/oracles/m1-lexer-scan-punct.sh {output}"
max_attempts: 5
---

Implement punctuator scanning, from `lexer-design.md §6`: "`scan_punct`
usa maior grafia primeiro... Grafia não reconhecida avança ao menos um
byte e fica preservada para o compilador C." This is `engine/` — no
`#include` of `stdio.h`, `stdlib.h`, `unistd.h`, `fcntl.h`, or anything
under `sys/`.

**Scope note:** this excludes the rare C digraphs (`<:` `:>` `<%` `%>`
`%:` `%:%:`) and trigraphs on purpose — they are essentially unused in
real source and the fallback rule (an unrecognized byte still advances
and is preserved) handles them safely as plain 1-byte punctuators if they
ever appear, just without special multi-byte recognition. Not a bug to
fix here.

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
own value (`0`-`255` as an `int`), or `-1` for end of input. `*width_out`
is how many physical bytes to advance by for that one logical byte,
already accounting for any line splice — a punctuator can itself contain
a splice (`<\<newline><` is still the two-character punctuator `<<`, just
spanning more physical bytes).

## The tables, longest first

```c
static const char *const k_punct_3[] = { "...", "<<=", ">>=" };

static const char *const k_punct_2[] = {
    "->", "++", "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||",
    "*=", "/=", "%=", "+=", "-=", "&=", "^=", "|=", "##", ".."
};

static const char *const k_punct_1[] = {
    "[", "]", "(", ")", "{", "}", ".", "&", "*", "+", "-", "~", "!", "/",
    "%", "<", ">", "^", "|", "?", ":", ";", "=", ",", "#"
};
```

(`..` in `k_punct_2` is keel's own range operator, not standard C — it is
in the table for the same reason as every other 2-byte spelling: it has
to be tried before the 1-byte `.` so `2..7` doesn't misread as two
separate dots.)

## Function

```c
size_t k_lexer_scan_punct(keel_slice_char source, size_t pos);
```

At `pos`, read up to 3 **logical** bytes (through `k_lexer_peek_at`,
stopping early at end of input) and try, in order:

1. Do the first 3 logical bytes spell an entry of `k_punct_3`? If so,
   consume all 3 (the sum of their three physical widths) and return.
2. Else, do the first 2 logical bytes spell an entry of `k_punct_2`? If
   so, consume both and return.
3. Else, does the first logical byte spell an entry of `k_punct_1`? If
   so, consume it and return.
4. Else (the byte matched nothing — this is the fallback): consume just
   that one logical byte's physical width anyway, and return. This
   function never fails and never returns `pos` unchanged.

Fewer than 3 (or 2) logical bytes available before end of input just
means those longer candidates cannot match — fall through to the shorter
checks normally, do not treat running out of input as an error.

Return the physical offset one past what you consumed.

## Worked examples

`source = "<<=x"` (4 bytes), `pos = 0`: the first 3 logical bytes are
`<`, `<`, `=` — that spells `k_punct_3`'s `"<<="`. Consume all 3 physical
bytes (no splices here, so width 1 each). Returns **3**.

`source = "<\\\n<x"` (`<`,`\`,`\n`,`<`,`x` — 5 bytes), `pos = 0`: logical
byte 1 is `<` (width 1). Logical byte 2, read from physical offset 1, is
`<` again — but it is reached through the splice at offset 1 (`\` +
`\n`), so its width is 3, not 1. Logical byte 3 (from physical offset 4)
is `x`. The first two logical bytes spell `<<`, which is in
`k_punct_2` — `...` (3-byte candidates) needs a third logical byte of
`=`, which `x` is not, so step 1 does not match; step 2 does. Consume the
physical span of both logical bytes: `1 + 3 = 4`. Returns **4** — `x` (at
physical offset 4) is left for the next call.

`source = "@x"` (2 bytes), `pos = 0`: `@` matches none of the three
tables (it is not a C punctuator keel recognizes here). Step 4, the
fallback, consumes its 1-byte width anyway. Returns **1**.

## What "correct" means here

- No dynamic allocation, no global state beyond the three `static const`
  tables above.
- Never index `source.ptr` directly.
- Compare logical bytes by value (`int` from `k_lexer_peek_at`), not by
  reading raw memory — a multi-byte candidate can straddle a splice, so
  there is no contiguous run of physical bytes to `memcmp` against the
  table entries.
