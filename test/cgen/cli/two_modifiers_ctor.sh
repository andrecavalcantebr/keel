#!/bin/sh
# A generic module with two modifiers (keel_libs finding A1, cgen 0.4.2): a function whose parameters
# cite only `T` belongs to the modifier its signature names (here the return type), not to every instance.
# Expected by hand: r3_two_box_i32.h holds make_box and no make_bag, r3_two_bag_i32.h the reverse, and the C
# compiles with -Wall -Werror even though `bag` has a field (`n`) that `box` lacks.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=${CGEN_BIN:-$PWD/tools/cgen/cgen}
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

mkdir -p "$tmp/r3"
cat > "$tmp/r3/two.k" <<'EOF'
module r3.two type T;
pub modifier box { T *data; }
pub modifier bag { T *data; size_t n; }
pub inline box make_box(T *p)           { box r = {0}; r.data = p; return r; }
pub inline bag make_bag(T *p, size_t n) { bag r = {0}; r.data = p; r.n = n; return r; }
EOF
cat > "$tmp/r3/use.k" <<'EOF'
module r3.use;
import r3.two as two types;
pub int run(void) {
    i32 v[4];
    box i32 x = two.make_box(v);
    bag i32 y = two.make_bag(v, 4);
    return (x.data == v && y.data == v && y.n == 4) ? 0 : 1;
}
EOF
out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=gen --dest-dir "$tmp/out" "$tmp/r3/use.k" 2>&1) || bad "cgen [$out]"
b="$tmp/out/r3/r3_two_box_i32.h"; g="$tmp/out/r3/r3_two_bag_i32.h"
grep -q 'make_box' "$b" || bad "box header lacks make_box"
grep -q 'make_bag' "$b" && bad "box header holds make_bag"
grep -q 'make_bag' "$g" || bad "bag header lacks make_bag"
grep -q 'make_box' "$g" && bad "bag header holds make_box"
cat > "$tmp/proof.c" <<'EOF'
#include "keel.type.h"
#include "r3/r3_use.h"
int main(void) { return r3_use_run(); }
EOF
cc -std=c2x -Wall -Werror -I "$tmp/out" "$tmp/proof.c" "$tmp/out/r3/r3_use.c" -o "$tmp/proof" 2>"$tmp/cc" ||
    bad "the C does not compile [$(head -3 "$tmp/cc")]"
[ $fail -eq 0 ] && { "$tmp/proof" || bad "wrong result"; }
[ $fail -eq 0 ] && echo ok
exit $fail
