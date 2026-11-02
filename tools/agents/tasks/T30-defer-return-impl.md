# T30-defer-return-impl — return com índice sob defer, e break de foreach sob defer

## Objetivo

Oráculo escrito por outro agente (piloto, revisado por mim): **`golden/cases/055-defer-return-index`** (`app/*.k`, `expected/c23/`, `expected/c11/`, `proof.c`). Leia o `.k` e o `expected/c23/` antes de tudo. Hoje o caso está com `|wip` em `test/cgen/gen/golden-cases.txt`. Faça o cgen produzi-lo byte a byte nos dois perfis.

Formas (hoje `implementation-limit` em `defer.c`, `plan_of`): `return v[0]` / `return v[i]` numa função com `defer` (o valor vai para `keel__rv0` antes do cleanup, o índice variável passa por `keel_index`), dois `defer` com um `return` com índice, e o cleanup inline antes do `break` e no fim natural do corpo de um `foreach` (backend §5.5 r1, r4 e r5; modelos: golden 005, 017, 032, 037). Levantamento: `CGEN=$PWD/tools/cgen/cgen sh tools/agents/survey/run3.sh | grep -E 'df06|w7'`.

## Arquivos

Pode editar, em `tools/cgen/src/engine/emit/` (e, se a forma exigir, o parser em `tools/cgen/src/engine/parser/`): o que for preciso. Pode apagar o `|wip` da linha `055-defer-return-index` de `test/cgen/gen/golden-cases.txt`. Não toque: normativos, `golden/**/expected/**`, `proof.c`, outros casos. Não faça commit nem `git add`. Se um teste (unit/ ou cli/) afirmar o contrário, não o edite: guarde o patch (sem o `golden-cases.txt`) fora do repositório e diga no relatório. **Se o esperado divergir da spec ou de um golden que já passa, pare nesse ponto e diga no relatório (o orquestrador arbitra: três vezes hoje o erro foi do oráculo).**

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # o caso sai de wip; 0 fail (o gen_golden agora também compila o C escrito)
golden/run.sh                   # "106 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
```

## Relatório

`tools/agents/reports/T30-defer-return-impl.md`, em pt-BR: o que mudou, a saída dos comandos, dúvidas. Entregue UMA vez pelo SubagentHandback e termine.
