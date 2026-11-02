# T35-defer-index-impl — índice dentro do corpo de um defer passa por keel_index

## Objetivo

Oráculo escrito por outro agente (piloto, revisado por mim): **`golden/cases/058-defer-index`** (`app/*.k`, `expected/c23/`, `expected/c11/`, `proof.c`). Leia o `.k` e o `expected/c23/` antes de tudo. Hoje o caso está com `|wip` em `test/cgen/gen/golden-cases.txt`. Faça o cgen produzi-lo byte a byte nos dois perfis.

Hoje o cleanup copia o corpo do `defer` sem as arestas de índice: `defer v[i] = 0;` sai `v[i] = 0;`, e a spec (§5.3 regra 4) manda `v[keel_index(i, 3)] = 0;`. O T30 já tem a mecânica na saída por `return` (`put_expr`/`put_span` em `defer.c`, que aplicam as arestas de `k_em_edits_index`); o corpo copiado por `put_body` precisa do mesmo. Quatro formas no caso: índice variável (fim natural e `return`), `[now]` (`v[keel_index(keel__c0.k, 3)]`), índice literal (fica `v[1]`) e corpo em chaves com dois índices.

## Arquivos

Pode editar, em `tools/cgen/src/engine/emit/` (e, se a forma exigir, o parser em `tools/cgen/src/engine/parser/`): o que for preciso. Pode apagar o `|wip` da linha `058-defer-index` de `test/cgen/gen/golden-cases.txt`. Não toque: normativos, `golden/**/expected/**`, `proof.c`, outros casos. Não faça commit nem `git add`. Se um teste (unit/ ou cli/) afirmar o contrário, não o edite: guarde o patch (sem o `golden-cases.txt`) fora do repositório e diga no relatório. **Se o esperado divergir da spec ou de um golden que já passa, pare nesse ponto e diga no relatório (o orquestrador arbitra: quatro vezes hoje o erro foi do oráculo).**

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # o caso sai de wip; 0 fail (o gen_golden também compila o C escrito)
golden/run.sh                   # "110 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
```

## Relatório

`tools/agents/reports/T35-defer-index-impl.md`, em pt-BR: o que mudou, a saída dos comandos, dúvidas. Entregue UMA vez pelo SubagentHandback e termine.
