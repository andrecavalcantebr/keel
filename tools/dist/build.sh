#!/bin/sh
# Reconstrói o cgen a partir de lib/src (o fonte do cgen, com a base de apoio
# dele já gerada: o cgen ainda não compila a si mesmo). Escreve ./cgen.
#
#   ./build.sh [compilador]    (padrão: cc)
set -eu
here=$(cd "$(dirname "$0")" && pwd)
cc=${1:-cc}
srcs=$(find "$here/src" -name '*.c' | sort)
# Procedência para `cgen --cgen-version`: o commit vem do VERSION da
# distribuição (sem git aqui); a data e o compilador são os desta reconstrução.
commit=$(sed -n 's/^commit //p' "$here/../../VERSION" 2>/dev/null || true)
date=$(date -u +%Y-%m-%dT%H:%M:%SZ)
ccid=$("$cc" --version 2>/dev/null | head -n1 | sed 's/"//g')
# shellcheck disable=SC2086
"$cc" -std=c2x -O2 -DNDEBUG -I "$here/src" -I "$here/gen" \
  -DCGEN_BUILD_COMMIT="\"${commit:-unknown}\"" -DCGEN_BUILD_DATE="\"$date\"" \
  -DCGEN_BUILD_CC="\"${ccid:-unknown}\"" $srcs -lm -o cgen
echo "ok: ./cgen"
