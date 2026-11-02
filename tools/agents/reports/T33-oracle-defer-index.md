# T33 — relatório do autor de oráculo: índice dentro do corpo de um `defer`

Caso escrito à mão: `golden/cases/058-defer-index` (módulo `app.di`, `app/di.k`).
Nenhum `tools/cgen/cgen` foi rodado sobre o caso, nenhum arquivo de `tools/cgen/src/`
foi lido, e não houve `make` nem `verify.sh`. Não há commit nem `git add`.

## Arquivos criados ou alterados

- `golden/cases/058-defer-index/app/di.k`: o fonte keel, 48 linhas.
- `golden/cases/058-defer-index/proof.c`: o arnês, que roda e confere os valores.
- `golden/cases/058-defer-index/expected/c23/app/app_di.{type.h,h,c}`.
- `golden/cases/058-defer-index/expected/c11/app/app_di.{type.h,h,c}`.
- `test/cgen/gen/golden-cases.txt`: uma linha, `058-defer-index|app/di.k|-|wip`.
- `golden/line-check.allow`: **nada acrescentado**. O caso não tem código injetado sem
  linha própria no `.k`. A struct de `[now]` sai na linha do `defer`, como em 014, e o
  temporário `{ int keel__rv0 = ...` sai na linha do `return`, como em 055. Nenhum dos
  dois está na lista hoje, e não precisam estar.
- Este relatório.

Não criei `DEBUG` (o cartão só pede se houver índice fora do intervalo, e não há) nem
`NOTES` (não está na lista de arquivos do cartão).

## O que o caso cobre

Todas as funções têm `array i32 v[3]` local. O `defer` escreve em `v`, e o efeito é
observado por um `priv int seen`, escrito por um segundo `defer` registrado antes, que
por isso roda por último e lê o que os outros deixaram.

| Função | Forma | Saída | Cleanup esperado |
| --- | --- | --- | --- |
| `clear_var(size_t i)` | `defer v[i] = 0;` | fim natural, `void` | `v[keel_index(i, 3)] = 0;` |
| `take_var(size_t i)` | `defer v[i] = 0;` | `return v[0];` | `v[keel_index(i, 3)] = 0;` no inline, com `keel__rv0` |
| `clear_now(void)` | `defer [now size_t k] v[k] = 0;`, com `k = 2` depois | fim natural | `v[keel_index(keel__c0.k, 3)] = 0;` e `struct { size_t k; } keel__c0 = { k };` |
| `clear_lit(void)` | `defer v[1] = 0;` | fim natural | `v[1] = 0;` (decimal contra dimensão decimal, §5.3 r6) |
| `two_vars(size_t i, size_t j)` | `defer { v[i] = 1; v[j] = 2; }` | fim natural | `{`, as duas linhas com `keel_index`, `}` |

Valores conferidos em `proof.c`: `clear_var(0)` deixa `last() == 0`, e `clear_var(2)`
deixa 7. `take_var` devolve 7 nos dois casos, porque o valor é lido antes do cleanup.
`clear_now` dá 0, o que mostra que a cópia de `k` era 1 e não 2. `clear_lit` dá 70.
`two_vars` dá 182, 712 e 729, e o último mostra a ordem das duas escritas.

Os cinco casos têm uma saída só, então o cleanup sai inline (§5.5.2, regra 1). A escada
não entra aqui.

## Fontes usadas, conforme o cartão

- Modelos lidos: `014-defer-later` (o `[now]` e a struct no ponto de registro),
  `017-defer-return`, `037-defer-forms` (o corpo em chaves, o `#line` antes e depois do
  cleanup de fim natural), `055-defer-return-index` (o `return` com índice, o `keel__rv0`
  inline com dois cleanups e o `#line` antes do `return`).
- Spec: `keel-c-backend.md` §5.3 regras 4 e 6, §5.5 regras 1, 2, 4, 5 e 6, §5.5.2 regra 1.
- Contagem de linhas do `.c`: uma linha de saída por linha de fonte, linha em branco
  para cada linha que não emite nada (o registro de `defer` e as linhas do corpo do
  `defer` que só saem na saída), e `#line` quando o contador diverge.

## Verificação

`sh golden/run.sh`, a saída final:

```
110 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
```

Os três `wip` são 022, 023 e 024, que já estavam assim. O 058 aparece como `ok` nos
dois perfis (compila, roda e o `proof` retorna 0). Também passaram `layer structure`,
`layout-check` e `line-check`.

## Escolhas e pontos abertos

1. **Observação por `seen`.** As formas do cartão são todas `defer ... v[...]`, e o
   cleanup de uma função `void` não tem como ser observado de fora sem um canal. Usei
   um `priv int seen` escrito por um `defer` registrado antes, que roda depois. É uma
   escolha minha; se o orquestrador preferir outro canal, troco.
2. **`k = 2` depois do `[now]`** em `clear_now`. Repete a ideia de 014 (`early`): a cópia
   no registro é que vale, e o `k` já estava com um local de valor 1 para isso.
3. **Sem `DEBUG`.** Como o cartão pediu, só índices válidos. Não há teste de
   `array-index-out-of-bounds` aqui.
4. **Caso marcado `wip`** na linha de `golden-cases.txt`. O emissor ainda não tem o
   cleanup com `keel_index`, então a comparação com a saída do cgen vai dar diferença
   até a implementação chegar. Isso é o esperado para um caso de oráculo.
5. **Estado fora do escopo deste cartão.** `golden/cases/057-walk-allman/` (não
   rastreado) e a linha `057-walk-allman|app/wa.k|-|wip` em `golden-cases.txt` são de
   outro trabalho (T32) e não foram tocados por mim. O `git diff` de
   `golden-cases.txt` mostra as duas linhas novas juntas: só a de 058 é minha.
