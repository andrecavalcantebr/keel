---
id: m0-args-partition
output: tools/cgen/src/tool/main.c
acceptance:
  - "make -C tools/cgen"
  - "sh tools/harness/oracles/m0-args-partition.sh"
max_attempts: 5
---

Implement `main()` for `cgen`, a driver in the style of `gcc`: it either
transpiles a `.k` source, or — with none given — transparently forwards to
the C compiler. This task covers only argument partition, `--cgen-version`,
transparent link, and three invocation errors. Everything else is out of
scope (see below) — leave it as an unimplemented stub that still compiles.

This project compiles under strict `-std=c2x` (no GNU extensions). Under
that mode, glibc hides `execvp`'s prototype unless a POSIX feature-test
macro is defined **before any `#include`** — the very first line of the
file must be:

```c
#define _POSIX_C_SOURCE 200809L
```

## The cgen option table

These are the tool's own options. Every one of them is **recognized and
consumed** — it never reaches the passthrough list, even if this task does
not yet act on its value:

| Option | Forms | Value |
| --- | --- | --- |
| `-I` | `-I dir`, `-Idir` | repeatable; **also** appended to the passthrough list (it is read by both sides) |
| `--dest-dir` | `--dest-dir dir`, `--dest-dir=dir` | path |
| `--base-dir` | `--base-dir dir`, `--base-dir=dir` | path |
| `--instance` | `--instance "U"`, `--instance="U"` | string |
| `--stop-after` | `--stop-after=F`, `--stop-after F` | `lex` \| `parse` \| `gen` |
| `--checks` | `--checks=V`, `--checks V` | `on` \| `off` |
| `--line` | `--line=V`, `--line V` | `on` \| `off` |
| `--main` | `--main M`, `--main=M` | module name |
| `--cc` | `--cc=P`, `--cc P` | program name, default `cc` |
| `--pedantic-names` | (flag) | — |
| `--parallel-lowering` | `=V` or ` V` | `auto` \| `serie` \| `openmp` |
| `--profile` | `=V` or ` V` | `auto` \| `c11` \| `c23` |
| `-f` | (flag) | — |
| `--cgen-version` | (flag) | prints `cgen 0.1.0` to stdout, exits 0 |
| `--cgen-help` | (flag) | out of scope for this task — just print nothing and exit 0 |

**`-I` is the one option with a single dash, and `cgen_match_long_option`
does not — and must not be made to — recognize it: that function only
matches the `--name` / `--name=value` shape, and `-I` is neither.** Give it
its own check, before you try `cgen_match_long_option` against anything:

```c
if (strcmp(w, "-I") == 0) {
    /* separate form: -I dir — the value is the next word */
    roots[root_count++] = argv[++i];
    passthrough[passthrough_count++] = w;
    passthrough[passthrough_count++] = roots[root_count - 1];
} else if (strncmp(w, "-I", 2) == 0 && w[2] != '\0') {
    /* glued form: -Idir — the value is everything after -I, same word */
    roots[root_count++] = w + 2;
    passthrough[passthrough_count++] = w;
}
```

Both forms append the **whole original word(s)** to the passthrough list
(that is the "read by both sides" from the table) as well as recording the
root. This is the only option that goes to both lists — every other cgen
option is consumed and never reaches passthrough at all.

Both forms (`--opt value` and `--opt=value`) are accepted for every long
option with a value. A value outside the enumerated set for `--profile`,
`--checks`, `--line`, or `--parallel-lowering` is the `invalid-option`
error (only `--profile` is exercised by the acceptance check, but implement
the validation generally — it is the same check for all four).

## Words that consume the next word, unless glued

This fixed list comes from `cc`. When one of these appears **without** a
glued value (`-o x`, not `-ox`), it and the next word travel together in
the passthrough list, and the next word is never examined as a possible
`.k` source:

```
-o  -MF  -MT  -MQ  -I  -D  -U  -L  -l  -x  -include  -imacros  -isystem
-iquote  -idirafter  -iprefix  -iwithprefix  -isysroot  -Xlinker
-Xassembler  -Xpreprocessor  -u  -T  -z  --param  -aux-info
```

## Partition algorithm

Recognizing whether `w` names a given cgen option, in either form
(`--name value` or `--name=value`), is **already implemented** in
`tools/cgen/src/tool/match_option.c` — declare and call it for step 1, do
not hand-roll `strcmp`/`strchr('=')` matching yourself:

