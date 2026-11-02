# T21b — `parallel` com cabeçalho, corpo e `}` numa linha

## Objetivo

Falta uma forma do caso **`golden/cases/048-parallel-more`** (a função `one_line` de `app/pm.k`; a `named_policy` já fecha): `parallel v ALL (…) { foreach (…) { … } }` tudo numa linha. Leia o `pm.k` e o `expected/c23/app/app_pm.c` (a função `app_pm_one_line`) antes de tudo. Saída esperada: o gestor como um texto só a partir dessa linha do fonte (as linhas até `keel_slice_i32 part = …;`), depois **`#line 16 "app/pm.k"`** (o corpo do worker leva `#line`, backend §5.9 r16), uma linha com o corpo (`{ keel_slice_i32 keel__c1 = part; … }`, o `foreach` de uma linha, com 12 espaços), e `keel__end0: ;`, `}`, `}`; o `#line 17` vem depois, antes do `return`. Hoje é `implementation-limit` (`ends_line` em `k_em_handles_parallel`, `parallel.c`).

## Arquivos

Pode editar: `tools/cgen/src/engine/emit/parallel.c`; se for inevitável, `emit.c` e `emit_internal.h`. Pode apagar o `|wip` da linha `048-parallel-more` de `test/cgen/gen/golden-cases.txt`. Pode criar `tools/agents/reports/T21b-parallel-oneline.md`. Não toque: normativos, `golden/**/expected/**`, `proof.c`, outros casos. Não faça commit nem `git add`. Se um teste (unit/ ou cli/) afirmar o contrário, não o edite: guarde o patch (sem o `golden-cases.txt`) fora do repositório e diga no relatório. Se o esperado divergir da spec, pare e diga.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # 048 sai de wip; 0 fail
golden/run.sh                   # "108 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
sh tools/agents/survey/run.sh   # q02 com OK nos dois perfis
```

## Relatório

`tools/agents/reports/T21b-parallel-oneline.md`, em pt-BR: o que mudou; a saída final dos comandos; dúvidas. Entregue UMA vez pelo SubagentHandback e termine.
