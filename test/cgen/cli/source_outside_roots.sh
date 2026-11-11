#!/bin/sh
# A .k under no -I root is reported as such ([source-outside-roots]), never as
# module-path-mismatch against an empty expected name (the empty name is the
# search's own "I don't know" sentinel, not the program's declared name).
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=$PWD/tools/cgen/cgen
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

mkdir "$tmp/elsewhere"
printf 'module out;\n\npub i32 f(i32 a) {\n    return a;\n}\n' > "$tmp/elsewhere/out.k"

for stop in parse gen; do
    extra=""; [ "$stop" = gen ] && extra="--dest-dir $tmp/dest"
    out=$("$BIN" --base-dir "$K" -I "$tmp/other" --stop-after=$stop $extra "$tmp/elsewhere/out.k" 2>&1); rc=$?
    [ $rc -ne 0 ] || bad "$stop: must be refused"
    echo "$out" | grep -q '\[source-outside-roots\]' || bad "$stop: no [source-outside-roots] [$out]"
    echo "$out" | grep -q 'module-path-mismatch' && bad "$stop: still module-path-mismatch [$out]"
done

# under the right root, the same file is accepted
out=$("$BIN" --base-dir "$K" -I "$tmp/elsewhere" --stop-after=parse "$tmp/elsewhere/out.k" 2>&1); rc=$?
[ $rc -eq 0 ] || bad "under its own root must pass: rc=$rc [$out]"

[ $fail -eq 0 ] && echo ok
exit $fail
