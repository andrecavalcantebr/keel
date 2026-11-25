#!/bin/sh
# `slice const T` with a struct `T` in a generic module (keel_coll finding A1): the instance spells the
# qualified type as C does (`const t_use_record *`), not as the identifier `const_t_use_record`, which
# no C declaration defines. Expected by hand: the instance header holds `const t_use_record *p` in `from`,
# holds no `const_t_use_record` outside of its own name `keel_slice_const_t_use_record`, and the C
# generated for the module compiles with -Wall -Werror. The scalar control keeps working.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=${CGEN_BIN:-$PWD/tools/cgen/cgen}
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

mkdir -p "$tmp/t"
cat > "$tmp/t/cs.k" <<'EOF'
module t.cs type T;
import keel.slice as slice;
pub modifier cs byref { T *values; size_t length; }
pub inline size_t length(cs *s) { return s->length; }
pub inline child slice.slice const T as_slice(parent cs *s, size_t a, size_t b) {
    return (slice.slice const T){ b - a, s->values + a };
}
EOF
cat > "$tmp/t/use.k" <<'EOF'
module t.use;
import keel types;
import t.cs as cs types;
import keel.slice as slice types;
pub typedef struct { i32 key; f64 payload; } record;
pub i32 run(cs record *c) {
    slice const record s = slice.of(c, 0, 1);
    return slice.ptr(s, 0)->key;
}
EOF
cat > "$tmp/t/use_scalar.k" <<'EOF'
module t.use_scalar;
import keel types;
import t.cs as cs types;
import keel.slice as slice types;
pub i32 run(cs i32 *c) {
    slice const i32 s = slice.of(c, 0, 2);
    return *slice.ptr(s, 0);
}
EOF
for m in use use_scalar; do
    out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=gen --dest-dir "$tmp/out_$m" "$tmp/t/$m.k" 2>&1); rc=$?
    [ $rc -eq 0 ] || bad "$m: cgen rc=$rc [$out]"
    cc -std=c2x -fsyntax-only -Wall -Werror -I "$tmp/out_$m" "$tmp/out_$m/t/t_$m.c" 2>"$tmp/cc_$m" || bad "$m: the C does not compile [$(head -3 "$tmp/cc_$m")]"
done
h="$tmp/out_use/keel/keel_slice_const_t_use_record.h"
grep -q 'keel_slice_const_t_use_record_from(const t_use_record \*p, size_t n)' "$h" || bad "from is not spelled with const t_use_record"
if sed 's/keel_slice_const_t_use_record/KSCTUR/g' "$h" | grep -q 'const_t_use_record'; then bad "the identifier const_t_use_record is still used as a type"; fi

[ $fail -eq 0 ] && echo ok
exit $fail
