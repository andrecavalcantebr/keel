# T18 — `defer`: cleanup em linhas próprias (D1 refeito) e saídas por `return` (D2): relatório

## O que mudou

Arquivo único: `tools/cgen/src/engine/emit/defer.c`. Nada mais foi editado: nem `emit.c`,
`emit_internal.h`, golden, oráculos, tabela `test/cgen/gen/golden-cases.txt` ou normativos.
Não há commit.

`git status --short` ao final:

```
 M tools/cgen/src/engine/emit/defer.c
```

### `defer.c`, por partes

- **Cleanup do fim natural (D1 refeito, `EDIT_LINES`).** Cada `}` que fecha um bloco com
  `defer` pendente recebe um registro `L<n>|<indentação do defer><corpo>;` por defer, na
  ordem inversa de registro, mapeado à linha do `defer` (`n`), e um `T|<indentação do }>`.
  A primeira metade (só brancos) não sai. A indentação de cada linha de cleanup é a do
  próprio `defer` (antes era uma só para o `}`).
- **Registro.** `edit_registration`: `defer` e `defer [later]` ficam vazios; `[now]` vira a
  struct `keel__c<N>`. Com o temporário da escada (ver abaixo), a linha do **primeiro** defer
  da função ganha `T keel__rv0;` (depois da struct, se houver), com a indentação preservada.
- **Contagem de saídas** (`plan_of`): `RET_NONE` (sem `return`: só D1); `RET_INLINE` (um
  único `return`, que é a última instrução do corpo, no nível do corpo); `RET_LADDER` (dois
  ou mais, ou um `return` seguido do fim natural). O fim natural conta uma saída quando o
  corpo não termina em `return`.
- **Inline** (`inline_return`): `return;` → `{ <cleanup> return; }` (um `EDIT_LINES` com
  `H|{` no `return`, e `EDIT_TEXT " }"` após o `;`). `return expr;` → uma só `EDIT_LINES`
  sobre a instrução inteira: `{ T keel__rv0 = <expr>;`, os `L`, e `return keel__rv0; }`. A
  `<expr>` sai por `k_em_put_range` (renomes aplicados), e as edições antigas da instrução
  são removidas (`drop_edits`).
- **Escada** (`ladder_return`, `close_ladder`): `return expr;` → `keel__rv0 = expr; goto
  keel__e<K>;` por edições de token (`return` substituído, `goto` inserido depois do `;`).
  Se o token anterior é `)` ou `else`, a saída ganha `{ … }`. No `}` que fecha a função, um
  `L<n>|keel__e<k>: <corpo>` por defer, em ordem inversa, e `T|<indent do 1º defer>return
  keel__rv0; ` (o `}` vem do fonte).
- **Recusa** (`k_em_handles_defer` → `plan_of(...).ok`): `break`, `continue`, `goto` (D3);
  `defer` em bloco aninhado numa função que tem `return`; função `void` com escada; `return`
  com forma inesperada (`return;` em função não `void`, ou `return expr;` em `void`, ou sem
  `;`); no inline, `return` em mais de uma linha, com comentário, ou com `[`. Mantidas as
  recusas de D1 (`defer` que ocupa mais de uma linha, comentário, chaves ou índice no corpo,
  `}` que não inicia a linha).

## Saídas

### `sh test/cgen/gen_golden.sh` (final)

```
wip    003-alignment
wip    006-dim
wip    007-instance
wip    017-defer-return (differs)
wip    005-defer-loop
8 ok, 0 fail, 5 wip
```

Antes da alteração: `7 ok, 0 fail, 6 wip`. **`014-defer-later` agora bate nos dois perfis**
(sai em silêncio, como qualquer `ok`). **`017-defer-return` não fecha por um motivo que não é
do `defer`** (ver dúvida 1): nos dois perfis, o `.c` gerado é igual ao golden byte a byte, e a
diferença está só no `.h`.

Diferença restante de `017` (idêntica nos dois perfis; `.h`):

```
< int  app_dr_consume(app_dr_Rec *r, int mode);     (golden: dois espaços)
> int app_dr_consume(app_dr_Rec *r, int mode);      (gerado: um espaço, como no .k)
< int  app_dr_two(app_dr_Rec *r, int early);
> int app_dr_two(app_dr_Rec *r, int early);
```

### `sh tools/agents/verify.sh` (final)

