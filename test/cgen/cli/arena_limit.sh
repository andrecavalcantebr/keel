#!/bin/sh
set -eu
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
# Force exhaustion inside recursive loading, independently of the host heap.
gcc -std=c2x -I tools/cgen/src -I tools/cgen/gen -DCGEN_ARENA_CAPACITY=2048 \
    -fsanitize=address,undefined -fno-sanitize-recover=all -g \
    $(find tools/cgen/src -name '*.c') -o "$WORK/cgen"
printf 'module small; import dep;\n' > "$WORK/small.k"
printf 'module dep;\n' > "$WORK/dep.k"
set +e
"$WORK/cgen" --base-dir base -I "$WORK" --stop-after=parse "$WORK/small.k" > "$WORK/out" 2> "$WORK/err"
rc=$?
set -e
[ "$rc" -eq 1 ] || [ "$rc" -eq 2 ]
[ ! -s "$WORK/out" ]
grep -q 'configured limit 2048 bytes.*CGEN_ARENA_CAPACITY.*\[implementation-limit\]' "$WORK/err"
! grep -Eq 'AddressSanitizer|runtime error:' "$WORK/err"
echo ok
