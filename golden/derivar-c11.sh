#!/usr/bin/env sh
# Deriva esperado/c11/ de esperado/c23/ aplicando as DIFERENÇAS DO §9.1, e
# nenhuma outra. A lista é fechada e está aqui para ser auditada:
#
#   static_assert          → _Static_assert
#   alignof / alignas      → _Alignof / _Alignas
#   [[nodiscard]]          → omitido
#   constexpr <T> <n> = v; → #define <n> ((<T>)v)  mais o objeto de conferência
#
# `bool` não aparece: quem resolve é o prelúdio do perfil, que inclui stdbool.
#   uso: ./derivar-c11.sh casos/00X-nome
set -eu
caso=$1
src="$caso/esperado/c23"; dst="$caso/esperado/c11"
rm -rf "$dst"; mkdir -p "$dst"
(cd "$src" && find . -type f) | while read -r f; do
  mkdir -p "$dst/$(dirname "$f")"
  sed -e 's|perfil C23|perfil C11|' \
      -e 's|\[\[nodiscard\]\] ||g' \
      -e 's|\balignof(|_Alignof(|g' \
      -e 's|\balignas(|_Alignas(|g' \
      -e 's|\bstatic_assert(|_Static_assert(|g' \
      -e 's|^constexpr \([a-z0-9_]*\) \([A-Za-z_][A-Za-z0-9_]*\) = \(.*\);$|#define \2 ((\1)\3)\nstatic const \1 \2__chk = \3;|' \
      "$src/$f" > "$dst/$f"
done
