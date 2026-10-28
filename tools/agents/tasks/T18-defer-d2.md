# T18 — `defer`: cleanup em linhas próprias (refaz o D1) e saídas por `return` (D2)

Leia primeiro `T13-16-contexto.md` e o relatório `T17-defer-d1.md` (o D1, integrado).

## O que mudou desde o T17

Em 2026-10-28 o André decidiu que **o cleanup do `defer` sai em linhas de saída
próprias, cada uma mapeada à linha do `defer` que o escreveu** (backend §5.5, regra 4;
§6, regra 2 e tabela; D61). Os golden `014-defer-later` e `017-defer-return`
já têm o layout novo, e o `014` hoje **falha** contra o seu `defer.c`: o D1 põe o
cleanup na linha do `}`. Esta tarefa (a) refaz a saída do D1 com o mecanismo novo e
(b) acrescenta as saídas por `return`. `break`/`continue`/`goto` (o D3, caso
`005-defer-loop`) **continuam recusando** o módulo.

## Arquivos

Edita só `tools/cgen/src/engine/emit/defer.c`; cria
`tools/agents/reports/T18-defer-d2.md`. Oráculos (não altere):
`golden/cases/014-defer-later` e `golden/cases/017-defer-return`
(`app/*.k`, `expected/{c23,c11}/app/*.c`), ambos na tabela de `gen_golden.sh`.
`005-defer-loop` está na tabela e fica `wip`/recusado até o D3.

## O mecanismo novo: `EDIT_LINES` (já pronto no `emit.c`)

`Edit` ganhou o tipo `EDIT_LINES` (leia o comentário em `emit_internal.h`). Ele corta a
linha de saída na posição `at` e escreve linhas próprias entre as duas metades. O
`text` é um payload em `k_em_scratch`, um registro por linha:

```
H|<texto>     acrescentado à primeira metade (ex.: "{"); opcional, o primeiro
L<n>|<texto>  uma linha de saída própria, mapeada à linha <n> do .k (um por defer)
T|<texto>     início da segunda metade (a indentação dela); o último
```

A primeira metade (o que já estava no buffer da linha, mais o `H`) sai como linha
própria, **salvo se só tiver brancos** (é o caso do `}` que abre a linha: nenhuma
linha em branco sai). A segunda metade começa com o `T` e continua com o fonte depois
da edição (`skip` bytes depois de `at`). Quem escreve o `#line` é o `KWriter`: cada
linha extra chama `k_emit_at(n, …)`, e ele põe o `#line` **só quando o contador
diverge**. Você nunca escreve `#line`.

## Contrato (leia os `expected` inteiros, são a verdade)

Escopo: funções cujos `defer` estão **todos no bloco do corpo** da função, com saídas
só por `return` e pelo fim natural. `defer` em bloco aninhado, `break`, `continue`
e `goto` no corpo: recusados (como no D1).

**1. O registro** (igual ao D1): a linha do `defer` fica vazia (`later`), ou vira
`struct {…} keel__c<N> = {…};` (`[now]`), com a indentação do `defer`.

**2. O cleanup do fim natural** (D1 refeito), `014`:

```c
    x = 3;
#line 12 "app/dl.k"
    app_dl_note(x);          // corpo do defer, indentação do defer, linha do defer (12)
#line 15 "app/dl.k"
}                            // o `}` na sua linha (15)
```

Uma `EDIT_LINES` no token `}` (skip 0): um `L<n>|<indent do defer><corpo>` por `defer`
pendente, em ordem inversa de registro, e `T|<indentação original da linha do }>`.

**3. Quantas saídas.** Cada `return` conta uma; o fim natural conta uma, **se o último
comando do corpo não é um `return`**. Com **duas ou mais**: **escada**. Com **uma**:
**inline**.

**4. Escada** (`017` `consume` e `two`):

