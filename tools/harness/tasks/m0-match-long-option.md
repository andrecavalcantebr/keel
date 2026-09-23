---
id: m0-match-long-option
output: tools/cgen/src/tool/match_option.c
acceptance:
  - "sh tools/harness/oracles/m0-match-long-option.sh {output}"
max_attempts: 5
---

Implement long-option name matching for `cgen`. Every cgen option with a
value accepts two forms (`cgen-tool-spec.md §4.1`, `design/cgen-tool.md`
D2): `--name value` (separate word) and `--name=value` (glued). This
function decides only whether `arg` spells the option `name`, and extracts
the value when it is the glued form — it does not look at any other
argument, and does not consume anything.

## Function

```c
#include <stdbool.h>

bool cgen_match_long_option(const char *arg, const char *name, bool *has_value, const char **value);
```

- Returns `false` if `arg` does not name this option at all. In that case
  `*has_value` and `*value` are left untouched.
- Returns `true` if `arg` is exactly `"--" + name` (the separate-word
  form): `*has_value = false`, `*value = NULL`. The caller is the one who
  decides whether to consume the next `argv` word — this function never
  looks past `arg`.
- Returns `true` if `arg` is `"--" + name + "="` followed by anything,
  including nothing (the glued form, `--name=` with an empty value is
  still the glued form): `*has_value = true`, `*value` points at the text
  after the `=` (it may be an empty string, never `NULL`).
- **Must not prefix-match.** `cgen_match_long_option("--ccache", "cc", ...)`
  is `false` — `"--ccache"` is not `"--cc"` and not `"--cc="` followed by
  anything; matching on `strncmp` alone without checking the boundary
  character (`=` or end of string) right after the name is the bug this
  task exists to avoid.
- `arg` without a leading `--` (a short option like `"-cc"`, or a bare
  word like `"cc"`) never matches, regardless of `name`.

## What "correct" means here

- No dynamic allocation.
- `*value`, when set, points into the original `arg` string (a suffix of
  it) — do not copy.
- `name` never itself contains `=` or a leading `-`; you can rely on that,
  it is always the bare option name like `"cc"` or `"dest-dir"`.
