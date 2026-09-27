#!/bin/sh
# Minimal oracle for the k_resolve_imports recognizer task. Not model-generated.
set -u
SRC=tools/cgen/src/engine/parser_imports.c
TESTMAIN=tools/cgen/test/unit/parser_imports_main.c
# Every engine source except the one(s) under test. A hand-written list
# is not what this oracle checks — it only mirrors the call graph, and it
# rots silently: on 2026-09-27 adding one call to an existing recognizer
# broke four of these scripts in an afternoon, each as a link error that
# reads like a test failure. `$SRC` may name more than one file, and an
# empty one (a task not yet accepted) is fine: it compiles to nothing.
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
