#!/bin/sh
# Oracle for the m2-parser-decl-keel task. Not model-generated.
#
# Depends on parser_declarator_head.c and parser_opaque_until.c — run
# m2-parser-declarator-head and m2-parser-opaque-until first; this task
# composes them and will not even compile until both exist.
set -u
SRC=tools/cgen/src/engine/parser_decl_keel.c
TESTMAIN=tools/cgen/test/unit/parser_decl_keel_main.c
DEPS="tools/cgen/src/engine/parser_declarator.c tools/cgen/src/engine/lexer.c tools/cgen/src/engine/lexer_peek.c \
tools/cgen/src/engine/lexer_scan_directive.c tools/cgen/src/engine/lexer_scan_identifier.c \
tools/cgen/src/engine/lexer_scan_number.c tools/cgen/src/engine/lexer_scan_punct.c \
tools/cgen/src/engine/lexer_scan_quoted.c tools/cgen/src/engine/lexer_skip_trivia.c \
tools/cgen/src/engine/token_predicates_shape.c tools/cgen/src/engine/token_predicates_words.c \
tools/cgen/src/engine/diag.c tools/cgen/src/engine/parser_known_type.c \
tools/cgen/src/engine/parser_declarator_head.c tools/cgen/src/engine/parser_opaque_until.c"
BIN=$(mktemp); trap 'rm -f "$BIN"' EXIT
gcc -std=c2x -I tools/cgen/src -I tools/cgen/gen -Wall -Wextra \
    -fsanitize=address,undefined -fno-sanitize-recover=all -g -O0 \
    "$SRC" $DEPS "$TESTMAIN" -o "$BIN" || { echo "FAIL: compile error"; exit 1; }
timeout 10 "$BIN" || { echo "FAIL: timed out or crashed (exit $?)"; exit 1; }
