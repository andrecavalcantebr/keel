#!/bin/sh
# `bitbuffer-width-invalid` is the C compiler's refusal (spec §5.8, rule 1; backend §5.20): the cgen
# writes the instance, and the `static_assert` that every instance carries fails the translation unit.
# Each bad instance below is written by hand with the reason it is bad; the good one compiles.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=$PWD/tools/cgen/cgen
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

# $1 the instance as it is written, $2 `yes` when the C compiler must accept it
try() {
    d=$tmp/$(printf %s "$1" | tr -c 'a-z0-9' _); mkdir -p "$d"
    cat > "$d/use.k" <<EOK
module use;
import keel.bitbuffer as bitbuffer types;
import keel.arena as arena types;
pub size_t f(arena *a) {
    $1 g = bitbuffer.make(a, 8);
    return bitbuffer.capacity(g);
}
EOK
    "$BIN" --base-dir "$K" -I "$d" --dest-dir "$d/out" "$d/use.k" -c -o "$d/use.o" >"$d/log" 2>&1; rc=$?
    if [ "$2" = yes ]; then
        [ $rc -eq 0 ] || bad "$1 must compile: $(grep -v 'info:' "$d/log" | head -3)"
    else
        [ $rc -ne 0 ] && grep -q 'bitbuffer-width-invalid' "$d/log" || bad "$1 must fail with bitbuffer-width-invalid: $(grep -v 'info:' "$d/log" | head -3)"
    fi
}

try 'bitbuffer(8) u8' yes           # the widest a u8 holds
try 'bitbuffer(1) bool' yes
try 'bitbuffer(9) u8' no            # 9 bits do not fit in a u8
try 'bitbuffer(65) u64' no          # nor 65 in a u64
try 'bitbuffer(3) i8' no            # a signed element
try 'bitbuffer(2) bool' no          # a bool holds one bit

[ $fail -eq 0 ] && echo ok
exit $fail
