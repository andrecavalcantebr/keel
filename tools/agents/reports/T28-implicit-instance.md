# T28 — instância implícita de `keel.array` no range-index

## O que mudou

- `tools/cgen/src/engine/parser/parser_islands_index.c` (função `index_group`, ramo
  `KT_ARRAY` com `range != SIZE_MAX`): depois da checagem de `as_slice` no
  protocolo `keel.array`, o range-index agora registra a instância `keel.array <elem>`
  no fecho, pelo mesmo mecanismo que um verbo de `keel.array` usa
  (`isl_kt_new` + `isl_record_seed(..., K_CLOSURE_INSTANCE, am)` em
  `parser_islands_types.c`). O símbolo é `<prefixo do módulo>_<elem>` (p.ex.
  `keel_array_i32`), o mesmo nome que o texto da ilha já usa em `_as_slice`. Com isso
  `stop_gen` escreve `keel/keel_array_<T>.h` e `.type.h`. Nenhuma mudança em
  `stop_gen.c`, `instances.c` ou `emit/uses.c`.
- `test/cgen/gen/golden-cases.txt`: removido `|wip` das linhas `044-array-range` e
  `046-array-leftovers` (só isso).
- Nenhum teste, normativo, `expected/` ou `proof.c` foi tocado. Nada foi commitado.

## Saídas dos comandos de aceitação

`sh test/cgen/gen_golden.sh`:
```
60 ok, 0 fail, 3 wip
```
(044 e 046 em `ok`; os 3 wip restantes são 054, 055, 056, como antes.)

`golden/run.sh` (última linha):
```
106 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
```

`sh tools/agents/verify.sh`:
```
ok   build (0 warnings, baseline 0)
ok   make check (53 lines ok)
ok   golden: 106 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

`CGEN=$PWD/tools/cgen/cgen sh tools/agents/survey/run3.sh | grep a09`:
```
a09 [c23 OK] [c11 OK]
```

Verificação extra: `sh test/cgen/run.sh` (suíte completa do cgen: unit, cli, lex,
parse, diag, gen) terminou com rc=0, sem nenhuma linha FAIL. Os testes de
`test/cgen/cli/islands.py` que fixam o texto das ilhas `range-index`
(`fx 2..5 → keel_array_f32_as_slice2 dim:1`, etc.) continuam passando, porque o
texto da ilha não mudou, só o fecho.

## Dúvidas e pontos abertos

- Nenhuma afirmação de teste contradiz o cartão, então não houve patch guardado.
- O cartão pede que os headers sejam iguais a `golden/c23/keel/keel_array_i32.{h,type.h}`
  e `golden/c11/…`. Isso vale para 044 e 046 (que comparam a árvore inteira em
  `expected/`), e o gen_golden passa, então a igualdade foi conferida pelo `cmp`.
- Escopo: o registro usa só `k->elem` como argumento (`keel.array <T>`). Para elementos
  que são modificadores ou outros tipos, o símbolo sai de `isl_kt_symbol(elem)`, igual
  ao texto da ilha; não foi testado um caso com elemento não primitivo.
