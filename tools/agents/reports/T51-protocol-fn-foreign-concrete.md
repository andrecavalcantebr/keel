# T51 — relatório: função sobre protocolo com tipo concreto de outro módulo

## Estado

Parcial. Os dois defeitos do cartão estão corrigidos no cgen, e o caso 071
passou a emitir quase tudo como o oráculo. **O 071 ainda não bate byte a byte**:
restam duas divergências entre o `expected/` (escrito à mão no T50) e a
implementação, ambas em pontos que o cartão manda decidir pela spec, e não
editando o oráculo. Por isso `|wip` continua na linha do 071 em
`test/cgen/gen/golden-cases.txt`. Nada foi commitado.

## O que mudou, arquivo a arquivo

- `tools/cgen/src/engine/parser/parser_islands_types.c`
  - **Defeito 1 (header da instância não sai).** No ramo em que o tipo concreto
    é instância de modificador de outro módulo, a semente da função sobre
    protocolo usava `ast_module(c, proto_home)`, nulo quando a função é do
    próprio arquivo. Agora usa o módulo resolvido como no ramo vizinho
    (`k_symbol_resolve(...)->origin`), e a semente leva também o módulo e o
    símbolo do tipo concreto (`isl_record_protocol_seed_plain`).
  - **Defeito 2 (corpo não reescreve `length(items)`).** Descoberta: no 065 o
    corpo funciona por acidente, porque `limit` é verbo do próprio módulo
    `pf` e a passagem comum resolve. No 071, `length` está em `app.ibox`, e
    `isl_proto_param` só tratava a forma com alias (`Proto C x`). Agora:
    - `isl_proto_param` aceita a forma sem alias (`IndexPtr items`) e marca o
      local com `proto_param`; o alias continua necessário só para `C.nome`.
    - `kt_of_local` dá ao local de protocolo o tipo concreto, em modo protocolo.
    - `call_core` redireciona um verbo chamado sobre o parâmetro de protocolo
      para o módulo do tipo concreto (como uma chamada qualificada), e o
      caminho genérico de receptor (`recv`/`direct`) resolve a instância e o
      `&items`. Base: spec §4.14 regra 5 (o corpo chama verbos do protocolo) e
      o exemplo da própria §4.14, `count(IndexPtr items) { return length(items); }`.
- `tools/cgen/src/engine/parser/parser_islands.c`
  - `k_collect_instance_islands`: uma função sobre protocolo em módulo **não
    genérico** também é lida. Nesse caso o módulo é emitido a partir das suas
    próprias islands e closure, então elas não são zeradas, e a passagem
    acrescenta. A restauração de tamanhos já é feita por
    `emit_protocol_functions` (`stop_gen.c`), que **não** foi editado.
  - `isl_emit`: com `keep_islands`, anexa em vez de inserir ordenado. A
    inserção ordenada deslocava as islands do arquivo e a restauração do
    tamanho descartava a última: o `count(b)` do `probe_count` saía sem
    reescrita. Esse foi um regressão que apareceu no meio do trabalho e está
    corrigida.
- `tools/cgen/src/engine/parser/parser_islands_internal.h`: campo
  `Local.proto_param` e `Ctx.keep_islands`.

Não foi tocado: normativos, `golden/`, outros casos, `stop_gen.c`, emitter
(`emit/*`). Nenhum `emit/` precisou mudar.

## Divergências do 071 (oráculo × implementação) — para decisão

1. **`[[nodiscard]]` em `app_ibox_box_i32_ptr` (c23, `app_ibox_box_i32.h`,
   linhas 9 e 13).** O cgen emite (`emit/roles.c`, `k_em_nodiscard`: verbo com
   retorno `child T *`, C23 apenas, §9.1). O oráculo (NOTES do T50) omite,
   por "nenhum verbo desta forma pode falhar". Mas o golden 063 tem exatamente
   essa forma (`pub inline child T *ptr(parent vec *v, size_t i)`) e o
   `fv_vec_i32_ptr` sai com `[[nodiscard]]`. O oráculo do 071 contradiz o 063.
   Em c11 não há diferença (`[[nodiscard]]` é só C23).
