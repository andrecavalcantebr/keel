#!/bin/sh
# The oracles of the project. With no argument, all of them, in the order the commit rule asks for:
#   test/run.sh            everything: run it before a push
#   test/run.sh AREA...    only the areas named, while working (one or more of):
#
#     unit      tool/ pieces in isolation              (~2.5 min)
#     cli       argv, options, --base-dir, errors      (~1 min)
#     lex       lex dumps                              (test/cgen/lex_dump.sh)
#     parse     parse dumps                            (test/cgen/parse_dump.sh)
#     diag      the diagnostic catalog and its cases   (test/cgen/diag.sh)
#     gen       --stop-after=gen against golden/       (gen.sh, gen_golden.sh, base.sh; ~45 s)
#     golden    compile and run every golden case      (the slow one)
#     golden:W  golden cases whose name contains W (golden:080 or golden:tags)
#     docs      the marked blocks of the README and keel-llm-context.md
#
# What to run after touching what:
#   parser/lexer (engine/lexer*, parser*)   lex parse diag gen
#   emitter (engine/emit/*)                 gen golden:<the case you touched> (golden whole before the push)
#   driver/CLI (tool/*, main.c)             unit cli
#   diagnostic messages / catalog           diag cli
#   a base module (base/)                   gen golden:<its case>
# Run from anywhere; the exit status is non-zero if any selected area fails.
cd "$(dirname "$0")/.." || exit 2
make -C tools/cgen all >/dev/null || exit 2
fail=0
if [ $# -eq 0 ]; then
    sh test/cgen/run.sh || fail=1
    sh golden/run.sh || fail=1
    sh test/docs.sh || fail=1
    exit $fail
fi
CG=test/cgen
run() { sh "$@" || fail=1; }
for area in "$@"; do
    case $area in
        unit) for t in $CG/unit/*/*.sh; do run "$t"; done ;;
        cli)  for t in $CG/cli/*.sh; do run "$t"; done ;;
        lex)  run $CG/lex_dump.sh ;;
        parse) run $CG/parse_dump.sh "${PARSE_LEVEL:-ilha}" ;;
        diag) run $CG/diag.sh ;;
        gen)  run $CG/gen.sh; run $CG/gen_golden.sh; run $CG/base.sh ;;
        golden) run golden/run.sh ;;
        golden:*) run golden/run.sh "${area#golden:}" ;;
        docs) run test/docs.sh ;;
        *) echo "unknown area: $area (see the header of test/run.sh)" >&2; exit 2 ;;
    esac
done
exit $fail
