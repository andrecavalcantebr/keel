# T23 — módulo genérico: relatório (parcial, parou no 053)

## Resultado

- **051-generic-forms: pronto** nos dois perfis (c23 e c11): todos os arquivos de `expected/` saem byte a byte.
- **053-generic-module-instance: não feito.** A forma pede uma regra da spec (§4.3 regra 6) que conflita com §5.10 regra 1 e com o golden 010-routine. Ver "Parada" abaixo.
- **g02 (`priv inline`): não feito.** A spec não cobre função `priv inline` num genérico; ver "Pontos abertos".

## O que mudou (arquivo a arquivo)

1. `tools/cgen/src/engine/emit/instance.c`
   - `put_gap`: um comentário sai do texto da lacuna e os brancos em volta ficam como escritos (antes: recusa). Bloco de comentário que cruza linha continua recusado.
   - `emit_modifier`: sai a checagem de uma linha só. Modificador de várias linhas sai com as quebras do autor, um `#line` só.
   - `carried`: aceita função `priv` não-`inline` (corpo vai ao `.c`); `priv inline` continua recusada.
   - `emit_file` (`.type.h`): só `pub` no `.h`; `has_function` e `has_inline` contam só o que é do instance.
   - `k_em_placement_c`: função não-`inline` de `priv` sai como `static` no `.c` de quem declara `instance`.
   - `k_emit_instance_redundant`: uma função não-`inline` (pub ou priv) já torna a instância não redundante.
   - `k_em_binder_use` e o laço de modificadores de `emit_file`: pulam modificadores de outro nome.
   - Cabeçalho do arquivo: lista de recusas atualizada.
2. `tools/cgen/src/engine/emit/emit.c`
   - Nova `k_em_other_modifier(e, n)`: verdadeiro para um modificador de outro nome, ou uma função cuja assinatura nomeia um modificador que não é o da instância. Motivo: o genérico `gx` do 051 tem dois modificadores, e a instância de um deles emitia o outro.
   - `k_em_skip`: pula essas funções (é o caminho do `.h` de instância, `emit_header`/`h_body`).
3. `tools/cgen/src/engine/emit/emit_internal.h`: declaração de `k_em_other_modifier`.
4. `test/cgen/gen/golden-cases.txt`: removido só o `|wip` da linha `051-generic-forms`. A linha `053-generic-module-instance` ficou com `|wip`.

Não houve commit, `git add`, nem edição de expected, proof, unit ou cli. Nenhum teste de unit ou cli afirmava o contrário do cartão (verificado em `test/cgen/unit/emit/emit_main.c` e `test/cgen/cli/instance.sh`).

## Saída final dos comandos

- `sh test/cgen/gen_golden.sh`: `60 ok, 0 fail, 4 wip` (wip: 053, 054, 055, 056).
- `sh test/cgen/base.sh`: `124 ok, 0 fail, 20 wip`.
- `golden/run.sh`: `108 ok, 0 fail, 0 xfail, 0 xpass, 3 wip`. (O cartão diz 102; o número real antes e depois da mudança é 108.)
- `sh tools/agents/verify.sh`: `build (0 warnings, baseline 0)`, `make check (53 lines ok)`, `golden: 108 ok, 0 fail, 0 xfail, 0 xpass, 3 wip`, `ALL OK`.
- `sh tools/agents/survey/run.sh`: g01 `[c23 LIMIT] [c11 LIMIT]`; g02 `[c23 LIMIT] [c11 LIMIT]`; g03 e g04 `OK` nos dois perfis. Entre os demais, só y02 dá `ERR` (`other.WORKERS` não conhecido em tempo de tradução), que não vem desta mudança e não foi investigado.

## Parada: conflito de regra no 053

A regra 6 do §4.3 (commit `006c600`, de hoje) diz que uma declaração de genérico que cita só parâmetro (`constexpr`, `typedef`) sai nos headers `M_<args>` e que o `.type.h` de cada instância de modificador a inclui. Por isso o 053 espera `gd_box_3_i32.type.h` separado de `gd_3_i32.type.h`.

Mas o §5.10 regra 1 diz que o `typedef` da participante "é da instância e sai no header de instância", e o golden 010-routine (passando) põe o `typedef keel_routine_ag2_Ag` e a struct do modificador `keel_routine_slot_ag2_Ag` no mesmo arquivo `keel/keel_routine_ag2_Ag.type.h`, sem `keel_routine_slot_ag2_Ag.type.h`. A regra 6 e o golden 010 não podem valer juntos: o `typedef` de `routine` cita só `C`, então a regra 6 o tiraria do arquivo do modificador. Não implementei nenhuma das duas escolhas. Isso é para o orquestrador arbitrar: ou a regra 6 vale para o `routine` (o golden 010 muda), ou a regra 6 deixa de valer para o caso do unit (o 053 muda).

## Pontos abertos

- **g02 (`priv inline`)**: a spec da tabela de camadas (§4.3.2) não diz onde vai o corpo de uma função `priv inline` num genérico. Hoje ela é recusada (`implementation-limit`). Se a decisão for `static inline` no `.h` da instância, a mudança é pequena, mas não a fiz sem decisão de spec.
- **Vazamento de include**: `k_em_instance_type_includes` (em `uses.c`, fora do escopo deste cartão) ainda percorre todos os modificadores do genérico. Só importa quando um argumento é ele mesmo uma instância, e não afeta o 051.
- A função `same_slice` em `emit.c` é `static` e tem o mesmo nome da de `instance.c`; são unidades diferentes, sem conflito.
