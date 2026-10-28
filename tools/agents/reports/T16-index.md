# T16 — relatório: índice verificado sobre `array`

## Resultado

**Não está em ALL OK.** O `index` (`test/cgen/gen/index`) sai `ok` nos dois
perfis, como pede o cartão. Mas `verify.sh` falha no `gen_golden`, por causa
do golden `008-block-constexpr`, que antes era `wip` e agora passa pela
construção de índice e chega a uma falha de comentário, que está fora deste
arquivo (ver "Dúvidas"). Não commitado.

## Arquivo por arquivo

- `tools/cgen/src/engine/emit/index.c`: preenchido. É o único arquivo tocado
  (além deste relatório). `emit.c`, `emit_core.c`, `emit_internal.h`,
  `array.c`, `foreach.c`, `constexpr_block.c` e os oráculos não foram
  alterados.

Conteúdo de `index.c`, em resumo:

- Lê o uso `v[..]`, `m[i,j]`, `m[1][j]` a partir do nome (âncora da ilha
  `K_ISLAND_ARRAY_INDEX`): os índices de cada colchete, com as vírgulas do nível
  externo entre eles.
- Acha a declaração: a `array` mais próxima antes do uso, na mesma função, com
  o mesmo nome. Lê as dimensões entre `[` e `]`. Recusa, como `array.c`, o
  binder (`size_t n`) e a segunda declaração (`array i32 a[3], b[4];`).
- Para cada índice: sai simples quando o índice é um literal decimal e a
  dimensão é conhecida e maior que ele (regra 6). Senão, insere `keel_index(`,
  `, ` + a dimensão (`EDIT_TOKS`, com os renomes) e `)`, nessa ordem, todos com
  `skip` 0.
- Cada vírgula entre índices de um colchete vira `][` (com os brancos que a
  seguem), sempre, independente dos índices.
- Conhecimento de dimensão (pedido do orquestrador, além do cartão): a dimensão
  é decimal se é um literal de um dígito só, ou um identificador que nomeia um
  `constexpr` cujo inicializador é um literal decimal. A busca é feita aqui,
  por tokens: (a) `constexpr` de bloco no corpo, o mais próximo antes da
  declaração do array, pulando blocos irmãos fechados, como `cb_use` em
  `constexpr_block.c`; se o mais próximo com esse nome não for literal, a
  dimensão é desconhecida; (b) senão, o `constexpr` de módulo de mesmo nome
  (`K_AST_CONSTEXPR`). `constexpr_block.c` não exporta essa busca (as funções
  são `static`), então ela foi reescrita aqui.
- `k_em_handles_index`: `true` quando a declaração foi achada, é simples e o
  número de índices escritos não passa do de dimensões.

Não há estado entre chamadas: cada linha recalcula a partir dos tokens.

## Saída do oráculo do índice

`sh test/cgen/gen_golden.sh` (trecho relevante):

```
wip    003-alignment
wip    006-dim
wip    007-instance
FAIL   008-block-constexpr c23/app/app_cx.c differs
       12,13c12,13
       < 
       < 
       ---
       >    `c->N`. That is why the backend generates a name of its own and rewrites
       >    the uses (§9.2). */
FAIL   008-block-constexpr c11/app/app_cx.c differs
       12,13c12,13
       < 
       < 
       ---
       >    `c->N`. That is why the backend generates a name of its own and rewrites
       >    the uses (§9.2). */
6 ok, 1 fail, 3 wip
```

Os `ok` são `shapes`, `tags`, `array`, `foreach`, `index` e `constblock`. A
linha `index` não aparece como falha: sai `ok` nos dois perfis, byte a byte
igual ao `expected/` (verificado com `gen_golden.sh`).

## Saída do `verify.sh`

```
ok   build (2 warnings, baseline 4)
FAIL make check
FAIL   gen_golden
FAIL   008-block-constexpr c23/app/app_cx.c differs
FAIL   008-block-constexpr c11/app/app_cx.c differs
```

O `make check` falha porque roda o mesmo `gen_golden`. O build está abaixo do
baseline (2 avisos, baseline 4): nenhum aviso novo deste arquivo.

## Verificação extra (o `.k` temporário em `/tmp`)

Dois módulos temporários (`/tmp/t16/ex/ex.k` e `/tmp/t16/cm/cm.k`), gerados
com `--stop-after=gen`. No módulo `ex`:

