---
id: m1-lexer-scan-directive
output: tools/cgen/src/engine/lexer_scan_directive.c
acceptance:
  - "sh tools/harness/oracles/m1-lexer-scan-directive.sh {output}"
max_attempts: 5
---

Implement preprocessor directive scanning, from `lexer-design.md §4`: "Um
`#` chama `scan_directive` somente com `line_clean == true`. A função
devolve uma única `KToken` até a próxima newline lógica não emendada,
incluindo a quebra física quando houver." This is `engine/` — no
`#include` of `stdio.h`, `stdlib.h`, `unistd.h`, `fcntl.h`, or anything
under `sys/`.

## Includes your file needs, at the top

```c
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "keel/keel_slice_char.type.h"
```

## Building block already implemented

```c
int k_lexer_peek_at(keel_slice_char source, size_t pos, size_t *width_out);
```

From `tools/cgen/src/engine/lexer_peek.c` — declare it `extern`, use it
for every byte, never index `source.ptr` yourself. It returns the logical
byte's own value (`0`-`255`), or `-1` for end of input — not a status
code. `*width_out` is the physical byte count to advance by for that one
logical byte.

## Type

```c
typedef enum {
    TK_PP_OTHER,
    TK_PP_IF,     /* #if, #ifdef, #ifndef */
    TK_PP_ELSE,   /* #elif, #elifdef, #elifndef, #else */
    TK_PP_ENDIF   /* #endif */
} TKPpKind;
```

## Function

```c
size_t k_lexer_scan_directive(keel_slice_char source, size_t pos, TKPpKind *pp_kind_out);
```

**Precondition, guaranteed by the caller:** the logical byte at `pos` is
`#`, and the caller has already confirmed `line_clean` was true (you do
not check that here). Two things this function does, in either order,
since they do not interfere with each other:

**A. Classify the directive**, into `*pp_kind_out` (when the pointer is
not NULL):

1. Skip the `#` itself.
2. Skip any horizontal whitespace (space `' '` or tab `'\t'`) right after
   it — `# if` is valid and equivalent to `#if`.
3. Read the run of ASCII letters that follows (there may be none — `#`
   alone, or `#123`, is valid too) into a small local buffer, **up to 8
   bytes** (`"elifndef"` is the longest keyword this classifies, at 8
   letters — stop reading letters once you have 8, you do not need more
   to tell the keywords apart, and do not risk a buffer overrun).
4. Compare that word against a **table**, exactly the same pattern already
   used in `tools/cgen/src/engine/token_predicates_words.c` (`strlen` +
   `memcmp`, looped) — **do not** write out `if (keyword_len == N &&
   keyword[0] == '…' && keyword[1] == '…' ...)` chains by hand, one `if`
   per keyword. That style is error-prone to transcribe (a previous attempt
   at this exact task miscounted `"ifdef"` as 6 letters instead of 5, and
   scrambled the letter order of `"ifndef"`) and gains nothing here — the
   table is 8 entries, a linear scan over it is trivial:

   ```c
   static const struct { const char *word; TKPpKind kind; } k_pp_words[] = {
       { "if",       TK_PP_IF    },
       { "ifdef",    TK_PP_IF    },
       { "ifndef",   TK_PP_IF    },
       { "elif",     TK_PP_ELSE  },
       { "elifdef",  TK_PP_ELSE  },
       { "elifndef", TK_PP_ELSE  },
       { "else",     TK_PP_ELSE  },
       { "endif",    TK_PP_ENDIF },
   };
   ```

   Loop over `k_pp_words`, and for each entry compare with `strlen(entry.word)
   == keyword_len && memcmp(entry.word, keyword, keyword_len) == 0`. First
   match wins (there are no ambiguous prefixes among these eight words, so
   order does not matter). No match, or no letters at all (`keyword_len ==
   0`, e.g. bare `#` or `#123`), or a word not in the table (`"define"`,
   `"include"`, `"pragma"`, ...) → `TK_PP_OTHER`.

**B. Find the end of the token**, independent of the classification: the
whole directive, from the `#` through **and including** the next logical
newline (`'\n'` or `'\r'`) — or through end of input, if there is no more
newline. This is the entire physical/logical line, not just up through
the keyword: `#if X\n` is one token of all 6 bytes, not just `#if`.
Return the physical offset one past that point (one past the newline, or
`source.len` if there was none).

Splices apply throughout, in both parts — `#en\<newline>dif\n` still
classifies as `TK_PP_ENDIF` (the word, read logically, is `endif`), and
the newline that ends the *token* is the real, unspliced one at the very
end, not the one consumed inside the splice partway through.

## Worked example

`source = "#if X\n"` (6 bytes: `#`,`i`,`f`,` `,`X`,`\n`), `pos = 0`:

- Classification: skip `#` (offset 1), no whitespace to skip, read
  letters `i`,`f` (stops at the space, which is not a letter) — the word
  is `"if"`, so `*pp_kind_out = TK_PP_IF`.
- End of token: starting back at `pos = 0`, scan forward through every
  byte until the newline — found at offset 5 — and include it. The token
  ends at offset 6.

Returns **6**.

## What "correct" means here

- No dynamic allocation, no global state.
- Never index `source.ptr` directly.
- The keyword buffer is a small fixed-size local array (8 bytes is
  enough) — never write past it even if the run of letters is longer
  (just stop reading into the buffer at 8, but you may still need to
  keep consuming/skipping the extra letters as part of finding where the
  word ends, if that matters to your approach — the buffer bound is only
  about not overflowing, not about where the classification word
  actually ends in the source).
