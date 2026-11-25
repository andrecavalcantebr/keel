#!/bin/sh
# Many declarations in one module (achados 2026-11-25, A4): k_em_declared_at scanned every node for
# each identifier of each emitted line, and the line loops (emit_line, emit_c) tested every node
# against every line, so 4 000 one-line functions cost 9.9 s in the debug build. Expected by hand:
# the module generates in under 6 s (it takes ~2.7 s), and the first and last functions come out
# renamed with the module prefix in owner.h.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=$PWD/tools/cgen/cgen
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

{ printf 'module owner;\n'
  awk 'BEGIN { for (i = 1; i <= 4000; i++) printf "pub inline i32 g%d(void) { i32 v = %d; return v + 1; }\n", i, i; }'; } > "$tmp/owner.k"
t0=$(date +%s%N)
out=$("$BIN" -I "$tmp" --base-dir "$K" --stop-after=gen --dest-dir "$tmp/gen" "$tmp/owner.k" 2>&1); rc=$?
t1=$(date +%s%N)
[ $rc -eq 0 ] || bad "4000 functions: rc=$rc [$out]"
ms=$(( (t1 - t0) / 1000000 ))
[ $ms -lt 6000 ] || bad "4000 functions took ${ms} ms (quadratic)"
grep -q "owner_g1(void)" "$tmp/gen/owner.h" || bad "owner_g1 missing from owner.h"
grep -q "owner_g4000(void)" "$tmp/gen/owner.h" || bad "owner_g4000 missing from owner.h"

[ $fail -eq 0 ] && echo ok
exit $fail
