#!/bin/sh
# A module header's dim/tags/type list past 8 binders is an [implementation-limit],
# not the generic "malformed or truncated declaration [unexpected-token]" that a
# real syntax error gets.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=$PWD/tools/cgen/cgen
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

printf 'module g8 type A,B,C,D,E,F,G,H;\n' > "$tmp/g8.k"
out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=parse "$tmp/g8.k" 2>&1); rc=$?
[ $rc -eq 0 ] || bad "8 type binders must pass: rc=$rc [$out]"

printf 'module g9 type A,B,C,D,E,F,G,H,I;\n' > "$tmp/g9.k"
out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=parse "$tmp/g9.k" 2>&1); rc=$?
[ $rc -ne 0 ] || bad "9 type binders must be refused"
echo "$out" | grep -q '\[implementation-limit\]' || bad "no [implementation-limit] [$out]"
echo "$out" | grep -q 'unexpected-token' && bad "still unexpected-token [$out]"

[ $fail -eq 0 ] && echo ok
exit $fail
