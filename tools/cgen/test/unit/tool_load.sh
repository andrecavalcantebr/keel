#!/bin/sh
# Hand-written oracle for tool/tool.c. Not model-generated. Run from the
# repository root: it loads the repository's own base/ tree.
set -u
SRC="tools/cgen/src/tool/tool.c tools/cgen/src/tool/paths.c tools/cgen/src/tool/read_source.c"
TESTMAIN=tools/cgen/test/unit/tool_load_main.c
DEPS=$(find tools/cgen/src/engine -name '*.c')
# O carregador não libera nada, de propósito: as fatias de nome dos
# símbolos apontam para os fontes lidos, que têm de sobreviver enquanto
# o módulo existir (cgen-tool §3.2, "nada é liberado antes do fim do
# processo"). Para o LeakSanitizer isso é vazamento; para o desenho é a
# regra. As outras checagens do ASan e o UBSan seguem ligados.
export ASAN_OPTIONS=detect_leaks=0
BIN=$(mktemp); trap 'rm -f "$BIN"' EXIT
gcc -std=c2x -I tools/cgen/src -I tools/cgen/gen -Wall -Wextra \
    -fsanitize=address,undefined -fno-sanitize-recover=all -g -O0 \
    $SRC $DEPS "$TESTMAIN" -o "$BIN" || { echo "FAIL: compile error"; exit 1; }
timeout 20 "$BIN" || { echo "FAIL: timed out or crashed (exit $?)"; exit 1; }
