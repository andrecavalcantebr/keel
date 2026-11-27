#!/bin/sh
# A decimal `dim` literal of ten digits (keel_libs finding A7, cgen 0.4.2): `box(1000000000)` was refused with
# nonconstant-dim, whose text called the literal unknown. Expected by hand: up to 18 digits are accepted (a long
# on LP64); 19 digits are refused, and the message names the 18-digit limit.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=${CGEN_BIN:-$PWD/tools/cgen/cgen}
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }
cat > "$tmp/big.k" <<'EOF'
module big dim N;
pub modifier box { u8 *p; }
EOF
use() { printf 'module %s;\nimport big types;\npub int main(void) { box(%s) x; (void)x; return 0; }\n' "$1" "$2" > "$tmp/$1.k"; }
use ten 1000000000;            use max 999999999999999999;   use over 9999999999999999999
out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=gen --dest-dir "$tmp/g1" "$tmp/ten.k" 2>&1) || bad "ten digits refused [$out]"
out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=gen --dest-dir "$tmp/g2" "$tmp/max.k" 2>&1) || bad "18 digits refused [$out]"
out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=gen --dest-dir "$tmp/g3" "$tmp/over.k" 2>&1) && bad "19 digits accepted"
printf '%s\n' "$out" | grep -q 'up to 18 digits' || bad "the message does not name the limit [$out]"
[ $fail -eq 0 ] && echo ok
exit $fail
