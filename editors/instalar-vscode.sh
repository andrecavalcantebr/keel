#!/usr/bin/env sh
# Instala a extensão do VSCode como extensão de desenvolvimento.
#
# O VSCode carrega qualquer diretório sob `extensions/` que tenha um
# `package.json` válido, e segue link simbólico — que é o que se quer aqui:
# rodar `gerar.py` de novo atualiza a extensão instalada sem reinstalar nada.
#
#   ./editors/instalar-vscode.sh              # link para editors/vscode/
#   ./editors/instalar-vscode.sh --copiar     # cópia, para levar a outra máquina
#
# Destino: $VSCODE_EXT, ou ~/.vscode/extensions. Para Remote-SSH use
# VSCODE_EXT=~/.vscode-server/extensions; para VSCodium, ~/.vscode-oss/extensions.
# Depois: paleta de comandos → `Developer: Reload Window`.
set -eu

origem=$(CDPATH= cd -- "$(dirname -- "$0")/vscode" && pwd)
modo=${1:---link}
raiz=${VSCODE_EXT:-$HOME/.vscode/extensions}

leia() { sed -n "s/.*\"$1\": *\"\([^\"]*\)\".*/\1/p" "$origem/package.json" | head -1; }
nome="$(leia publisher).$(leia name)-$(leia version)"
destino="$raiz/$nome"

[ -d "$raiz" ] || { echo "não achei $raiz — o VSCode está instalado?" >&2; exit 1; }

if [ -e "$destino" ] && [ ! -L "$destino" ]; then
    echo "recusando: $destino existe e não é link." >&2
    echo "apague-o, ou instale com VSCODE_EXT apontando para outro lugar." >&2
    exit 1
fi
rm -f "$destino"

case "$modo" in
  --link)
    ln -s "$origem" "$destino"
    echo "link: $destino -> $origem" ;;
  --copiar)
    mkdir -p "$destino"
    (cd "$origem" && find . -type f -print) | while read -r f; do
        mkdir -p "$destino/$(dirname "$f")"
        cp "$origem/$f" "$destino/$f"
    done
    echo "cópia em: $destino" ;;
  *) echo "uso: $0 [--link|--copiar]" >&2; exit 2 ;;
esac

echo
echo "no VSCode: paleta de comandos → 'Developer: Reload Window'"
echo "para conferir: abra um .k e rode 'Developer: Inspect Editor Tokens and Scopes'"
