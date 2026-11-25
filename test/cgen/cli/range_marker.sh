#!/bin/sh
# The marker of an interval's upper bound (spec §4.5, 0.4.0): `a..<b` is `a..b`, `a..=b` includes `b`, `..<=` is invalid-range-marker,
# and the spaces around the marker do not matter. The debug abort of `keel_inclusive` is in golden 087.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=${CGEN_BIN:-$PWD/tools/cgen/cgen}
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

# gen NAME 'body' : a module t with the body in main; the generated t.c in $tmp/NAME/out/t.c
gen() {
    d=$tmp/$1; mkdir -p "$d"
    printf 'module t;\nimport keel.slice as slice types;\nimport keel.array as array;\npub int main(void) {\n    array i32 v[6] = {1, 2, 3, 4, 5, 6};\n    slice i32 s = slice.of(v, 0, 6);\n%s\n    return 0;\n}\n' "$2" > "$d/t.k"
    "$BIN" --base-dir "$K" -I "$d" --stop-after=gen --dest-dir "$d/out" "$d/t.k" >"$d/log" 2>&1; echo $? > "$d/rc"
}
has() { grep -qF "$2" "$tmp/$1/out/t.c" || bad "$1: wanted '$2' in t.c: $(grep 'as_slice\|for (' $tmp/$1/out/t.c | head -3)"; }

gen incl '    slice i32 a = s[1..=3];';           has incl 'keel_slice_i32_as_slice(s, 1, keel_inclusive(3))'
gen excl '    slice i32 a = s[1..<3];';           has excl 'keel_slice_i32_as_slice(s, 1, 3)'
gen plain '    slice i32 a = s[1..3];';           has plain 'keel_slice_i32_as_slice(s, 1, 3)'
gen spaced '    slice i32 a = s[1 ..= 3];';       has spaced 'keel_inclusive( 3)'
gen tight '    slice i32 a = s[1..=(2 + 1)];';    has tight 'keel_inclusive((2 + 1))'
gen openeq '    slice i32 a = s[4..=];';          has openeq 'keel_slice_i32_as_slice(s, 4, keel_slice_i32_length(s))'
gen openlt '    slice i32 a = s[4..<];';          has openlt 'keel_slice_i32_as_slice(s, 4, keel_slice_i32_length(s))'
gen headeq '    slice i32 a = s[..=2];';          has headeq 'keel_slice_i32_as_slice(s, 0, keel_inclusive(2))'
gen loop '    size_t n = 0;
    foreach (size_t i : 0..=4) n += i;';        has loop 'i < keel_inclusive(4)'
gen lit '    range r = 2..=4;';                   has lit 'keel_range_of(2, keel_inclusive(4))'

# `..<=` is `..` and `<=`, and the lexer says so
gen bad '    slice i32 a = s[1..<=3];'
d=$tmp/bad
grep -q 'invalid-range-marker' "$d/log" && [ "$(cat $d/rc)" != 0 ] || bad "..<= must be invalid-range-marker: $(head -2 $d/log)"
# an inclusive range inverted by its decimals: [5, 4+1) is empty, and [5, 3+1) is inverted
gen inv_ok '    slice i32 a = s[5..=4];'
[ "$(cat $tmp/inv_ok/rc)" = 0 ] || bad "s[5..=4] is empty, not inverted: $(grep -v info: $tmp/inv_ok/log | head -2)"
gen inv_bad '    slice i32 a = s[5..=3];'
grep -q 'inverted-range-index' "$tmp/inv_bad/log" || bad "s[5..=3] must be inverted-range-index: $(grep -v info: $tmp/inv_bad/log | head -2)"
gen inv_excl '    slice i32 a = s[5..<4];'
grep -q 'inverted-range-index' "$tmp/inv_excl/log" || bad "s[5..<4] must be inverted-range-index"

[ $fail -eq 0 ] && echo ok
exit $fail
