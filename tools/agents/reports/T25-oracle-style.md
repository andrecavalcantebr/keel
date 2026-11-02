# T25 — relatório do autor de oráculo: `054-style-forms`

## O que foi escrito (arquivo por arquivo)

- `golden/cases/054-style-forms/app/sf.k` — módulo `app.sf` (37 linhas): `constexpr` em cadeia `A = 2`, `B = A * A`, `C = B + A`; `pub size_t c(void)` que devolve `C`; `typedef` de `Node`, `List` e `cursor`; verbos `pub inline` `begin`, `has_next`, `next`; `walk` com cursor escrito `app.sf.cursor c` (função `sum`); `foreach` em estilo Allman sobre `slice.slice i32 s` (função `allman`).
- `golden/cases/054-style-forms/proof.c` — arnês: confere `A == 2`, `B == 4`, `C == 6`, `c() == 6`; `sum` = 8 numa lista de três nós e 0 na vazia; `allman` = 15 sobre `{5,6,7}` e 0 sobre o slice vazio. Inclui `keel.type.h`, `keel/keel_slice_i32.h` e `app/app_sf.h`.
- `golden/cases/054-style-forms/expected/c23/app/app_sf.{type.h,h,c}` — escritos à mão.
- `golden/cases/054-style-forms/expected/c11/app/app_sf.{type.h,h,c}` — escritos à mão, separados do c23 (não derivados).
- `test/cgen/gen/golden-cases.txt` — acrescentada a linha `054-style-forms|app/sf.k|-|wip`, no fim do arquivo.
- `golden/line-check.allow` — acrescentada uma linha, só para o `__chk` de `A` em C11 (código injetado; ver abaixo).

Nada foi commitado nem adicionado ao índice. Não há arquivo `WIP` no caso: ele já tem `expected/` completo.

## Verificação

`sh golden/run.sh` (rodado após as edições), saída filtrada:

```
ok     054-style-forms          c23  (run)
ok     054-style-forms          c11  (run)
ok     layout-check                   (codegen E6)
ok     line-check                     (backend §6)
108 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
```

O `cgen` não foi rodado sobre o caso, nem `tools/cgen/src/` lido, nem `make` ou `verify.sh`.

## Escolhas feitas onde a spec ou o cartão não decidiam

1. **Allman no `foreach`.** A linha `foreach (...)` vira `{ keel_slice_i32 keel__c0 = s; size_t keel__n0 = ...; for (...)` sem chave, e a linha `{` seguinte vira `{ i32 x = keel_slice_i32_get(keel__c0, i);`, com `} }` no fim. Segui o cartão. O §5.7, regra 5, diz que "tudo até a abertura do corpo cabe numa linha", e com Allman o cabeçalho ocupa duas linhas do `.k`. Vale o orquestrador confirmar que a regra 5 continua valendo para o caso de chave na linha seguinte: o mapeamento aqui é 1:1 e não pede `#line`.
2. **Slice por valor na assinatura.** O `.h` inclui `keel/keel_slice_i32.type.h` na seção 1 (§4.3.2, regra 3.1), porque `slice` é passado por valor (§5.2). Não usei `import ... types`, porque nenhum tipo do `.type.h` usa o slice.
3. **`.c` inclui só `keel/keel_slice_i32.h`.** Não inclui `keel/keel_slice.h`, pelo precedente de `004-parallel` e `011-else`.
4. **Cursor qualificado pelo próprio módulo.** `app.sf.cursor` é o `qualified-name` da gramática (`IDENT { '.' IDENT }`), e sai `app_sf_cursor`. É o mesmo padrão de `019-walk`, que usa `lst.cursor` dentro de `lst`.
5. **Nomes dos parâmetros dos verbos.** Usei `cur` nos verbos (não estava no cartão), para não sombrear a função `c`. O binder do `walk` continua `c`, como o cartão pede. Em C, `app_sf_c` e `c` são nomes diferentes, então não há colisão; mas a spec não trata um binder com o nome de uma função do módulo. Se for indesejado, é uma mudança de um nome no `.k`.
6. **`__chk` de `A` em C11 na allow list.** O objeto de conferência `static const size_t app_sf_A__chk = 2; ...` não casa com o identificador `A` do `.k`, porque o sufixo `__chk` impede o casamento de `line-check.py`. É o mesmo caso de `app_cfg_MAX__chk` (`001`) e `app_cl_K__chk` (`036`), e é código injetado (README do golden, lista de "o `__chk` do `constexpr` em C11"). Os `__chk` de `B` e `C` casam pelos operandos `app_sf_A` e não precisaram de entrada.
7. **Comentários do `.k` em inglês**, como nos casos vizinhos (o código é em inglês; a prosa do NOTES é pt-BR, mas não criei NOTES, porque o cartão não o lista).

## Pontos em aberto para o orquestrador

- A escolha 1 (Allman e a regra 5 do §5.7) é a única que pode mudar a spec. Não alterei nenhum normativo.
- A escolha 5 (binder `c` com o nome da função `c`) é um ponto de estilo; se a spec proibir, o `.k` muda numa linha e o `expected` numa linha de cada perfil.
- Durante o trabalho, outros agentes acrescentaram os casos `055-defer-return-index` e `056-length-fields` e as linhas correspondentes em `golden-cases.txt` e `line-check.allow`, e alteraram `tools/cgen/src/`. Não toquei nessas partes. O `golden/run.sh` que deu 108 ok incluiu o estado desses casos no momento da execução.