```c
int app_dr_two(app_dr_Rec *r, int early) {
    int keel__rv0;                                   // linha do 1º defer: o temporário
    if (early) { keel__rv0 = -1; goto keel__e0; }    // return com chaves: era corpo de if sem chaves
                                                     // linha do 2º defer: vazia
    keel__rv0 = -2; goto keel__e1;                   // return solto: sem chaves
#line 25 "app/dr.k"
keel__e1: app_dr_mark_b(r);                          // degrau: rótulo + corpo, linha do defer
#line 23 "app/dr.k"
keel__e0: app_dr_mark_a(r);
#line 27 "app/dr.k"
    return keel__rv0; }                              // linha do `}`
```

- Temporário: `T keel__rv0;` na linha do 1º `defer` (indentação dele; `[now]` antes, a struct
  primeiro); `T` = tokens do tipo de retorno (sem `pub`/`priv`).
- `return expr;` com `defer` registrado antes → `keel__rv0 = expr; goto keel__e<K>;`
  (`K` = último registrado antes dele); por edições de token (`return` → `keel__rv0 =`,
  e logo depois do `;` final ` goto keel__e<K>;`). Se o token anterior ao `return` é `)`
  ou `else` (corpo sem chaves), ele leva chaves: `{ keel__rv0 = … goto keel__e<K>; }`.
  `return` sem nenhum `defer` antes: **intocado**.
- Degraus: `EDIT_LINES` no `}` que fecha a função: um `L<n>|keel__e<k>: <corpo>` por
  `defer`, em ordem inversa (`n` = linha do `defer`), e
  `T|<indent do defer>return keel__rv0; ` (o `}` do fonte vem depois).

**5. Inline** (uma só saída), `017` `nothing`:

```c
void app_dr_nothing(app_dr_Rec *r) {
                                  // linha do defer: vazia (função void: sem temporário)
    r->v = 9;
    {                             // 1ª metade: indentação + "{"        (linha 33)
#line 31 "app/dr.k"
    app_dr_reset(r);              // o cleanup, linha do defer
#line 33 "app/dr.k"
    return; }                     // 2ª metade: o `return;` do fonte + " }"
}
```

- `return;` → `EDIT_LINES` em `at` = o token `return`, `H|{`, os `L`, `T|<indentação da
  linha do return>`, e um `EDIT_TEXT " }"` logo depois do `;`.
- `return expr;` de função não `void` → **uma só** `EDIT_LINES` que cobre a instrução
  inteira (`skip` do `return` ao `;`): `H|{ T keel__rv<n> = <expr>;` (a `<expr>` copiada
  por `k_em_put_range`, que aplica os renomes; monte o texto em `e->buf` e copie para o
  scratch, zerando `e->n`), os `L`, e `T|<indentação>return keel__rv<n>; }`. `n` é o
  contador por função desses temporários inline, de 0.
- O fim natural também é uma saída se o último comando não é `return`: ele ganha o
  cleanup como em 2.

`k_em_handles_defer`: `true` para as ilhas `K_ISLAND_DEFER` das funções dentro do escopo;
`false` nas demais. **Função `void` com escada** (duas ou mais saídas): sem golden;
recuse e registre.

## Passos sugeridos

1. Leia o `defer.c` atual (D1) e os `expected` de 014 e 017 (c23 e c11, iguais salvo o
   cabeçalho).
2. Refaça o fim natural com `EDIT_LINES` e confira o 014 (`gen_golden.sh`).
3. A contagem de saídas; depois a escada; depois o inline.
4. Fora do oráculo, em `/tmp`: `return` com valor e uma saída (`int f(void) { defer
   c(); return x; }`), `void` com dois `return` (deve recusar), `break` (deve
   recusar). Compile o gerado com `-std=c2x` e `-std=c11`.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # -> 014 e 017 ok (c23 e c11); 005 recusado (wip); os demais como estavam
sh tools/agents/verify.sh       # -> ALL OK
```

## Relatório

`tools/agents/reports/T18-defer-d2.md`: edições, saídas coladas, dúvidas.
