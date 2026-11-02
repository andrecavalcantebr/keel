#!/bin/sh
# -f writes every generated file even when its content is the same, so the timestamp moves
# (cgen-tool-spec §4.1, §6 rule 2); without it an identical file is not touched.
cd "$(dirname "$0")/../../.." || exit 2
BIN=$PWD/tools/cgen/cgen
K=$PWD/base
CASE=$PWD/golden/cases/033-debug-more
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }
run() { "$BIN" -std=c2x --base-dir "$K" -I "$CASE" --dest-dir "$tmp/gen" --stop-after=gen "$@" "$CASE/app/dbg.k" >/dev/null 2>&1; }

run || bad "first generation failed"
f=$tmp/gen/app/app_dbg.c
touch -d '2001-01-01 00:00:00' "$f"
run
[ "$(stat -c %Y "$f")" = "$(date -d '2001-01-01 00:00:00' +%s)" ] || bad "an identical file was touched without -f"
run -f
[ "$(stat -c %Y "$f")" -gt "$(date -d '2001-01-01 00:00:00' +%s)" ] || bad "-f did not move the timestamp of an identical file"
[ $fail -eq 0 ] && echo "force: ok"
exit $fail
