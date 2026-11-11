#!/bin/sh
# An internal capacity of the emitter (the line buffer, LINE_CAP) is reported as
# such: [implementation-limit] names the limit and its value (cgen-tool-spec
# §7.1), not "does not cover this module's constructions yet", which is for a
# construction the emitter really lacks.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=$PWD/tools/cgen/cgen
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

# a prototype of ~1400 bytes does not fit the 1024-byte line buffer
args=""
i=0
while [ $i -lt 60 ]; do
    [ -n "$args" ] && args="$args, "
    args="${args}i32 argument_number_$i"
    i=$((i + 1))
done
printf 'module lc;\n\npub i32 big(%s) {\n    return argument_number_0;\n}\n' "$args" > "$tmp/lc.k"

out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=gen --dest-dir "$tmp/out" "$tmp/lc.k" 2>&1); rc=$?
[ $rc -eq 2 ] || bad "long prototype: rc=$rc [$out]"
echo "$out" | grep -q 'LINE_CAP' || bad "the message names no limit [$out]"
echo "$out" | grep -q '1024' || bad "the message gives no value [$out]"
echo "$out" | grep -q '\[implementation-limit\]' || bad "no [implementation-limit] [$out]"
echo "$out" | grep -q 'does not cover' && bad "still the 'does not cover' message [$out]"

# a short module still generates
printf 'module ok;\n\npub i32 small(i32 a) {\n    return a;\n}\n' > "$tmp/ok.k"
out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=gen --dest-dir "$tmp/out2" "$tmp/ok.k" 2>&1); rc=$?
[ $rc -eq 0 ] || bad "short module: rc=$rc [$out]"

[ $fail -eq 0 ] && echo ok
exit $fail
