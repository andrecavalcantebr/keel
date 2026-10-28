# T13 — relatório: `array` (declaração local e de parâmetro)

## O que mudou

Arquivo por arquivo:

- `tools/cgen/src/engine/emit/array.c` — preenchido. Único arquivo alterado
  (`git status --short`: `M tools/cgen/src/engine/emit/array.c`). Nada commitado.

Conteúdo do arquivo, em resumo:

- `array_decl(ast, is, &d)`: reconhece uma ilha `K_ISLAND_ARRAY` cujo token
  âncora é a palavra `array`, acha a função envolvente (`enclosing_fn`, por
  intervalo de tokens de um `K_AST_FUNCTION`), acha o primeiro `[` depois da
  palavra (recusa se antes aparecer `;`, `,`, `)`, `{` ou `}`), acha o `]`
  correspondente por contagem de `([{` / `)]}`, e recusa:
  - dimensão vazia;
  - **binder**: duas palavras seguidas dentro dos colchetes (`size_t n`);
  - **declarador múltiplo em local**: `]` seguido de `,` quando
    `anchor >= fn->body_first`. Em parâmetro esse `,` é só o separador da
    lista de parâmetros, então não conta.
- `k_em_handles_array` devolve `array_decl(...)`.
- `k_em_edits_array`: para cada ilha aceita cujo âncora cai em `[fn->first, fn->end)`:
  1. `EDIT_DROP` em `array` com `skip` = tamanho da palavra + brancos (espaço/tab)
     seguintes;
  2. `EDIT_TEXT` `][` em cada vírgula do nível externo dos colchetes, com
     `skip` = vírgula + brancos seguintes;
  3. (só parâmetro) `EDIT_TEXT` `static ` em `skip` 0, logo após o `[`.
  Toda edição só é empilhada se o `at` cai em `[a, b)`.

Nenhum outro arquivo foi tocado: `emit.c`, `emit_core.c`, `emit_internal.h`,
golden, oráculos e normativos ficaram como estavam.

## Medições feitas antes de decidir a detecção

`cgen --stop-after=parse` nos casos (saídas reais):

```
ilha	array	a [3]	multi_a.k:4:5          # array i32 a[3], b[4];
ilha	array	b [4]	multi_a.k:4:5          #   -> duas ilhas com a MESMA âncora
ilha	array	v [size_t n]	multi_b.k:3:14 # binder
ilha	array	v [2, 3]	multi_d.k:3:14 # param com espaço duplo "2,  3"
ilha	array	loc [2, 3]	multi_d.k:4:5  # local em duas linhas "[2,\n 3]"
```

E um caso que o próprio parser recusa:

```
multi_d.k:3:51: error: the one-dimensional `array` parameter `w` has no binder: write `array T name[size_t n]` ... [array-1d-as-parameter]
```

Ou seja: o declarador múltiplo aparece como duas ilhas com a mesma âncora
(a detecção pelo token `,` depois do `]` é a que de fato recusa, e vale para
as duas); o binder aparece no detalhe como `size_t n` (duas palavras). O
parâmetro 1D sem binder é recusado pelo parser antes de chegar ao emissor.

## Saídas coladas

`sh test/cgen/gen_golden.sh` (final, depois da última alteração):

```
wip    003-alignment
wip    006-dim
wip    007-instance
wip    008-block-constexpr
wip    test/cgen/gen/foreach
wip    test/cgen/gen/index
wip    test/cgen/gen/constblock
3 ok, 0 fail, 7 wip
```

`test/cgen/gen/array` (`arr.k`) entra entre os 3 `ok` (tabela
`test/cgen/gen/golden-cases.txt`). Conferido à mão também, por `cgen -std=c23`
e `-std=c11` com `--stop-after=gen` em `/tmp`: `diff -r` contra
`test/cgen/gen/array/expected/{c23,c11}` dá só `keel.h` e `keel.type.h`
extras (o oráculo não os julga, e `arr.c`, `arr.h`, `arr.type.h` são idênticos).

Casos em `/tmp/t13` (não são oráculo, só conferência):

- `multi_d.k` (param `array i32 v[2,  3]`, `i32 n`, e local `array i32 loc[2,\n 3];`),
  saída C23:
  ```c
  size_t multi_d_d(i32 v[static 2][3], i32 n) {
      i32 loc[2][
          3];
  ```
- `multi_a.k` (`array i32 a[3], b[4];`) e `multi_b.k` (`array i32 v[size_t n]`):
  `cgen: error: ... the emitter does not cover this module's constructions yet [implementation-limit]`,
  saída 2. Ou seja, continuam recusados.

`sh tools/agents/verify.sh` (saída inteira):

```
ok   build (2 warnings, baseline 4)
ok   make check (48 lines ok)
ok   golden: 46 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

Nenhum dos avisos de build vem de `array.c` (conferido com `touch` e
recompilação: zero linhas `array.c.*warning`).

## Dúvidas e pontos abertos

1. **Quebra de linha dentro da dimensão.** Os brancos a consumir param na
   quebra de linha: `[2,` + quebra + `3]` vira `[2][` + quebra + `3]`, que é C
   válido e aparece no caso `multi_d` acima. Não é a forma "mais mínima" possível,
   mas não atravessa a fronteira da linha, que é o limite do `copy_edited`.
2. **`static ` quando o `[` é o último byte da linha.** A inserção fica em
   `at == fim da linha`, então cai em `[a, b)` da linha seguinte e seria escrita
   antes dos brancos dessa linha. Não testei (nenhum caso do oráculo tem `[`
   no fim da linha). É uma linha como `i32 v[` + quebra + `2,3]`; conviria um
   caso no oráculo se isso for relevante.
3. **Forma `array i32 q[2][3]`.** Não medi o que o parser dá para
   `[..][..]` depois da palavra `array`; o `array_decl` pega só o primeiro par
   de colchetes, então se o parser aceitar essa forma a saída seria
   `i32 q[2][3]`, que está certo por acidente. Não é o que o backend §5.1
   descreve, e não há caso de oráculo para ela.
4. **Declaração de módulo.** `array` fora de função: `enclosing_fn` devolve NULL,
   então `handles` é `false` e o módulo é recusado. Não medi o parse de um
   `array` no nível do módulo.
5. **Ordem das edições.** `k_em_edits_array` empilha as edições na ordem das
   ilhas; `node_edits` reordena por posição (`k_em_sort_edits`), e nenhuma
   posição coincide com outra (`array`, vírgulas e `[`+1 são posições distintas),
   então a ordem relativa não importa para este arquivo.
6. **Sem alocação, sem libc.** `array.c` só usa `emit_internal.h`; nenhuma
   inclusão nova. `make boundary` passou (faz parte do `make`).
