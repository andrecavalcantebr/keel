#!/bin/sh
# One long function (achados 2026-11-25, A4b): every emitted line ran all ~20 analyzers of a
# function body, each scanning every token of it, so a 1 600-line array initializer cost 1.7 s in
# the debug build (quadratic). Expected by hand: the function with 1 600 (and 6 400) elements
# generates under 0.5 s, and the array text is carried through unchanged, one element per line.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=${CGEN_BIN:-$PWD/tools/cgen/cgen}
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

for n in 1600 6400; do
    { printf 'module owner;\npub inline i32 f(void) { int a[%d] = {\n' "$n"
      awk -v n="$n" 'BEGIN { for (i = 1; i <= n; i++) printf "  %d,\n", i; }'
      printf '}; return a[0]; }\n'; } > "$tmp/owner.k"
    t0=$(date +%s%N)
    out=$("$BIN" -I "$tmp" --base-dir "$K" --stop-after=gen --dest-dir "$tmp/gen_$n" "$tmp/owner.k" 2>&1); rc=$?
    t1=$(date +%s%N)
    [ $rc -eq 0 ] || bad "$n elements: rc=$rc [$out]"
    ms=$(( (t1 - t0) / 1000000 ))
    [ -n "${CGEN_BIN:-}" ] || [ $ms -lt 500 ] || bad "$n elements took ${ms} ms (quadratic)"
    grep -q "^  $n,$" "$tmp/gen_$n/owner.h" || bad "$n elements: the last element is missing from owner.h"
done

[ $fail -eq 0 ] && echo ok
exit $fail
