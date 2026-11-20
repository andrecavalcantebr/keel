#!/bin/sh
# A tags set is not capped at 16 items (xprk finding 1): 25 items, with and without written values, in
# a module that uses it from another, and past the limit of 128 the refusal is `tags-limit-exceeded`.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=$PWD/tools/cgen/cgen
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

# $1 item count, $2 `yes` to write values
mk() {
    i=1; out=
    while [ $i -le "$1" ]; do
        if [ "$2" = yes ]; then out="$out  T$i = $i,
"; else out="$out  T$i,
"; fi
        i=$((i + 1))
    done
    printf 'module t;\npub tags T [\n%s];\n' "$out"
}

for spec in "17 no" "17 yes" "25 yes" "25 no" "100 yes"; do
    set -- $spec
    d=$tmp/$1$2; mkdir -p "$d"
    mk "$1" "$2" > "$d/t.k"
    "$BIN" --base-dir "$K" -I "$d" --dest-dir "$d/out" "$d/t.k" -c -o "$d/t.o" >"$d/log" 2>&1 \
        || bad "$1 items ($2 values) must compile: $(grep -v 'info:' "$d/log" | head -3)"
done

d=$tmp/over; mkdir -p "$d"; mk 500 yes > "$d/t.k"
"$BIN" --base-dir "$K" -I "$d" --dest-dir "$d/out" "$d/t.k" -c -o "$d/t.o" >"$d/log" 2>&1; rc=$?
[ $rc -ne 0 ] && [ $rc -lt 128 ] && grep -q 'more than 128 constants.*\[tags-limit-exceeded\]' "$d/log" \
    || bad "500 items must fail with tags-limit-exceeded naming 128 (rc=$rc): $(head -3 "$d/log")"

[ $fail -eq 0 ] && echo ok
exit $fail
