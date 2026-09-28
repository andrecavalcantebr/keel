#!/bin/sh
# Hand-written oracle for tool/paths.c. Not model-generated.
set -u
SRC=tools/cgen/src/tool/paths.c
TESTMAIN=tools/cgen/test/unit/paths_main.c
BIN=$(mktemp); trap 'rm -f "$BIN"' EXIT
gcc -std=c2x -I tools/cgen/src -I tools/cgen/gen -Wall -Wextra \
    -fsanitize=address,undefined -fno-sanitize-recover=all -g -O0 \
    "$SRC" "$TESTMAIN" -o "$BIN" || { echo "FAIL: compile error"; exit 1; }
timeout 10 "$BIN" || { echo "FAIL: timed out or crashed (exit $?)"; exit 1; }