```c
bool cgen_match_long_option(const char *arg, const char *name, bool *has_value, const char **value);
```

It tells you whether `arg` names the option and, for the glued form, the
value — but it never looks at the *next* `argv` word. For the separate
form (`*has_value == false`), consuming the next word as the value is
still this task's job: check it exists (end of argv with a value-less
match on an option that requires one is `invalid-option`) and take it.

For each word `w` of `argv[1..]`, in order:

1. If `w` matches a cgen option from the table above, by trying
   `cgen_match_long_option(w, name, ...)` against each: consume it — glued
   value from `*value`, or the next word if `*has_value` came back false.
2. Else if `w` is in the list above and has no glued value: append `w` and
   the next word to the passthrough list, in that order; do not examine the
   next word further.
3. Else if `w` starts with `-`: append `w` to the passthrough list.
4. Else if `w` ends in `.k`: record it as a source (remember it, do not
   append to passthrough).
5. Else: append `w` to the passthrough list (a `.c`/`.o`/`.a`/... argument).

Do not implement `-W<name>` catalog filtering (§2.3 of the tool spec) —
that needs a diagnostic table this task does not have. Treat every `-W...`
as an ordinary word starting with `-` (step 3): it always passes through.
This is a deliberate, temporary simplification, not a bug to fix here.

After the loop: if zero `-I` were given, the passthrough list still needs
nothing added — the default root is `.`, used only for source-root checks,
never emitted as an actual `-I` to the child compiler.

## What to do with the result

- **Both `.k` sources found (2 or more):** error `multiple-sources`, exit 2.
- **Exactly one `.k` source, under more than one `-I` root:** error
  `source-in-multiple-roots`, exit 2. Root containment is **already
  implemented** in `tools/cgen/src/tool/roots.c` — declare and call it,
  do not reimplement the check:

  ```c
  bool cgen_source_under_root(const char *source, const char *root);
  bool cgen_source_in_multiple_roots(const char *source, const char *const *roots, int root_count);
  ```

  Build the `roots` array from every `-I` given (default `"."` when none
  were given, since the default root only matters for this check — it is
  never emitted to the child compiler); then call
  `cgen_source_in_multiple_roots(k_file, roots, root_count)`.
- **An enumerated option got a value outside its set:** error
  `invalid-option`, exit 2.
- **`--cgen-version` given:** print `cgen 0.1.0\n` to stdout, exit 0. (Give
  this priority over everything else — check it first.)
- **Zero `.k` sources, no `--instance`, and not `--cgen-version`:**
  transparent link. Build `argv` for the child as: the program from
  `--cc` (default `"cc"`), followed by the passthrough list, in order,
  nothing added or removed. Call `execvp` with it. `execvp` replacing the
  process is exactly how the child's exit code becomes ours — do not
  `fork`/`waitpid` and re-exit manually.
- **Exactly one `.k` source, no error above:** out of scope for this task.
  Print `cgen: not yet implemented\n` to stderr and exit 1. Do not attempt
  to open the file or call into any lexer/parser — none exists yet.

## Error message format

**Write one function and route every error through it — do not call
`fprintf(stderr, ...)` directly at each error site.** A format requirement
repeated by hand at a dozen call sites is a format requirement that gets
forgotten at one of them:

```c
static void fatal(const char *diagnostic_id, const char *message) {
    fprintf(stderr, "cgen: error: %s [%s]\n", message, diagnostic_id);
    exit(2);
}
```

Every one of the three errors below is a call to `fatal("multiple-sources", "...")`,
`fatal("invalid-option", "...")`, `fatal("source-in-multiple-roots", "...")` —
never a bare `fprintf` + `exit(2)` pair. The brackets around the id are not
optional decoration; the acceptance check greps for the literal text
`[multiple-sources]` (and the other two ids) in stderr, and a message
missing that exact bracketed suffix fails even if the wording is
otherwise sensible. The wording before the id is yours to choose.

## Out of scope for this task

- `--base-dir` resolution logic (a separate task owns it).
- `--cgen-help` output text.
- `-W<name>` catalog filtering.
- Anything about `--stop-after`, `--instance`, `--main`, actually reading
  or lexing a `.k` file.
- Reading `.k` file existence (`source-not-found` etc.) — none of that is
  checked here.
