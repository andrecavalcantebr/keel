# T29-style-impl — estilo: constexpr que cita constexpr, foreach com { na linha seguinte, walk com cursor qualificado

## Objetivo

Oráculo escrito por outro agente (piloto, revisado por mim): **`golden/cases/054-style-forms`** (`app/*.k`, `expected/c23/`, `expected/c11/`, `proof.c`). Leia o `.k` e o `expected/c23/` antes de tudo. Hoje o caso está com `|wip` em `test/cgen/gen/golden-cases.txt`. Faça o cgen produzi-lo byte a byte nos dois perfis.

Formas (hoje o cgen aceita e gera C que não compila, ou recusa): (1) `pub constexpr size_t B = A * A;` — os nomes dentro do `INIT` de `constexpr` de módulo são renomeados como qualquer uso (`app_sf_A`), nos dois perfis; (2) `foreach` com a `{` do corpo em linha própria (backend §5.7 r5, texto novo): a linha do `foreach` sai sem chave, a linha `{` vira `{ i32 x = …;`, a `}` vira `} }`; (3) `walk (i32 *p, app.sf.cursor c : l)` com o cursor escrito com o nome qualificado do próprio módulo (`app_sf_cursor c`). Sobre o item 3, o parser pode precisar deixar a ilha de tipo no nome qualificado de vários segmentos. Levantamento: `CGEN=$PWD/tools/cgen/cgen sh tools/agents/survey/run3.sh | grep -E 'ce04|fe05|wk01'` deve passar a `OK`.

## Arquivos

Pode editar, em `tools/cgen/src/engine/emit/` (e, se a forma exigir, o parser em `tools/cgen/src/engine/parser/`): o que for preciso. Pode apagar o `|wip` da linha `054-style-forms` de `test/cgen/gen/golden-cases.txt`. Não toque: normativos, `golden/**/expected/**`, `proof.c`, outros casos. Não faça commit nem `git add`. Se um teste (unit/ ou cli/) afirmar o contrário, não o edite: guarde o patch (sem o `golden-cases.txt`) fora do repositório e diga no relatório. **Se o esperado divergir da spec ou de um golden que já passa, pare nesse ponto e diga no relatório (o orquestrador arbitra: três vezes hoje o erro foi do oráculo).**

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # o caso sai de wip; 0 fail (o gen_golden agora também compila o C escrito)
golden/run.sh                   # "106 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
```

## Relatório

`tools/agents/reports/T29-style-impl.md`, em pt-BR: o que mudou, a saída dos comandos, dúvidas. Entregue UMA vez pelo SubagentHandback e termine.
