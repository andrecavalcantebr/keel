# T11 — WP1: formas de declaração de nível de arquivo (relatório)

**Situação: o cartão não está concluído.** A implementação passa nos casos
pedidos, mas quebra um oráculo do `make check` (unit `emit`) que afirma o
contrário do que o cartão pede. Pelo cartão ("se algum caso que passava deixar
de passar, o defeito é seu; não altere oráculo nem baseline"), parei no último
estado que passa a aceitação: `emit.c` no commit, com o `|wip` restaurado nos
dois casos. A implementação completa está guardada como patch, fora do
repositório:

`/tmp/claude-1000/-home-andre-code-cprojects-keel/6df5c4e9-8d62-4717-9b96-cb209ab7b812/scratchpad/t11/T11-emit.patch`

(169 linhas, só `tools/cgen/src/engine/emit/emit.c`; aplica-se a `HEAD`, pois o
arquivo não mudou desde `78048eb`.)

## O que foi feito e o que não foi

### Implementação (no patch, não no repositório)

Arquivo: `tools/cgen/src/engine/emit/emit.c`.

1. `supported()`, `K_AST_IMPORT_C`: aceita a forma com aspas
   (`import_c "x";`, um só token). `emit_import` já copiava o token, então só a
   recusa mudou.
2. `supported()`, `K_AST_TYPE`: aceita `struct` e `union` sem `typedef`
   (`[pub|priv] struct NOME { … };`). A recusa de vários declaradores passou a
   valer só quando o nó anterior não é `K_AST_TYPE` (um `typedef` com vários
   declaradores é um nó por nome, no mesmo span).
3. `supported()`, `K_AST_VARIABLE`: a vírgula de nível 0 só é recusada para a
   lista `array` (array.c ainda trata um declarador só).
4. `first_of_span()` (novo) e `node_edits()`: a tag anônima de
   `typedef struct {…} P, *PP;` (`EDIT_TAG`) é escrita só pelo primeiro
   declarador; sem isso sairia duas vezes (`drop_nested` não remove inserções
   de tamanho zero).
5. `emit_extern()`: passou a receber o índice do nó e a escrever um grupo de
   declaradores (`pub int x = 1, y = 2;`) numa linha só: `h_body` chama a função
   só no primeiro nó do grupo. Cada `= INIT` é cortado do fim do nome até o
   `,` ou `;` seguinte; as edições dentro de um inicializador saem com ele; as
   edições passam por `k_em_sort_edits` e `drop_nested` antes da cópia.

Verificado com `--stop-after=gen` (c23 e c11):

- `golden/cases/038-decl-forms`: `app_dc.type.h`, `app_dc.h` e `app_dc.c`
  iguais byte a byte ao esperado, nos dois perfis;
- `test/cgen/gen/quoted`: `q.type.h`, `q.h` e `q.c` iguais, nos dois perfis.

### Alterações fora do `emit.c`

- `test/cgen/gen/golden-cases.txt`: as duas linhas (`quoted` e `038-decl-forms`)
  tinham `|wip` removido por mim. O commit `d5981e0` (outra sessão) levou essa
  remoção junto. Com o `HEAD` assim, `gen_golden.sh` falha nesses dois casos
  (FAIL). Restaurei o `|wip` na árvore de trabalho; o diff contra o `HEAD` são
  essas duas linhas. Não commitei.

Nenhum arquivo de normativo, nenhum `expected/`, nenhum `proof.c`, nenhum outro
caso foi tocado por mim.

## Por que parei: o oráculo de unit contradiz o cartão

Com o patch aplicado, `sh tools/agents/verify.sh` falha em `make check`:

```
FAIL   unit/emit/emit
FAIL: two declarators in one variable must be K_EMIT_UNSUPPORTED
FAIL: timed out or crashed (exit 1)
```

O trecho é `test/cgen/unit/emit/emit_main.c`, linhas 88–94 (bloco "Outside the
covered slice"): `pub int x, y;` tem de voltar `K_EMIT_UNSUPPORTED`. É
exatamente a forma 5 do cartão ("vários declaradores numa variável"), que o
cartão manda aceitar. Os dois pedidos não podem ser cumpridos ao mesmo tempo.
Não editei o oráculo: o cartão proíbe, e `test/cgen/unit/` não está na lista de
arquivos. Decisão necessária (André ou o orquestrador): trocar esse exemplo por
um que continue recusado, por exemplo `array int a[2], b[3];` (a lista
`array` continua recusada pelo item 3 acima), ou aceitar que o teste mude de
propósito.

## Aceitação no estado final (sem o patch, com `|wip` restaurado)

Saídas exatas:

`sh test/cgen/gen_golden.sh`
```
wip    038-decl-forms
wip    039-array-forms
wip    040-match-forms
wip    041-parallel-forms
wip    042-else-lines
44 ok, 0 fail, 6 wip
```
(o `quoted` também aparece como `wip`; `tail -6` cortou a linha). O cartão
esperava `46 ok, 0 fail, 0 wip`; os casos 039 a 042 vieram de outra sessão.

`golden/run.sh`
```
80 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
```
(wip: 022, 023 e 024. O cartão esperava `72 ok … 3 wip`; o número estava
desatualizado.)

`sh tools/agents/verify.sh`
```
ok   build (0 warnings, baseline 0)
ok   make check (51 lines ok)
ok   golden: 80 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

`sh tools/agents/survey/run.sh` (sem o patch, todos em LIMIT):
```
d01 [c23 LIMIT] [c11 LIMIT]  ...  d09 [c23 LIMIT] [c11 LIMIT]
```

Com o patch aplicado (medido antes de reverter):

- `gen_golden.sh`: `46 ok, 0 fail, 2 wip` (os wip eram 039 e 040);
- `golden/run.sh`: 038 `ok` em c23 e c11; o único FAIL era `line-check`, por
  causa de `expected/` de 041 escrito pela outra sessão (ainda sem
  `line-check.allow`), não do emissor;
- survey: `d01`…`d07` `[c23 OK] [c11 OK]`; `d08` e `d09` `LIMIT`, como o cartão
  prevê (`extern_c` plano e `priv` esperam regra do André);
- `verify.sh`: `FAIL make check`, pelo oráculo acima.

## Dúvidas e incoerências (cartão × código × oráculo)

1. **Oráculo de unit × forma 5**: ver acima. Bloqueante.
2. **Commit `d5981e0` com os `|wip` removidos**: o `HEAD` atual fica vermelho
   em `gen_golden.sh` até que a árvore volte a ter o `|wip` (ou até que a
   implementação entre). Decisão de commit é do orquestrador.
3. **Contagens do cartão desatualizadas**: `46 ok, 0 wip` e `72 ok … 3 wip`
   não batem com o repositório (que já tem 039–042 e 80 casos no golden).
4. **Concorrência**: durante a tarefa, outra sessão criou e commitou 039 a 042,
   editou `golden/line-check.allow` e `test/cgen/gen/golden-cases.txt`, e fez
   `make` no mesmo diretório. Os resultados de `golden/run.sh` e `verify.sh`
   acima são do momento em que rodei; o `line-check` mudou durante a tarefa.
5. **Fatos do parser que o patch usa** (confirmados no `--stop-after=parse`):
   um `typedef` com vários declaradores e uma variável com vários declaradores
   geram um nó por nome, no mesmo `first`/`end`; `typedef struct Node Node;`
   tem a tag renomeada pela busca por nome de `k_em_declared_at`, sem precisar
   de ilha; `import_c "x";` tem o nome num único token.

## Próximo passo sugerido

1. Decidir o oráculo de unit (`emit_main.c:91`), por exemplo com `array` no
   lugar de `int x, y`.
2. Aplicar o patch (`git apply` do arquivo acima), rodar os quatro comandos de
   novo, remover `|wip` das duas linhas de `golden-cases.txt` e commitar tudo
   junto.
3. Se for o caso, o `d08` (`extern_c` plano) e o `d09` (`priv`) ficam para a
   regra do André.
