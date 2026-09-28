#!/bin/sh
set -eu
BIN=$(mktemp)
trap 'rm -f "$BIN"' EXIT
gcc -std=c2x -I tools/cgen/src -I tools/cgen/gen -Wall -Wextra \
    -fsanitize=address,undefined -fno-sanitize-recover=all -g \
    tools/cgen/test/unit/storage_buffers_main.c \
    $(find tools/cgen/src/engine -name '*.c') -o "$BIN"
"$BIN"
