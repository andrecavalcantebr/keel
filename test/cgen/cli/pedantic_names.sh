#!/bin/sh
# `--pedantic-names` was withdrawn (keel-c-backend D22). The cgen still
# recognizes it and refuses it by name, so that a real need is seen instead of
# reaching the C compiler as an unknown option.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=$PWD/tools/cgen/cgen
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

printf 'module lc;\n\npub i32 f(i32 a) {\n    return a;\n}\n' > "$tmp/lc.k"
out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=gen --dest-dir "$tmp/out" --pedantic-names "$tmp/lc.k" 2>&1); rc=$?
[ $rc -eq 2 ] || bad "rc=$rc [$out]"
echo "$out" | grep -q '\[implementation-limit\]' || bad "no [implementation-limit] [$out]"
echo "$out" | grep -q '255' || bad "the message does not give the limit [$out]"
echo "$out" | grep -q 'D22' || bad "the message does not point to D22 [$out]"
echo "$out" | grep -q 'hint: .*report' || bad "no hint to report the need [$out]"

[ $fail -eq 0 ] && echo ok
exit $fail
