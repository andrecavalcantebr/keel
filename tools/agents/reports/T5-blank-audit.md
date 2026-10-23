# T5 — auditoria das linhas vazias nos arquivos gerados do golden

Cartão: `tools/agents/tasks/T5-blank-audit.md`. Auditoria só de leitura: o golden
não foi alterado, nada foi commitado, e não rodei `make` nem `verify.sh`.

## O que foi feito

- Criado `golden/blank-check.py` (Python 3, só biblioteca padrão). Ele reusa
  `golden/line-check.py` (importado por `importlib`) para ler os arquivos, o
  contador de `#line` e a resolução dos `.k`. Cobre os mesmos 155 arquivos por
  perfil que o `line-check.py`.
- Uso: `python3 golden/blank-check.py [--verbose]` faz a auditoria;
  `python3 golden/blank-check.py --simulate /tmp/<dir> [--rules top,sec3]` aplica
  a regra (b) numa cópia (golden + base) em `/tmp` e roda
  `line-check.py --check` sobre ela. O golden original não é tocado.
- Nenhum `__pycache__` foi criado.

Totais: 467 linhas vazias por perfil, 934 no total.

## 1. Categorias (linha vazia, primeira que vale)

| categoria | c11 | c23 | total | exemplos (arquivo:linha) |
| --- | ---: | ---: | ---: | --- |
| TOP-AFTER-SYNTH | 114 | 114 | 228 | `001-arena-buffer-defer/expected/c11/app/app_cfg.h:6`, `001-arena-buffer-defer/expected/c11/app/app_cfg.type.h:5` |
| SECTION-3 | 14 | 14 | 28 | `010-routine/expected/c11/keel/keel_array_keel_routine_slot_ag2_Ag.h:25`, `010-routine/expected/c11/keel/keel_routine_ag2_Ag.h:23` |
| BEFORE-ENDIF | 0 | 0 | 0 | (nenhuma, ver nota) |
| MAPPED-SOURCE | 191 | 191 | 382 | `001-arena-buffer-defer/expected/c11/app/app_cfg.c:8`, `001-arena-buffer-defer/expected/c11/app/app_cfg.c:13` |
| MAPPED-OTHER | 125 | 125 | 250 | `001-arena-buffer-defer/expected/c11/app/app_cfg.c:7`, `001-arena-buffer-defer/expected/c11/app/app_cfg.c:9` |
| UNMAPPED | 7 | 7 | 14 | `010-routine/expected/c11/keel/keel_slice_keel_routine_slot_ag2_Ag.h:10`, `golden/c11/keel/keel_buffer_i32.h:12` |
| MAPPED-PAST-END (fora do card) | 16 | 16 | 32 | `010-routine/expected/c11/keel/keel_outcome_keel_routine_slot_ag2_Ag.h:18`, `010-routine/expected/c11/keel/keel_outcome_keel_slice_keel_routine_slot_ag2_Ag.h:18` |
| MAPPED-NO-K | 0 | 0 | 0 | |
| **total** | **467** | **467** | **934** | |

Notas sobre a tabela:

- **BEFORE-ENDIF vale zero, de propósito da ordem.** Há 10 arquivos por perfil
  que só têm includes entre a guarda e o `#endif`, com linha vazia antes do
  `#endif`. Essa linha é `top_last + 1`, então cai antes em TOP-AFTER-SYNTH.
- **TOP-AFTER-SYNTH inclui 8 blocos "interior" por perfil**: uma linha vazia
  *entre* dois includes sintéticos do topo, e não depois do último. Estão nos
  `.type.h` de `keel/keel_outcome_*.type.h` (ex.: `010-routine/expected/c23/keel/keel_outcome_keel_routine_slot_ag2_Ag.type.h:5`,
  e `golden/c23/keel/keel_outcome_keel_buffer_i32.type.h:5`). Sem os interiores,
  TOP-AFTER-SYNTH seria 106 por perfil.
- **MAPPED-PAST-END** não está entre as seis categorias do cartão. São linhas
  vazias sob um `#line` cujo número de `.k` passa do fim do `.k` (ex.: `outcome.k`
  tem 32 linhas e o contador põe a linha 18 do `.h` em `keel/outcome.k:33`). Ver §4.

## 2. Topo: top-blank contra top-noblank

Contagem por (perfil, extensão, origem). Origem: `module` = nome do arquivo igual ao
mangling do `.k`; `instance` = outro nome (ex.: `keel_array_keel_routine_slot_ag2_Ag`);
`prelude` = `keel.h`/`keel.type.h`; `entry` = `main_*.c` ("entry unit").
c11 e c23 têm a mesma contagem, exceto no prelúdio `.type.h` (ver nota).

