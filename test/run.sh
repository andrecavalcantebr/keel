#!/bin/sh
# Both oracles of the project, in the order the commit rule asks for:
#   test/cgen/run.sh   the cgen suite (unit, cli, lex, parse, diag, gen)
#   golden/run.sh      what the cgen must produce compiles and behaves
#   test/docs.sh       the marked blocks of the README and design/keel-llm-context.md are excerpts of golden/
# Run from anywhere; the exit status is non-zero if either fails.
cd "$(dirname "$0")/.." || exit 2
make -C tools/cgen all >/dev/null || exit 2
fail=0
sh test/cgen/run.sh || fail=1
sh golden/run.sh || fail=1
sh test/docs.sh || fail=1
exit $fail
