#!/usr/bin/env sh
# golden/run.sh — compila o que o cgen deve produzir, nos dois perfis, e roda a
# prova de cada caso.
#
#   casos/<caso>/caso.k              o fonte keel (mais outros .k, se houver)
#   casos/<caso>/esperado/<perfil>/  o que o cgen deve produzir
#   casos/<caso>/prova.c             o arnês — NÃO é saída do transpilador
#   casos/<caso>/VERIFICA            afirma o que não é "compila"; o script decide
#   casos/<caso>/PROBLEMA            xfail: espera-se que NÃO compile
#
# -Werror=vla cobra uma afirmação normativa: a linguagem §4.4 diz que keel não
# emite VLA nem alloca em lugar nenhum.
CC=${CC:-gcc}
FLAGS="-pedantic-errors -Wall -Wextra -Wno-unused-parameter -Wvla -Werror=vla"
ok=0; falha=0; xfail=0; xpass=0

# ---------------------------------------------------------------- estrutura
# O corte em camadas do backend §4.3.2 é verificável sem compilar nada, e é
# verificado aqui: um gerado que o viole compilaria mesmo assim, e a suíte
# deixaria de ser oráculo justamente da regra que mata os ciclos de inclusão.
estrutura=0
for h in $(find c23 c11 casos -name '*.h' ! -name '*.type.h' ! -name '*.impl.h' | sort); do
  case "$h" in */prelude.h) continue ;; esac
  base=${h%.h}
  for camada in .type.h .impl.h; do
    [ -f "$base$camada" ] || { printf 'ESTRUT %s — falta %s\n' "$h" "$base$camada"; estrutura=$((estrutura+1)); }
  done
done

# I1: um .type.h inclui apenas .type.h — é o que torna o grafo de layout um DAG
for t in $(find c23 c11 casos -name '*.type.h' | sort); do
  mau=$(grep '^#include "' "$t" | grep -v '\.type\.h"' | grep -v 'prelude\.h"')
  [ -z "$mau" ] || { printf 'ESTRUT %s — .type.h incluindo fora da camada:\n%s\n' "$t" "$mau"; estrutura=$((estrutura+1)); }
done

# I2: um .h não inclui o .h de outro módulo — protótipo não precisa de protótipo
for d in $(find c23 c11 casos -name '*.h' ! -name '*.type.h' ! -name '*.impl.h' | sort); do
  case "$d" in */prelude.h) continue ;; esac
  n=$(basename "$d" .h)
  mau=$(grep '^#include "' "$d" | grep -v '\.type\.h"' | grep -v '\.impl\.h"' \
        | grep -v 'prelude\.h"' | grep -v "/$n\.h\"\|\"$n\.h\"")
  [ -z "$mau" ] || { printf 'ESTRUT %s — .h incluindo .h de outro módulo:\n%s\n' "$d" "$mau"; estrutura=$((estrutura+1)); }
done
[ $estrutura -eq 0 ] && printf 'ok     estrutura de camadas         (backend §4.3.2)\n'
falha=$((falha+estrutura))

for caso in casos/*/; do
  nome=$(basename "$caso")
  for perfil in c23 c11; do
    [ "$perfil" = c23 ] && std=c2x || std=c11
    ger="$caso/esperado/$perfil"
    [ -d "$ger" ] || { printf 'SEM    %-24s %s  — sem esperado/%s\n' "$nome" "$perfil" "$perfil"; falha=$((falha+1)); continue; }

    if [ -x "$caso/VERIFICA" ]; then
      if "$caso/VERIFICA" "$CC" "-std=$std" "$perfil"; then
        printf 'ok     %-24s %s  (verifica)\n' "$nome" "$perfil"; ok=$((ok+1))
      else
        printf 'FALHA  %-24s %s  (verifica)\n' "$nome" "$perfil"; falha=$((falha+1))
      fi
      continue
    fi

    srcs=$(find "$ger" -name '*.c' | sort | tr '\n' ' ')
    [ -f "$caso/prova.c" ] && srcs="$srcs $caso/prova.c"
    inc="-I$perfil -I$ger -I$caso"

    # todo módulo com `pub` tem de ter `.h` — foi o que passou despercebido 16 vezes
    if grep -q '^pub ' "$caso"/*.k 2>/dev/null && ! find "$ger" -name '*.h' ! -name '*.type.h' ! -name '*.impl.h' | grep -q .; then
      printf 'FALHA  %-24s %s  — módulo com `pub` e sem .h gerado\n' "$nome" "$perfil"; falha=$((falha+1)); continue
    fi

    modo=compila; exe=/dev/null
    grep -lq '^int main' $srcs 2>/dev/null && { modo=executa; exe=$(mktemp); }
    omps=""; grep -lq '#pragma omp' $srcs 2>/dev/null && omps=" -fopenmp"

    bom=1
    for omp in "" $omps; do
      [ -z "$omp" ] && extra=-Wno-unknown-pragmas || extra=""
      $CC "-std=$std" $FLAGS $extra $omp $inc $([ $modo = compila ] && echo -c) $srcs -o "$exe" 2>/dev/null \
        && { [ $modo = compila ] || "$exe" >/dev/null; } || bom=0
    done
    [ -n "$omps" ] && modo="$modo, ±omp"

    if [ $bom -eq 1 ]; then
      if [ -f "$caso/PROBLEMA" ]; then
        printf 'XPASS  %-24s %s  — compilou, mas há PROBLEMA\n' "$nome" "$perfil"; xpass=$((xpass+1))
      else
        printf 'ok     %-24s %s  (%s)\n' "$nome" "$perfil" "$modo"; ok=$((ok+1))
      fi
    elif [ -f "$caso/PROBLEMA" ]; then
      printf 'xfail  %-24s %s  — %s\n' "$nome" "$perfil" "$(head -1 "$caso/PROBLEMA")"; xfail=$((xfail+1))
    else
      printf 'FALHA  %-24s %s\n' "$nome" "$perfil"; falha=$((falha+1))
      for s1 in $srcs; do
        $CC "-std=$std" $FLAGS -Wno-unknown-pragmas $omps $inc -fsyntax-only "$s1" 2>&1 | sed 's/^/         /'
      done | head -8
    fi
  done
done
printf '\n%d ok, %d falha, %d xfail, %d xpass\n' $ok $falha $xfail $xpass
[ $falha -eq 0 ] && [ $xpass -eq 0 ]
