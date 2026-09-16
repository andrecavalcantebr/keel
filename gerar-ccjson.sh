#!/usr/bin/env sh
# Emite os bancos de compilação que o clangd (Zed, VSCode, vim) lê para resolver
# os `#include` e, com eles, os tipos.
#
# O repositório tem VÁRIAS árvores de header com os MESMOS nomes e conteúdos
# diferentes — `gen/keel/keel_buffer_i32.type.h` não é o de `golden/c23/keel/`.
# Um `-I` global escolheria um dos dois para todo mundo e daria definição errada
# em silêncio. Então cada árvore tem o seu banco, e o clangd acha o certo subindo
# a partir do arquivo aberto:
#
#   ./compile_commands.json          src/ e as árvores geradas de src/base
#   golden/compile_commands.json     a suíte, nos dois perfis
#
# Os `-I` são RELATIVOS ao campo "directory" de cada entrada, que é a raiz do
# repositório. Só ele é absoluto; mover o repo é reexecutar este script.
#
# Headers entram no banco como entradas próprias. Sem isso o clangd adivinha as
# flags de um `.h` a partir do `.c` "mais parecido", e num projeto que é quase
# todo header essa adivinhação erra.
set -eu
raiz=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$raiz"

STD=c2x
sep=""

entrada() {  # $1 = arquivo relativo, $2... = flags
  arq=$1; shift
  printf '%s  {"directory":"%s","file":"%s","command":"cc -std=%s %s -c %s"}\n' \
         "$sep" "$raiz" "$arq" "$STD" "$*" "$arq"
  sep=","
}

{
  printf '[\n'

  # o executável do projeto: vê a base gerada em gen/ e os próprios fontes
  for c in $(find src -maxdepth 1 -name '*.c' | sort); do
    entrada "$c" "-I gen -I src"
  done

  # as ferramentas de bootstrap: fecham em si mesmas
  for c in $(find tools/codegen/src -name '*.c' 2>/dev/null | sort); do
    entrada "$c" "-I tools/codegen/src"
  done

  # cada árvore de header gerada responde pela própria raiz: é o que faz
  # `#include "keel.type.h"` achar o prelúdio e, com ele, i32 e os demais
  for arvore in gen src/gen src/prova; do
    [ -d "$arvore" ] || continue
    for h in $(find "$arvore" -name '*.h' | sort); do
      entrada "$h" "-I $arvore"
    done
  done

  printf ']\n'
} > compile_commands.json

echo "compile_commands.json: $(grep -c '"file"' compile_commands.json) entradas"
[ -x golden/gerar-ccjson.sh ] && ./golden/gerar-ccjson.sh
