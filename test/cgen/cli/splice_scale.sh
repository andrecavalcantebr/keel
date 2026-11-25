#!/bin/sh
# Many `\`-newline splices between the tokens of one logical line (achados 2026-11-25, A3):
# k_em_comment_edits restarted its scan at the start of the gap for every physical line, so
# 40 000 splices cost 1.7 s (quadratic). Expected by hand: the 40 000-splice file generates in
# well under the 1.5 s the quadratic scan needed, and a block comment opened before the
# splices and closed after them is still dropped (the case the restart protected).
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=${CGEN_BIN:-$PWD/tools/cgen/cgen}
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

{ printf 'module owner;\nint x = 1'
  awk 'BEGIN { for (i = 0; i < 40000; i++) printf " \\\n"; }'
  printf ' + 2;\n'; } > "$tmp/owner.k"
t0=$(date +%s%N)
out=$("$BIN" -I "$tmp" --base-dir "$K" --stop-after=gen --dest-dir "$tmp/gen" "$tmp/owner.k" 2>&1); rc=$?
t1=$(date +%s%N)
[ $rc -eq 0 ] || bad "40000 splices: rc=$rc [$out]"
ms=$(( (t1 - t0) / 1000000 ))
[ -n "${CGEN_BIN:-}" ] || [ $ms -lt 800 ] || bad "40000 splices took ${ms} ms (quadratic scan)"

{ printf 'module c;\nint y = 1 /* opened'
  awk 'BEGIN { for (i = 0; i < 50; i++) printf " \\\n"; }'
  printf ' closed */ + 2;\nint z = 3;\n'; } > "$tmp/c.k"
out=$("$BIN" -I "$tmp" --base-dir "$K" --stop-after=gen --dest-dir "$tmp/genc" "$tmp/c.k" 2>&1); rc=$?
[ $rc -eq 0 ] || bad "comment across splices: rc=$rc [$out]"
grep -rq "opened\|closed" "$tmp/genc" && bad "comment across splices: text leaked into the output"

[ $fail -eq 0 ] && echo ok
exit $fail
