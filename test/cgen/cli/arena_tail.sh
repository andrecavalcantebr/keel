#!/bin/sh
# The instance table was reserved per token, 280 bytes each, though a module has few distinct
# instances (achados 2026-11-25, A5): it was 52% of the arena, so ~250 000 tokens (~1 MB of
# source) was the ceiling. Expected by hand: with the arena cut to 8 MiB, a module of 1 000
# functions (~19 000 tokens, which the per-token tables of 536 bytes would not fit) generates, and
# its last function is in owner.h. Without the change it stops with "arena capacity exhausted
# allocating KInstanceUse".
cd "$(dirname "$0")/../.." && cd .. || exit 2
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }
gcc -std=c2x -I tools/cgen/src -I tools/cgen/gen -DCGEN_ARENA_CAPACITY=$((8 * 1024 * 1024)) -O0 \
    $(find tools/cgen/src -name '*.c') -lm -o "$tmp/cgen" 2>"$tmp/cc" || { echo "FAIL: build [$(head -3 "$tmp/cc")]"; exit 1; }
{ printf 'module owner;\n'
  awk 'BEGIN { for (i = 1; i <= 1000; i++) printf "pub inline i32 g%d(void) { i32 v = %d; return v + 1; }\n", i, i; }'; } > "$tmp/owner.k"
out=$("$tmp/cgen" -I "$tmp" --base-dir "$PWD/base" --stop-after=gen --dest-dir "$tmp/gen" "$tmp/owner.k" 2>&1); rc=$?
[ $rc -eq 0 ] || bad "1000 functions in 8 MiB: rc=$rc [$(echo "$out" | head -3)]"
grep -q "owner_g1000(void)" "$tmp/gen/owner.h" || bad "owner_g1000 missing from owner.h"
[ $fail -eq 0 ] && echo ok
exit $fail
