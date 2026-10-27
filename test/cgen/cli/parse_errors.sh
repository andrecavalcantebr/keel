#!/bin/sh
# The parse entry point must preserve lexical diagnostics and suppress a dump.
set -u
f=$(mktemp --suffix=.k)
err=$(mktemp)
trap 'rm -f "$f" "$err"' EXIT
printf 'module sample;\n#define keel_bad 1\n' > "$f"
out=$(tools/cgen/cgen --base-dir base --stop-after=parse "$f" 2>"$err")
rc=$?
if [ "$rc" -ne 1 ] || [ -n "$out" ] ||
   ! grep -q '\[define-over-keel-name\]' "$err"; then
    echo "FAIL: parse did not report the lexical error (exit $rc)"
    cat "$err"
    exit 1
fi
echo ok
