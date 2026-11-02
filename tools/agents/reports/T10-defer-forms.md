# T10 — `defer` na forma que o programador escreveu (relatório)

## Estado

**Cartão não fechado.** O `defer` com corpo em chaves, em várias linhas, com comentário de linha
e com índice foi implementado em `defer.c`, e a saída do 037 bate com o oráculo em todas as linhas
exceto uma. Essa linha depende de um defeito do **parser** (`parser_islands_types.c`), fora dos
arquivos que o cartão permite. Por isso `gen_golden` fica em `43 ok, 0 fail, 1 wip` (o cartão pede
`44 ok, 0 fail, 0 wip`). Nada foi commitado, e o `|wip` do 037 continua na tabela (o caso não fechou).

## O que mudou (arquivo por arquivo)

- `tools/cgen/src/engine/emit/defer.c` (único arquivo alterado; `git status` mostra só este):
  - `analyze`: aceita corpo `{ ... }` (o `}` que casa com o `{` é o `semi`, sem `;`) e corpo de
    instrução simples com `[` e quebras de linha; continua recusando `{`/`}` numa instrução simples.
    Saiu a exigência de uma linha sem comentário (`plain_span` segue só para os `return`).
  - `in_body` (nova): o intervalo `(kw, semi]` de um `defer` é opaco para as varreduras de
    `break`/`continue`/`goto`/`return` (`plan_of`, emissão das saídas), para `inline_ordinal` e para
    a varredura dos `}` (`close_ladder`/`close_block`).
  - `put_body`: copia os tokens até `semi` inclusive; cada `\n` do texto vira um registro novo
    `L<n>|` (n = linha do `.k` seguinte). O `;` final não é mais acrescentado. `open_record_line`
    foi extraída; `open_record`, `close_record`, `put_cleanup` e `close_ladder` acompanham.
  - `edit_registration`: o registro do `defer` apaga cada linha da própria parte do statement
    (`[pa, pb)` por linha); o `keel__c<N>`/`keel__rv0` do `[now]`/`temp` fica na linha do `defer`.
  - `k_em_edits_defer`: o laço de registros pega todo `defer` cujo statement cruza a linha.
  - Cabeçalho do arquivo e comentário de `analyze`/`Defer.semi` atualizados às novas recusas.
- `emit.c`, `emit_internal.h`, `golden/**`, `test/cgen/gen/golden-cases.txt`, normativos: **não tocados**.

## Saída final dos três comandos (árvore de trabalho, sem commit)

`sh test/cgen/gen_golden.sh`:
```
wip    037-defer-forms (differs)
43 ok, 0 fail, 1 wip
```
(Antes da mudança: `43 ok, 0 fail, 1 wip`, com o 037 como `wip`. Não há regressão.)

`golden/run.sh` (dentro de `golden/`, última linha):
```
70 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
```

`sh tools/agents/verify.sh`:
```
ok   build (0 warnings, baseline 0)
ok   make check (51 lines ok)
ok   golden: 70 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

## Diferença do 037 (c23 e c11)

Com a árvore atual, `app/app_df.c` difere do oráculo em **uma linha**, a 42 do C gerado:
```
< app_df_slots[1] = 2;     (oráculo, linha 29 do .k)
> slots[1] = 2;
```
Todo o resto bate, incluindo as quatro formas do cartão (`braced`, `spread`, `indexed`, `returned`)
e os `#line`.

### Causa (defeito de parser, fora do escopo)

`tools/cgen/src/engine/parser/parser_islands_types.c:1179` (`isl_c_declares`): a lista de palavras
que indicam "instrução, não declaração" não tem `defer`. Então `defer slots[1] = 9;` é lido como a
declaração `defer slots[1]`: `slots` vira um local, e `isl_name_island` (linha 1080) não emite a
ilha de nome dos usos seguintes de `slots`. A regra só pega o token logo depois de `defer`, por
isso `defer total = 9;` não perde nada. Verificado com `--stop-after=parse`: sem o `defer`, `slots`
tem ilha em toda linha; com `defer slots[1] = 9;`, nenhuma.

Proposta (não aplicada): acrescentar `|| isl_word(a, p, "defer")` à lista da linha 1179.

