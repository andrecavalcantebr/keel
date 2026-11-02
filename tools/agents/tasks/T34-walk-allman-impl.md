# T34-walk-allman-impl — walk com a { do corpo em linha própria

## Objetivo

Oráculo escrito por outro agente (piloto, revisado por mim): **`golden/cases/057-walk-allman`** (`app/*.k`, `expected/c23/`, `expected/c11/`, `proof.c`). Leia o `.k` e o `expected/c23/` antes de tudo. Hoje o caso está com `|wip` em `test/cgen/gen/golden-cases.txt`. Faça o cgen produzi-lo byte a byte nos dois perfis.

Hoje o `walk` com a `{` do corpo na linha seguinte gera C que não compila: a linha da `{` não recebe a declaração do binder (`{ i32 *p = app_wa_next(keel__c0, &c);`). A regra é a do `foreach` Allman, que já funciona (`foreach.c`, `body_brace_line`, T29): o cabeçalho sai sem a chave do corpo (`… while (app_wa_has_next(keel__c0, &c))` termina a linha), a linha da `{` abre o corpo e leva o binder, e a `}` do corpo fecha também o bloco (`} }`).

## Arquivos

Pode editar, em `tools/cgen/src/engine/emit/` (e, se a forma exigir, o parser em `tools/cgen/src/engine/parser/`): o que for preciso. Pode apagar o `|wip` da linha `057-walk-allman` de `test/cgen/gen/golden-cases.txt`. Não toque: normativos, `golden/**/expected/**`, `proof.c`, outros casos. Não faça commit nem `git add`. Se um teste (unit/ ou cli/) afirmar o contrário, não o edite: guarde o patch (sem o `golden-cases.txt`) fora do repositório e diga no relatório. **Se o esperado divergir da spec ou de um golden que já passa, pare nesse ponto e diga no relatório (o orquestrador arbitra: quatro vezes hoje o erro foi do oráculo).**

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # o caso sai de wip; 0 fail (o gen_golden também compila o C escrito)
golden/run.sh                   # "110 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
```

## Relatório

`tools/agents/reports/T34-walk-allman-impl.md`, em pt-BR: o que mudou, a saída dos comandos, dúvidas. Entregue UMA vez pelo SubagentHandback e termine.
