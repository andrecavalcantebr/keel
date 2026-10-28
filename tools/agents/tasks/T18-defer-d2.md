# T18 — `defer`, degrau D2: `return` (escada e inline)

Leia primeiro `T13-16-contexto.md` e o relatório `tools/agents/reports/T17-defer-d1.md`
(o D1, já integrado: `defer.c` trata o fim natural do bloco).

## Objetivo

Estender `tools/cgen/src/engine/emit/defer.c` com as saídas por **`return`**
(backend §5.5, regra 5, e §5.5.2, regras 1 e 5: o texto foi reescrito em
2026-10-28; leia). É o caso `golden/cases/017-defer-return`. `break`, `continue`
e `goto` no corpo são o D3: continuam **recusando** o módulo.

## Arquivos

Edita só `tools/cgen/src/engine/emit/defer.c`; cria
`tools/agents/reports/T18-defer-d2.md`. Oráculos (não altere):
`golden/cases/017-defer-return/` (`app/dr.k`, `expected/{c23,c11}/app/app_dr.c`),
já na tabela, e o `014-defer-later`, que continua `ok`.

## Contrato

Escopo desta tarefa: funções cujos `defer` estão **todos no bloco do corpo da
função** (filhos diretos), com saídas só por `return` e pelo fim natural. Um
`defer` em bloco aninhado, ou `break`/`continue`/`goto`, continua recusado.

**Quantas saídas.** Conte as saídas da função: cada `return`, mais **1 para o fim
natural se o último comando do corpo não é um `return`**. Com **duas ou mais**,
e todas `return`/fim natural, o cleanup sai como **escada**; com **uma**, **inline**
(o D1 já é o inline do fim natural).

**Escada** (017 `consume` e `two`; leia o `expected` inteiro):

```keel
pub int two(Rec *r, int early) {   // 22
    defer mark_a(r);               // 23
    if (early) return -1;          // 24
    defer mark_b(r);               // 25
    return -2;                     // 26
}                                  // 27
```
```c
int app_dr_two(app_dr_Rec *r, int early) {
    int keel__rv0;                                   // linha do 1º defer: o temporário
    if (early) { keel__rv0 = -1; goto keel__e0; }    // return com chaves: era corpo de if sem chaves
                                                     // linha do 2º defer: vazia
    keel__rv0 = -2; goto keel__e1;                   // return solto: sem chaves
keel__e1: app_dr_mark_b(r); keel__e0: app_dr_mark_a(r); return keel__rv0; }   // linha do `}`
```

1. **Temporário.** `T keel__rv0;` na linha do **primeiro** `defer` da função, com a
   indentação do `defer`; `T` são os tokens do tipo de retorno escrito (`int`),
   sem `pub`/`priv`. Se esse `defer` tem `[now]`, a struct do D1 e o temporário
   ficam na mesma linha, a struct primeiro, separados por um espaço. Os outros
   `defer` ficam com a linha vazia (como no D1).
2. **Rótulos.** `keel__e<k>`, `k` = ordem de registro (0 é o primeiro `defer`).
3. **`return expr;`** com pelo menos um `defer` registrado antes (no texto): vira
   `keel__rv0 = expr; goto keel__e<K>;` onde `K` é o último `defer` registrado antes
   dele. Por edição de token: `return` → `keel__rv0 =`, e logo depois do `;` final,
   ` goto keel__e<K>;`. Se o `return` é o **corpo de um `if`/`else`/laço sem chaves**
   (o token anterior ao `return` é `)` ou `else`), ele ganha chaves: `{ keel__rv0 =
   … ; goto keel__e<K>; }`. Um `return` com **nenhum** `defer` registrado antes fica
   exatamente como está.
4. **A escada** vai na linha do `}` que fecha a função, antes do `}`, sem
   indentação própria (o `}` está na coluna 0 no fonte): para cada `defer`, em
   **ordem inversa**, `keel__e<k>: <corpo-do-cleanup> `, e por fim
   `return keel__rv0; ` e o `}`. O corpo do cleanup é o do D1 (renomes, `[now]` →
   `keel__c<N>.x`, terminado em `;`).

**Inline** (uma só saída; 017 `nothing`; e o D1):

```keel
pub void nothing(Rec *r) {   // 30
    defer reset(r);          // 31   → linha vazia (sem temporário: função void)
    r->v = 9;
    return;                  // 33   → { cleanup return; }
}
```
```c
void app_dr_nothing(app_dr_Rec *r) {
                                          // vazia
    r->v = 9;
    { app_dr_reset(r); return; }          // chaves sempre, no inline
}                                         // o fim natural é morto (o último comando é return): sem cleanup
```

5. No inline, `return;` vira `{ <cleanup> return; }`; `return expr;` de função não
   `void` vira `{ T keel__rv<n> = expr; <cleanup> return keel__rv<n>; }`, com `n`
   contador por função, de 0, dos temporários inline. O cleanup é o dos `defer`
   registrados antes, em ordem inversa, separados por espaço. Se o fim natural também
   é uma saída (o último comando não é `return`), ele recebe o cleanup como no D1.
6. **Função `void` com escada** (duas ou mais saídas): não há golden. **Recuse** o
   módulo (`handles` devolve `false`) e registre no relatório.

`k_em_handles_defer(ast, ilha)`: `true` para as ilhas `K_ISLAND_DEFER` das funções
dentro do escopo acima; `false` nas demais.

## Passos sugeridos

1. Imprima os tokens de `dr.k` e leia os `expected` de c23 e c11 (iguais salvo o
   cabeçalho).
2. A contagem de saídas e a escolha escada/inline; depois o temporário na linha do
   1º `defer`; depois o `return`; depois a escada no `}`; depois o inline.
3. `sh test/cgen/gen_golden.sh` a cada passo (linhas `014-defer-later` e
   `017-defer-return`).
4. Fora do oráculo, em `/tmp`: um `return` com valor e inline (`int f(void) {
   defer c(); return x; }`), um `void` com dois `return` (deve recusar), um
   `break` (deve recusar), um `defer` em bloco aninhado (deve recusar). Compile o
   gerado com `-std=c2x` e `-std=c11`.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # -> 014 e 017 ok (c23 e c11); os demais como estavam
sh tools/agents/verify.sh       # -> ALL OK
```

## Relatório

`tools/agents/reports/T18-defer-d2.md`: edições, saídas coladas, dúvidas, e o que
recusou.
