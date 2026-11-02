#!/bin/sh
# Invocation errors of the tool that have a name (cgen-tool-spec §7.1): all exit 2.
cd "$(dirname "$0")/../../.." || exit 2
BIN=$PWD/tools/cgen/cgen
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=gen "$tmp/missing.k" 2>&1); rc=$?
[ $rc -eq 2 ] && echo "$out" | grep -q '\[source-not-found\]' || bad "a missing .k: rc=$rc [$out]"
out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=parse "$tmp/missing.k" 2>&1); rc=$?
[ $rc -eq 2 ] && echo "$out" | grep -q '\[source-not-found\]' || bad "a missing .k under parse: rc=$rc [$out]"
printf 'module a;\npub int f(void) { return 1; }\n' > "$tmp/a.k"
printf 'module b;\npub int g(void) { return 1; }\n' > "$tmp/b.k"
out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=gen "$tmp/a.k" "$tmp/b.k" 2>&1); rc=$?
[ $rc -eq 2 ] && echo "$out" | grep -q '\[multiple-sources\]' || bad "two sources: rc=$rc [$out]"
printf 'module e;\nextern_c [type_h] { typedef int T; }\n' > "$tmp/e.k"
out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=gen --dest-dir "$tmp/g" "$tmp/e.k" 2>&1); rc=$?
[ $rc -eq 2 ] && echo "$out" | grep -q 'implementation-limit' || bad "extern_c on one line must be refused, not lost: rc=$rc [$out]"

[ $fail -eq 0 ] && echo "source errors: ok"
exit $fail
