# E2 — auditoria: exemplos desatualizados nos normativos (só leitura)

## Objetivo
Achar, nos exemplos de C gerado dos documentos, nomes e formas que divergem
do que o cgen e o golden emitem hoje. Não editar nada além do relatório.

## Arquivos
Ler: `keel-c-backend.md`, `keel-spec.md`, `cgen-tool-spec.md`,
`keel-rationale.md`, `design/*.md`, `README.md`. Fonte da verdade: os
`golden/cases/*/expected/c23/` (e `c11`) e `base/`. Criar somente
`tools/agents/reports/E2.md`. Não tocar a §5.10 do backend (outro agente a
edita agora).

## Contrato
Convenções vigentes, conferidas no golden 010:
- instância de genérico com modificador: nome do módulo instanciado,
  `keel_routine_ag2_Ag`, `keel_routine_slot_ag2_Ag`, verbo
  `keel_routine_ag2_Ag_par` (módulo, argumento, verbo; não `keel_routine_par_Ag`);
- função sobre protocolo: `keel_slice_of_<concreto>(…)`, ex.
  `keel_slice_of_keel_array_keel_routine_slot_ag2_Ag(steps, 2)`; não
  `keel_slice_…_from`;
- cada inicializador designado `{ .f = …, .ctx = … }`.
Para cada bloco de C gerado nos documentos, compare com o que o golden
equivalente tem (procure o `.k` ou o padrão correspondente por `grep -rn` em
`golden/cases`). Registre toda divergência de: nomes, ordem de argumentos,
`#line`, includes, qualificadores, formas de inicializador.

## Passos sugeridos
1. `grep -n '^```c' ` em cada documento para listar os blocos.
2. Por seção, comparar com o golden. Anotar só o que tem evidência.
3. Não "corrigir por gosto": se o exemplo é ilustrativo de propósito
   (abreviado, `/* … */`), diga isso e não conte como divergência.

## Aceitação
`git status --short` não mostra nenhum arquivo modificado (só o relatório
novo).

## Relatório
`tools/agents/reports/E2.md`, tabela: arquivo:linha | trecho do documento |
o que o golden tem (caso:arquivo:linha) | gravidade (nome errado / forma
diferente / só ilustrativo). Ao fim, as dúvidas.
