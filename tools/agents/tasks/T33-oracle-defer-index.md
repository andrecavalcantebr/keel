# T33 — autor de oráculo: índice dentro do corpo de um `defer`

Você é o **autor do oráculo** (skill `haiku-task`, seção "Oracle-author cards"). Escreva à mão o caso **`golden/cases/058-defer-index`** (módulo `app.di`, `app/di.k`, `proof.c`, `expected/c23/`, `expected/c11/`). **Não rode `tools/cgen/cgen` sobre o caso nem leia `tools/cgen/src/`**; não rode `make` nem `verify.sh`.

## A forma (hoje o cleanup copia o corpo sem a verificação de índice)

O backend §5.3 regra 4 manda **todo índice sobre `array`** passar por `keel_index(i, d)` contra a dimensão declarada, e o corpo de um `defer` copiado a uma saída é keel como outro qualquer. Escreva funções (todas com `array i32 v[3]` local e o `defer` escrevendo em `v`):

1. `defer v[i] = 0;` com índice **variável** (parâmetro `size_t i`), saída por fim natural de função `void` e por `return` com valor: o cleanup sai `v[keel_index(i, 3)] = 0;`.
2. `defer [now size_t k] v[k] = 0;`: a captura é copiada no registro (`struct { size_t k; } keel__c0 = { k };`) e o cleanup é `v[keel_index(keel__c0.k, 3)] = 0;` (backend §5.5 r6 e §5.3 r4).
3. Índice **literal** (`defer v[1] = 0;`): continua `v[1] = 0;` (§5.3 r6: decimal contra dimensão decimal).
4. Corpo em chaves com dois índices variáveis (`defer { v[i] = 1; v[j] = 2; }`).

Modelos: `golden/cases/014-defer-later` (o `[now]`), `017-defer-return`, `037-defer-forms` (o corpo em chaves e em várias linhas), `055-defer-return-index` (o `return` com índice e o formato do cleanup inline com `#line`). Leia os quatro.

## Arquivos e aceitação

`golden/cases/058-defer-index/…`; a linha `058-defer-index|app/di.k|-|wip` em `test/cgen/gen/golden-cases.txt`; `golden/line-check.allow` só para código injetado; `proof.c` que roda e confere os valores (inclua uma execução com índice fora do intervalo **só se** o caso tiver um arquivo `DEBUG` como o `026-debug-checks`; senão, só índices válidos); `golden/run.sh` sem `FAIL`. Não toque em mais nada, nem faça commit. Não use nomes locais iguais a símbolos do módulo (spec §2.5). Relatório em `tools/agents/reports/T33-oracle-defer-index.md` (pt-BR), com as escolhas feitas onde a spec for ambígua. Entregue UMA vez pelo SubagentHandback e termine.
