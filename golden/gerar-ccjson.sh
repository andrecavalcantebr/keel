#!/usr/bin/env sh
# Emite golden/compile_commands.json para o clangd (Zed, VSCode, vim) resolver
# `#include "keel/prelude.h"` e os headers gerados de cada caso.
#
# Os headers fixos vivem em golden/<perfil>/keel/; os gerados, em
# casos/<caso>/esperado/<perfil>/. Sem este arquivo o editor não tem como saber
# disso e não acha nada.
#
# O perfil de cada arquivo vem do CAMINHO dele. Arquivo que não é de perfil
# nenhum — prova.c, que é arnês e não saída — entra uma vez, com o c23.
set -eu
raiz=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$raiz"

entrada() {  # $1 = arquivo relativo, $2 = perfil, $3 = caso
  [ "$2" = c23 ] && std=c2x || std=c11
  inc="-I$raiz/$2"
  [ -d "$3/esperado/$2" ] && inc="$inc -I$raiz/$3/esperado/$2"
  [ -d "$3/$2" ]          && inc="$inc -I$raiz/$3/$2"
  inc="$inc -I$raiz/$3"
  printf '%s  {"directory":"%s","file":"%s/%s","command":"cc -std=%s %s -c %s/%s"}\n' \
         "$sep" "$raiz" "$raiz" "$1" "$std" "$inc" "$raiz" "$1"
  sep=","
}

printf '[\n' > compile_commands.json
sep=""
for caso in casos/*; do
  for f in $(find "$caso" -name '*.c' | sort); do
    case "$f" in
      */esperado/c23/*) p=c23 ;;
      */esperado/c11/*) p=c11 ;;
      *.c11.c)          p=c11 ;;
      *)                p=c23 ;;     # prova.c, aux.c, e os casos ainda não convertidos
    esac
    entrada "$f" "$p" "$caso" >> compile_commands.json
  done
done
printf ']\n' >> compile_commands.json
echo "compile_commands.json: $(grep -c '"file"' compile_commands.json) entradas"
