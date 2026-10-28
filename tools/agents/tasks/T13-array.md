# T13 — `array`: declaração de vetor local e de parâmetro

Leia primeiro `T13-16-contexto.md`.

## Objetivo

Preencher `tools/cgen/src/engine/emit/array.c`: o lowering do marcador `array`
(backend §5.1, regras 2 e 3): a palavra `array` some, as dimensões separadas por
vírgula viram colchetes em sequência, e a dimensão 0 de um **parâmetro** ganha
`static`.

## Arquivos

Edita só `tools/cgen/src/engine/emit/array.c`; cria
`tools/agents/reports/T13-array.md`. Oráculo (não altere):
`test/cgen/gen/array/` (`arr.k` e `expected/{c23,c11}/`), já na tabela de
`test/cgen/gen_golden.sh`.

## Contrato

```keel
pub size_t bytes(void) {            // local
    array i32 v[4];                 →   i32 v[4];
    array f32 m[2,3];               →   f32 m[2][3];
pub size_t count(array i32 v[2,3]) →   size_t arr_count(i32 v[static 2][3])
```

- A ilha do parser: `array` com âncora na **palavra `array`**, detalhe `v [4]`,
  `m [2,3]`. Para o parâmetro, a âncora é o `array` dentro da assinatura (índice
  de token menor que `fn->body_first`). A mesma edição vale para o protótipo do
  `.h` (o emissor chama o gancho para a assinatura também).
- Edição 1: apague o token `array` **e os brancos que o seguem** (`EDIT_DROP`,
  com `skip` cobrindo token e brancos).
- Edição 2: o colchete das dimensões é o primeiro `[` depois da âncora (um
  especificador de tipo não tem `[`). Cada `,` no nível externo desse colchete,
  com os brancos que o seguem, vira `][` (`EDIT_TEXT`, `skip` cobrindo a vírgula
  e os brancos).
- Edição 3 (só parâmetro): logo depois do `[` de abertura, insira `static `
  (`EDIT_TEXT`, `skip` 0).
- `k_em_handles_array(ast, ilha)`: `true` para ilhas `K_ISLAND_ARRAY` cujo
  declarador é o simples acima. **`false`** (continua recusado) quando: a
  declaração tem mais de um declarador (`array i32 a[3], b[4];`), ou o
  parâmetro usa binder de dimensão (`array T v[size_t n]`, backend §5.18: duas
  palavras seguidas dentro do colchete). Meça com `--stop-after=parse` o que o
  parser dá nesses casos antes de decidir como detectá-los.

## Passos sugeridos

1. Rode o parse do oráculo e confira as ilhas e as posições dos tokens.
2. Edição 1, depois 2, depois 3; compile e rode `sh test/cgen/gen_golden.sh`
   a cada uma.
3. `handles` e os casos recusados (escreva-os num `.k` temporário em `/tmp`,
   confira o `implementation-limit`).

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # -> test/cgen/gen/array ok (c23 e c11); os demais como estavam
sh tools/agents/verify.sh       # -> ALL OK
```

## Relatório

`tools/agents/reports/T13-array.md`: o que mudou, as saídas coladas, as
dúvidas (em especial: como detectou o binder e o declarador múltiplo).
