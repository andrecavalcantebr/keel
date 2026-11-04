#!/bin/sh
# Reconstrói o cgen a partir de lib/src (o fonte do cgen, com a base de apoio
# dele já gerada: o cgen ainda não compila a si mesmo). Escreve ./cgen.
#
#   ./build.sh [compilador]    (padrão: cc)
set -eu
here=$(cd "$(dirname "$0")" && pwd)
cc=${1:-cc}
srcs=$(find "$here/src" -name '*.c' | sort)
# shellcheck disable=SC2086
"$cc" -std=c2x -O2 -DNDEBUG -I "$here/src" -I "$here/gen" $srcs -lm -o cgen
echo "ok: ./cgen"
