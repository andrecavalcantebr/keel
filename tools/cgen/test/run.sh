#!/bin/sh
# The cgen test suite (cgen design §10). Run from anywhere:
#
#   sh tools/cgen/test/run.sh            # or: make -C tools/cgen check
#
#   unit/        tool/ pieces in isolation: roots, base resolution, long
#                options, reading the source
#   cli/         a sanitized cgen: argv partition, invocation errors, --base-dir
#   lex_dump.sh  --stop-after=lex: fixed dumps, the reference lexer over every
#                .k of golden/ and base/, and the lexical diagnostics
#   parse_dump.sh --stop-after=parse at level $PARSE_LEVEL (header, decl, inst,
#                ilha, or ilha:<kinds> for the island stages of parser design
#                §3.2). The default is stages 4a and 4b: the islands type, name,
#                call, from-stack, ref, implicit-init, array, array-index, index
#                and range-index.
#
# Each test prints its last line; the suite fails if any test fails.
cd "$(dirname "$0")/../../.." || exit 2
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
for t in tools/cgen/test/unit/*.sh; do
    name="unit/$(basename "$t" .sh)"
    src=$(sed -n 's/^SRC=//p' "$t" | head -1)
    if [ -n "$src" ] && [ -f "$src" ] && [ ! -s "$src" ]; then
        printf 'wip    %-16s %s\n' "$name" "— $src ainda vazio (tarefa não aceita)"
        continue
    fi
    run "$name" sh "$t"
done
for t in tools/cgen/test/cli/*.sh;  do run "cli/$(basename "$t" .sh)"  sh "$t"; done
run lex_dump sh tools/cgen/test/lex_dump.sh
PARSE_LEVEL=${PARSE_LEVEL:-ilha:type,name,call,from-stack,ref,implicit-init,array,array-index,index,range-index}
run "parse_dump/$PARSE_LEVEL" sh tools/cgen/test/parse_dump.sh "$PARSE_LEVEL"
exit $fail