```c
/* c23 */                              /* c11 (só o que muda) */
m[1 ][2] = 0;                          m[1 ][2] = 0;
m[1][keel_index(j, 3)] = 1;
m[keel_index(i, 2)][keel_index(j, 3)] = 2;
w[1] = 3;                              /* M = 3, module constexpr literal: simples */
w[keel_index(i, ex_M)] = 4;
q[keel_index(0, ex_K)] = 5;           /* K = 2 + 1: não é literal, fica keel_index */
r[1] = 6;                              r[1] = 6;      /* L = 2, block constexpr literal */
r[keel_index(L - 1, L)] = 7;           r[keel_index(keel__L_0 - 1, keel__L_0)] = 7;
return m[0][0] + w[2] + q[keel_index(1, ex_K)];
```

Com `M` e `L` literais, o colchete literal sai simples. `K` com `2 + 1` não
é literal, então fica `keel_index`. No C11, `L` vira `keel__L_0` só nas
dimensões que passam por `keel_index`. Os demais detalhes: `m[1 ][2]` mantém
o branco antes da vírgula (o cartão só manda trocar a vírgula e os brancos
que a seguem).

Golden 008 gerado direto (`--main app.cx`):

- `other`: `{ constexpr size_t N = 2; i32 a[N]; a[0] = 1; ... }` (c23) e
  `{ static const size_t keel__N_1__chk = 2; ... i32 a[keel__N_1]; a[0] = 1; ... }`
  (c11). `a[0]` sai simples, como pediu o orquestrador (`N` = 2 pelo
  `constexpr` de bloco, literal).
- `sum`: `v[keel_index(i, N)]` (c23) e `v[keel_index(i, keel__N_0)]` (c11),
  como antes.

## Dúvidas e pontos em aberto

1. **Regressão do 008 (wip → FAIL), fora do escopo deste arquivo.** Antes
   deste trabalho, `008-block-constexpr` era `wip`, porque o índice de `array`
   era recusado e o módulo caía em `implementation-limit`. Agora o índice é
   tratado, o módulo passa e a emissão falha na diferença de comentário.
   Causa: um comentário de bloco de várias linhas, no nível do arquivo. Teste
   mínimo sem arrays (`/tmp/t16/cm/cm.k`):

   ```c
   /* Block comment across lines:
      the second line of it. */
   ```

   sai como

   ```c
   
      the second line of it. */
   ```

   A primeira linha do comentário some e a continuação fica. Isso é um bug
   pré-existente de tratamento de comentário em `emit.c` (backend §6, regra
   3), e o C gerado é inválido. Não é do índice: o problema aparece sem nenhum
   `array`. O `expected/` do 008 espera as linhas em branco. Não mexi em
   `emit.c`, que não está no escopo.

2. **Condição do `verify.sh`.** O cartão pede `ALL OK`, e isso não acontece
   por causa do ponto 1. Decisão para o orquestrador: consertar o comentário
   (em `emit.c`, tarefa separada) ou aceitar o 008 como `wip` até lá. Não
   altero o oráculo do 008.

3. **Dimensão `constexpr` de módulo, via `constexpr_at`/`constexpr_value`.**
   Assumo que o nó `K_AST_CONSTEXPR` tem o nome em `name_first` e o `=` logo
   depois. Confirmado pelo teste de `ex` (`pub constexpr size_t M = 3;` →
   `w[1]` simples), mas não por leitura do parser.

4. **Índice literal maior ou igual à dimensão.** O parser já recusa
   (`array-index-above-dimension`), então não chega ao emissor. Por segurança,
   o emissor também mantém `keel_index` nesse caso (condição `iv < dv`).

5. **Parser não diz `constexpr` aos emissores.** `isl_decimal_of_token` (no
   parser) resolve `N` por `Local`/`K_SYM_CONSTANT`, mas o emissor não tem
   acesso a isso. Por isso refiz a busca de `constexpr` em `index.c`. Se o
   parser passasse o valor em uma ilha, a busca sumiria; é um refactor
   opcional.

6. **Duplicação.** `read_decl` repete a leitura de `array_decl` de `array.c`
   (e `blanks_after`, `enclosing_fn`, `tok_is_word`), porque as funções de
   `array.c` são `static`. Se a API ganhar uma função de leitura de
   declaração, os dois arquivos podem compartilhá-la.

## Não feito

- Não commitado (o cartão manda deixar para o orquestrador).
- `emit.c`, `emit_core.c`, `emit_internal.h`, `array.c`, `foreach.c`,
  `constexpr_block.c`, `golden/*` e `test/cgen/gen/*/expected` não foram
  tocados.
