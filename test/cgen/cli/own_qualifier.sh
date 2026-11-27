#!/bin/sh
# The module's own name qualifies its own symbols (spec §2.5; keel_libs finding A9, cgen 0.4.2). Inside module
# `own`, `own.mat(C, R)` is the modifier `mat`, as `mat(C, R)` is: in a generic module (a type argument of
# `outcome`, a local) and in a plain one (`app.Rec`). It used to go on as C text and stop in the C compiler
# (`unknown type name`). Expected by hand: no `own.` or `app.` reaches the C, the code compiles with
# -Wall -Wextra -Werror and the program returns 0.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=${CGEN_BIN:-$PWD/tools/cgen/cgen}
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }
cat > "$tmp/own.k" <<'EOF'
module own dim R, C;
import keel.outcome as outcome types;

pub modifier mat { double d[R * C]; }

pub inline outcome own.mat(C, R) tr(mat *a) {
    outcome own.mat(C, R) r = {0};
    own.mat(C, R) v = {0};
    for (size_t i = 0; i < R; ++i)
        for (size_t j = 0; j < C; ++j) v.d[j * R + i] = a->d[i * C + j];
    return outcome.win(r, v);
}
EOF
cat > "$tmp/app.k" <<'EOF'
module app;
import own types;
import keel.outcome as outcome types;

pub typedef struct { int a; } Rec;

pub int g(void) { app.Rec r = {7}; return r.a; }

pub int main(void) {
    mat(2, 3) a = {{1, 2, 3, 4, 5, 6}};
    outcome mat(3, 2) t = own.tr(a);
    return (g() == 7 && outcome.ok(t)) ? 0 : 1;
}
EOF
out=$("$BIN" --base-dir "$K" -I "$tmp" --dest-dir "$tmp/g" -std=c2x -Wall -Wextra -Werror "$tmp/app.k" -o "$tmp/app" 2>&1) ||
    bad "cgen/cc failed [$(printf '%s\n' "$out" | grep -v 'info:' | head -3)]"
grep -rq 'own\.mat\|app\.Rec' "$tmp/g" && bad "a qualified own name reached the C"
[ $fail -eq 0 ] && { "$tmp/app" || bad "wrong result"; }
[ $fail -eq 0 ] && echo ok
exit $fail