2. **`#include "app/app_ibox_box_i32.h"` no `app_ibox_use.c`.** O cgen inclui;
   o oráculo não inclui (é a dúvida 2 do T50). O `.c` declara `app_ibox_box_i32 b`
   por valor, mas não chama verbo do tipo. §4.3.2 regra 4 ("o `.h` de cada
   módulo ou instância que **usa**") lê-se como uso de tipo; regra 7 ("os
   `#include` são derivados das chamadas já resolvidas") lê-se como uso por
   chamada. O cgen segue a primeira; o oráculo, a segunda. Isto é normativo
   (§4.3.2), não me cabe decidir.

Todo o restante do 071 bate: o header da instância `app_ibox_use_count_app_ibox_box_i32.h`
(e seu `.type.h`), o corpo `return app_ibox_box_i32_length(&items);`, o
`app_ibox_box_i32.h`, os `.type.h` e o `app_ibox_use.h`/`.type.h`/`.c`
(exceto o include acima). Diferença só de `keel.h`/`keel.type.h`, que o
oráculo não cobre (`Somente em`, esperado).

## Caso extra (sem oráculo): `count(IndexPtr items)` sobre `keel.buffer.buffer i32`

Fonte em `/tmp/t51-extra/src/app/extra.k` (fora do repo). `count(b)` com
`buffer.of(v)` de um `array i32`. Gerado com `cgen --profile=c23`:
- a instância é `app_extra_count_keel_buffer_i32(keel_buffer_i32 *items)`
  (o buffer é `byref`), com `keel_buffer_i32_length(items)` no corpo, e a
  chamada `app_extra_count_keel_buffer_i32(&b)`;
- compila sem aviso com `gcc -std=c2x -Wall -Wextra -pedantic-errors` (o gcc
  daqui não aceita `-std=c23`), e roda: `run=3`, rc 0.

## Saída dos comandos de aceitação

- `make -C tools/cgen -j8`: sem warning nem error (baseline 0).
- `sh tools/agents/verify.sh`: `ok build (0 warnings, baseline 0)`,
  `ok make check (59 lines ok)`, `ok golden: 142 ok, 0 fail, 0 xfail, 0 xpass, 0 wip`,
  `ALL OK`, `EXIT=0`.
- `test/run.sh > /tmp/t51.out`: `EXIT=0`. `gen_golden`: `80 ok, 0 fail, 1 wip`
  (o 071, ainda `|wip`). `base`: `144 ok, 0 fail, 0 wip`. Golden:
  `142 ok, 0 fail, 0 xfail, 0 xpass, 0 wip`. Docs: `45 ok, 0 fail`. Os `Aborted`
  no meio do log são dos casos de debug (026, 033), que esperam abortar.
- 071 byte a byte (c23 e c11, `--stop-after=gen`): **não bate**, pelas duas
  divergências acima. Em c11, só a divergência 2.

## Pontos abertos

- Decidir as divergências 1 e 2 (uma delas pode exigir mudar o oráculo do
  071 ou o emitter); enquanto isso o `|wip` do 071 fica.
- Não rodei `verify.sh compare` contra o snapshot `/tmp/keel-snap-T51` (tirado
  antes da mudança). A mudança de comportamento é intencional e restrita a
  funções sobre protocolo de módulos não genéricos; o `make check` e o golden
  (que cobrem parse e geração dos casos existentes) passaram. O snapshot de
  parse dos `.k` existentes ficou sem comparação dedicada.
- O `unavailable` do closure é acrescentado (não restaurado) pelo caminho novo
  de módulo não genérico. Não observei efeito na saída, mas não verifiquei a
  impressão de `KAst.unavailable`.
