#!/bin/sh
# A `pub inline` prototype with one parameter per line, whose total text passes 1024 bytes
# while every physical line stays short (achados 2026-11-25, A6): `k_em_emit_proto` copied
# the whole declaration, up to the `)`, into one buffer bounded by LINE_CAP — the limit the
# message calls "per line, not per file" was in fact per declaration. Expected by hand: the
# 30-parameter form (1048 bytes of declaration text, 33-byte lines) compiles, with every
# parameter renamed and present, same as the 24-parameter control that never crossed the cap.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=$PWD/tools/cgen/cgen
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

gen_proto() {
    n=$1
    { printf 'module owner;\npub inline bool validate(\n'
      i=0
      while [ $i -lt "$n" ]; do
          sep=,; [ $((i + 1)) -eq "$n" ] && sep=
          printf '    i32 requested_token_count_%02d%s\n' "$i" "$sep"
          i=$((i + 1))
      done
      printf ') { return true; }\n'; } > "$tmp/owner.k"
}

gen_proto 30
out=$("$BIN" -I "$tmp" --base-dir "$K" --stop-after=gen --dest-dir "$tmp/gen_30" "$tmp/owner.k" 2>&1); rc=$?
[ $rc -eq 0 ] || bad "30 params: rc=$rc [$out]"
grep -q "owner_validate($" "$tmp/gen_30/owner.h" || bad "30 params: owner.h does not hold the multi-line prototype"
grep -q "requested_token_count_29" "$tmp/gen_30/owner.h" || bad "30 params: the last parameter is missing"
cc -std=c2x -fsyntax-only -I "$tmp/gen_30" "$tmp/gen_30/owner.c" 2>"$tmp/cc_30" || bad "30 params: the C does not compile [$(head -3 "$tmp/cc_30")]"

gen_proto 24
out=$("$BIN" -I "$tmp" --base-dir "$K" --stop-after=gen --dest-dir "$tmp/gen_24" "$tmp/owner.k" 2>&1); rc=$?
[ $rc -eq 0 ] || bad "control, 24 params: rc=$rc [$out]"

[ $fail -eq 0 ] && echo ok
exit $fail
