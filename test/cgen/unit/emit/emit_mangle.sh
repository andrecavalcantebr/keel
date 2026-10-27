#!/bin/sh
# Oracle of engine/emit/mangle.c (M3, tools/agents/tasks/T6-writer-mangle.md):
# expected values derived by hand from engine/emit/mangle.h. Not model-generated.
set -u
SRC=tools/cgen/src/engine/emit/mangle.c
TESTMAIN=test/cgen/unit/emit/emit_mangle_main.c
DEPS=""
for f in $(find tools/cgen/src/engine -name '*.c' | sort); do
    case " $SRC " in *" $f "*) continue ;; esac
    DEPS="$DEPS $f"
done
BIN=$(mktemp); trap 'rm -f "$BIN"' EXIT
gcc -std=c2x -I tools/cgen/src -I tools/cgen/gen -Wall -Wextra \
    -fsanitize=address,undefined -fno-sanitize-recover=all -g -O0 \
    "$SRC" $DEPS "$TESTMAIN" -o "$BIN" || { echo "FAIL: compile error"; exit 1; }
timeout 10 "$BIN" || { echo "FAIL: timed out or crashed (exit $?)"; exit 1; }
