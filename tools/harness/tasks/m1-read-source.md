---
id: m1-read-source
output: tools/cgen/src/tool/read_source.c
acceptance:
  - "sh tools/harness/oracles/m1-read-source.sh {output}"
max_attempts: 5
---

Implement whole-file reading for `cgen`. This is the first step of
`lexer-design.md §1`: "O `cgen` lê o arquivo para um `buffer char`." The
parser and lexer never open a file themselves (`cgen-tool-spec.md §3`) —
this is the one place that does, on their behalf.

This project compiles under strict `-std=c2x`. `fileno`/`fstat` need a
POSIX feature-test macro defined **before any `#include`**:

```c
#define _POSIX_C_SOURCE 200809L
```

## Function

```c
#include <stdbool.h>
#include "keel/keel_buffer_char.type.h"

bool cgen_read_source(const char *path, keel_buffer_char *out);
```

- Opens `path` for reading, in **binary** mode (`"rb"`) — the bytes must
  reach the lexer exactly as they are on disk, no platform newline
  translation.
- Reads the **entire** file into a single heap-allocated (`malloc`) block
  sized to the file's exact byte length — determine the size first (e.g.
  `fseek`/`ftell`, or `fstat` on the descriptor), then `fread` that many
  bytes in one call. Do not grow the buffer incrementally.
- On success: sets `out->ptr` to the allocated block, `out->len` and
  `out->cap` to the byte count read, and returns `true`. The caller owns
  `out->ptr` and is responsible for `free`ing it later — this function
  never frees it itself.
- On any failure to open, stat, allocate, or read the file (including
  reading fewer bytes than the file's reported size): prints

  ```
  cgen: error: <message> [source-not-found]
  ```

  to stderr (message wording is yours to choose) and returns `false`.
  `*out` is left unmodified — the caller must not read from it after a
  `false` return.
- An **empty file** (zero bytes) is not an error: `out->len = out->cap = 0`,
  `out->ptr` can be `NULL` or a zero-length allocation, either is fine
  (`keel_buffer_char_as_slice` treats a zero-length buffer as an empty
  slice either way — this is what the lexer sees as immediate EOF).
- No partial state on failure: if opening succeeds but a later step fails
  (stat, malloc, short read), close the file and free anything you
  allocated before returning `false`.

## What "correct" means here

- Never read past what `fread` actually returned; never assume it always
  reads the requested count.
- The file descriptor/`FILE *` is always closed before returning, success
  or failure.
- No global state.
