---
id: m0-roots-check
output: tools/cgen/src/tool/roots.c
acceptance:
  - "sh tools/harness/oracles/m0-roots-check.sh {output}"
max_attempts: 5
---

Implement path-root containment, used by `cgen` to detect a `.k` source
that lives under more than one `-I` root (`source-in-multiple-roots`,
`cgen-tool-spec.md §4.6`). This is pure string logic — no filesystem
access, the paths need not exist.

## Functions

```c
#include <stdbool.h>

bool cgen_source_under_root(const char *source, const char *root);
bool cgen_source_in_multiple_roots(const char *source, const char *const *roots, int root_count);
```

`cgen_source_under_root(source, root)` is true when `root` "contains"
`source`:

- `source` and `root` are byte-identical, **or**
- `source` starts with `root`, immediately followed by `/`.

**Special case, and the one to get right:** the root `"."` (current
directory) contains **every** source path that does not itself start with
`/` — that is what "current directory" means. `"."` is not a literal
string prefix here: `cgen_source_under_root("src/a.k", ".")` must be
`true`, even though `"src/a.k"` does not start with the character `.`.

`cgen_source_in_multiple_roots(source, roots, root_count)` is true when
`cgen_source_under_root(source, roots[i])` holds for **more than one**
distinct index `i` in `[0, root_count)`. A root that appears twice in
`roots` still counts as covering the source only once — count distinct
matching roots, not matching occurrences. (`-I . -I .` is unusual but not
forbidden by this function; it is not a second root.)

## What "correct" means here

- No dynamic allocation, no global state.
- Case-sensitive, byte-for-byte comparison — no path normalization, no
  resolving `..` or repeated slashes. The caller passes roots and sources
  exactly as written on the command line.
- `root` never ends in `/` when this function is called (the caller
  strips it) — you do not need to handle a trailing slash in `root`.
- A root longer than the source can never contain it (guard the `strncmp`
  length correctly — reading past `source`'s end is a bug even if it
  happens not to crash).
