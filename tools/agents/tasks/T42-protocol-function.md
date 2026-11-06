<!--
   FILE: T42-protocol-function.md
   DESC: cartão-registro — função sobre protocolo (spec §4.14) sobre um tipo
         comum, fora de --instance. Integração cirúrgica: escrito direto por
         Opus (sem agente Haiku), por tocar a máquina de closure compartilhada
         com instanciação genérica normal (risco de regressão em --instance).
   AUTHOR: Claude Opus 5.5 (orquestrador)
   DATE: Nov, 2026
-->

# T42 — função sobre protocolo (spec §4.14) sobre tipo comum, fora de `--instance`

## Por que este cartão é um registro, não uma tarefa aberta

A investigação de 2026-11-06 (`.claude/memory/cgen-pendencias.md`) apontava
dois bugs em `stop_gen.c:emit_protocol_functions`. Abrir o oráculo revelou
que o gap era bem maior — nove lacunas encadeadas, no parser e no emissor —
e que corrigi-las tocava a máquina de closure que `--instance` também usa.
Por isso não foi delegado a um Haiku: implementei direto, registrando aqui o
antes/depois para quem reler o histórico.

## O problema

Nenhum golden exercitava uma função sobre protocolo (`pub inline i32
span(Countable items) { ... }`) chamada da tradução comum do seu próprio
módulo — todo exemplo do backend (§5.19) e todo uso real da base
(`keel.slice.of`) passa por `--instance`. `golden/cases/065-protocol-function/`
é o oráculo novo, escrito à mão e verificado batendo byte a byte com o cgen
corrigido antes de ser copiado para `expected/`.

## As nove lacunas e o conserto de cada uma

1. **`parser_islands_types.c`, seed do closure**: o ramo `else if
   (proto_of)` (tipo concreto comum, `KT_NAMED`) calculava o símbolo da
   chamada para o texto da ilha, mas nunca chamava `isl_record_seed` — só o
   ramo `if (gen)` (tipo concreto que é instância de genérico, `KT_MODIFIER`)
   gravava. Sem entrada no closure, `emit_protocol_functions` não tinha
   nada para emitir. Acrescentei `isl_record_protocol_seed_plain`
   (`parser_islands.c`), que grava o módulo e o símbolo do tipo concreto
   `KClosure.proto_concrete_module`/`arg_text[0]` (campo e reaproveitamento
   novos em `storage_types.h`) — o único jeito de levar essa informação do
   tempo de parse ao tempo de emissão sem reconstruir por mangling reverso.
2. **`parser_islands_types.c`, marca de adaptação**: a marca `&`/`*` da
   chamada usava o ponteiro do verbo redirecionado (`first`/`limit`, achado
   por `isl_verb_for_return` só para resolver o tipo de retorno) em vez do
   `byref` do tipo concreto (regra 7 da §4.14). Troquei para `isl_spec_byref`
   sobre o tipo concreto mesmo, só quando `proto_of && k==0`.
3. **`files.c:emit_c`**: a tradução comum do módulo (o `.c` linha a linha)
   não consultava `k_em_generic_out`/`over_protocol` para função — tentava
   emitir `span` com `Countable` literal, sem reescrita. Acrescentei o
   mesmo teste que já suprime a declaração do `.h`.
4. **`instance.c:k_emit_instance`**: o portão de entrada exigia
   `k_em_module_is_generic(generic)` mesmo quando `inst->proto_fn.len` já
   dizia que a chamada era de uma função sobre protocolo — recusava toda
   função sobre protocolo de um módulo sem binder. Troquei para só exigir
   isso quando **não** há `proto_fn`.
