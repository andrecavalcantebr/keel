# T22 — adaptação `*x` de argumento por ponteiro e `foreach` sobre `range *` e sobre chamada

## Objetivo

Oráculo: **`golden/cases/049-adapt-foreach`** (escrito à mão: `app/af.k`, `expected/c23/app/`, `expected/c11/app/`, `proof.c`). Leia o `af.k` e o `expected/c23/app/app_af.c` antes de tudo.

1. **Adaptação de argumento** (linguagem §4.4, regra 5): parâmetro **por valor** com argumento **ponteiro** `x` emite `*x`. Hoje o emissor passa `x` (C inválido): `slice.length(sp)` com `slice.slice i32 *sp` sai `keel_slice_i32_length(sp)`, e deve sair `keel_slice_i32_length(*sp)`; idem `range.first(r)` e `range.limit(r)` com `range *r`. (A direção contrária, `&x`, já funciona.) O caso é um **defeito de tradução**, não só uma recusa: ache onde a adaptação é decidida (`uses.c`, `k_em_edits_use`/a escolha de `&`) e acrescente o `*`.
2. **`foreach (size_t v : r)` com `range *r`** → `{ size_t keel__f0 = keel_range_first(*r); size_t keel__l0 = keel_range_limit(*r); for (size_t v = keel__f0; v < keel__l0; v++) t += v; }` (usa a adaptação do item 1).
3. **`foreach (i32 x, size_t i : pick(xs))`**, contêiner que é **chamada** que devolve ponteiro: `{ keel_buffer_i32 *keel__c0 = app_af_pick(xs); size_t keel__n0 = keel_buffer_i32_length(keel__c0); for (…) { i32 x = keel_buffer_i32_get(keel__c0, i); t += x; } }` — o temporário recebe o resultado da chamada (backend §5.7 r1: o contêiner é avaliado uma vez). O tipo do temporário vem do retorno declarado da função (`buffer.buffer i32 *`). Chamada que devolve por valor: recuse neste cartão.

## Arquivos

Pode editar, em `tools/cgen/src/engine/emit/`: `uses.c`, `foreach.c`, `emit.c`, `emit_internal.h`. Pode apagar o `|wip` da linha `049-adapt-foreach` de `test/cgen/gen/golden-cases.txt`. Pode criar `tools/agents/reports/T22-adapt-foreach.md`.

Não toque: normativos, `golden/**/expected/**`, nenhum `proof.c`, nenhum outro caso. Não faça commit nem `git add`. Se um teste de unit afirmar o contrário, não o edite: guarde o patch fora do repositório e diga no relatório.

## Contrato

- Código existente: o comentário do alto de `foreach.c` (formas suportadas e "stays refused"), `uses.c` (como o `&` é decidido pela forma do parâmetro e do argumento).
- A adaptação `*x` só se aplica a argumento **conhecido** como ponteiro (declarado `T *x` no escopo); desconhecido, copie como escrito (como hoje).

## Passos sugeridos

1. `sh test/cgen/gen_golden.sh`: veja o `wip 049-adapt-foreach`; gere a saída e compare. **Não copie a saída para o esperado.**
2. Item 1 primeiro (muitos casos usam verbos: rode o `gen_golden` inteiro), depois 2 e 3.
3. `sh tools/agents/survey/run.sh`: e02 e e03 devem ir a `OK`.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # 049 sai de wip; 0 fail
golden/run.sh                   # "102 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
sh tools/agents/survey/run.sh   # e02, e03 com OK nos dois perfis
```

## Relatório

`tools/agents/reports/T22-adapt-foreach.md`, em pt-BR: o que mudou; a saída final exata dos quatro comandos; toda dúvida ou incoerência. Entregue o relatório UMA vez pelo SubagentHandback e termine.
