# T27 — autor de oráculo: `keel.length`/`keel.dim` sobre campo e sobre dimensão por `constexpr`; range-index de campo

Você é o **autor do oráculo** (skill `haiku-task`, seção "Oracle-author cards"). Escreva à mão o caso **`golden/cases/056-length-fields`** (módulo `app.lf`, `app/lf.k`, `proof.c`, `expected/c23/`, `expected/c11/`). **Não rode `tools/cgen/cgen` sobre o caso nem leia `tools/cgen/src/`**.

## Formas (hoje recusadas com `implementation-limit`)

1. **`keel.length` sobre campo `array` de struct**, por ponteiro e por valor: `keel.length(b->v)` e `keel.length(b.v)` com `typedef struct { array i32 v[5]; } Box;` → `sizeof(b->v)/sizeof(i32)` (backend §5.3 r7: o elemento vem da declaração do campo).
2. **`keel.length` sobre `array` de módulo cuja dimensão é `constexpr`**: `pub constexpr size_t K = 3; priv array i32 v[K];` e `keel.length(v)` → `sizeof(app_lf_v)/sizeof(i32)`.
3. **`keel.dim(v, 0)` sobre parâmetro sem binder**: `pub size_t d(array i32 v[2, 3]) { return keel.dim(v, 1); }` → o literal da dimensão pedida (`3`); o protótipo é `size_t app_lf_d(i32 v[static 2][3]);` (§5.1 r3).
4. **Range-index de campo `array` de struct**: `slice.slice i32 s = b->v[1..4];` → `keel_array_i32_as_slice2(b->v, 5, 1, 4)` (a dimensão entregue é a do campo; §5.3 r7 e §5.18 r2). Importe `keel.array` e `keel.slice` como em `golden/cases/044-array-range` e `046-array-leftovers` (leia os dois; as instâncias `keel/keel_array_i32.h` e `keel_slice_i32.h` vêm do golden da base).

## Arquivos e aceitação

Como no cartão T25: `golden/cases/056-length-fields/…`, a linha `056-length-fields|app/lf.k|-|wip` em `test/cgen/gen/golden-cases.txt`, `golden/line-check.allow` só para código injetado, `proof.c` que roda e confere cada valor, `golden/run.sh` sem `FAIL`. Não toque em mais nada, nem faça commit. Relatório em `tools/agents/reports/T27-oracle-length-fields.md` (pt-BR), com as escolhas feitas onde a spec for ambígua. Entregue UMA vez pelo SubagentHandback e termine.
