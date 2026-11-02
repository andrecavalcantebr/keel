# T27 — relatório do autor de oráculo: `keel.length`/`keel.dim` sobre campo e sobre dimensão `constexpr`; range-index de campo

## O que foi feito

Caso `golden/cases/056-length-fields/`, escrito à mão a partir da spec
(`keel-c-backend.md` §5.1 r3, §5.3 r7, §5.18 r2, §9.2) e dos goldens análogos
(`044-array-range`, `046-array-leftovers`, `006-dim`, `041-parallel-forms`,
`020-extent`). Não rodei `tools/cgen/cgen` sobre o caso, não li `tools/cgen/src/`,
não rodei `make` nem `verify.sh`.

Arquivos novos (nada mais foi tocado, e nada foi commitado):

- `golden/cases/056-length-fields/app/lf.k` — módulo `app.lf`, com as quatro
  formas do cartão: `Box` com campo `array i32 v[5]`; `pub constexpr size_t K = 3;`
  e `priv array i32 v[K];`; `field_ptr(Box *b)` e `field_val(Box b)` com
  `keel.length` sobre o campo; `mod_len` com `keel.length(v)`; `d(array i32 v[2, 3])`
  com `keel.dim(v, 1)`; `field_slice(Box *b)` com `slice.slice i32 s = b->v[1..4]`.
- `golden/cases/056-length-fields/proof.c` — confere os cinco valores: 5, 5, 3, 3, 3.
- `golden/cases/056-length-fields/expected/c23/app/app_lf.{type.h,h,c}`
- `golden/cases/056-length-fields/expected/c11/app/app_lf.{type.h,h,c}`
- `test/cgen/gen/golden-cases.txt`: linha `056-length-fields|app/lf.k|-|wip`, no fim do arquivo.
- `golden/line-check.allow`: uma entrada, o `__chk` do `constexpr` de módulo em
  `expected/c11/app/app_lf.type.h` (é código injetado sem linha própria no `.k`,
  como as outras entradas do mesmo tipo).
- Este relatório.

Não criei arquivo `WIP` no caso: o `expected/` existe. O `|wip` na linha de
`golden-cases.txt` é o marcador que diz ao cgen que o caso ainda não bate com o
emissor, e não pertence ao golden em si.

## Formas do cartão, e o que o esperado afirma

1. `keel.length(b->v)` e `keel.length(b.v)` sobre campo `array`: `sizeof(b->v)/sizeof(i32)`
   e `sizeof(b.v)/sizeof(i32)` (§5.3 r7: o tipo do elemento vem da declaração do campo).
2. `keel.length(v)` sobre `priv array i32 v[K]` com `K` `constexpr`:
   `sizeof(app_lf_v)/sizeof(i32)`. A dimensão da declaração sai com o nome mangled
   `app_lf_K` (como em `020-extent`: `u8 r[app_pos_H][app_pos_W]`).
3. `keel.dim(v, 1)` sobre parâmetro sem binder: `return 3;`, e o protótipo
   `size_t app_lf_d(i32 v[static 2][3]);` (§5.1 r3, §5.3 r7).
4. `b->v[1..4]` sobre campo: `keel_array_i32_as_slice2(b->v, 5, 1, 4)`, com a
   dimensão 5 vinda do campo (§5.18 r2; §5.3 r4). O intervalo é meio aberto, como
   em `044-array-range` (`v[1..4]` tem 3 elementos).

## Escolhas onde a spec não diz tudo

- **Constante de módulo na dimensão, sob C23.** `static i32 app_lf_v[app_lf_K];`
  com `constexpr size_t app_lf_K = 3;` no `.type.h`. Pela spec, a dimensão sai com
  o nome da constante. Que o C23 aceite um `constexpr` como tamanho de vetor de
  escopo de arquivo depende de ser uma *named constant* numa expressão constante
  inteira; o GCC aceitou (`-std=c2x`, `run.sh` verde). Ponto para conferir com
  clang, se a suíte passar a rodar com ele. A spec, pelo que li, não diz isso em
  lugar nenhum.
- **Sob C11**, o `app_lf_K` vira `#define app_lf_K ((size_t)3)` e o `.c` continua
  escrevendo `app_lf_K`: o preprocessador expande. Segue §9.2 r1 e r2, com o
  `#line` antes do `__chk` como no `041-parallel-forms`.
- **Nome compartilhado.** O campo `v` de `Box`, o `priv array v` do módulo e o
  parâmetro `v` de `d` têm o mesmo nome, como pede o cartão. Em C não há conflito
  (campo é de struct; o parâmetro sombreia o global), e `run.sh` aceitou. A spec
  não diz se o parâmetro pode sombrear um símbolo de módulo. Se isso for proibido
  pela linguagem, o caso precisa de outro nome, e o esperado muda só nas linhas
  de `d`.
- **Layout de linhas.** Os `#line` do `.h` e do `.type.h` seguem a regra de §6 e
  os exemplos de `046-array-leftovers` e `041-parallel-forms`. Não há injeção no
  `.c` deste caso, só o `__chk` do C11.

## Verificação

`sh golden/run.sh` (só este comando; sem `make`, sem `verify.sh`), saída relevante:

```
ok     056-length-fields        c23  (run)
ok     056-length-fields        c11  (run)
ok     layout-check                   (codegen E6)
ok     line-check                     (backend §6)

104 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
exit=0
```

Os três `wip` são os casos com arquivo `WIP` já existentes (022, 023 e 024), não este.

## Pontos em aberto

- Confirmar com clang a dimensão `constexpr` no C23 (ver acima).
- Confirmar se a linguagem aceita parâmetro com o mesmo nome de um símbolo de módulo.
- Não rodei o cgen sobre o caso, de propósito: a comparação com a implementação
  fica para quem constrói o cgen.