### Segundo defeito (emit.c, `drop_nested`)

Um comentário de linha própria dentro do corpo (`// ...` ou `/* ... */` sozinho na linha) dá
`implementation-limit`. `comment_edits` empurra um DROP para o comentário com `at = a` (o
espaço antes é absorvido), e o DROP da linha do `defer` tem o mesmo `at = a`. `drop_nested`
(`emit.c:371`) só descarta uma substituição que começa **estritamente depois** da anterior, então
as duas ficam e se sobrepõem (`copy_edited` falha).

Proposta (não aplicada): no `drop_nested`, descartar também substituições (`skip > 0`) de mesma
posição: `if (j > 0 && ed[k].at < end && (ed[k].at > ed[j - 1].at || ed[k].skip > 0)) continue;`.
Inserções (`skip == 0`) na posição de uma substituição continuam sendo mantidas.

### Teste dos dois ajustes (cópia descartável em `/tmp/claude-1000/t10/exp`, não na árvore)

Com os dois ajustes acima sobre a árvore atual:
- 037 em c23 e c11: bate com o oráculo byte a byte (`diff` vazio nos dois).
- `sh test/cgen/gen_golden.sh`: `44 ok, 0 fail, 0 wip`.
- O caso do comentário de linha própria gera a saída esperada (ver abaixo).

Ou seja: a aceitação do cartão passa com esses dois ajustes, que estão fora da lista de arquivos.
Decisão para o orquestrador: aplicar os dois (parser e `emit.c`), ou autorizar outro caminho.

## Testes à mão (passo 4 do cartão)

Saída do cgen (`-std=c23`, `--stop-after=gen`) e `cc -std=c11 -fsyntax-only` (flags do
`golden/run.sh`), na árvore atual. Fontes em `/tmp/claude-1000/t10/hand/<caso>/app/h.k`.

| Caso | Forma | cgen | cc c11 | Observação |
|---|---|---|---|---|
| h1 | `defer [now int x] { add(x); }` | ok | ok | `struct {int x;} keel__c0 = { x };` na linha do defer; cleanup usa `keel__c0.x`; `#line 8` e `#line 12` corretos |
| h2 | corpo com `if`, chaves aninhadas | ok | ok | 8 linhas em branco, cleanup de `{` a `}` mapeado a 7..14 |
| h3 | dois `defer { }` na mesma função | ok | ok | ordem inversa: `#line 10` (add 2) antes de `#line 7` (add 1) |
| h4 | `defer { }` com duas saídas (escada) | ok | ok | `keel__e0: {` mapeado à linha 7; `return keel__rv0;` na 13 |
| h5a | comentário `/* */` sozinho na linha, dentro do corpo | **implementation-limit** | - | defeito 2 acima |
| h5b | comentário `// x` sozinho na linha, dentro do corpo | **implementation-limit** | - | defeito 2 acima |
| h5c | `add(5); // x` no fim da linha, dentro do corpo | ok | ok | comentário copiado |
| h6 | `break` dentro de `for` que está dentro do corpo | ok | ok | o `break` fica no laço do corpo; não vira saída da função |

O caso h5 (comentário de linha própria) foi rodado também na cópia com o ajuste do `drop_nested`:
gera `// a line comment alone` e `/* only a block comment */` no cleanup, com `#line` corretos.

Não foi feito o teste de `[now]` com corpo em chaves combinado com `return` (h1 cobre o
`[now]` sem saída extra), nem a comparação com snapshot de `verify.sh` (não é refatoração; o
cartão não pedia snapshot).

## Dúvidas e incoerências (cartão × código × spec)

1. O cartão diz que o `Defer.semi` é o último token. O código agora faz isso, mas `k_em_handles_defer`
   ainda exige que o `}` do bloco englobante comece a linha (`first_on_line`); não mudei.
2. Comentário de linha própria dentro do corpo (defeito 2): o cartão pede que "comentário dentro do
   corpo é copiado como está". Só o caso com código na mesma linha funciona hoje.
3. O cartão pede que, no caso do `defer` com índice, o `slots[...]` do corpo saia renomeado. O
   resultado depende do defeito de parser (defeito 1); o cleanup em si (`app_df_slots[1] = 9;`)
   já sai correto.
