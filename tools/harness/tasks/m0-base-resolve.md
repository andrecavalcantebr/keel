---
id: m0-base-resolve
output: tools/cgen/src/tool/base_resolve.c
acceptance:
  - "sh tools/harness/oracles/m0-base-resolve.sh {output}"
max_attempts: 5
---

Implement base-directory resolution for `cgen`, Linux only (no macOS,
FreeBSD or Windows branches — this task is scoped to `/proc/self/exe`).

This project compiles under strict `-std=c2x` / `-std=c11` (no GNU
extensions). Under that mode, glibc hides `readlink`, `PATH_MAX` and
`strdup` unless a POSIX feature-test macro is defined **before any
`#include`** — the very first line of the file must be:

```c
#define _POSIX_C_SOURCE 200809L
```

## Function

```c
char *cgen_resolve_base_dir(const char *explicit_base_dir);
```

Returns a heap-allocated (`malloc`) C string with the resolved base
directory path, or `NULL` on failure (after printing a diagnostic — see
below). The caller owns the returned string.

## Resolution, in this order

1. If `explicit_base_dir` is not `NULL`: the candidate directory is
   `explicit_base_dir`, used as given (do not resolve symlinks or make it
   absolute).
2. Else: resolve the real path of the running executable via
   `readlink("/proc/self/exe", ...)` (the kernel gives the real target, not
   the symlink text — this is *not* the same as reading `argv[0]`). The
   candidate directory is `<directory containing that real executable
   path>/../lib/base`. You do not need to collapse the `..` — a path that
   still contains it is fine, as long as it names the right directory.

`readlink` does not NUL-terminate the buffer it fills — size the buffer
generously (e.g. `PATH_MAX` from `<limits.h>`) and terminate it yourself
using the return value (the byte count written). Treat a `readlink` failure
the same as "candidate has no `keel.k`" (case below) — do not crash, and
still print the diagnostic.

## Validating the candidate

Whichever candidate was produced above (there is only ever one — this is
not "try the explicit one, then fall back to the default if it fails"),
check whether `<candidate>/keel.k` exists (e.g. `access()` or `stat()`). If
it does not:

```
cgen: error: base directory not found (no keel.k under '<candidate>') [base-not-found]
```

to stderr, and return `NULL`. If it does exist, return the candidate path
(heap-allocated).

## What "correct" means here

- No global mutable state.
- `explicit_base_dir` is never modified or freed by this function — it is
  the caller's string.
- The returned string, when not `NULL`, is safe to `free()`.
- Do not read or parse `keel.k`'s contents — only check that the file
  exists.
