---
name: cgen-harness-task
description: Write a task card for the local model (qwen3-coder:30b-a3b-q4_K_M, via tools/harness/loop.py) to implement one function/file of tools/cgen/src/engine or tool, and prepare the oracle it will be accepted against — including hand-writing a new expected dump (tools/cgen/test/parse/<case>.parse) and wiring it into cases.txt when the parser (M2+) needs a golden case it does not yet cover. Use when André asks to delegate a piece of cgen work to the local model, or to extend parse_dump.sh coverage for a new golden case.
---

# cgen harness task — writing a card for the local model

`tools/harness/` runs `qwen3-coder:30b-a3b-q4_K_M` against self-contained task
cards (`tasks/*.md`) and an acceptance command, without Claude in the retry
loop (`tools/harness/README.md`, "Execução sem o Claude no laço", since
2026-09-22). Your job here is **level 2, done in advance**: write the task
card and, if needed, the oracle it is accepted against — then hand both to
André. You do not run `loop.py` yourself unless he explicitly asks you to.

## Before writing a task at all

Delegating only pays off when the task is **small, mechanical, and the
contract has no ambiguity you can already see** — see
`tools/harness/README.md` §"O balanço de custo não fechou como esperado":
tasks that hit ambiguity cost more in Claude-side diagnosis than writing the
function by hand would have. If the target needs judgment calls (e.g. "decide
how the container-position rule interacts with X"), write the code yourself
instead of building a task. Good targets are one function, whose contract is
already fixed in a normative document or design doc — a recognizer, a table
lookup, a single grammar production.

## Where things are

| Path | What |
| --- | --- |
| `tools/harness/tasks/*.md` | one task card per file — the format below |
| `tools/harness/loop.py` | the retry loop; you don't invoke it, André does |
| `tools/harness/README.md` | the whole design, the 4 lessons, state log — read it if you haven't in this session |
| `tools/cgen/test/` | the oracle suite (`make -C tools/cgen check`); task acceptance commands point here now, not at `tools/harness/oracles/` (moved 2026-09-25) |
| `tools/cgen/test/parse_dump.sh [header\|decl\|inst\|ilha]` | the parser oracle (M2+): compares `cgen --stop-after=parse` against hand-written dumps |
| `tools/cgen/test/parse/cases.txt` | `<case-dir> <entry.k>` pairs the dump runner iterates over |
| `tools/cgen/test/parse/<case>.parse` | the hand-written expected dump for that case, full `ilha` level; `parse_dump.sh` filters it down per level |
| `golden/cases/<NNN-name>/` | the `.k` sources and hand-written `expected/` C the dumps must agree with |
| `design/cgen-tool.md` §5.2 | the exact dump format and field meaning, per island kind |
| `design/parser-design.md` §3.2 | the etapa table (header → decl → inst → ilha, by golden case) — use it to know what a given task should cover next |
| `tools/cgen/src/engine/*.c`, `*.h` | **real, compiling, already-accepted API** — every function a new task reuses (`k_lexer_next`, `k_token_is_*`, `k_scan_qualified_name`, ...) has a real declaration and at least one real call site somewhere in here. Quote it; do not paraphrase it (see the lesson below). |
| `tools/cgen/gen/keel/*.h`, `base/`, `golden/` | the keel standard library itself, already generated to real C (`keel_arena`, `keel_buffer_*`, `keel_outcome_*`, `keel_slice_*`) — a large body of this project's own C idiom, underused as reference material so far. Worth a `grep` before describing any pattern from memory. |

## Writing the task card

Frontmatter, same shape as any existing card (see `tasks/m0-args-partition.md`
or `tasks/m1-read-source.md` for full examples):

```yaml
---
id: m2-<slug>
output: tools/cgen/src/engine/<file>.c
acceptance:
  - "make -C tools/cgen"
  - "sh tools/cgen/test/parse_dump.sh header"
max_attempts: 5
---
```

`{output}` in an acceptance command is substituted with the real path.
Acceptance commands run at the repo root, in order; the first failure's
output feeds the next attempt. Point at the real suite
(`make -C tools/cgen`, `sh tools/cgen/test/parse_dump.sh <level>`, or a
narrower `sh tools/cgen/test/...` script), never at something bespoke.

The body (English, plain prose with `##` sections — no literal "Contrato"
heading needed, follow the style of the existing cards) has to be
**self-contained**: the model never reads `keel-spec.md` or `design/*.md`. So:

- **Paste the relevant grammar/contract excerpt verbatim**, not "see spec
  §2.2" — quote the EBNF production, the dump-format row, or the design
  paragraph the function has to satisfy.
- **State what "correct" means beyond the acceptance command** — invariants
  the oracle can't prove by itself (e.g. "no global mutable state", "does not
  read past `source.len`").
- **End with an explicit "Out of scope" list** — this is what stops the model
  from inventing calls into code that doesn't exist yet.

### Rule zero: quote real code for every API the task reuses — never paraphrase it

Before writing a single line of prose about what an existing function does,
`grep` the repo for it and copy what you find, verbatim, into the task —
its declaration (with doc-comment) from the `.h`, **and** at least one real
call site from a `.c` that already compiles and has already passed a task's
oracle. Prefer a call site from code the local model itself generated and
that was accepted (e.g. `engine/parser_scan_qualified_name.c`) — it is proof
the exact syntax you are about to ask for again already worked once. Never
compose your own pseudocode paraphrase of a signature you could instead
paste.

This is not optional polish; it is what `m2-parser-header` (2026-09-26)
proved the hard way. The task described `k_lexer_next` in prose only
("returns the next token, advancing `lexer`") and the model spent all 5
attempts inventing a wrong convention for it (treating the second parameter
as an out-pointer for the token itself, and a nonexistent `.slice` member on
`KToken`) — the exact same wrong belief, every attempt, immune to the
retry loop's rising temperature, because it was a stable misconception, not
sampling noise. The sibling task written the same day
(`m2-parser-scan-qualified-name`) had shown the same function as literal
code — `tok = k_lexer_next(lexer, next_pp_kind_out);` — and passed on the
first attempt. The only difference between the two tasks, for this one
function, was prose versus pasted code.

### The five lessons — check every one before finalizing

From `tools/harness/README.md`, each has already caused a real 5-attempt
block at least once:

1. **Repeat every `#include` the code needs, verbatim, even if a type only
   appears inside a signature.** The model carries no context between tasks.
2. **Quote real code for every reused function's contract — rule zero
   above.** Restating a contract in prose is not enough on its own; the
   words have to be backed by a pasted, real call site.
3. **Prefer a linear scan over a binary search on any small table** (tens of
   entries). This model reliably breaks a sort-by-length-then-`strcmp`
   comparator; avoid the shape entirely rather than warning about it.
4. **Give a concrete worked numeric example for any position/size
   arithmetic** — prose alone ("`*width_out` receives the byte count...")
   has failed 5/5 where a `pos=0 → width_out=3`, step-by-step example passed
   on the first attempt.
5. **When one task bundles several functions that share a pattern (several
   grammar productions "together"), the cost of skipping 1, 2 or 4 for any
   one of them is multiplied, not just repeated** — a missing call-site
   quote for a function all three recognizers call breaks all three the
   same way, in the same attempt. Bundling amplifies whatever the task gets
   wrong; it does not average it out.

## Preparing the oracle (a single recognizer — the default for M2 tasks so far)

Every recognizer task so far (`k_scan_qualified_name`, the header trio,
`k_scan_braced_opaque`/`k_scan_extern_c`, `k_scan_ident_list`,
`k_scan_modifier_decl`) used the same three-file shape, not `parse_dump.sh`
(that oracle is for a later, integrated level — see below). Mirror it:

- `tools/cgen/src/engine/<name>.c` — the task's `output`, empty until accepted.
- `tools/cgen/test/unit/<name>_main.c` — a `main()` with a handful of
  cases, each printing `FAIL: ...` and counting a `failures` variable;
  `ok`/exit 0 only when `failures == 0`. Keep it to the cases that matter —
  one happy path per function, plus whatever field just broke last time
  (`next_out` is the field that has broken twice so far; always assert it).
- `tools/cgen/test/unit/<name>.sh` — compiles `SRC` + every `engine/*.c`
  dependency it calls (list them explicitly, mirror an existing `.sh`) +
  `TESTMAIN` under `-fsanitize=address,undefined -fno-sanitize-recover=all`,
  then **`timeout 10 "$BIN" || { echo "FAIL: timed out or crashed (exit $?)"; exit 1; }`**
  — never a bare `"$BIN"`. A hang is a real outcome a wrong recognizer can
  produce (see below), not a hypothetical.

**Never write a solving implementation to validate the oracle — not even a
throwaway one.** This was the practice through `m2-parser-modifier-decl`
(2026-09-26) and André corrected it the same day: a reference implementation
*is* the answer. If it exists, delegating is pointless — at best it costs
exactly what writing the function yourself would have, at worst more (the
task-writing and harness round-trip on top of it). The project's own
standing rule already said this, for the other oracle shape: `.parse` files
"are NOT regenerated from cgen" (`test/parse_dump.sh`'s own header comment;
`golden-perfis-autorais` memory says the same of the golden profiles). A
unit-test oracle is not an exception just because it happens to be C instead
of a text dump.

Instead, **hand-derive the expected values from the grammar/contract**,
exactly as a `.parse` file is derived from a `.k` and the spec — trace the
algorithm on paper, token by token, the same way a "worked example" section
of the task itself is written (it already *is* this derivation; lift its
numbers straight into the oracle's assertions instead of redoing the work).
For a case with token-consumption bookkeeping (does `next_out` end up past
the terminator, or on it?), write out the trace as prose before committing
to the expected value — this is where `k_scan_import_c`'s bug actually got
caught (§ below), by re-deriving the trace carefully, not by running code.

The one thing worth compiling before delivering: `TESTMAIN` alone, against
a **trivial, obviously-wrong stub** of the target function (a body that
just zeroes `*out` and returns) — not to check the *answer*, only that your
own scaffolding has no mistake in it (a missing `#include`, a wrong type in
an assertion, a dependency left out of the `.sh`'s compile line). It must
fail every assertion; if it does not, the oracle itself is the thing that's
broken. Delete the stub before handing off, same as before.

**On hangs, specifically:** a bad recognizer can produce an infinite loop,
not just a wrong answer — `m2-parser-modifier-decl`'s stub-testing session
found that skipping the `byref` check feeds the wrong token into the
brace-balancer as if it were `'{'`, which then never finds a matching `'}'`.
You do not need to run that failure to know it is possible: any recognizer
that hands a token to another recognizer as an "already-consumed opener"
has this risk if the token count before that hand-off can be wrong, and the
`timeout 10 "$BIN"` guard (above) is what makes that failure mode cheap
regardless. `tools/harness/loop.py`'s `subprocess.run(..., timeout=300)`
now also catches `TimeoutExpired` itself, so a hang scores as an ordinary
failure instead of crashing the whole batch — but the oracle's own 10s
guard is still what keeps one bad attempt from burning 5 minutes of budget.

## Preparing the oracle (parser tasks, M2+ — `parse_dump.sh`, a later level)

If the task's acceptance is `parse_dump.sh <level>` and the golden case it
needs is not yet in `test/parse/cases.txt` at that level, you have to extend
the oracle **before** writing the task — never after, and never by running
`cgen` and capturing its output (that would test the tool against itself; the
same principle as `golden/README.md`'s hand-written profiles). Steps:

1. Pick the golden case from `golden/cases/` that exercises the construction
   this task's function recognizes (`design/parser-design.md` §3.2 maps each
   etapa to specific case numbers).
2. **Derive the expected dump by hand** from that case's `.k` source and its
   `expected/` C — the same method already used for 001, 009 and 013. Follow
   `design/cgen-tool.md` §5.2's field-by-field table exactly (position
   anchors, the `→` separator, adaptation marks like `&1` or `type:2`).
3. Add the new case to `test/parse/cases.txt` (`<case-dir> <entry.k>`, one
   line) and write `test/parse/<case>.parse` at full `ilha` level — even if
   this task only exercises `header`/`decl`, write the whole file now; later
   tasks light up the rest of its lines for free, the same way 001 already
   does.
4. **Validate before delivering**: re-read the new `.parse` lines against the
   grammar production and the §5.2 table, line by line, as if you were the
   parser — confirm it would reject a plausible wrong implementation (a
   swapped position, a missing adaptation mark) and accept the one the
   golden's `expected/` C actually implies. This is the same check the
   project already applies to the lexer's reference dumps
   (`.claude/skills/keel-lexer/SKILL.md`, rule 2) — an oracle that can't be
   shown to catch a real bug isn't done.
5. Run `sh tools/cgen/test/parse_dump.sh <level>` yourself before handing off
   — it will currently fail (no parser yet, or the earlier etapa not done),
   but a syntax error in your own `.parse` file or `cases.txt` line should
   fail loudly right there, not surface as a confusing report from the local
   model's first attempt.

## Delivering

Hand André: the task file's path, the oracle files touched (if any), and the
one command he runs (`python3 tools/harness/loop.py qwen3-coder:30b-a3b-q4_K_M
tools/harness/tasks/<id>.md`, per `loop.py`'s own usage: `<model>
[task-file ...]`). **Do not invoke `loop.py`** — that decision belongs to
André (`tools/harness/README.md`, "harness-sonnet-opus-verificador"
memory: Sonnet builds the harness and the tasks, Opus verifies in batch,
neither runs the grinder loop itself as a matter of course).

After André reports the run (accepted, or `runs/<id>/blocked-report.md`),
append one dated line to `tools/harness/README.md`'s "Estado atual" section —
that section is the project's running log of what got built and how, and
every prior milestone entry follows that pattern.

## When stuck

If a task you already delivered blocked 5 attempts, read
`runs/<id>/blocked-report.md` before rewriting anything — check first whether
the failure matches one of the four lessons above (it usually does). If it's
genuinely a new failure mode, add it to `tools/harness/README.md`'s lessons
list once fixed, so it isn't rediscovered on the next task.
