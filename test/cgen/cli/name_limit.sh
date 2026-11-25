#!/bin/sh
# A generated name is capped at 215 characters (backend §2.4): the instance's symbol is also its file
# name, and the file systems take 255 bytes per component, with room left for `.type.h`, the
# temporary file and the tools' own suffixes (E01). 215 passes and writes the files, 216 is
# `name-too-long`, and nothing in between is `write-failure` or a capacity.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=${CGEN_BIN:-$PWD/tools/cgen/cgen}
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

# the symbol is app_pbox_box_3_app_use_<T>: 23 characters and the type's name
try() {
    d=$tmp/$1; mkdir -p "$d/app"
    cat > "$d/app/pbox.k" <<'K'
module app.pbox dim N type T;
pub constexpr size_t CAP = N;
pub modifier box byref { T v[N]; }
pub inline size_t cap(box *b) { return CAP; }
K
    t=T$(printf 'x%.0s' $(seq 1 $(($2 - 1))))
    printf 'module app.use;\nimport app.pbox types;\ntypedef int %s;\npub size_t probe(void) { box(3) %s b; (void)b; return cap(b); }\n' "$t" "$t" > "$d/app/use.k"
    "$BIN" --base-dir "$K" -I "$d" --stop-after=gen --dest-dir "$d/out" "$d/app/use.k" >"$d/log" 2>&1
    echo $? > "$d/rc"
}

try ok 192      # 23 + 192 = 215
[ "$(cat $tmp/ok/rc)" = 0 ] || bad "215 characters must pass: $(grep -v 'info:' $tmp/ok/log | head -3 | cut -c1-200)"
n=$(find "$tmp/ok/out" -name '*.type.h' | wc -l)
[ "$n" -ge 1 ] || bad "215 characters must write the .type.h"
try over 193    # 216
rc=$(cat $tmp/over/rc)
[ "$rc" != 0 ] && grep -q 'limit of 215 characters.*\[name-too-long\]' "$tmp/over/log" \
    || bad "216 characters must be name-too-long naming 215 (rc=$rc): $(grep -v 'info:' $tmp/over/log | head -2 | cut -c1-200)"

[ $fail -eq 0 ] && echo ok
exit $fail
