#!/bin/sh
# A construction the emitter does not cover is refused with its site: `file:line:col: error: ... (at `tok`)
# [implementation-limit]` (cgen-tool-spec §7.1: the position of the construction, when it is known).
# Case: `walk` over a container whose module has a `dim` (keel_coll finding A7). The expected line is
# written by hand from the source below: `walk` is at line 8, column 5.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=$PWD/tools/cgen/cgen
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

mkdir -p "$tmp/c"
cat > "$tmp/c/m.k" <<'EOF'
module c.m dim N type T;
pub modifier m byref { T *v; size_t len; }
pub typedef struct { size_t i; } cursor;
pub inline child cursor begin(parent m *s) { (void)s; return (cursor){0}; }
pub inline bool has_next(m *s, cursor *c) { return c->i < s->len; }
pub inline child const T *next(parent m *s, cursor *c) { if (c->i >= s->len) return NULL; return &s->v[c->i++]; }
EOF
cat > "$tmp/use.k" <<'EOF'
module use;
import keel types;
import c.m as mm types;
pub int main(void) {
    array u32 buf[4] = {1,2,3,4};
    m(4) u32 t; t.v = buf; t.len = 4;
    u32 s = 0;
    walk (const u32 *p, mm.cursor c : t) s += *p;
    return (int)s;
}
EOF
out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=gen --dest-dir "$tmp/out" "$tmp/use.k" 2>&1); rc=$?
[ $rc -eq 2 ] || bad "walk over a dim module: rc=$rc [$out]"
echo "$out" | grep -q 'use\.k:8:5: error: the emitter does not cover this construction yet (at `walk`) \[implementation-limit\]' ||
    bad "the refusal does not name its site [$out]"

[ $fail -eq 0 ] && echo ok
exit $fail
