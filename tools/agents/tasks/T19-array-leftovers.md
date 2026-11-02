# T19 — arrays: `v[a..]`, `v[..]`, campo com vários declaradores, `keel.capacity`/`keel.length` sem binder, elemento de tipo do módulo

## Objetivo

Oráculo: **`golden/cases/046-array-leftovers`** (escrito à mão: `app/al.k`, `expected/c23/app/`, `expected/c11/app/`, `proof.c`). Leia o `al.k` e o `expected/c23/app/app_al.c` antes de tudo. Formas hoje recusadas (`implementation-limit`):

1. **Range-index aberto sobre `array`** (backend §5.3 r7): `v[2..]` → `keel_array_i32_as_slice2(v, 6, 2, keel_array_i32_length(v, 6))`; `v[..]` → `keel_array_i32_as_slice2(v, 6, 0, keel_array_i32_length(v, 6))`. É o `as_slice` de três parâmetros com a dimensão entregue depois do vetor (como `v[1..4]`, T17), e o limite aberto é o `length` do módulo `keel.array`, que também recebe a dimensão. O `#include "keel/keel_array_i32.h"` já sai.
2. **Campo de struct com vários declaradores**: `typedef struct { array i32 a[2], b[3]; } Pair;` → `typedef struct app_al_Pair { i32 a[2], b[3]; } app_al_Pair;`, e `p->a[0] + p->b[1]` → índices literais com dimensão literal ficam como estão (cada campo tem a sua dimensão).
3. **`keel.capacity(v)` sobre parâmetro com binder** → `keel__n` (como o `length`).
4. **`keel.length(v)` sobre parâmetro sem binder** `array i32 v[2, 3]` → o produto literal das dimensões, `2 * 3` (backend §5.3 r7); o protótipo é `size_t app_al_plain(i32 v[static 2][3]);`.
5. **`keel.length(bs)` sobre `array` cujo elemento é tipo do módulo** (`array Box bs[3]`) → `sizeof(bs)/sizeof(app_al_Box)`: o tipo do elemento é renomeado dentro do texto gerado.

## Arquivos

Pode editar, em `tools/cgen/src/engine/emit/`: `subscript.c`, `index.c`, `array.c`, `core.c`, `uses.c`, `emit.c`, `emit_internal.h`. Pode editar `test/cgen/unit/emit/emit_main.c` só se uma asserção de recusa deixar de valer (troque o exemplo por outro ainda recusado). Pode apagar o `|wip` da linha `046-array-leftovers` de `test/cgen/gen/golden-cases.txt`. Pode criar `tools/agents/reports/T19-array-leftovers.md`.

Não toque: normativos, `golden/**/expected/**`, nenhum `proof.c`, nenhum outro caso. Não faça commit nem `git add`.

## Contrato

- Código existente (leia nos arquivos): o comentário do alto de `core.c` (a lista "Refused"), `subscript.c` (o range-index de `array` do T17), `array.c` (`k_em_edits_struct_arrays`, T12), `index.c`.
- Sem dimensão conhecida: recuse.

## Passos sugeridos

1. `sh test/cgen/gen_golden.sh`: veja `wip 046-array-leftovers`; gere a saída numa pasta temporária e compare com o esperado. **Não copie a saída para o esperado.**
2. Uma forma por vez; `make -C tools/cgen -j8` e `sh test/cgen/gen_golden.sh` inteiro a cada passo.
3. `sh tools/agents/survey/run.sh`: r01–r06 devem ir a `OK`.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # 046 sai de wip; 0 fail
golden/run.sh                   # "100 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
sh tools/agents/survey/run.sh   # r01-r06 com OK nos dois perfis
```

## Relatório

`tools/agents/reports/T19-array-leftovers.md`, em pt-BR: o que mudou; a saída final exata dos quatro comandos; toda dúvida ou incoerência. Entregue o relatório UMA vez pelo SubagentHandback e termine.
