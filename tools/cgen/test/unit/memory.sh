#!/bin/sh
set -eu
BIN=$(mktemp); ERR=$(mktemp)
trap 'rm -f "$BIN" "$ERR"' EXIT
gcc -std=c2x -I tools/cgen/src -I tools/cgen/gen -Wall -Wextra \
    -fsanitize=address,undefined -fno-sanitize-recover=all -g \
    tools/cgen/test/unit/memory_main.c -o "$BIN"
"$BIN" 2>"$ERR"
grep -q 'configured limit 256 bytes.*CGEN_ARENA_CAPACITY.*\[implementation-limit\]' "$ERR"