| tipo | origem | arquivos | top-blank | top-noblank | o que vem depois do bloco |
| --- | --- | ---: | ---: | ---: | --- |
| `.type.h` | instância | 31 | 17 | 14 | `#line` 25, `#endif` 6 |
| `.type.h` | módulo | 32 | 32 | 0 | `#line` 22, `#endif` 10 |
| `.type.h` | prelúdio | 1 | 0 | 1 (c11) | `#line` (c11); sem topo (c23, `#line 12` vem antes dos includes de `<stdint.h>`) |
| `.h` | instância | 31 | 31 | 0 | `#line` 24, código 7 |
| `.h` | módulo | 32 | 26 | 6 | `#line` 15, código 11, `#endif` 6 |
| `.h` | prelúdio | 1 | 0 | 1 | `#endif` |
| `.c` | módulo | 23 | 0 | 23 | `#line` 23 |
| `.c` | entry (`main_*.c`) | 4 | 0 | 4 | código 4 |

Totais por perfil: `.type.h` com linha vazia 49, sem 15 (contando o prelúdio c11;
o c23 não tem topo no prelúdio); `.h` com 57 e sem 7; `.c` sem nenhuma (o `#line`
vem logo depois dos includes, em 27 arquivos).

**Os números do cartão não reproduzem.** O cartão dizia 53 `.type.h` com linha vazia
e 10 sem, e 22 `.h` com e 42 sem. Medi com a definição do próprio cartão (a linha
imediatamente depois do último include do topo): 49/15 e 57/7. Não consegui achar
de onde vieram 53/10 nem 22/42 (a divisão entre `golden/cases/` e `golden/c11|c23`
também não bate). Pode ser de outro estado da árvore; vale conferir antes de fixar
a regra.

Dez arquivos por perfil só têm includes entre a guarda e o `#endif` (13 sem linha
vazia antes do `#endif`, 10 com; a soma é 23 por perfil: 6 `.h` módulo, 1 `.h`
prelúdio, 6 `.type.h` instância, 10 `.type.h` módulo).

## 3. Seção 3 dos `.h`

Há 14 blocos de includes de `.h` (não `.type.h`) fora do topo, por perfil. **Só existe
uma forma:** nenhum bloco tem linha vazia antes; todos têm linha vazia depois; o que
vem antes é código (protótipo ou `typedef`); o que vem depois é a linha vazia. Ou
seja, "uma linha vazia antes e uma depois" acrescenta uma linha vazia antes dos 14
blocos, e nenhuma depois.

## 4. MAPPED-OTHER e UNMAPPED, uma a uma

**MAPPED-OTHER (125 por perfil; os conjuntos de c11 e c23 são idênticos, a menos do
nome do perfil).** Cada uma é uma linha vazia sob um `#line` cujo `.k` tem código.
Formato: linha C → linha do `.k`, por arquivo (c23; o c11 é igual trocando o perfil).

