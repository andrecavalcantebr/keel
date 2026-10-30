# E3 — `keel_slice_char` → `keel_slice_const_char` nos documentos (autorizado pelo André)

## Objetivo
O golden emite `keel_slice_const_char` (e `_const_char_from`, `_length`,
`keel_buffer_keel_slice_const_char` etc.) e nunca `keel_slice_char`. Os
documentos ainda dizem `keel_slice_char`. André mandou aplicar em todos os
lugares.

## Arquivos
Editar: `keel-c-backend.md`, `design/lexer-design.md`,
`design/parser-design.md`, `design/codegen-design.md`,
`design/diag-design.md`, `design/cgen-tool.md`. Criar
`tools/agents/reports/E3.md`. Não editar `tools/agents/tasks/*`,
`tools/agents/reports/*` (exceto o seu), `.claude/`, `golden/`, código.

## Contrato
1. Lista de ocorrências: `grep -rn "keel_slice_char" --include=*.md .`
   (cerca de 55 linhas nos arquivos acima).
2. Substitua por `keel_slice_const_char` toda ocorrência que se refira ao
   slice de texto (o que o golden chama de `keel_slice_const_char`). Isso
   inclui derivados: `keel_buffer_keel_slice_char` →
   `keel_buffer_keel_slice_const_char`, nomes de header
   (`keel_buffer_keel_slice_char.type.h`), `keel_slice_char_from`, etc.
   Mantenha o resto da linha intacto, inclusive o alinhamento de colunas em
   blocos de código: se a troca alargar um nome que estava alinhado, reponha
   o alinhamento dos vizinhos no mesmo bloco.
3. Se, no contexto, o slice for claramente de `char` mutável (sem `const`),
   NÃO troque: liste no relatório com arquivo:linha e a razão. Na dúvida,
   não troque e liste.
4. Antes de editar, confira no golden: `grep -rn "keel_slice_const_char"
   golden/cases/*/expected/c23 | head` para ver as formas reais; se achar
   alguma derivada que o golden escreva de outro jeito, relate.

## Aceitação
- `grep -rn "keel_slice_char" --include=*.md . | grep -v "tools/agents\|\.claude"`
  só imprime as linhas que você listou como "não trocadas" no relatório
  (idealmente nenhuma).
- `git diff --stat` lista só os seis arquivos acima.
- `git diff -U0 | grep '^[-+]' | grep -v '^[-+][-+]'` : toda linha `-` tem
  uma `+` correspondente que difere só pela troca (e realinhamento).

## Relatório
`tools/agents/reports/E3.md`: contagem de trocas por arquivo, a lista de
não trocadas, a saída dos comandos de aceitação.
