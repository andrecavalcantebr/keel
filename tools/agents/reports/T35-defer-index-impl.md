# T35 — índice dentro do corpo de um defer passa por keel_index (relatório)

Status: **parado no ponto de divergência do oráculo** (cartão: "se o esperado
divergir de um golden que já passa, pare e diga no relatório"). A implementação
das arestas de índice no corpo do defer está pronta e verde; o caso
`058-defer-index` ainda não sai de `wip`, porque a diferença que resta está no
`expected/`, não no emissor.

## O que mudou

- `tools/cgen/src/engine/emit/defer.c`:
  - `put_body` passou a aplicar as arestas de índice do corpo, com
    `k_em_edits_index` sobre os bytes `[tok_off(body), tok_end(semi))` e
    `k_em_sort_edits`, da mesma forma que `put_expr` faz para o `return` em
    linha. Os bytes entre as arestas saem por `put_body_bytes`.
  - `put_body_bytes` é a antiga lógica token a token (gaps como escritos, quebra
    de linha fecha e abre registro `L<n>|`, renomeação de captura de `[now]`),
    limitada a um intervalo de bytes.
  - `put_body` recebe `fn` (dois chamadores: `put_cleanup` e `close_ladder`).
- Nenhum outro arquivo foi tocado. `test/cgen/gen/golden-cases.txt` está como
  antes: a linha `058-defer-index|app/di.k|-|wip` **não** foi alterada, porque o
  caso não passa.
- Patch da implementação, fora do repositório:
  `/tmp/claude-1000/-home-andre-code-cprojects-keel/6df5c4e9-8d62-4717-9b96-cb209ab7b812/scratchpad/T35-defer-body-index.patch`

## O que bate com o oráculo

Gerado com `cgen --stop-after=gen` para os dois perfis, o caso 058 agora sai
igual ao `expected/` em todas as linhas dos cleanups:

- `v[keel_index(i, 3)] = 0;` no fim natural (`clear_var`) e no `return`
  (`take_var`);
- `v[keel_index(keel__c0.k, 3)] = 0;` com `[now]` (`clear_now`);
- `v[1] = 0;` com índice literal (`clear_lit`);
- corpo em chaves com dois índices (`two_vars`), com os `#line` no lugar.

## Divergência que impede a aceitação

`sh test/cgen/gen_golden.sh` marca `058-defer-index (differs)`. A única diferença
é de espaço em duas declarações de `pub  int  last(void)`, em `app_di.c` e
`app_di.h`, nos dois perfis:

```
< int  app_di_last(void) { return app_di_seen; }     (cgen)
> int app_di_last(void) { return app_di_seen; }      (expected)
< int  app_di_last(void);                             (cgen)
> int app_di_last(void);                              (expected)
```

O `.k` tem dois espaços antes de `int` e antes de `last`. O emissor copia o
intervalo entre o tipo e o nome como está escrito (`put_rettype`,
`k_em_put_range(ret_first, name_first)`), e isso já é o comportamento de goldens
que passam hoje, com o mesmo formato de declaração:

- `golden/cases/014-defer-later/app/dl.k:5` (`pub  int  last(void)`) gera
  `int  app_dl_last(void)` em `expected/c23/app/app_dl.c:8`;
- `golden/cases/005-defer-loop/app/loop.k:6` (`priv int  acquire`) gera
  `static int  app_loop_acquire` em `expected/c23/app/app_loop.c:9`.

Pelo convencional do repositório, o `expected/` de 058 deveria ter `int  app_di_last`
com dois espaços. Não editei o `expected/` (regra do cartão e do AGENTS.md). O
orquestrador decide: ou o oráculo ganha os dois espaços (nos dois perfis, no `.c`
e no `.h`), ou o caso passa a ter o `.k` com um espaço só. Em qualquer dos dois
casos, o emissor não muda nesse ponto.

## Aceitação (estado atual, com o caso ainda em `wip`)

`sh test/cgen/gen_golden.sh`: o caso 058 **não** saiu de `wip` (`wip 058-defer-index (differs)`,
`64 ok, 0 fail, 1 wip`). Não é a saída esperada pelo cartão.

`sh tools/agents/verify.sh` (saída final, copiada da execução):

```
ok   build (0 warnings, baseline 0)
ok   make check (53 lines ok)
ok   golden: 110 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

`golden/run.sh` faz parte desse `verify.sh` e dá `110 ok, 0 fail, 0 xfail, 0 xpass, 3 wip`,
a mesma linha que o cartão pede.

## Testes que afirmam o contrário

Nenhum. `grep -rn defer test/cgen` mostra só `test/cgen/cli/statements.sh` (que
checa as linhas `ilha defer`, não o texto do corpo), `test/cgen/parse/*.parse`
(ilhas, não emissão) e os `.k` de `test/cgen/gen/defervoid`, que passam. `make check`
e a bateria inteira continuam verdes com a mudança.

## Dúvidas abertas

1. Confirmar a convenção do espaço duplo (acima) e corrigir o oráculo ou o `.k`
   do 058; depois apagar o `|wip` da linha 71 de `test/cgen/gen/golden-cases.txt`
   e rodar a aceitação de novo.
2. Este é um relatório de parada, não de conclusão: nada foi commitado nem
   adicionado ao índice (`git status` mostra só `tools/cgen/src/engine/emit/defer.c`
   modificado, mais este relatório).
