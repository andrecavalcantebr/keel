# T15 — `foreach` sobre intervalo literal

Leia primeiro `T13-16-contexto.md`.

## Objetivo

Preencher `tools/cgen/src/engine/emit/foreach.c`: `foreach (binder : A..B)` com
**um binder** e **literal de intervalo** (backend §5.7; §5.3 regra 9: “em
`foreach`, o literal não constrói `range`, e os limites entram direto no
laço”). Os outros `foreach` (sobre contêiner, `range` em variável, dois
binders) **continuam recusados**.

## Arquivos

Edita só `tools/cgen/src/engine/emit/foreach.c`; cria
`tools/agents/reports/T15-foreach.md`. Oráculo (não altere):
`test/cgen/gen/foreach/` (`loop.k`, `expected/{c23,c11}/`), já na tabela.

## Contrato

```keel
    foreach (auto i : 0..n) t += i;
    foreach (auto j : 2..n) { t += j; }
    foreach (u32 k : 1..4) { t += k; }
```
```c
    { for (size_t i = 0; i < n; i++) { t += i; } }
    { for (size_t j = 2; j < n; j++) { t += j; } }
    { for (u32 k = 1; k < 4; k++) { t += k; } }
```

Por edição de token (o texto do autor passa; troque só estas palavras):

| Token(s) | Passa a ser |
| --- | --- |
| `foreach` | `{ for` (`EDIT_TEXT`) |
| `auto` (tipo do binder) | `size_t`; um tipo escrito (`u32`) fica |
| `:` | `=` |
| `..` | `; <binder> < ` (o texto com o nome do binder, em `k_em_scratch`) |
| `)` que fecha o cabeçalho | `; <binder>++)` |
| corpo com `{ … }` | o `}` que o fecha ganha ` }` depois |
| corpo sem chaves (`t += i;`) | `{ ` antes do primeiro token e ` } }` depois do `;` |

O corpo sem chaves vale só para **uma instrução simples**: tokens até o primeiro
`;` de nível zero, sem `{` nem as palavras `if`, `for`, `while`, `foreach`,
`switch`, `do`, `else` no caminho. Se não for, `k_em_handles_foreach` devolve
`false` e o módulo segue recusado.

- A ilha: espécie `K_ISLAND_FOREACH`, âncora na palavra `foreach`, detalhe
  `i : 0..n` (literal de intervalo: **sem** o tipo do contêiner entre
  parênteses; sobre contêiner o detalhe termina em `(<tipo>)`). `handles` olha o
  detalhe **e** os tokens: um binder, `A..B`, corpo aceitável.
- **Meça** como o lexer corta `0..n` (um `0.` de ponto flutuante seria um
  desastre) e `1..4`; a ilha existe, então o parser já resolve isso — ache os
  tokens `A`, `..`, `B` pelo que ele entrega.
- O nome do binder e o limite `B` são texto do autor: se `B` usar um nome do
  módulo ou de um `constexpr` de bloco, os renomes de outras edições já valem,
  porque você não reescreve `A` nem `B`.

## Passos sugeridos

1. Imprima os tokens de `loop.k` (índices, offsets) e as ilhas.
2. Cabeçalho (`foreach`, `auto`, `:`, `..`, `)`), depois o corpo com chaves,
   depois o sem chaves; `sh test/cgen/gen_golden.sh` a cada passo.
3. `handles` e as formas recusadas (um `.k` em `/tmp`).
4. Compile o gerado nos dois perfis.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # -> test/cgen/gen/foreach ok (c23 e c11); os demais como estavam
sh tools/agents/verify.sh       # -> ALL OK
```

## Relatório

`tools/agents/reports/T15-foreach.md`: edições feitas, saídas coladas, dúvidas.
