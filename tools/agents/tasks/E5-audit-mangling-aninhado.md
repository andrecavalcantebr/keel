# E5 — auditoria: nomes manglados de instâncias aninhadas (só leitura)

## Objetivo
O André corrigiu a regra 3 do §2.1 do backend: `buffer slice i32` sai
`keel_buffer_keel_slice_i32`, não `keel_buffer_slice_i32`. O argumento de uma
instância carrega a própria qualificação (§2.1 regra 1), e o `slice` do
módulo `keel.slice` tem o símbolo `keel_slice_<T>` (o `keel_` é do módulo).
Ache outros lugares dos documentos com o mesmo erro: o nome de um tipo
aninhado em que falta o prefixo do módulo do tipo interno.

## Arquivos
Ler: `keel-c-backend.md`, `keel-spec.md`, `keel-rationale.md`,
`cgen-tool-spec.md`, `README.md`, `design/*.md`. Fonte da verdade:
`golden/cases/*/expected/c23/` e `base/`. Criar somente
`tools/agents/reports/E5.md`. Não editar mais nada.

## Contrato
Padrões a procurar (todos com `grep -rn`, ignore `tools/agents` e `.claude`):
- `keel_(buffer|array|outcome|optional|slice)_(slice|buffer|array|outcome)_`
  — o correto tem `keel_` entre os dois (ex. `keel_buffer_keel_slice_i32`,
  `keel_outcome_keel_slice_i32`, `keel_slice_of_keel_array_i32`);
- o mesmo para módulos que não são `keel` (`coll_stack_…`);
- os nomes `*_slice_char` / `*_slice_i32` etc. em tabelas de mangling e em
  includes (`#include "keel/….h"`) e nomes de arquivo `.h`.
Para cada achado, confirme no golden ou em `base/` a forma real (cite o
arquivo) antes de dizer que é erro. Exemplos de forma aninhada que o golden
tem: `keel_outcome_keel_slice_const_char`, `keel_slice_of_keel_buffer_i32`,
`keel_slice_of_keel_array_keel_routine_slot_ag2_Ag`.
Se o texto nomeia algo que o golden não cobre, marque "sem prova", não "erro".
`keel_slice_char` e `keel_slice_const_char` são tipos diferentes e ambos
legítimos: não relate um como erro do outro.

## Aceitação
`git status --short` não mostra arquivo modificado (só o relatório novo).

## Relatório
`tools/agents/reports/E5.md`, tabela: arquivo:linha | trecho | forma
correta | prova (arquivo do golden/base, ou "sem prova") .
