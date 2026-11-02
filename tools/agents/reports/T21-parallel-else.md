# T21 — relatório: `parallel` com política por `constexpr`, `else` com default na linha seguinte

## Estado

**Parcial. Parei em um ponto do cartão**, como ele pede para o caso em que o esperado diverge da spec.

- Forma 1 (política por `constexpr`): **feita**. O `named_policy` de `048` sai byte a byte igual ao esperado (conferido com um `.k` de teste em scratchpad, fora do repositório).
- Forma 3 (`else` com o default na linha seguinte): **feita**. `050-else-default` sai byte a byte igual ao esperado nos dois perfis (`c23` e `c11`). Retirei o `|wip` dessa linha.
- Forma 2 (`parallel` com cabeçalho, corpo e `}` numa linha): **não feita**. Motivo abaixo (dúvida 1). Por isso `048-parallel-more` continua `|wip`, e `q02` continua `LIMIT`.

## Dúvida 1 — o esperado de `one_line` (048) diverge da spec

`keel-c-backend.md` §5.9, regra 16: "um `#line` precede o corpo do worker, e outro vem depois do bloco (§6)". A tabela de §6 diz o mesmo para o gestor de `parallel`: "uma antes do corpo do worker, e uma depois do bloco".

No `expected/c23/app/app_pm.c` do `one_line`, o corpo `{ keel_slice_i32 keel__c1 = part; … }` sai **sem** `#line` antes dele. O gestor tem várias linhas de C e todas caem na linha 16 do `.k`. Sem o `#line 16 "app/pm.k"` antes do corpo, um diagnóstico do compilador C nesse corpo cai em um número de linha errado. A spec manda `#line` quando os dois contadores divergem (§6, invariante), e aqui divergem.

O cartão diz "Sem `#line` dentro" e o esperado segue o cartão. Não escrevi o `#line` no esperado (é oráculo, cabe ao orquestrador) e não implementei a forma. **Arbitragem pedida:** (a) o esperado ganha `#line 16 "app/pm.k"` antes do corpo, ou (b) a spec muda para dispensar esse `#line`. Com (a), a implementação é direta: o `#line` vai na frente do corpo, como nos outros casos de `parallel`.

Consequência: `sh tools/agents/survey/run.sh` mostra `q02 [c23 LIMIT] [c11 LIMIT]` (era o esperado `OK`), e `048` fica `wip` em `gen_golden.sh` e em `golden/run.sh`.

## Dúvida 2 — arquivos fora da lista do cartão (emit.c e emit_internal.h)

O cartão permite `emit.c` e `emit_internal.h` "se for inevitável". Foi inevitável para a forma 3, e a causa está em `uses.c`, que não está na lista:

- Na forma 3, o `W(&s, ` precisa ir no início do valor, na linha seguinte do `else`. Esse ponto é também o primeiro token da chamada `slice.from(...)`, que é uma chamada de outro módulo.
- `uses.c:362-367` (`drop_edits`) remove toda edição com `at` em `[lo, hi)` quando reescreve a chamada (`uses.c:419`, `:442`, `:461`). Como `else.c` empilhava o `W(&x, ` antes de `uses.c` rodar, a edição era removida. Confirmei isso com uma marca de teste (`@@`), que não chegava à saída; a marca foi retirada.
- Solução feita: `k_em_edits_else_tail` (novo, em `else.c`) empilha o `W(&x, `, e `node_edits` em `emit.c` a chama logo depois de `k_em_edits_uses`, seguida de `k_em_sort_edits` sobre as edições novas. A declaração fica em `emit_internal.h`.
- A alternativa seria mudar `drop_edits` em `uses.c` para poupar inserções (`skip == 0`) no limite inferior. Mexe em outros construtos (`defer.c` tem o mesmo `drop_edits`), por isso não fiz.

Peço que o orquestrador confirme se a solução em `emit.c` está aceitável.

## O que mudou (arquivo por arquivo)

