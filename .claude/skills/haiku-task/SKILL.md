---
name: haiku-task
description: Write a task card for a Haiku subagent to implement, refactor or audit a piece of keel/cgen (tools/agents/tasks/<id>.md), prepare its oracle, launch it via the Agent tool with model haiku, and review its result. Use when André asks to delegate cgen work to Haiku agents, or when a Haiku agent is told to follow a card.
---

# haiku-task — delegating cgen work to a Haiku subagent

The workflow since 2026-10-22 (`tools/agents/README.md`): Opus orchestrates,
Haiku implements. This skill has two halves: what the orchestrator does, and
what the Haiku agent does. The card is the contract between them.

## Orchestrator: before writing a card

- **Decide whether it is delegable.** Delegate: a new file or function with a
  clear contract, a mechanical refactor with a behaviour snapshot, an audit
  that produces a report. Do not delegate: anything that needs a decision on
  a normative document, or a surgical integration whose contract lives only in
  this conversation — write a fuller card or do it yourself.
- **The oracle comes first and is written by hand.** Expected dumps
  (`tools/cgen/test/parse/*.parse`), unit-test assertions and golden
  `expected/` are derived by reasoning over the `.k`, the spec and the
  backend — never by running cgen and capturing its output (that tests the
  tool against itself; `golden/README.md`).
- **Measure, don't read comments.** When an expected value depends on what an
  existing function returns, compile a three-line program that calls it and
  prints the result. A comment in `parser.h` was wrong once
  (`k_scan_braced_opaque` and the trivia at the braces) and cost a whole task.
- **Quote real code.** For every existing API the task reuses, paste the
  declaration and one real call site with `file:line`. Paraphrase is where
  wrong conventions come from.
- **Give a worked example** for any position/size arithmetic.
- **A refactor takes the snapshot before anyone edits:**
  `sh tools/agents/verify.sh snapshot /tmp/keel-snap-<id>`.
- **Oracles must survive a hang:** a unit test wraps its binary in
  `timeout 10`, as the existing `tools/cgen/test/unit/*.sh` do.

## Orchestrator: the card (`tools/agents/tasks/<id>.md`)

Sections, in this order: **Objetivo**; **Arquivos** (what it may create, edit,
delete — nothing else); **Contrato** (the exact behaviour, with pasted code);
**Passos sugeridos**; **Aceitação** (the exact commands, and what output means
pass); **Relatório** (`tools/agents/reports/<id>.md`: what changed, the verify
output, every doubt or incoherence found). Write card prose in pt-BR, code in
English.

Launch with the Agent tool, `model: "haiku"`, a prompt that says: read
`.claude/skills/haiku-task/SKILL.md` (the agent half) and then
`tools/agents/tasks/<id>.md`, and do it. Two agents that build cgen must not
run at the same time in the same tree (`verify.sh` runs `make clean`); an
audit that only reads can run alongside a build task.

## Orchestrator: after the agent reports

Read the report and the `git diff`, not just the "ALL OK". Re-run
`verify.sh` yourself. Check the scope (`git status` lists only the card's
files). Then commit, with the attribution line, or send the agent back with
the specific defect (SendMessage keeps its context).

## Haiku agent: how to do a card

1. Read the card fully, then `AGENTS.md`. The card's file list is your scope:
   touch nothing else, never edit normative documents or golden expected
   files, never commit, never push.
2. If the card says refactor: confirm the snapshot it names exists; if it does
   not, take it before your first edit.
3. Work in small steps, and build after each one (`make -C tools/cgen -j8`).
   Fix every error and every new warning before moving on.
4. Run the acceptance commands of the card. If something fails, find the
   cause in your change; do not weaken a test, an oracle or the baseline to
   make it pass. If you believe the oracle itself is wrong, stop and say so in
   the report, with the evidence.
5. If the card, the code and the documents disagree, or the card is
   ambiguous, do not guess: write the question in the report and stop at the
   last state that passes the acceptance.
6. Write `tools/agents/reports/<id>.md` in pt-BR: what you changed (file by
   file), the exact final output of the acceptance commands, and the open
   points. Your final message is a short summary of that report.
