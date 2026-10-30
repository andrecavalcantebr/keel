# M6b — `foreach` sobre `range` numa variável

Leia antes: `.claude/skills/haiku-task/SKILL.md` (a metade do agente), `AGENTS.md`,
`tools/agents/tasks/M6-contexto.md`, o backend `keel-c-backend.md` §5.7 (`foreach`) e
§5.3 regra 9, e o `foreach.c` atual inteiro (ele já baixa o intervalo literal `A..B` e o
contêiner com dois binders).

## Objetivo

Baixar `foreach` sobre uma variável do tipo `range`, com um binder e com dois, para que o
caso `015-foreach-range` saia pelo cgen igual aos `expected/`, nos dois perfis.

## Arquivos

Edita só `tools/cgen/src/engine/emit/foreach.c`. Cria `tools/agents/reports/M6b-foreach-range.md`.

## Contrato

As ilhas do caso (do parser):

```
ilha foreach  v, i : xs (buffer i32)   app/ap.k:14:5     ← já baixado hoje (contêiner)
ilha foreach  v, i : xs (buffer i32)   app/ap.k:15:5
ilha foreach  v : r (range)            app/ap.k:22:5     ← novo: range numa variável, um binder
ilha foreach  v, i : r (range)         app/ap.k:23:5     ← novo: dois binders
```

O que distingue a forma nova é o tipo `(range)` no detalhe. A saída esperada está em
`golden/cases/015-foreach-range/expected/{c23,c11}/app/app_ap.c`, linhas 22 e 23 do `.k`;
ela lê os limites do `range` pelos campos do tipo (`keel_range { size_t first, limit; }`,
`base/keel/range.k`), e o backend §5.7 diz como. Mantenha as formas que já funcionam
(os outros casos da tabela não podem mudar: `verify.sh` e `gen_golden.sh` conferem).

## Passos sugeridos

1. Leia o `.k` e o `.c` esperado do 015; rode o cgen e veja a diferença (M6-contexto).
2. Estenda `k_em_handles_foreach` para aceitar `(range)` e o baixe em `k_em_edits_foreach`.
3. Compile a cada passo, sem warning novo.

## Aceitação

`sh tools/agents/verify.sh` → `ALL OK`; `sh test/cgen/gen_golden.sh` não lista mais
`015-foreach-range` como `wip`, e continua sem `fail`.

## Relatório

`tools/agents/reports/M6b-foreach-range.md`: o que mudou, a saída final dos comandos, e toda dúvida.
