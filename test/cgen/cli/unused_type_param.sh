#!/bin/sh
# A `type T` parameter the body never names (keel_libs finding A6, cgen 0.4.2): the two hidden parameters
# (`keel__T_size`, `keel__T_align`) were unused, and `-Wall -Wextra -Werror` stopped a valid program.
# Expected by hand: the C compiles with -Wall -Wextra -Werror, the call returns its argument; a body that
# reads only `sizeof(T)` casts only the alignment.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=${CGEN_BIN:-$PWD/tools/cgen/cgen}
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

cat > "$tmp/t.k" <<'EOF'
module t;
pub inline i32 pick(type T, i32 x) { return x; }
pub inline size_t width(type T) { return sizeof(T); }
pub int main(void) { return (pick(u32, 3) == 3 && width(u64) == 8) ? 0 : 1; }
EOF
out=$("$BIN" --base-dir "$K" -I "$tmp" --dest-dir "$tmp/g" -std=c2x -Wall -Wextra -Werror "$tmp/t.k" -o "$tmp/t" 2>&1) ||
    bad "cgen/cc failed [$(printf '%s\n' "$out" | head -3)]"
grep -q 'pick(size_t keel__T_size, size_t keel__T_align, i32 x) { (void)keel__T_size; (void)keel__T_align; return x; }' "$tmp/g/t.h" ||
    bad "pick lacks the void casts"
grep -q "width(size_t keel__T_size, size_t keel__T_align) { (void)keel__T_align; return" "$tmp/g/t.h" || bad "width should cast only the alignment"
[ $fail -eq 0 ] && { "$tmp/t" || bad "wrong result"; }
[ $fail -eq 0 ] && echo ok
exit $fail
