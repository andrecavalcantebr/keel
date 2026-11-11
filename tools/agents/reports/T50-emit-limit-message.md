# T50 — relatório

**Nota do orquestrador.** O agente Haiku fez a maior parte da implementação
(itens 1–3 do contrato: os códigos `K_EMIT_LIMIT_*`, `k_em_limit`,
`k_emit_limit_text`, os sítios de `engine/emit/` e os 4 pontos de
`stop_gen.c`), mas foi interrompido antes de rodar a verificação final e
escrever este relatório — a interrupção fazia parte de uma pergunta do
usuário que acabou cancelando a chamada em curso do agente, não um problema
do próprio agente. O orquestrador assumiu dali: achou e corrigiu um defeito
(abaixo), rodou a verificação e escreve o relatório.

## O que o Haiku implementou

1. `emit.h`: `K_EMIT_LIMIT_LINE/EDITS/SCRATCH/NAME`, mais dois que o cartão não
   previa e que o Haiku identificou sozinho ao auditar os `ok = false`:
   `K_EMIT_LIMIT_INCLUDES` (`MAX_INCLUDES`, em `uses.c`) e
   `K_EMIT_LIMIT_HEADER` (o cabeçalho de um bloco `parallel`, hoje com `1000`
   literal em `parallel.c`, nomeado `PAR_HEADER_CAP` em `emit_internal.h`).
   `K_EMIT_IS_FAILURE` cobre os 7 códigos (`UNSUPPORTED` e os 6 limites).
2. `Emit.limit` (novo campo); `k_em_limit(e, code)` grava o primeiro código,
   como o cartão pedia (`if (!e->limit) e->limit = code`).
3. Cada site de capacidade (`k_em_put_bytes`, `k_em_put_mangled`,
   `k_em_push_edit`, `k_em_scratch`, `decls.c:120/223`, `foreach.c`,
   `instance.c` (4 sites), `uses.c` (7 sites), `parallel.c` (3 sites),
   `files.c:fit`) passou a chamar `k_em_limit` em vez de só `e->ok = false`.
   Os sites que **não** são de capacidade (construção não coberta:
   `k_em_placement_c`'s `k_emit_instance_of` falhando,
   `k_em_use_includes`'s `k_em_plain_arg` falhando) ficaram como
   `e->ok = false` puro — classificação correta, confirmada pelo orquestrador
   lendo os dois sites de novo.
4. `k_emit_file` e `k_emit_instance`: `return e.limit ? e.limit : K_EMIT_UNSUPPORTED`.
5. `stop_gen.c`: `report_refused(need, len, what, cover)` centraliza a
   mensagem; os 4 pontos (`emit_one`, `emit_protocol_functions`,
   `emit_instances`, `cgen_stop_after_instance`) chamam
   `K_EMIT_IS_FAILURE(need)` em vez de `== K_EMIT_UNSUPPORTED`.

## Defeito achado e corrigido pelo orquestrador

Os 3 textos de `cover` que `stop_gen.c` passava já terminavam em "yet"
("this module's constructions **yet**", "this function over a protocol
**yet**", "this instance **yet**"), e `report_refused` também apêndice
" yet" no formato de `K_EMIT_UNSUPPORTED`. O resultado, não notado porque o
agente foi interrompido antes de rodar nada, seria uma mensagem duplicada
("... yet yet [implementation-limit]"). Corrigido tirando o "yet" das 3
strings — `report_refused` é a única que o escreve agora.

## Oráculo

`test/cgen/cli/emit_limit.sh`, escrito pelo orquestrador antes de lançar a
tarefa (reprodutor: função de 60 parâmetros, ~1400 bytes de protótipo, que
hoje passa de `LINE_CAP = 1024`).

## Verificação final (orquestrador)

```
$ make -C tools/cgen -j8         # limpo, 0 avisos novos
$ sh test/cgen/cli/emit_limit.sh
ok
$ sh test/run.sh                 # isolado, suíte inteira
...
140 ok, 0 fail, 0 xfail, 0 xpass, 0 wip
docs: 45 ok, 0 fail
exit=0
```

## Dúvidas abertas

Nenhuma do item 4 do cartão (um segundo limite provocado à mão): o
orquestrador não tentou provocar `MAX_EDITS`/`SCRATCH_CAP` manualmente —
`emit_limit.sh` cobre só `LINE_CAP`. Os outros 5 códigos têm o texto
(`k_emit_limit_text`) e o sítio, mas não têm reprodutor nos oráculos. Fica
registrado para quem precisar de um caso real.
