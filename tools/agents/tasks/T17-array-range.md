# T17 — WP2b: `v[a..b]` sobre `array`, e declaradores múltiplos de `array` de módulo

## Objetivo

Duas formas de `array` ainda recusadas (`implementation-limit`). O oráculo é o caso
**`golden/cases/044-array-range`** (escrito à mão: `app/rg.k`, `expected/c23/app/`, `expected/c11/app/`,
`proof.c`); leia o `rg.k` e o `expected/c23/app/app_rg.c` antes de tudo:

1. **Range-index sobre `array`** (backend §5.3 r7 e §5.18): `v[1..4]` → `keel_array_i32_as_slice2(v, 6, 1, 4)`
   — o `as_slice` de três parâmetros do módulo `keel.array`, com a dimensão do `array` entregue logo depois
   do vetor (a mesma regra da tabela do §5.18) e o sufixo de aridade `2`; `v[..3]` passa `0` como início
   (`…as_slice2(v, 6, 0, 3)`). A forma aberta `v[a..]` e `v[..]` usam o `length` do protocolo e **ficam
   recusadas** neste cartão.
2. **Declaradores múltiplos de `array` de módulo**: `priv array i32 p[3] = {1, 2, 3}, q[2] = {4, 5};` →
   `static i32 app_rg_p[3] = {1, 2, 3}, app_rg_q[2] = {4, 5};` (hoje só o `array` local aceita vários; T12).

Para o 1, o cabeçalho `keel_array_i32.h` (com `as_slice2`) vem do golden da base (`golden/c23/keel/`); o
emissor só precisa gerar a chamada e o `#include "keel/keel_array_i32.h"`.

## Arquivos

Pode editar, em `tools/cgen/src/engine/emit/`: `subscript.c`, `index.c`, `array.c`, `core.c`, `uses.c`,
`emit.c`, `emit_internal.h`. Pode **editar** `test/cgen/unit/emit/emit_main.c`: a asserção
`pub array i32 a[2], b[3];` deve ser `K_EMIT_UNSUPPORTED` deixa de valer; troque o exemplo por outra
construção que continue recusada (confirme rodando o teste). Pode apagar o `|wip` da linha
`044-array-range` de `test/cgen/gen/golden-cases.txt`. Pode criar `tools/agents/reports/T17-array-range.md`.

Não toque: normativos, `golden/**/expected/**`, nenhum `proof.c`, nenhum outro caso. Não faça commit.

## Contrato

- O `range-index` do `array` hoje é recusado em `subscript.c` ("a bracket group"/`..`); veja como o
  `range-index` de `buffer`/`slice` é emitido (golden 025: `x[a..b]` → `as_slice(x, a, b)` do módulo do
  tipo do objeto) e como a chamada de verbo sobre `array` entrega a dimensão (`keel_array_i32_get(v, 6, i)` em
  `golden/cases/025-of-range/expected/c23/app/app_ofr.c`).
- A dimensão entregue vem da declaração do `array` (literal, aqui `6`); parâmetro com binder entrega
  `keel__n`; sem dimensão conhecida, recuse.
- A verificação de intervalo em debug é do verbo (backend §5.17 r7a), não do emissor.

## Passos sugeridos

1. `sh test/cgen/gen_golden.sh`: veja o `wip 044-array-range`; gere a saída numa pasta temporária e compare com
   o esperado. **Não copie a saída para o esperado.**
2. Uma forma por vez; `make -C tools/cgen -j8` e `sh test/cgen/gen_golden.sh` inteiro a cada passo.
3. `sh tools/agents/survey/run.sh`: a05 deve ir a `OK`.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # 044 sai de wip; 0 fail
golden/run.sh                   # "84 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
sh tools/agents/survey/run.sh   # a05 com OK nos dois perfis
```

## Relatório

`tools/agents/reports/T17-array-range.md`, em pt-BR: o que mudou; a saída final exata dos quatro comandos; toda
dúvida ou incoerência. Entregue o relatório UMA vez pelo SubagentHandback e termine.
