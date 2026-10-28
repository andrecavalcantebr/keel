# T16 — índice verificado sobre `array`

Leia primeiro `T13-16-contexto.md`. **Depende do T13** (o oráculo declara
`array`): só começa com o `array.c` integrado.

## Objetivo

Preencher `tools/cgen/src/engine/emit/index.c`: `v[i]`, `m[i,j]` e `m[1][j]` sobre
um `array` (backend §5.3, regras 3, 4 e 6). Cada índice passa por
`keel_index(i, d)` contra a dimensão declarada correspondente; índice **e**
dimensão literais decimais saem como estão; vírgulas viram `][`.

## Arquivos

Edita só `tools/cgen/src/engine/emit/index.c`; cria
`tools/agents/reports/T16-index.md`. Oráculo (não altere):
`test/cgen/gen/index/` (`idx.k`, `expected/{c23,c11}/`), já na tabela.

## Contrato

```keel
    array i32 v[4];
    array i32 m[2,3];
    v[i] = 7;
    m[i,j] = v[i];
    return m[1][j] + v[2];
```
```c
    i32 v[4];
    i32 m[2][3];
    v[keel_index(i, 4)] = 7;
    m[keel_index(i, 2)][keel_index(j, 3)] = v[keel_index(i, 4)];
    return m[1][keel_index(j, 3)] + v[2];
```

- A ilha: `K_ISLAND_ARRAY_INDEX`, âncora no **nome** (`v`, `m`), detalhe `m rank 2`
  (o rank conta os índices escritos, num colchete ou em vários: `m[i,j]` e
  `m[i][j]` são `rank 2`). A declaração do `array` é a ilha `K_ISLAND_ARRAY`
  anterior de mesmo nome (a mais próxima antes do uso, na mesma função, local
  ou parâmetro); as dimensões são os tokens entre `[` e `]` dela, separados por
  vírgula de nível externo.
- Para cada índice escrito (ordem de leitura = ordem das dimensões):
  - se o índice é **um só token decimal** e a dimensão também, nada muda;
  - senão, insira `keel_index(` no começo da expressão do índice (`EDIT_TEXT`,
    `skip` 0), e no fim dela `, ` + a dimensão + `)`: três edições de inserção
    na mesma posição, nesta ordem — `EDIT_TEXT ", "`, `EDIT_TOKS` com os tokens
    da dimensão (`tok_a`/`tok_b`; copiados com os renomes, de modo que uma
    dimensão `N` de `constexpr` de bloco saia `keel__N_0` sob C11 depois do T14),
    `EDIT_TEXT ")"`.
- Cada `,` de nível externo entre dois índices de um mesmo colchete, com os
  brancos que o seguem, vira `][` (`EDIT_TEXT`, `skip` cobrindo a vírgula e os
  brancos).
- Cada índice é avaliado **uma vez** (a chamada de `keel_index` recebe a
  expressão como escrita); `v[i++]` não é reescrito de outra forma.
- `k_em_handles_index(ast, ilha)`: `true` quando a declaração do `array` foi
  achada e o número de índices escritos não passa do de dimensões; senão
  `false`.
- As edições devem cair em `[a, b)` (o gancho roda uma vez por linha); o índice
  de uma subexpressão que atravesse linhas é raro, mas não pode produzir edição
  duplicada.

## Passos sugeridos

1. Imprima ilhas e tokens do `idx.k`.
2. Achar a declaração e as dimensões; índice simples; depois `m[i,j]`; depois o
   encadeado `m[1][j]` e o literal `v[2]`.
3. `sh test/cgen/gen_golden.sh` a cada passo; compile o gerado nos dois perfis.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # -> test/cgen/gen/index ok (c23 e c11); os demais como estavam
sh tools/agents/verify.sh       # -> ALL OK
```

## Relatório

`tools/agents/reports/T16-index.md`: edições, saídas coladas, dúvidas.
