# T22 — relatório: adaptação `*x` e `foreach` sobre `range *` e sobre chamada

## Status

Parado no ponto de divergência. Os itens 1 e 3 do cartão batem com o oráculo; o item 2 diverge do oráculo só por chaves. O `|wip` do caso 049 **não foi apagado**. A verificação completa (`tools/agents/verify.sh`) estava em curso no momento do relatório, sem resultado final.

## O que mudou

- `tools/cgen/src/engine/parser/parser_islands_types.c`: na adaptação de argumentos de chamada, parâmetro por valor com argumento ponteiro, sendo nome simples e não array, recebe a marca `*<k>` (spec §4.4, regra 5).
- `tools/cgen/src/engine/emit/uses.c`: `marks_of3` aceita `*<k>` (novo parâmetro `deref`); `marks_of` removido (sem uso); `k_em_edits_uses` (chamada estrangeira e chamada do próprio módulo) emite `*` antes do argumento; `k_em_range_use` devolve a lista `star`; novo `k_em_type_island_at`.
- `tools/cgen/src/engine/emit/emit_internal.h`, `emit_core.c`: `KRangeUse` ganha `star[16]`/`nstar`; `k_em_put_range` escreve `*`.
- `tools/cgen/src/engine/emit/foreach.c`:
  - `foreach (T v, I i : f(args))` com `f` deste módulo que devolve `X *`: temporário `X *keel__c<N>` recebe a chamada uma vez. Retorno por valor, `[` ou `..` nos argumentos: recusado, como antes. Campo `cont_end` novo em `Over`.
  - `analyze_range_var` aceita `range *r`; `RangeVar.by_pointer` faz as temporárias lerem `*r`.

Não foram tocados normativos, `expected/`, `proof.c`, nem testes unit/cli.

## Saídas

- `make -C tools/cgen`: 0 warnings após remover `marks_of`.
- `sh test/cgen/gen_golden.sh`: `58 ok, 0 fail, 6 wip`. 049 aparece como `wip (differs)`.
- `golden/run.sh`: `108 ok, 0 fail, 0 xfail, 0 xpass, 3 wip`.
- `sh tools/agents/survey/run.sh`: e02 `[c23 OK] [c11 OK]`, e03 `[c23 OK] [c11 OK]`.
- `sh tools/agents/verify.sh`: ainda rodando; a etapa de build deu `ok (0 warnings)`. Resultado final pendente.

## Divergência (motivo da parada)

Gerado para `/tmp/…/scratchpad/t22/` e comparado com o oráculo. O item 1 (`keel_slice_i32_length(*sp)`, `keel_range_first(*r)`) e o item 3 (`keel_buffer_i32 *keel__c0 = app_af_pick(xs)`) saem idênticos. O item 2 difere só nas chaves do corpo simples:

- oráculo 049: `for (size_t v = keel__f0; v < keel__l0; v++) t += v; }`
- emissor: `for (size_t v = keel__f0; v < keel__l0; v++) { t += v; } }`

O emissor segue a convenção de todas as outras formas (`edits_over`, `edits_range_var`, forma literal), e o golden 015, que passa, fixa a mesma convenção para `foreach (auto v : r) t += v;` (`{ t += v; }`). O backend §5.7 regra 5 só mostra corpos com chaves, sem dizer o que vale para corpo simples. Cabe ao André decidir se o oráculo 049 muda, ou se a forma com chaves é a correta. Não editei o oráculo nem o `|wip`.

## Dúvidas abertas

- A regra "chamada que devolve ponteiro" só aceita função deste módulo, com um `*` exato e tipo com ilha de tipo no início do retorno. Chamada qualificada (`m.f(xs)`) continua recusada.
- A marca `*` também afeta `else` (que só reconhece `&1`), que passa a recusar a forma em vez de aceitar. Recusa, não miscompilação.
- Nos testes de unit/cli não encontrei asserção contrária ao cartão.