- `golden/c23/keel/keel_parallel.h`: 14→`keel/parallel.k`:43
- `golden/c23/keel/keel_range.h`: 15→`keel/range.k`:29
- `001-arena-buffer-defer/expected/c23/app/app_cfg.c`: 7→1, 9→3, 10→4, 11→5, 12→6, 14→8
- `002-from-array/expected/c23/app/app_pool.c`: 5→1, 6→2
- `003-alignment/expected/c23/app/app_align.c`: 5→1, 6→2, 7→3, 8→4, 10→6
- `004-parallel/expected/c23/app/app_val.c`: 7→1, 8→2, 9→3, 10→4, 19→13
- `005-defer-loop/expected/c23/app/app_loop.c`: 4→1
- `006-dim/expected/c23/app/app_g.c`: 5→1, 6→2, 7→3, 9→5; `006-dim/expected/c23/grids_grid_3_f32.h`: 10→`grids.k`:11
- `007-instance/expected/c23/app/app_p.c`: 6→1, 7→2, 8→3; `007-instance/expected/c23/coll_stack_i32.h`: 10→`coll.k`:14
- `008-block-constexpr/expected/c23/app/app_cx.c`: 4→1, 5→2, 7→4, 9→6
- `009-match-tags/expected/c23/ag.c`: 6→1, 7→2, 8→3, 9→4, 11→6, 13→8; `009-match-tags/expected/c23/keel/keel_tagged_ag_Cycle_void.h`: 11→`keel/tagged.k`:22
- `010-routine/expected/c23/ag2.c`: 8→1, 9→2, 10→3, 11→4, 12→5, 13→6, 15→8; `010-routine/expected/c23/keel/keel_routine_ag2_Ag.h`: 14→`keel/routine.k`:34, 17→`keel/routine.k`:53
- `011-else/expected/c23/app/app_el.c`: 7→1, 8→2, 9→3, 10→4
- `012-parallel-any/expected/c23/app/app_bs.c`: 8→1, 9→2, 10→3, 11→4, 19→12
- `013-sugar/expected/c23/app/app_ac.c`: 6→1, 7→2, 8→3, 9→4
- `014-defer-later/expected/c23/app/app_dl.c`: 4→1, 18→15
- `015-foreach-range/expected/c23/app/app_ap.c`: 6→1, 7→2, 8→3
- `016-line/expected/c23/app/app_ml.c`: 5→1, 6→2, 7→3, 9→5
- `017-defer-return/expected/c23/app/app_dr.c`: 4→1, 5→2, 7→4, 29→26, 36→33
- `018-corot-outcome/expected/c23/results.c`: 7→1, 8→2, 9→3
- `019-walk/expected/c23/lst.c`: 5→1, 6→2, 7→3, 12→8, 13→9, 14→10, 16→12, 17→13, 18→14; `019-walk/expected/c23/lst.h`: 17→`lst.k`:36
- `020-extent/expected/c23/app/app_pos.c`: 6→1, 8→3, 9→4, 11→6, 12→7, 16→11, 17→12, 18→13, 19→14, 20→15, 21→16, 24→19, 25→20, 26→21, 27→22, 30→25, 31→26, 32→27, 33→28, 34→29; `020-extent/expected/c23/app/app_pos.h`: 29→`app/pos.k`:78
- `021-else-assign/expected/c23/app/app_reg.c`: 7→1, 9→3, 10→4, 14→8, 15→9, 16→10, 17→11, 18→12
- `025-of-range/expected/c23/app/app_ofr.c`: 12→1, 14→3, 15→4, 16→5, 17→6, 18→7

Explicação: são **preenchimentos de alinhamento**. Cada uma é uma linha do `.k` que
não gera código naquela posição (`module`, `import`, `import_c`, `constexpr` que foi
para o `.type.h`, protótipo sem corpo), e a linha vazia a ocupa para que o contador
não precise de um `#line` ali (backend §6). Exemplo: `app_cfg.c` tem `#line 1` e
depois linhas vazias para os `.k` 1, 3, 4, 5, 6 e 8 (`module`, quatro `import`,
`pub constexpr`); o `.k` 2 é a linha vazia do `.k`, que é MAPPED-SOURCE. Com
`--check`, pôr uma linha vazia dentro de um `#line` faz o `line-check.py` falhar
(ver §5).

**UNMAPPED (7 por perfil).** São linhas vazias **antes do primeiro `#line`** do
arquivo, num ponto onde não há regime de contador. Estão sempre entre uma linha de
código (declaração sem `#line`) e o primeiro `#line`:

- `010-routine/expected/c11|c23/keel/keel_slice_keel_routine_slot_ag2_Ag.h`:10
- `021-else-assign/expected/c11|c23/keel/keel_slice_const_char.h`:9
- `golden/c11|c23/keel/keel_buffer_i32.h`:12
- `golden/c11|c23/keel/keel_buffer_size_t.h`:12
- `golden/c11|c23/keel/keel_slice_f32.h`:10
- `golden/c11|c23/keel/keel_slice_i32.h`:10
- `golden/c11|c23/keel/keel_slice_size_t.h`:10

Nenhuma regra do cartão as explica. Não são tocadas pela regra (b): não estão no
topo nem na seção 3. Vale decidir se são separadores que a regra deve fixar, ou se
devem sair.

**MAPPED-PAST-END (16 por perfil, fora do cartão).** A linha vazia cai depois do
fim do `.k` (ex.: `keel/outcome.k` tem 32 linhas e o contador chega a 33). Estão
em `keel_outcome_*.h` (c23 lista: `010-routine/.../keel_outcome_keel_routine_slot_ag2_Ag.h`:18,
`010-routine/.../keel_outcome_keel_slice_keel_routine_slot_ag2_Ag.h`:18,
`021-else-assign/.../keel_outcome_f64.h`:18, `021-else-assign/.../keel_outcome_keel_slice_const_char.h`:18,
`golden/c23/keel/keel_outcome_{f32,i32,size_t,u32,void,keel_buffer_i32,keel_buffer_size_t,keel_slice_f32,keel_slice_i32,keel_slice_size_t}.h`:18
(`void` é 16), `golden/c23/keel/keel_arena.h`:22 (`arena.k`:74) e `golden/c23/keel/keel_corot.h`:17 (`corot.k`:40)).
São espaçadores que sobram depois da última declaração do `.k`, seguidos de `#line`.
Como não há linha do `.k` para casar, o `line-check.py` não confere nada neles.

