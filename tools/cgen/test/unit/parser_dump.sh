#!/bin/sh
# Test of engine/ast.c and parser_dump.c against a self-contained
# synthetic module. Not model-generated (this is Claude-written code, not
# a harness task — see its own file header).
set -u
SRC="tools/cgen/src/engine/ast.c tools/cgen/src/engine/parser_dump.c"
TESTMAIN=tools/cgen/test/unit/parser_dump_main.c
DEPS="tools/cgen/src/engine/lexer.c tools/cgen/src/engine/lexer_peek.c \
tools/cgen/src/engine/lexer_scan_directive.c tools/cgen/src/engine/lexer_scan_identifier.c \
tools/cgen/src/engine/lexer_scan_number.c tools/cgen/src/engine/lexer_scan_punct.c \
tools/cgen/src/engine/lexer_scan_quoted.c tools/cgen/src/engine/lexer_skip_trivia.c \
tools/cgen/src/engine/token_predicates_shape.c tools/cgen/src/engine/token_predicates_words.c \
tools/cgen/src/engine/diag.c tools/cgen/src/engine/parser_scan_qualified_name.c \
tools/cgen/src/engine/parser_header.c tools/cgen/src/engine/parser_extern_c.c \
tools/cgen/src/engine/parser_ident_list.c tools/cgen/src/engine/parser_modifier_decl.c \
tools/cgen/src/engine/parser_tags_decl.c tools/cgen/src/engine/parser_struct_decl.c \
tools/cgen/src/engine/parser_known_type.c tools/cgen/src/engine/parser_declarator_head.c \
tools/cgen/src/engine/parser_opaque_until.c tools/cgen/src/engine/parser_decl_keel.c \
tools/cgen/src/engine/parser_decl_constexpr.c \
tools/cgen/src/engine/parser_declarator.c tools/cgen/src/engine/parser_decl_typedef.c tools/cgen/src/engine/parser_extent_decl.c"
BIN=$(mktemp); trap 'rm -f "$BIN"' EXIT
gcc -std=c2x -I tools/cgen/src -I tools/cgen/gen -Wall -Wextra \
    -fsanitize=address,undefined -fno-sanitize-recover=all -g -O0 \
    $SRC $DEPS "$TESTMAIN" -o "$BIN" || { echo "FAIL: compile error"; exit 1; }
timeout 10 "$BIN" || { echo "FAIL: timed out or crashed (exit $?)"; exit 1; }
