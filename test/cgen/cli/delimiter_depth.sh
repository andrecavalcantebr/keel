#!/bin/sh
# Delimiters nested past the checker's stack (256) are reported as the limit they hit
# ([implementation-limit]), never as a balanced source accused of an unpaired `)`.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=$PWD/tools/cgen/cgen
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

mk() { # depth -> $tmp/pd.k with `return ((((a))));`
    open=$(printf '(%.0s' $(seq 1 "$1")); close=$(printf ')%.0s' $(seq 1 "$1"))
    printf 'module pd;\n\npub i32 f(i32 a) {\n    return %sa%s;\n}\n' "$open" "$close" > "$tmp/pd.k"
}
run() { out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=parse "$tmp/pd.k" 2>&1); rc=$?; }

mk 250; run     # 250 + the `(` of the parameter list and the `{` stay under 256
[ $rc -eq 0 ] || bad "depth 250 must pass: rc=$rc [$out]"
mk 300; run
[ $rc -ne 0 ] || bad "depth 300 must be refused"
echo "$out" | grep -q '\[implementation-limit\]' || bad "no [implementation-limit] [$out]"
echo "$out" | grep -q 'unmatched-delimiter' && bad "a balanced source is accused of an unpaired delimiter [$out]"

[ $fail -eq 0 ] && echo ok
exit $fail
