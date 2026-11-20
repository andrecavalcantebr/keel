#!/bin/sh
# A library is linked after the units that use it (cgen-tool-spec §4.3): the generated `.c` come to the
# C compiler before the `.a`, `.so`, `-l`, `-L` and `-Wl,` the user wrote, wherever he wrote them. The
# linker resolves an archive only against what came before it, so the old order was `undefined reference`.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=$PWD/tools/cgen/cgen
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

mkdir -p "$tmp/src/geom" "$tmp/app"
cat > "$tmp/src/geom/vec.k" <<'EOK'
module geom.vec;
pub i32 twice(i32 x) { return x * 2; }
EOK
cat > "$tmp/app/main.k" <<'EOK'
module main;
import_c <stdio.h>;
import geom.vec as vec;
pub int main(void) { printf("%d\n", vec.twice(21)); return 0; }
EOK
"$BIN" --base-dir "$K" -I "$tmp/src" --dest-dir "$tmp/lib/gen" "$tmp/src/geom/vec.k" -c -o "$tmp/vec.o" >"$tmp/log" 2>&1 \
    || { cat "$tmp/log"; exit 1; }
ar rcs "$tmp/libgeom.a" "$tmp/vec.o"
gcc -shared -fPIC -o "$tmp/libgeom.so" "$tmp/vec.o" || exit 1

# $1 name, then the arguments the user writes after the `.k`
try() {
    name=$1; shift
    "$BIN" --base-dir "$K" -I "$tmp/src" -I "$tmp/app" --dest-dir "$tmp/out_$name" "$tmp/app/main.k" "$@" -o "$tmp/p_$name" >"$tmp/log_$name" 2>&1 \
        || { bad "$name must link: $(grep -v 'info:' "$tmp/log_$name" | head -3)"; return; }
    [ "$(LD_LIBRARY_PATH=$tmp "$tmp/p_$name")" = 42 ] || bad "$name linked and printed the wrong thing"
}

try archive          "$tmp/libgeom.a"
try dash_l           -L"$tmp" -lgeom
try dash_l_separate  -L "$tmp" -l geom
try whole_archive    -Wl,--whole-archive "$tmp/libgeom.a" -Wl,--no-whole-archive
try shared_path      "$tmp/libgeom.so"

# the library written before the `.k`
"$BIN" --base-dir "$K" -I "$tmp/src" -I "$tmp/app" --dest-dir "$tmp/out_first" "$tmp/libgeom.a" "$tmp/app/main.k" -o "$tmp/p_first" >"$tmp/log_first" 2>&1 \
    || bad "a library before the .k must link: $(grep -v 'info:' "$tmp/log_first" | head -3)"
[ "$("$tmp/p_first" 2>/dev/null)" = 42 ] || bad "a library before the .k printed the wrong thing"

[ $fail -eq 0 ] && echo ok
exit $fail
