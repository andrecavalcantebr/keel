#!/bin/sh
# An `extent` past the cgen's capacity is refused with a diagnostic, never left as text in the .type.h
# (E03): 4 groups `[k, n]` pass and the 5th is `extent-limit-exceeded`; 8 `array` columns pass and the
# 9th is an `implementation-limit` (its message is still the generic one: E03).
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=$PWD/tools/cgen/cgen
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

# $1 groups: an extent with that many groups, one column
groups() {
    g=; names=; i=0
    while [ $i -lt "$1" ]; do g="$g[d$i, 2]"; names="${names}d$i, "; i=$((i + 1)); done
    printf 'module t;\npub extent struct s %s {\n    array i32 *v;\n    size_t %s_pad;\n};\n' "$g" "$names"
}
# $1 columns
columns() {
    cols=; i=0
    while [ $i -lt "$1" ]; do cols="$cols    array i32 *c$i;
"; i=$((i + 1)); done
    printf 'module t;\npub extent struct s [n, 4] {\n%s    size_t n;\n};\n' "$cols"
}

run() { d=$tmp/$1; mkdir -p "$d"; cat > "$d/t.k"; "$BIN" --base-dir "$K" -I "$d" --stop-after=gen --dest-dir "$d/out" "$d/t.k" >"$d/log" 2>&1; echo $? > "$d/rc"; }

groups 4 | run g4
[ "$(cat $tmp/g4/rc)" = 0 ] && ! grep -q '^pub extent' "$tmp/g4/out/t.type.h" \
    || bad "4 groups must translate: $(grep -v 'info:' $tmp/g4/log | head -3)"
groups 5 | run g5
rc=$(cat $tmp/g5/rc)
[ "$rc" != 0 ] && grep -q 'more than 4 groups.*\[extent-limit-exceeded\]' "$tmp/g5/log" \
    || bad "5 groups must fail with extent-limit-exceeded naming 4 (rc=$rc): $(head -3 $tmp/g5/log)"
columns 8 | run c8
[ "$(cat $tmp/c8/rc)" = 0 ] || bad "8 columns must translate: $(grep -v 'info:' $tmp/c8/log | head -3)"
columns 9 | run c9
rc=$(cat $tmp/c9/rc)
[ "$rc" != 0 ] && grep -q '\[implementation-limit\]' "$tmp/c9/log" \
    || bad "9 columns must fail with implementation-limit (rc=$rc): $(head -3 $tmp/c9/log)"

[ $fail -eq 0 ] && echo ok
exit $fail
