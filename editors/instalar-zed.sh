#!/usr/bin/env sh
# Publica a extensão do Zed num diretório PRÓPRIO, com `git init`.
#
# É necessário: o Zed exige que uma extensão seja um repositório Git, e recusa
# um subdiretório de outro repositório — verificado na prática, não só nos docs.
# E `editors/` não pode simplesmente morar fora do repositório do keel, porque
# `gerar.py` lê a tabela do §3.7 de `keel-spec.md`, que fica ao lado.
#
#   ./editors/instalar-zed.sh [destino]     (padrão: ~/.local/share/keel-zed)
#
# Depois, no Zed: paleta de comandos → `zed: install dev extension` → escolher o
# destino. Idempotente: rodar de novo atualiza os arquivos e faz um commit se
# algo mudou. Mexeu no highlights.scm? Rode de novo e `zed: reload extensions`.
set -eu

origem=$(CDPATH= cd -- "$(dirname -- "$0")/zed" && pwd)
destino=${1:-$HOME/.local/share/keel-zed}

[ -e "$destino/.git" ] || [ ! -e "$destino" ] || {
    echo "recusando: $destino existe e não é repositório Git" >&2
    echo "escolha outro destino, ou apague esse diretório." >&2
    exit 1
}

mkdir -p "$destino"
(cd "$origem" && find . -type f -not -path './.git/*' -print) | while read -r f; do
    mkdir -p "$destino/$(dirname "$f")"
    cp "$origem/$f" "$destino/$f"
done

if [ ! -d "$destino/.git" ]; then
    git -C "$destino" init -q
    echo "git init em $destino"
fi
git -C "$destino" add -A
if git -C "$destino" diff --cached --quiet; then
    echo "nada mudou."
else
    git -C "$destino" commit -qm "keel: extensão do Zed, gerada de editors/zed/"
    echo "atualizado."
fi

echo
echo "extensão em: $destino"
echo "no Zed: paleta de comandos → 'zed: install dev extension' → esse diretório"
