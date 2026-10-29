#!/bin/sh
# Oracle of --instance (cgen design §2.8, §8.7, §8.8): the invocation errors, by
# exit code and diagnostic id, and the files a valid use writes. Expected values
# written by hand from the design. Run from the repository root.
cd "$(dirname "$0")/../../.." || exit 2
BIN=$PWD/tools/cgen/cgen
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
bad=0 n=0
# use | exit code | id in stderr
while IFS='|' read -r use code id; do
    n=$((n+1))
    "$BIN" -std=c23 --base-dir base --stop-after=gen --dest-dir "$tmp/out" --instance "$use" >/dev/null 2>"$tmp/err"
    rc=$?
    if [ $rc -ne "$code" ] || ! grep -q "\[$id\]" "$tmp/err"; then
        printf 'FAIL   --instance "%s": exit %s, want %s [%s]\n' "$use" "$rc" "$code" "$id"; sed 's/^/       /' "$tmp/err"; bad=$((bad+1))
    fi
done <<'CASES'
keel.buffer i32|2|invalid-instance
keel.range.range i32|2|invalid-instance
keel.buffer.nope i32|2|invalid-instance
keel.buffer.buffer|2|invalid-instance
keel.buffer.buffer i32 i32|2|invalid-instance
keel.nope.x i32|2|module-not-found
keel.buffer.buffer i32|0|redundant-instance
keel.array i32|0|redundant-instance
CASES
# §8.8: a generic module is not a source of its own
n=$((n+1))
"$BIN" -std=c23 -I base --base-dir base --stop-after=gen --dest-dir "$tmp/g" base/keel/buffer.k >/dev/null 2>"$tmp/err"
rc=$?
if [ $rc -ne 2 ] || ! grep -q '\[generic-source-without-instance\]' "$tmp/err"; then
    printf 'FAIL   base/keel/buffer.k: exit %s, want 2 [generic-source-without-instance]\n' "$rc"; bad=$((bad+1))
fi
# §8.7: the instance's two headers and its .c, which holds only the include
rm -rf "$tmp/out"
"$BIN" -std=c23 --base-dir base --stop-after=gen --dest-dir "$tmp/out" --instance "keel.buffer.buffer i32" 2>/dev/null
for f in keel/keel_buffer_i32.type.h keel/keel_buffer_i32.h keel/keel_buffer_i32.c keel.h keel.type.h; do
    n=$((n+1)); [ -f "$tmp/out/$f" ] || { printf 'FAIL   %s not written\n' "$f"; bad=$((bad+1)); }
done
n=$((n+1))
printf '/* keel/keel_buffer_i32.c \342\200\224 generated from keel/buffer.k by cgen, C23 profile. */\n#include "keel/keel_buffer_i32.h"\n' > "$tmp/want.c"
cmp -s "$tmp/want.c" "$tmp/out/keel/keel_buffer_i32.c" || { printf 'FAIL   keel_buffer_i32.c\n'; bad=$((bad+1)); }
[ $bad -eq 0 ] && echo "ok ($n checks)" || { echo "$bad of $n failed"; exit 1; }
