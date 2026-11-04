#!/bin/sh
# Remove o symlink que o install.sh criou, e só ele: se $bin/cgen não apontar
# para esta árvore, não é tocado.
#
#   ./uninstall.sh [diretório]    (padrão: ~/.local/bin)
set -eu
here=$(cd "$(dirname "$0")" && pwd)
bin=${1:-$HOME/.local/bin}
if [ -L "$bin/cgen" ] && [ "$(readlink "$bin/cgen")" = "$here/bin/cgen" ]; then
    rm "$bin/cgen"
    echo "removido: $bin/cgen"
else
    echo "uninstall.sh: $bin/cgen não é um link para $here/bin/cgen; nada a fazer" >&2
fi