## 5. Simulação da regra (b)

Cópias em `/tmp/t5-sim-*` (golden + base inteiros), `line-check.py --check` sobre
cada uma. Nenhuma linha MAPPED foi tocada (nenhum "kept (mapped)" apareceu).

| regra aplicada | arquivos alterados | linhas + | linhas − | `--check` |
| --- | ---: | ---: | ---: | --- |
| nenhuma (controle) | 0 | 0 | 0 | passa (rc 0) |
| `top` (uma linha depois do último include do topo) | 97 | 97 | 0 | passa |
| `sec3` (uma linha antes e uma depois de cada bloco da seção 3) | 28 | 28 | 0 | passa |
| `top,sec3` | 125 | 125 | 0 | passa |

Contagens que o cartão pediu:

- **Topo (`top`):** 97 linhas acrescentadas. 54 vão antes do `#line` dos `.c`
  (27 por perfil). 26 vão antes do `#endif` de arquivos só com includes
  (13 por perfil). Os 17 restantes são `.h` e `.type.h` que vão para `#line` ou código.
- **Seção 3 (`sec3`):** 28 linhas antes (14 por perfil), 0 depois (todos os blocos
  já têm linha vazia depois).

**Controle negativo:** pôr uma linha vazia em `app_cfg.c` (c11, linha 10, dentro do
`#line 1`) faz o `--check` falhar com 10 erros e rc 1. O `--check` é sensível a esse
tipo de mudança, então o passa das quatro simulações é significativo.

Conclusão: a regra (b), nas duas partes (topo e seção 3), não quebra o mapeamento.
O `#line` continua exigido e nenhuma linha MAPPED muda de número.

## 6. Perguntas para o André fixar a regra

1. **Arquivo só com includes e guarda** (23 por perfil; 13 sem linha vazia antes do
   `#endif`): a (b) põe linha vazia antes do `#endif` (26 linhas no total)? A
   simulação fez isso, porque a regra diz "uma linha vazia entre o último include e
   a primeira linha seguinte", e o `#endif` conta como a primeira linha seguinte.
2. **Os `.c`:** a (b) vale para eles? Hoje 27 por perfil não têm linha vazia entre os
   includes e o `#line` (ou o código, nos `main_*.c`). A simulação os incluiu (54
   linhas).
3. **Linha vazia interior no topo** (8 por perfil, nos `keel_outcome_*.type.h`,
   entre o `keel.type.h` e o primeiro include de `.type.h` do módulo): a (b) só
   fixa a linha depois do último include. Ela fica como está, ou sai?
4. **Seção 3:** a forma é uniforme (14 blocos, todos sem linha antes). Confirmar
   "uma linha vazia antes e uma depois" como texto da regra, já que hoje só a de
   antes falta.
5. **As 7 UNMAPPED por perfil** (linha vazia antes do primeiro `#line`, depois de
   declaração sem `#line`): são separadores a fixar pela regra, ou devem sair?
6. **As 16 MAPPED-PAST-END por perfil** (espaçador além do fim do `.k`): o cartão
   não as cobre. Queremos uma regra para elas, ou elas são só consequência do
   contador?
7. **Números do cartão** (53/10 e 22/42): não reproduzem com a definição do próprio
   cartão. Vale confirmar de qual estado da árvore vieram antes de fixar a regra.

## Estado do repositório

- `golden/blank-check.py` criado. Nenhum arquivo do golden foi alterado; nada foi
  commitado.
- `golden/run.sh`: `45 ok, 0 fail, 0 xfail, 0 xpass, 3 wip` (rc 0). Os 3 WIP são
  022, 023 e 024 (linux-list), que o próprio `run.sh` marca.
- `git status` mostra mudanças que não são minhas e que eu não toquei: `M tools/transform/README.md` e os
  não rastreados `tools/cgen/src/engine/emit/`, `tools/cgen/test/unit/emit_mangle*`,
  `tools/cgen/test/unit/emit_writer*`. Não estavam no `git status` do início da
  sessão; vale ver se outro agente está trabalhando na mesma árvore.