```
ok   build (2 warnings, baseline 4)
ok   make check (48 lines ok)
ok   golden: 46 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

Os 2 avisos são de `src/tool/main.c` (`f_flag` e `pedantic_names` não usados), anteriores a
este trabalho; nenhum aviso de `defer.c`. `make -C tools/cgen boundary`: `ok engine boundary`.

### Compilação do gerado dos golden 014 e 017

```
017 c23: gcc -std=c2x -Wall -Wextra -c app/app_dr.c -> rc=0, sem aviso
017 c11: gcc -std=c11 -Wall -Wextra -c app/app_dr.c -> rc=0, sem aviso
014 (c23 e c11): compilados na T17, rc=0, sem aviso; o gerado de hoje bate byte a byte.
```

### Casos extras, em `/tmp` (fora do repositório)

- `/tmp/t18-src/app/ret.k`, gerado em C11 e compilado com `-std=c11` e `-std=c2x`, `-Wall
  -Wextra`, sem aviso. Cobre: `f` (inline com valor), `g` (inline `void`), `h` (escada,
  dois `return`), `m` (escada com fim natural), `early` (um `return` antes do defer fica
  intocado). Saída de `f`:

  ```c
  int app_ret_f(int *r) {

      { int keel__rv0 = *r + 1;
  #line 7 "app/ret.k"
      app_ret_reset(r);
      return keel__rv0; }
  }
  ```

- `/tmp/t18-src/app/bad_void.k` (`void` com dois `return`): recusado,
  `cgen: error: … [implementation-limit]`, rc 2.
- `/tmp/t18-src/app/bad_break.k` (`break` no `while`): recusado, mesmo diagnóstico, rc 2.
- `/tmp/t18-src/app/bad_nest.k` (`defer` em `if` com `return` depois): recusado, rc 2. O
  parser emite antes o aviso `defer-in-control-block`, que já existia.

## Dúvidas e incoerências

1. **Golden `017`: o `.h` tem dois espaços onde o `.k` tem um (bloqueia `017` `ok`).** O
   cartão pede `017` `ok` nos dois perfis. Nos `.c` ele já bate byte a byte. Em
   `expected/{c23,c11}/app/app_dr.h`, as linhas são `int  app_dr_consume(...)` e `int
   app_dr_two(...)`, com dois espaços. Em `app/dr.k` estão `pub int consume(` e `pub int
   two(`, com um. O emissor do protótipo (`emit.c`, que não toquei) copia o texto da fonte, e
   o `014` confirma isso: o golden dele tem `int  app_dl_last` porque o `.k` tem `pub  int
   last`, com dois espaços. Logo o `.h` de `017` contradiz o `.k`, e a regra "a emissão segue o
   `.k` sempre" aponta o golden. Não editei o oráculo. **Precisa de decisão do orquestrador:**
   corrigir o golden `017` (`.h`, nos dois perfis) ou o `.k`.

2. **Defer em bloco aninhado.** O cartão diz que `defer` em bloco aninhado é recusado "como no
   D1". O D1 não recusava: o `nest.k` da T17 gera e compila. Fiz assim: sem `return` na função,
   o aninhado continua aceito (como o D1); com qualquer `return`, o aninhado é recusado (a saída
   do `return` teria de limpar os blocos de dentro). É uma escolha minha. **Pergunta:** é isso
   que o cartão quer, ou o aninhado deve ser recusado também sem `return`?

3. **Escada com fim natural em função não `void`.** Quando o corpo cai no fim sem `return`,
   o fluxo passa pelos rótulos e chega a `return keel__rv0;`, que não tem valor atribuído.
   Não testei se a linguagem já recusa uma função não `void` que cai no fim. **Pergunta:** isso
   é erro da linguagem (então não precisa de tratamento), ou devo recusar a escada com fim
   natural em função não `void`?

4. **Indentação de linhas múltiplas.** Cada linha `L` de cleanup usa a indentação do próprio
   `defer` (o cartão diz "indentação do defer"; a T17 usava a do último). A linha `T` da escada
   usa a do primeiro defer (onde mora `keel__rv0`). Nenhum golden cobre defers com indentações
   diferentes.

5. **`keel__rv<n>` do inline.** O cartão fala em contador por função. O inline só existe com
   uma saída, que é um único `return`, então `n` é sempre 0 e o nome é fixo `keel__rv0`.

6. **Recusas no inline não cobertas pelo golden.** `return` com `[` (índice) no valor, e
   `return` em mais de uma linha, são recusados (é o caso de `drop_edits` e do `EDIT_LINES`
   de uma linha só). Na escada eles passam, porque a escada só troca a palavra `return` e
   acrescenta o `goto` após o `;`.

7. **Não testado:** dois `defer` na mesma linha (o espaço entre eles fica no lugar); função de
   uma linha `int f(){ defer a(); return 1; }` (recusada pela regra do `}` que inicia a linha);
   `return` dentro de `match`, que não é emitido por este módulo.

8. **Não toquei a tabela** `test/cgen/gen/golden-cases.txt`: o marcador `wip` de `014` e `017`
   continua. Sugiro tirar o de `014` na integração (já sai `ok`). O de `017` só sai depois da
   dúvida 1.
