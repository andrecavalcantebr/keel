#!/bin/sh
# Oracle of engine/emit/emit.c (M3, tools/agents/tasks/T7-emit.md): expected
# values written by hand from cgen design §8.3 and golden/<p>/keel.*. Not
# model-generated. Run from the repository root (tools/cgen/test/run.sh does).
set -u
SRC=tools/cgen/src/engine/emit/emit.c
TESTMAIN=tools/cgen/test/unit/emit_main.c
DEPS=""
for f in $(find tools/cgen/src/engine -name '*.c' | sort); do
    case " $SRC " in *" $f "*) continue ;; esac
    DEPS="$DEPS $f"
done
BIN=$(mktemp); trap 'rm -f "$BIN"' EXIT
gcc -std=c2x -I tools/cgen/src -I tools/cgen/gen -Wall -Wextra \
    -fsanitize=address,undefined -fno-sanitize-recover=all -g -O0 \
    "$SRC" $DEPS "$TESTMAIN" -o "$BIN" || { echo "FAIL: compile error"; exit 1; }
ASAN_OPTIONS=detect_leaks=0 timeout 10 "$BIN" || { echo "FAIL: timed out or crashed (exit $?)"; exit 1; }
