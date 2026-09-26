#!/bin/sh
# Minimal oracle for the m2-parser-modifier-decl task. Not model-generated.
set -u
SRC=tools/cgen/src/engine/parser_modifier_decl.c
TESTMAIN=tools/cgen/test/unit/parser_modifier_decl_main.c
DEPS="tools/cgen/src/engine/lexer.c tools/cgen/src/engine/lexer_peek.c \
tools/cgen/src/engine/lexer_scan_directive.c tools/cgen/src/engine/lexer_scan_identifier.c \
tools/cgen/src/engine/lexer_scan_number.c tools/cgen/src/engine/lexer_scan_punct.c \
tools/cgen/src/engine/lexer_scan_quoted.c tools/cgen/src/engine/lexer_skip_trivia.c \
tools/cgen/src/engine/token_predicates_shape.c tools/cgen/src/engine/token_predicates_words.c tools/cgen/src/engine/diag.c \
tools/cgen/src/engine/parser_extern_c.c"
BIN=$(mktemp); trap 'rm -f "$BIN"' EXIT
gcc -std=c2x -I tools/cgen/src -I tools/cgen/gen -Wall -Wextra \
    -fsanitize=address,undefined -fno-sanitize-recover=all -g -O0 \
    "$SRC" $DEPS "$TESTMAIN" -o "$BIN" || { echo "FAIL: compile error"; exit 1; }
timeout 10 "$BIN" || { echo "FAIL: timed out or crashed (exit $?)"; exit 1; }
