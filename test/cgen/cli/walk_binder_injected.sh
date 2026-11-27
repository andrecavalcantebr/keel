#!/bin/sh
# The cursor binder of `walk (const T *p, C c : x)` when the element binder starts with a qualifier
# (keel_libs findings A4 and A5, cgen 0.4.2). `const u32 *p, cursor c` was read as one declaration with two
# declarators, so `cursor` (or the alias `mm` of `mm.cursor`) counted as a declared name: the emitter left
# `cursor` unqualified and the parser warned about a shadowed name that nobody declared.
# (-Wno-instance-field-access: the test reads the fields of the support module on purpose.)
# Expected by hand: no shadowed-injected-name and no keel-name-shadowed, the C compiles with -Wall -Werror and
# the sum of {1,2,3,4} is 10, for the bare injected name and for the aliased one.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=${CGEN_BIN:-$PWD/tools/cgen/cgen}
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

mkdir -p "$tmp/c"
cat > "$tmp/c/m.k" <<'EOF'
module c.m type T;
pub modifier m byref { T *v; size_t len; }
pub typedef struct { size_t i; } cursor;
pub inline child cursor begin(parent m *s) { (void)s; return (cursor){0}; }
pub inline bool has_next(m *s, cursor *c) { return c->i < s->len; }
pub inline child const T *next(parent m *s, cursor *c) { if (c->i >= s->len) return NULL; return &s->v[c->i++]; }
EOF
for form in bare alias; do
    if [ $form = bare ]; then imp='import c.m types;'; ty=cursor; else imp='import c.m as mm types;'; ty=mm.cursor; fi
    cat > "$tmp/$form.k" <<EOF
module $form;
$imp
pub u32 total(void) {
    array u32 buf[4] = {1,2,3,4};
    m u32 t; t.v = buf; t.len = 4;
    u32 s = 0;
    walk (const u32 *p, $ty c : t) s += *p;
    return s;
}
pub int main(void) { return total() == 10 ? 0 : 1; }
EOF
    out=$("$BIN" --base-dir "$K" -I "$tmp" --dest-dir "$tmp/g_$form" -std=c2x -Wall -Wextra -Wno-instance-field-access -Werror "$tmp/$form.k" -o "$tmp/$form" 2>&1) ||
        { bad "$form: cgen/cc failed [$(printf '%s\n' "$out" | grep -v 'info:' | head -3)]"; continue; }
    printf '%s\n' "$out" | grep -q 'shadowed' && bad "$form: spurious shadow warning"
    "$tmp/$form" || bad "$form: wrong sum"
done
[ $fail -eq 0 ] && echo ok
exit $fail
