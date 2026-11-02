# T31-length-fields-impl — keel.length de campo, dimensão por constexpr, keel.dim sem binder, range-index de campo

## Objetivo

Oráculo escrito por outro agente (piloto, revisado por mim): **`golden/cases/056-length-fields`** (`app/*.k`, `expected/c23/`, `expected/c11/`, `proof.c`). Leia o `.k` e o `expected/c23/` antes de tudo. Hoje o caso está com `|wip` em `test/cgen/gen/golden-cases.txt`. Faça o cgen produzi-lo byte a byte nos dois perfis.

Formas (hoje `implementation-limit` em `core.c` e `subscript.c`): `keel.length(b->v)` e `keel.length(b.v)` sobre campo `array` de struct → `sizeof(b->v)/sizeof(i32)`; `keel.length` sobre `array` de módulo cuja dimensão é `constexpr`; `keel.dim(v, 1)` sobre parâmetro sem binder → o literal; `b->v[1..4]` sobre campo `array` → `keel_array_i32_as_slice2(b->v, 5, 1, 4)` (a dimensão do campo). Levantamento: `CGEN=$PWD/tools/cgen/cgen sh tools/agents/survey/run3.sh | grep -E 'a11|a07|ce01|a10'`.

## Arquivos

Pode editar, em `tools/cgen/src/engine/emit/` (e, se a forma exigir, o parser em `tools/cgen/src/engine/parser/`): o que for preciso. Pode apagar o `|wip` da linha `056-length-fields` de `test/cgen/gen/golden-cases.txt`. Não toque: normativos, `golden/**/expected/**`, `proof.c`, outros casos. Não faça commit nem `git add`. Se um teste (unit/ ou cli/) afirmar o contrário, não o edite: guarde o patch (sem o `golden-cases.txt`) fora do repositório e diga no relatório. **Se o esperado divergir da spec ou de um golden que já passa, pare nesse ponto e diga no relatório (o orquestrador arbitra: três vezes hoje o erro foi do oráculo).**

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # o caso sai de wip; 0 fail (o gen_golden agora também compila o C escrito)
golden/run.sh                   # "106 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
```

## Relatório

`tools/agents/reports/T31-length-fields-impl.md`, em pt-BR: o que mudou, a saída dos comandos, dúvidas. Entregue UMA vez pelo SubagentHandback e termine.