5. **`stop_gen.c:emit_protocol_functions`, campo errado**: o `!base` olhava
   `c->module` (o módulo que *declara* a função, quase sempre o próprio, não
   `keel`) em vez do módulo do tipo concreto — inofensivo hoje, porque
   nenhum dos três call sites do binário passa `base=false`, mas a semântica
   do campo (documentada no próprio `storage_types.h`: "the protocol's
   module for a function") pedia o tipo concreto. Movido para depois de
   `concrete_module` ser conhecido.
6. **`stop_gen.c`, reset de closure/ilhas**: ao terminar cada instância, o
   código zerava `generic->islands.len`/`closure.len` para `0`. Correto
   quando `generic` é um módulo genérico à parte (o caso de sempre); quando
   `generic == ast` (a função sobre protocolo pertence ao próprio módulo
   sendo compilado, o caso comum), isso apagava as ilhas que `emit_one`,
   chamado mais adiante na mesma função de `stop_gen.c`, ainda precisava —
   `app.h`/`app.c` saíam sem nenhum nome manglado. Troquei para salvar o
   tamanho antes de `k_collect_instance_islands` e restaurar esse tamanho
   (não zero) depois. Também passei a **não chamar**
   `k_collect_instance_islands` quando o tipo concreto não é instância de
   genérico (`!k`): essa função zera as ilhas e devolve falso de cara
   quando o módulo não é genérico (`parser_islands.c:1073-1076`) — para o
   caso comum as ilhas originais do parse já são as certas, e chamá-la só
   apagava tudo sem repor nada.
7. **`uses.c:own_call_rewritten`**: só reconhecia sufixo de aridade puro
   (dígitos no fim, `length2`); uma chamada a uma função sobre protocolo
   comum (`span` → `app_span_app_range2`) não é "foreign" por esse teste, e
   ficava com o nome simples (sem o sufixo do tipo concreto) — decls.c
   renomeava só para `app_span`. Acrescentei `own_protocol_call`, que
   resolve a declaração pelo nome escrito e pergunta `k_em_over_protocol`.
8. **`roles.c:proto_edits`**: assumia sempre a forma com o identificador
   opcional do tipo concreto (`Proto C x`, 3 tokens); a forma sem ele
   (`Proto x`, 2 tokens — a da regra 1 da §4.14, e a mais comum) fazia
   `x = p0 + 2` apontar depois do nome do parâmetro, e a função devolvia sem
   reescrever nada. Corrigido com um teste de limite (`,`/`)` dois tokens
   depois).
9. **Inclusão do header**: três pontas.
   - `use_paths.c:instance_of` só considerava `K_CLOSURE_INSTANCE`; uma
     função sobre protocolo (`K_CLOSURE_PROTOCOL_FUNCTION`) não era achada
     ao perguntar "de qual header é este símbolo", e `k_em_header_of`
     confundia a instância com o compartilhado do próprio módulo.
   - `uses.c:k_em_h_type_includes` usava `k_mangle_instance_path` direto
     para o `.type.h` do tipo concreto — certo só quando ele é instância de
     genérico; troquei para `k_em_header_of`, que já sabe diferençar.
   - `uses.c:k_em_h_body_includes` (o include antes do corpo, de quem o
     corpo chama) só considerava chamada "foreign"
     (`k_em_handles_use`/`k_em_own_instance`) — uma chamada a outra função
     do **mesmo módulo** (`limit`/`first`, declaradas no mesmo `.k` que
     `span`) não é foreign, mas vive num `.h` **separado** (o da instância
     da função sobre protocolo, não o do módulo). Afrouxei as duas checagens
     só quando `e->inst->proto_fn.len`.
   Uma primeira versão acrescentava um include a mais direto em
   `k_em_h_type_includes` (a `.h` de verbos do tipo concreto) — e duplicava
   o que o mecanismo de `k_em_h_body_includes` (ponto 3) já escrevia para o
   caso real `keel.slice.of` sobre `keel.buffer.buffer i32`, que `test/cgen/base.sh`
   já cobria. `test/run.sh` pegou a duplicata; removi, e o lugar certo era
   mesmo afrouxar `k_em_h_body_includes`, não duplicar em outro lugar.

## Achado correlato, fora do escopo

`test/cgen/cli/protocols.sh` tinha `count(l)` sem a marca `*1` — `List` não
é `byref`, e a regra 7 pede valor, não ponteiro, então o argumento (já
`List *`) precisa de `*1` (regra 5 da §4.4). A marca antiga nunca foi
verificada além de `--stop-after=parse` ("Hand-written expectation... not
generated by the parser under test", comentário do próprio arquivo).
Corrigido para bater com o comportamento certo (item 2 acima).

## Arquivos tocados

- `tools/cgen/src/engine/storage_types.h` — campo novo `proto_concrete_module`
  em `KClosure`, comentário de `arg_text`/`arg_len` para o uso novo.
- `tools/cgen/src/engine/parser/parser_islands.c` — `seeds[]` ganha
  `concrete_module`/`concrete_at`/`concrete_len`/`fn_name`;
  `isl_record_protocol_seed_plain` nova; a conversão de seed para `KClosure`
  em `close_instances` copia os campos novos.
- `tools/cgen/src/engine/parser/parser_islands_internal.h` — protótipo de
  `isl_record_protocol_seed_plain`.
- `tools/cgen/src/engine/parser/parser_islands_types.c` — seed do caso
  comum (ponto 1); marca de adaptação por `byref` do tipo concreto (ponto 2).
- `tools/cgen/src/engine/emit/files.c` — supressão da função sobre protocolo
  na tradução comum (ponto 3).
- `tools/cgen/src/engine/emit/instance.c` — portão de `k_emit_instance`
  (ponto 4).
- `tools/cgen/src/tool/stop_gen.c` — concrete_module antes do `!base`
  (ponto 5); salvar/restaurar em vez de zerar, e só chamar
  `k_collect_instance_islands` quando `k` (ponto 6); caminho `else if
  (c->proto_concrete_module...)` para o tipo comum, ao lado do `k`
  encontrado.
- `tools/cgen/src/engine/emit/uses.c` — `own_protocol_call` (ponto 7);
  `k_em_h_type_includes` usa `k_em_header_of` para o `.type.h` do tipo
  concreto (ponto 9); `k_em_h_body_includes` afrouxa as duas checagens de
  "foreign" quando `proto_fn.len` (ponto 9).
- `tools/cgen/src/engine/emit/roles.c` — `proto_edits` sem o alias opcional
  (ponto 8).
- `tools/cgen/src/engine/emit/use_paths.c` — `instance_of` também considera
  `K_CLOSURE_PROTOCOL_FUNCTION` (ponto 9).
- `golden/cases/065-protocol-function/**` — oráculo novo (`pf.k`,
  `expected/{c23,c11}`, `proof.c`, `NOTES`).
- `test/cgen/gen/golden-cases.txt` — linha do caso 065, sem `|wip`.
- `test/cgen/cli/protocols.sh` — marca `*1` em `count(l)` (achado correlato).

## Verificação

`test/run.sh` inteiro, verde: `test/cgen/run.sh` (unit/cli/lex/parse/diag/gen,
incluindo `base.sh` com `--instance "keel.slice.of keel.buffer.buffer i32"`,
o uso real que quase regrediu no ponto 9), `golden/run.sh` (128 ok, 0 fail,
0 xfail, 0 xpass, 0 wip) e `test/docs.sh` (45 ok, 0 fail).

## Ainda não coberto (achado durante a investigação, sem cartão aberto)

Função sobre protocolo que recebe um `array` (`inst.proto_array`, regra 9 da
§4.14) não foi exercitada por este caso — `box`/`span` não usam `array`.
Também não foi exercitado: duas funções sobre protocolo distintas no mesmo
módulo. Nenhum dos dois apareceu na investigação como quebrado, só como não
testado.