- `tools/cgen/src/engine/emit/parallel.c`: `analyze` aceita a política por `constexpr` de decimal (`k_em_constexpr_value`, como os workers; inicializador não decimal, ou valor negativo, é recusado). O comentário de cabeçalho foi atualizado. Nenhuma outra mudança.
- `tools/cgen/src/engine/emit/else.c`: `k_em_handles_else` deixa de exigir o `;` do default na mesma linha. `k_em_edits_else` cuida do cabeçalho (na linha do `else`) e do `)` no `;`, em qualquer linha. Novo `k_em_edits_else_tail` põe o `W(&x, ` no início do valor quando ele está numa linha seguinte. O comentário de cabeçalho foi atualizado (o `;` numa linha seguinte deixou de ser recusado).
- `tools/cgen/src/engine/emit/emit.c`: em `node_edits`, um bloco de 6 linhas depois de `k_em_edits_uses`, para `k_em_edits_else_tail` (ver dúvida 2).
- `tools/cgen/src/engine/emit/emit_internal.h`: uma declaração (`k_em_edits_else_tail`).
- `test/cgen/gen/golden-cases.txt`: só a linha `050-else-default` sem `|wip`.

Não toquei em `golden/**/expected/**`, em `proof.c`, em normativos, nem em outro caso. Não fiz `git add` nem commit.

## Incidente de escopo (já corrigido)

Durante a depuração, instrumentei `emit.c` com `#include <stdio.h>` e uma chamada de `fprintf`. A compilação falhou pela regra de arquitetura do engine (`engine/` não faz E/S, desenho §3.1). Reverti com `git checkout -- tools/cgen/src/engine/emit/emit.c` **antes** de qualquer nova edição; o diff de `emit.c` naquele momento tinha só as três linhas da instrumentação. A depuração seguinte foi feita em uma cópia em scratchpad, fora do repositório. A marca `@@` em `else.c` também foi retirada (restaurei `else.c` a partir de uma cópia em scratchpad e reconfirmei o build).

Também criei e removi `tmp_none/` na raiz do repositório, por um `--dest-dir` errado em um teste.

## Estado de outras partes do repositório (não são meus)

`git status` mostra mudanças que não fiz: `golden/line-check.allow` (modificado), `golden/cases/054-style-forms/`, `055-defer-return-index/`, `056-length-fields/` (não rastreados), e `tools/agents/reports/T25-…`, `T26-…`, `T27-…` (não rastreados). Os casos 022, 023, 024 e 049, 051, 053, 054, 055, 056 são de outros cartões. Não toquei neles. Nada disso está no escopo deste cartão; o orquestrador deve separar na hora do commit.

## Saída final dos comandos de aceitação

Rodados depois da última mudança de código (a última mudança posterior foi só um comentário em `else.c`; recompilei e reconferi o `050` byte a byte).

1. `sh test/cgen/gen_golden.sh`:
```
wip    048-parallel-more
wip    049-adapt-foreach
wip    051-generic-forms
wip    053-generic-module-instance
wip    056-length-fields
wip    055-defer-return-index
wip    054-style-forms
57 ok, 0 fail, 7 wip
```
Resultado: **0 fail**. `050` saiu de `wip`. `048` continua `wip` (dúvida 1). Os demais `wip` são de outros cartões.

2. `golden/run.sh` (`tail`): `ok     line-check   (backend §6)` e
```
108 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
```
Resultado: **0 fail**. Os 3 `wip` são `022-linux-list-import-c`, `023-linux-list-extern-c` e `024-linux-list-module` (esqueletos sem `expected/`). O cartão previa `102 ok`; a árvore tem mais casos agora.

3. `sh tools/agents/verify.sh`:
```
ok   build (0 warnings, baseline 0)
ok   make check (53 lines ok)
ok   golden: 108 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

4. `sh tools/agents/survey/run.sh` (linhas pedidas):
```
l01 [c23 OK] [c11 OK]
q01 [c23 OK] [c11 OK]
q02 [c23 LIMIT] [c11 LIMIT]
```
Resultado: **q02 não passa** (dúvida 1). `q01` e `l01` passam nos dois perfis.

## Testes unitários e de CLI

Nenhum teste em `test/cgen/unit/` ou `test/cgen/cli/` afirma o contrário do que o cartão pede (verifiquei `unit/emit`, `unit/engine`, `cli/*.sh|py`). `test/cgen/diag/parallel/w.k` continua recusando `parallel c ANY (… 0..k …)` e `parallel d seen (…)` (política não constexpr), como antes. O `--parallel-lowering=serie` e o oráculo `test/cgen/gen/serie` continuam passando (estão dentro de `gen_golden.sh` e `make check`).

## Resumo para o orquestrador

1. Decidir a dúvida 1 (`#line` antes do corpo na forma de uma linha): é o que bloqueia `048` e `q02`.
2. Aprovar, ou não, a solução em `emit.c` / `emit_internal.h` (dúvida 2).
3. Após a decisão, a forma 2 é pequena: só falta o gestor, o corpo e o `}` numa linha.
