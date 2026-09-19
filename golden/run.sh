#!/usr/bin/env sh
# golden/run.sh — compila o que o cgen deve produzir, nos dois perfis, e roda a
# prova de cada caso.
#
#   casos/<caso>/<caminho-do-módulo>.k   o fonte keel; o caminho vem do `module`
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
for h in $(find c23 c11 casos -name '*.h' ! -name '*.type.h' | sort); do
  [ -f "${h%.h}.type.h" ] || { printf 'ESTRUT %s — falta %s\n' "$h" "${h%.h}.type.h"; estrutura=$((estrutura+1)); }
done
for d in $(find c23 c11 casos -name '*.proto.h' | sort); do
  printf 'ESTRUT %s — o corte é em dois, .type.h e .h (backend §4.3.2)\n' "$d"; estrutura=$((estrutura+1))
done

# O caminho do .k é o nome do `module`: é `module-fora-do-caminho` (spec §4.1),
# e a suíte já o violava em 19 dos 22 fontes — todos chamados `caso.k`, enquanto
# os `#line` do gerado ao lado já citavam o caminho certo.
for caso in casos/*/; do
  for k in $(find "$caso" -name '*.k' | sort); do
    mod=$(grep -m1 '^module' "$k" | sed 's/^module *//; s/[; ].*//')
    esperado="$caso$(printf '%s' "$mod" | tr '.' '/').k"
    [ "$k" = "$esperado" ] || { printf 'ESTRUT %s — declara `module %s`, deveria ser %s\n' "$k" "$mod" "$esperado"; estrutura=$((estrutura+1)); }
  done
done

# O nome do gerado e o do fonte seguem regras DIFERENTES, e as duas são cobradas:
# o .k mora no caminho do módulo (acima), e o header leva o símbolo manglado sob
# o diretório dos componentes-pai (backend §4.1). `module app.cfg;` mora em
# app/cfg.k e gera app/app_cfg.h. E o módulo do caso é o que a invocação compila,
# então gera também o .c (backend §4.1) — sempre, mesmo que só com o include —,
# salvo o genérico, que não compila sozinho (`fonte-generico`).
for caso in casos/*/; do
  for perfil in c23 c11; do
    ger="$caso/esperado/$perfil"
    [ -d "$ger" ] || continue
    for k in $(find "$caso" -name '*.k' | sort); do
      mod=$(grep -m1 '^module' "$k" | sed 's/^module *//; s/[; ].*//')
      dir=$(printf '%s' "$mod" | sed 's/\.[^.]*$//; t; s/.*//' | tr '.' '/')
      sim=$(printf '%s' "$mod" | tr '.' '_')
      [ -n "$dir" ] && alvo="$ger/$dir/$sim.h" || alvo="$ger/$sim.h"
      [ -f "$alvo" ] || { printf 'ESTRUT %s — `module %s` deveria gerar %s\n' "$k" "$mod" "$alvo"; estrutura=$((estrutura+1)); }
      # genérico não é unidade compilada (fonte-generico): tem os headers, não o .c
      if grep -m1 '^module' "$k" | grep -Eq '[[:space:]](type|dim|tags)[[:space:]]'; then
        [ ! -f "${alvo%.h}.c" ] || { printf 'ESTRUT %s — `module %s` é genérico e não gera .c\n' "$k" "$mod"; estrutura=$((estrutura+1)); }
      else
        [ -f "${alvo%.h}.c" ] || { printf 'ESTRUT %s — `module %s` deveria gerar %s\n' "$k" "$mod" "${alvo%.h}.c"; estrutura=$((estrutura+1)); }
      fi
    done
  done
done

# I1: um .type.h inclui apenas .type.h — é o que torna o grafo de layout um DAG
for t in $(find c23 c11 casos -name '*.type.h' | sort); do
  mau=$(grep '^#include "' "$t" | grep -v '\.type\.h"')
  [ -z "$mau" ] || { printf 'ESTRUT %s — .type.h incluindo fora da camada:\n%s\n' "$t" "$mau"; estrutura=$((estrutura+1)); }
done

# I2: as seções do .h (regra 2) — todo #include de .type.h vem antes do primeiro
# #include de .h. Os protótipos ficam entre os dois blocos, e é essa ordem que
# faz todo protótipo alcançável chegar antes do primeiro corpo.
for h in $(find c23 c11 casos -name '*.h' ! -name '*.type.h' | sort); do
  mau=$(grep '^#include "' "$h" | awk '/\.type\.h"/ { if (visto) print; next } { visto = 1 }')
  [ -z "$mau" ] || { printf 'ESTRUT %s — .type.h incluído depois de um .h:\n%s\n' "$h" "$mau"; estrutura=$((estrutura+1)); }
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
    if find "$caso" -name '*.k' -exec grep -lq '^pub ' {} + 2>/dev/null && ! find "$ger" -name '*.h' ! -name '*.type.h' | grep -q .; then
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
