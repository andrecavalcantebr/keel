# T26 — autor de oráculo: `return` com índice sob `defer`, e `break` de `foreach` sob `defer`

Você é o **autor do oráculo** (skill `haiku-task`, seção "Oracle-author cards"). Escreva à mão o caso **`golden/cases/055-defer-return-index`** (módulo `app.dr2`, `app/dr2.k`, `proof.c`, `expected/c23/`, `expected/c11/`). **Não rode `tools/cgen/cgen` sobre o caso nem leia `tools/cgen/src/`**.

## Formas (hoje recusadas com `implementation-limit`)

1. **`return` com expressão que tem índice, numa função com `defer`**: `pub int tail(void) { array i32 v[3] = {7, 8, 9}; defer v[1] = 0; return v[0]; }` — o valor é lido **antes** do cleanup (backend §5.5 r5): `{ int keel__rv0 = v[0]; <cleanup>; return keel__rv0; }`. Variantes a cobrir: `return v[keel_index(i, 3)]` (com índice variável, que passa por `keel_index`, §5.3 r4), e dois `defer` (ordem inversa).
2. **`break` dentro de `foreach` com `defer` no corpo do laço**: `foreach (i32 x, size_t i : s) { defer count++; if (x == 0) break; }` (a saída por `break` leva o cleanup inline, §5.5 r1). Modelos: `golden/cases/005-defer-loop`, `017-defer-return`, `032-defer-jump` (leia os três: o formato do cleanup inline, do `#line` antes e depois, e do `keel__rv`).

## Arquivos e aceitação

Como no cartão T25: `golden/cases/055-defer-return-index/…`, a linha `055-defer-return-index|app/dr2.k|-|wip` em `test/cgen/gen/golden-cases.txt`, `golden/line-check.allow` só para código injetado, `proof.c` que roda e confere (a ordem do cleanup e o valor devolvido são o que importa), `golden/run.sh` sem `FAIL`. Não toque em mais nada, nem faça commit. Relatório em `tools/agents/reports/T26-oracle-defer-return.md` (pt-BR), com as escolhas feitas onde a spec for ambígua. Entregue UMA vez pelo SubagentHandback e termine.
