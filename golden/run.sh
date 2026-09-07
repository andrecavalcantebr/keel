#!/usr/bin/env sh
# golden/run.sh — compila o C esperado de cada caso, nos dois perfis.
# Um caso com PROBLEMA é xfail: espera-se que NÃO compile, e o arquivo diz por quê.
CC=${CC:-gcc}
# -Werror=vla cobra uma afirmação normativa: a linguagem §4.4 diz que keel não
# emite VLA nem alloca em lugar nenhum. Se um `constexpr` deixar de ser
# expressão constante, o vetor vira VLA e a suíte inteira acusa.
FLAGS="-pedantic-errors -Wall -Wextra -Wno-unused-parameter -Wvla -Werror=vla"
# `parallel` é a única construção cujo gerado depende de algo fora do C: a spec
# promete que ele compila e roda igual sem OpenMP, então cada caso com `#pragma
# omp` roda nas duas configurações.
ok=0; falha=0; xfail=0; xpass=0
for caso in casos/*/; do
  nome=$(basename "$caso")
  for perfil in c23 c11; do
    [ "$perfil" = c23 ] && std=c2x src="$caso/esperado.c"     || std=c11
    [ "$perfil" = c11 ] && { src="$caso/esperado.c11.c"; [ -f "$src" ] || src="$caso/esperado.c"; }
    [ -f "$src" ] || continue
    exe=/dev/null; modo=compila
    # um caso pode ter unidades a mais — `instance` mora num .k do usuário, e o
    # corpo extern da instância vive no .c dele (linguagem §4.9)
    aux=""
    if [ "$perfil" = c11 ]; then
      [ -f "$caso/aux.c11.c" ] && aux="$caso/aux.c11.c" || { [ -f "$caso/aux.c" ] && aux="$caso/aux.c"; }
    else
      [ -f "$caso/aux.c" ] && aux="$caso/aux.c"
    fi
    grep -q '^int main' "$src" && { exe=$(mktemp); modo=executa; }
    [ -n "$aux" ] && modo="$modo, 2 TUs"
    omps=""
    grep -q '#pragma omp' "$src" && omps=" -fopenmp"
    bom=1
    for omp in "" $omps; do
      [ -z "$omp" ] && extra=-Wno-unknown-pragmas || extra=""
      $CC -std=$std $FLAGS $extra $omp -I"$perfil" -I"$caso/$perfil" -I"$caso" $(case $modo in compila*) echo -c;; esac) "$src" $aux -o "$exe" 2>/dev/null \
        && { case $modo in compila*) : ;; *) "$exe" >/dev/null ;; esac; } || bom=0
    done
    [ -n "$omps" ] && modo="$modo, ±omp"
    if [ $bom -eq 1 ]; then
      if [ -f "$caso/PROBLEMA" ]; then
        printf 'XPASS  %-28s %s  — compilou, mas há PROBLEMA registrado\n' "$nome" "$perfil"; xpass=$((xpass+1))
      else
        printf 'ok     %-28s %s  (%s)\n' "$nome" "$perfil" "$modo"; ok=$((ok+1))
      fi
    else
      if [ -f "$caso/PROBLEMA" ]; then
        printf 'xfail  %-28s %s  — %s\n' "$nome" "$perfil" "$(head -1 "$caso/PROBLEMA")"; xfail=$((xfail+1))
      else
        printf 'FALHA  %-28s %s\n' "$nome" "$perfil"; falha=$((falha+1))
        $CC -std=$std $FLAGS -Wno-unknown-pragmas $omps -I"$perfil" -I"$caso/$perfil" -I"$caso" -c "$src" -o /dev/null 2>&1 | sed 's/^/         /' | head -6
      fi
    fi
  done
done
printf '\n%d ok, %d falha, %d xfail, %d xpass\n' $ok $falha $xfail $xpass
[ $falha -eq 0 ] && [ $xpass -eq 0 ]
