#!/usr/bin/env sh
# golden/run.sh — compila o C esperado de cada caso, nos dois perfis.
# Um caso com PROBLEMA é xfail: espera-se que NÃO compile, e o arquivo diz por quê.
CC=${CC:-gcc}
FLAGS="-pedantic-errors -Wall -Wextra -Wno-unused-parameter"
ok=0; falha=0; xfail=0; xpass=0
for caso in casos/*/; do
  nome=$(basename "$caso")
  for perfil in c23 c11; do
    [ "$perfil" = c23 ] && std=c2x src="$caso/esperado.c"     || std=c11
    [ "$perfil" = c11 ] && { src="$caso/esperado.c11.c"; [ -f "$src" ] || src="$caso/esperado.c"; }
    [ -f "$src" ] || continue
    if $CC -std=$std $FLAGS -I"$perfil" -c "$src" -o /dev/null 2>/dev/null; then
      if [ -f "$caso/PROBLEMA" ]; then
        printf 'XPASS  %-28s %s  — compilou, mas há PROBLEMA registrado\n' "$nome" "$perfil"; xpass=$((xpass+1))
      else
        printf 'ok     %-28s %s\n' "$nome" "$perfil"; ok=$((ok+1))
      fi
    else
      if [ -f "$caso/PROBLEMA" ]; then
        printf 'xfail  %-28s %s  — %s\n' "$nome" "$perfil" "$(head -1 "$caso/PROBLEMA")"; xfail=$((xfail+1))
      else
        printf 'FALHA  %-28s %s\n' "$nome" "$perfil"; falha=$((falha+1))
        $CC -std=$std $FLAGS -I"$perfil" -c "$src" -o /dev/null 2>&1 | sed 's/^/         /' | head -6
      fi
    fi
  done
done
printf '\n%d ok, %d falha, %d xfail, %d xpass\n' $ok $falha $xfail $xpass
[ $falha -eq 0 ] && [ $xpass -eq 0 ]
