# T21 — `parallel` com política por `constexpr` e em uma linha; `else` com o default na linha seguinte

## Objetivo

Oráculos: **`golden/cases/048-parallel-more`** (`app/pm.k`) e **`golden/cases/050-else-default`** (`app/ed.k`), escritos à mão, com `expected/c23/`, `expected/c11/` e `proof.c`. Leia os `.k` e os `expected/c23/` antes de tudo. Formas hoje recusadas (`implementation-limit`):

1. **Política escrita por `constexpr`** (`parallel v NEED (…)` com `pub constexpr size_t NEED = 2;`): `.target = 2`, e o `win` compara com o literal (`… + 1 >= 2`), como a política decimal (backend §5.9 r10). Use `k_em_constexpr_value` (T14) como nos workers; inicializador que não seja decimal: recuse.
2. **`parallel` com cabeçalho, corpo e `}` numa linha** (`parallel v ALL (…) { foreach (…) { … } }`): o gestor, o corpo e o fechamento saem como **um texto só** a partir dessa linha do fonte: o gestor (as linhas até `keel_slice_i32 part = …;`), uma linha com o corpo (`{ keel_slice_i32 keel__c1 = part; … }`, a do `foreach` de uma linha, com a indentação de 12 espaços), e `keel__end0: ;`, `}`, `}`. Sem `#line` dentro; o `#line` da linha seguinte do fonte vem depois (o esperado tem `#line 17`).
3. **`else` com o default na linha seguinte** (`… else\n        slice.from(i32, base, 0);`): o `else` vira `; if (F(x))` no fim da linha e a **linha seguinte** começa pela chamada de `win1(&x, ` e termina com o valor e `);`: `        keel_outcome_keel_slice_i32_win1(&s, keel_slice_i32_from(base, 0));`. Hoje o T15 recusa o default cujo `;` está em outra linha (`same_line` em `k_em_handles_else`).

## Arquivos

Pode editar: `tools/cgen/src/engine/emit/parallel.c` e `else.c`; se for inevitável, `emit.c` e `emit_internal.h`. Pode apagar o `|wip` das linhas `048-parallel-more` e `050-else-default` de `test/cgen/gen/golden-cases.txt`. Pode criar `tools/agents/reports/T21-parallel-else.md`.

Não toque: normativos, `golden/**/expected/**`, nenhum `proof.c`, nenhum outro caso. Não faça commit nem `git add`.

## Contrato

- Código existente: `parallel.c` (`Par`, a análise do cabeçalho, o texto do gestor, `ends_line`) e `else.c` (T15: `spanned_end`, o corte quando a cauda não está na linha).
- `--parallel-lowering=serie` e o oráculo `test/cgen/gen/serie` continuam valendo.

## Passos sugeridos

1. `sh test/cgen/gen_golden.sh`: veja os `wip`; gere a saída e compare. **Não copie a saída para o esperado.**
2. Uma forma por vez; `make -C tools/cgen -j8` e `sh test/cgen/gen_golden.sh` inteiro a cada passo (004, 012, 031, 041, 011, 021, 042 usam essas construções).
3. `sh tools/agents/survey/run.sh`: q01, q02 e l01 devem ir a `OK`.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # 048 e 050 saem de wip; 0 fail
golden/run.sh                   # "100 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
sh tools/agents/survey/run.sh   # q01, q02, l01 com OK nos dois perfis
```

## Relatório

`tools/agents/reports/T21-parallel-else.md`, em pt-BR: o que mudou; a saída final exata dos quatro comandos; toda dúvida ou incoerência. Entregue o relatório UMA vez pelo SubagentHandback e termine.
