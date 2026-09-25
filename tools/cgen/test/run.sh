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
#                ilha). Skipped while PARSE_LEVEL is unset: the parser (M2)
#                has not started.
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
for t in tools/cgen/test/unit/*.sh; do run "unit/$(basename "$t" .sh)" sh "$t"; done
for t in tools/cgen/test/cli/*.sh;  do run "cli/$(basename "$t" .sh)"  sh "$t"; done
run lex_dump sh tools/cgen/test/lex_dump.sh
if [ -n "${PARSE_LEVEL:-}" ]; then
    run "parse_dump/$PARSE_LEVEL" sh tools/cgen/test/parse_dump.sh "$PARSE_LEVEL"
else
    printf 'skip   %-16s PARSE_LEVEL unset (M2 not started)\n' parse_dump
fi
exit $fail
