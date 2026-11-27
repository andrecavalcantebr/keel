#!/bin/sh
# An instance the emitter does not cover now says where the program asked for it (keel_libs finding A8, cgen
# 0.4.2): the message was `cgen: error: rows_shape_f64: the emitter does not cover this instance yet`, with no
# position. Expected by hand: `<file>.k:<line>:<col>: error: ... the instance `rows_shape_f64` ... (at `mx`)
# [implementation-limit]`, the position being that of `mx.from(...)` on line 5 of the user's file; exit 2.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=${CGEN_BIN:-$PWD/tools/cgen/cgen}
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }
cat > "$tmp/rows_shape.k" <<'EOF'
module rows_shape dim ROWS, COLS;
pub modifier matrix { f64 data[ROWS * COLS]; }
pub inline matrix from(array const f64 values[size_t n][COLS]) {
    matrix r = {0};
    if (n != ROWS) return r;
    for (size_t i = 0; i < ROWS; ++i)
        for (size_t j = 0; j < COLS; ++j) r.data[i * COLS + j] = values[i][j];
    return r;
}
EOF
cat > "$tmp/rows_shape_use.k" <<'EOF'
module rows_shape_use;
import rows_shape as mx types;
pub int main(void) {
    array f64 values[2][3] = {{1,2,3},{4,5,6}};
    matrix(2,3) a = mx.from(values);
    (void)a;
    return 0;
}
EOF
out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=gen --dest-dir "$tmp/g" "$tmp/rows_shape_use.k" 2>&1); rc=$?
[ $rc -eq 2 ] || bad "exit $rc"
printf '%s\n' "$out" | grep -q '^rows_shape_use.k:5:[0-9]*: error: the emitter does not cover the instance `rows_shape_f64` yet (at `mx`) \[implementation-limit\]$' ||
    bad "no position in [$out]"
[ $fail -eq 0 ] && echo ok
exit $fail
