#!/bin/sh
# Instala o cgen: um symlink de bin/cgen em um diretório do PATH. Nada é copiado
# nem escrito fora do prefixo; a base é achada a partir do executável real
# (<executável>/../lib/base), então mover esta árvore não quebra nada.
#
#   ./install.sh                 symlink em ~/.local/bin
#   ./install.sh /usr/local/bin  symlink no diretório dado
set -eu
here=$(cd "$(dirname "$0")" && pwd)
bin=${1:-$HOME/.local/bin}
[ -x "$here/bin/cgen" ] || { echo "install.sh: $here/bin/cgen não existe" >&2; exit 1; }
mkdir -p "$bin"
ln -sf "$here/bin/cgen" "$bin/cgen"
echo "cgen instalado: $bin/cgen -> $here/bin/cgen"
case ":$PATH:" in *":$bin:"*) ;; *) echo "atenção: $bin não está no PATH" ;; esac
