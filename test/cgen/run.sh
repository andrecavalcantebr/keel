#!/bin/sh
# The cgen test suite (cgen design §10). Run from anywhere:
#
#   sh test/cgen/run.sh            # or: make -C tools/cgen check
#
#   unit/        tool/ pieces in isolation: roots, base resolution, long
#                options, reading the source
#   cli/         a sanitized cgen: argv partition, invocation errors, --base-dir
#   gen_golden.sh  --stop-after=gen over golden/cases against expected/ (wip while
#                the emitter does not cover a case)
#   lex_dump.sh  --stop-after=lex: fixed dumps, the reference lexer over every
#                .k of golden/ and base/, and the lexical diagnostics
#   diag.sh      the catalog table is current with keel-spec §6.2, and each
#                failure case (diag/, DIAG markers in the sources) draws exactly
#                the diagnostics its sources ask for; the last line is the
#                catalog's coverage (diag-design §7)
#   parse_dump.sh --stop-after=parse at level $PARSE_LEVEL (header, decl, inst,
#                ilha, or ilha:<kinds> for the island stages of parser design
#                §3.2). The default is plain `ilha`: every kind of island, as
#                stages 4a to 4h are all lit.
#
# Each test prints its last line; the suite fails if any test fails.
cd "$(dirname "$0")/../.." || exit 2
fail=0
run() {
    name=$1; shift
    out=$("$@" 2>&1); rc=$?
    last=$(printf '%s\n' "$out" | tail -1)
    if [ $rc -eq 0 ]; then
        printf 'ok     %-16s %s\n' "$name" "$last"
    else
        printf 'FAIL   %-16s\n' "$name"; printf '%s\n' "$out" | tail -15 | sed 's/^/       /'
        fail=1
    fi
}
# A unit test whose SRC is an empty file is waiting for its implementation
# — the oracle was written first, which is the point (the task card is
# handed to the model with the oracle already in place). Report it `wip`
# instead of failing, the same marker golden/run.sh uses. The moment the
# file has content the test goes live again, so nothing has to be undone.
for t in test/cgen/unit/*/*.sh; do
    name="unit/$(basename "$(dirname "$t")")/$(basename "$t" .sh)"
    src=$(sed -n 's/^SRC=//p' "$t" | head -1)
    if [ -n "$src" ] && [ -f "$src" ] && [ ! -s "$src" ]; then
        printf 'wip    %-16s %s\n' "$name" "— $src ainda vazio (tarefa não aceita)"
        continue
    fi
    run "$name" sh "$t"
done
for t in test/cgen/cli/*.sh;  do run "cli/$(basename "$t" .sh)"  sh "$t"; done
# gen.sh exits 3 while --stop-after=gen is not implemented: wip, not failure.
out=$(sh test/cgen/gen.sh 2>&1); rc=$?
if [ $rc -eq 3 ]; then
    printf 'wip    %-16s %s\n' gen "— $(printf '%s\n' "$out" | tail -1)"
elif [ $rc -eq 0 ]; then
    printf 'ok     %-16s %s\n' gen "$(printf '%s\n' "$out" | tail -1)"
else
    printf 'FAIL   %-16s\n' gen; printf '%s\n' "$out" | tail -15 | sed 's/^/       /'; fail=1
fi
run gen_golden sh test/cgen/gen_golden.sh
run lex_dump sh test/cgen/lex_dump.sh
run diag sh test/cgen/diag.sh
PARSE_LEVEL=${PARSE_LEVEL:-ilha}
run "parse_dump/$PARSE_LEVEL" sh test/cgen/parse_dump.sh "$PARSE_LEVEL"
exit $fail
