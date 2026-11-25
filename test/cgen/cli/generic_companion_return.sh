#!/bin/sh
# A generic module that returns, or builds, an instance of ANOTHER generic module over its own binders
# (keel_coll finding A4): `holder K V` has `pair.pair K V at(holder *h) { return pair.make(h->k, h->v); }`.
# Expected by hand: the instance `r3_holder_i32_i64` includes the type header of `r3_pair_i32_i64` (not the
# generic's `r3_pair.type.h`), the call `pair.make` becomes `r3_pair_i32_i64_make`, the C compiles with
# -Wall -Werror, and running it gives the pair's value (2). Two forms of the target: `return` and an initializer.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=${CGEN_BIN:-$PWD/tools/cgen/cgen}
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

mkdir -p "$tmp/r3"
cat > "$tmp/r3/pair.k" <<'EOF'
module r3.pair type K, V;
pub modifier pair { K key; V value; }
pub inline pair make(K k, V v) { return (pair){k, v}; }
EOF
for form in ret init; do
    mkdir -p "$tmp/$form/r3"
    cp "$tmp/r3/pair.k" "$tmp/$form/r3/pair.k"
    if [ $form = ret ]; then
        body='return pair.make(h->k, h->v);'
    else
        body='pair.pair K V p = pair.make(h->k, h->v); return p;'
    fi
    cat > "$tmp/$form/r3/holder.k" <<EOF
module r3.holder type K, V;
import r3.pair as pair;
pub modifier holder byref { K k; V v; }
pub inline pair.pair K V at(holder *h) { $body }
EOF
    cat > "$tmp/$form/r3/use.k" <<'EOF'
module r3.use;
import keel types;
import r3.pair as pair types;
import r3.holder as hd types;
pub i64 run(void) {
    holder i32 i64 x = {1, 2};
    pair i32 i64 p = hd.at(x);
    return p.value;
}
EOF
    out=$("$BIN" --base-dir "$K" -I "$tmp/$form" --stop-after=gen --dest-dir "$tmp/out_$form" "$tmp/$form/r3/use.k" 2>&1); rc=$?
    [ $rc -eq 0 ] || { bad "$form: cgen rc=$rc [$out]"; continue; }
    h="$tmp/out_$form/r3/r3_holder_i32_i64.h"
    grep -q '#include "r3/r3_pair_i32_i64.type.h"' "$h" || bad "$form: no include of the pair instance's type header"
    grep -q 'r3/r3_pair.type.h' "$h" && bad "$form: includes the generic's header r3_pair.type.h"
    grep -q 'r3_pair_i32_i64_make(h->k, h->v)' "$h" || bad "$form: pair.make was not renamed"
    cat > "$tmp/proof_$form.c" <<'EOF'
#include "keel.type.h"
#include "r3/r3_use.h"
int main(void) { return r3_use_run() == 2 ? 0 : 1; }
EOF
    cc -std=c2x -Wall -Werror -I "$tmp/out_$form" "$tmp/proof_$form.c" "$tmp/out_$form/r3/r3_use.c" -o "$tmp/proof_$form" 2>"$tmp/cc_$form" ||
        { bad "$form: the C does not compile [$(head -3 "$tmp/cc_$form")]"; continue; }
    "$tmp/proof_$form" || bad "$form: the program returns the wrong value"
done

[ $fail -eq 0 ] && echo ok
exit $fail
