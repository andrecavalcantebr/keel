#!/bin/sh
# A `typedef` of an instance as the target of a verb that takes its type from the target (keel_libs finding N2,
# cgen 0.4.2). `typedef buffer u8 text;` then `text t = buffer.of(src, 4);` was refused with from-without-target.
# Expected by hand: the alias is the instance it names, in the three places a target is read (initializer,
# assignment to a known symbol, `return`), the instance stays `keel_buffer_u8` (the generated call is
# keel_buffer_u8_of, never a name of the alias), the C compiles with -Wall -Werror and the program returns 0.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=${CGEN_BIN:-$PWD/tools/cgen/cgen}
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }
cat > "$tmp/textmod.k" <<'EOF'
module textmod;
import keel.buffer as buffer types;

pub typedef buffer u8 text;

text make(u8 *src) { return buffer.of(src, 4); }

pub int main(void) {
    u8 src[4] = {'a', 'b', 'c', 'd'};
    text t = buffer.of(src, 4);
    text u = {0};
    u = buffer.of(src, 2);
    text v = make(src);
    return (buffer.length(t) == 4 && buffer.length(u) == 2 && buffer.length(v) == 4) ? 0 : 1;
}
EOF
out=$("$BIN" --base-dir "$K" -I "$tmp" --dest-dir "$tmp/g" -std=c2x -Wall -Wextra -Werror "$tmp/textmod.k" -o "$tmp/textmod" 2>&1) ||
    bad "cgen/cc failed [$(printf '%s\n' "$out" | grep -v 'info:' | head -3)]"
grep -q 'textmod_text.*= keel_buffer_u8_of(' "$tmp/g/textmod.c" || bad "the call is not keel_buffer_u8_of"
grep -q 'text_of\|textmod_text_' "$tmp/g/textmod.c" && bad "an instance of the alias appeared"
[ $fail -eq 0 ] && { "$tmp/textmod" || bad "wrong result"; }
[ $fail -eq 0 ] && echo ok
exit $fail
