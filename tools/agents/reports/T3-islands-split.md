# T3 — relatório: partição do `parser_islands.c`

Status: **concluída.** O `parser_islands.c` foi partido em seis arquivos e um
header interno, sem mudar código. Os três aceites passam. Nada foi commitado.

Este relatório substitui a primeira versão, que parou antes de editar. A
parada foi por causa dos quatro `static` de estado da seção `seeds from
calls`. A decisão do orquestrador (opção A) é que essa seção fica no
`parser_islands.c`, e o cartão foi atualizado de acordo.

## Partição (arquivo → seções → linhas do `HEAD`)

Os 23 marcos `/* ---- ... */` batem com a tabela do cartão.

| Arquivo | Seções | Linhas do `HEAD` | Linhas hoje |
| --- | --- | --- | --- |
| `parser_islands.c` (fica) | comentário de topo, includes e helpers `tok`…`close_bracket`; `the detail column`; `locals`; `the module a call lands in`; `protocols`; `calls`; `seeds from calls` (com os quatro `static`); `extern_c and main` com o `k_collect_islands` | 1–668, 3670–3891, 4705–4850 (sem as linhas 21–130 dos tipos do topo, que vão para o header) | 919 |
| `parser_islands_internal.h` | tipos, macros e protótipos (novo) | linhas 21–130 do topo, `Sig` (427), `K_CLOSURE_MAX` (3598) e `Shape` (634) | 242 |
| `parser_islands_types.c` | `container types`; `names`; `declarations` (a primeira) | 669–1887 | 1213 |
| `parser_islands_index.c` | `stage 4b: array, array-index, index, range-index` | 1888–2196 | 311 |
| `parser_islands_flow.c` | `defer, foreach, walk, match`; `parallel, win, fail`; `extent`; `the else of a result` | 2197–3376 | 1185 |
| `parser_islands_closure.c` | `the closure of the instances`; `provenance and invalidation`; `instance (spec §4.3)` | 3377–3669, 3892–4053 | 462 |
| `parser_islands_checks.c` | `names across modules`; `declarations (passage 2 …)`; `protocol declarations`; `parameters type and over a protocol`; `layout` | 4054–4704 | 656 |

Nenhum arquivo passa de 1400 linhas. A ordem das funções dentro de cada
arquivo é a do arquivo antigo.

## Funções exportadas por arquivo

Exportadas são as que deixaram de ser `static` porque outro arquivo as chama.
Cada uma tem protótipo no header. Total: **103**. A contagem por arquivo
abaixo vem de uma regex sobre os `.c`, e ela dá 104; o gerador dá 103, e a
diferença é uma linha a mais que a regex pegou. Todas as 103 são chamadas de
outro arquivo (conferido por grep).

- `parser_islands.c` (47): `tok live punct word ident index_at enter close_paren
  close_bracket put put_str put_uint put_tokens put_glued put_source keep emit
  find_local digits decimal_of_token decimal_of dims_of add_local rank_of node_at
  module_node generic put_module_prefix module_named binder modifier_of params_of
  find_verb module_alias module_by_name protocol_decl satisfies
  returns_associated verb_for_return range_text diag3 alias_of enter_text
  container_shape arg_local record_seed k_collect_islands`
- `parser_islands_types.c` (21): `kt kt_symbol kt_new kt_set_symbol ast_of
  kt_from_text binders_of kt_from_home ret_type type_of resolution_begin
  resolution_end call_at tag_of_some_set name_island qualified_name is_arena
  note_shadow binder_of c_declares declaration`
- `parser_islands_index.c` (8): `array_decl container_verb path_forms
  signature_of block_constexpr assignment_op after_operand check_uses`
- `parser_islands_flow.c` (7): `put_range enclosing_brace control_before
  tags_decl extent_island column_access walk`
- `parser_islands_closure.c` (11): `param_type call_args verb_available
  instances_in kt_deep local_of produce roles_at_call instance_decl
  check_generic check_modifiers`
- `parser_islands_checks.c` (10): `check_collisions check_alias_types check_name
  spec_byref check_arguments check_declarations check_constexpr check_protocols
  check_jumps check_layout`

As demais funções ficaram `static` no arquivo que as define.

## Renomeações, junções e mudanças além do corte puro

- **Nenhuma renomeação com `isl_`.** O link não acusou colisão com símbolo de
  outro arquivo do motor.
- **Nenhuma junção de seções.** A regra 2 do cartão agora é só dado de
  relatório, como o orquestrador decidiu.
- **Três tipos foram para o header, além dos do topo** (regra "outros tipos
  usados por mais de um arquivo"): `Sig` (linha 427 do `HEAD`, usado por
  `find_verb` e pelo resto), `K_CLOSURE_MAX` (3598, usado por closure e pela
  seção seeds) e `Shape` (634, que aparece na assinatura de `container_shape`,
  que vai no header).
- **Dois protótipos de várias linhas foram reescritos.** `declaration` e
  `roles_at_call` têm a primeira linha igual à da definição movida. O
  `check_split.py` conta essa linha duas vezes e acusou `UNEXPLAINED` na linha
  de continuação. Por isso o protótipo foi reescrito (uma linha quando cabe em
  120 colunas; quebrado num vírgula diferente quando não cabe). Não é mudança
  de código: só a forma do protótipo no header.
- **`static` tirado** só das 103 funções exportadas, e dos protótipos
  adiantados delas. Os quatro estados globais da seção `seeds` (`seed_nodes`,
  `seed_node_count`, `seeds`, `seed_count`) ficam `static` no
  `parser_islands.c`, como decidiu o orquestrador.
- **README do motor:** a linha de `parser_islands.c` foi reescrita e foram
  acrescentadas cinco linhas (uma por arquivo novo) e uma para o header.

## Aceitação (saídas exatas)

```
$ python3 tools/agents/check_split.py HEAD:tools/cgen/src/engine/parser/parser_islands.c \
      tools/cgen/src/engine/parser/parser_islands.c tools/cgen/src/engine/parser_islands_*.c \
      tools/cgen/src/engine/parser/parser_islands_internal.h
ok   split: all 4603 non-blank lines of tools/cgen/src/engine/parser/parser_islands.c are in the new files, the rest is glue

$ sh tools/agents/verify.sh compare /tmp/keel-snap-T3
ok   build (4 warnings, baseline 4)
ok   make check (41 lines ok)
ok   golden: 44 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ok   behaviour identical to /tmp/keel-snap-T3
ALL OK

$ wc -l tools/cgen/src/engine/parser_islands*.c
   919 parser_islands.c
   656 parser_islands_checks.c
   462 parser_islands_closure.c
  1185 parser_islands_flow.c
   311 parser_islands_index.c
  1213 parser_islands_types.c
  (header: 242 linhas)
```

Também: `make -C tools/cgen boundary` deu `ok engine boundary`. Os 4 avisos de
build são os de `src/tool/main.c` já na baseline; nenhum aviso novo.

Escopo do `git status`: `parser_islands.c` e `README.md` modificados; cinco
`parser_islands_*.c` e `parser_islands_internal.h` novos; este relatório. O
cartão `T3-islands-split.md` aparece modificado porque o orquestrador o
atualizou; não foi tocado por mim.

## Dúvidas

1. **Número de exportações (103).** O cartão dizia que um arquivo com mais de
   uns 40 nomes exportados era sinal para juntar seções. O orquestrador
   decidiu que isso é só dado. O `parser_islands.c` exporta 47, que é o maior.
   Se quiserem reduzir, o caminho é mover helpers pequenos (`tok`, `live`,
   `put*`) para o header como `static inline`, mas isso muda o texto das
   linhas e fica fora deste corte.
2. **Contagem de `static` do cartão (194 contra 168 definições).** A varredura
   do cartão não confere com a do arquivo, que tem 168 definições. Não era
   critério da tarefa, e não foi usada na verificação.
3. **Prefixo dos comentários de topo dos arquivos novos.** Escrevi um
   comentário de topo em inglês para cada arquivo novo, sem números de etapa
   (o cartão pede inglês no código). A cópia `stages 4a to 4h` do topo do
   `parser_islands.c` ficou como estava, porque é texto original.
4. **Tamanho do `parser_islands.c` (919, não ~1040).** A estimativa do cartão
   contava as linhas 21–130 dos tipos do topo, que foram para o header. A
   soma dos arquivos novos é 4988 linhas, contra 4850 do original. A diferença
   são as linhas de cola novas (includes, guardas, protótipos, comentários de
   topo dos arquivos novos); o `check_split.py` confirma que nenhuma linha de
   código se perdeu.
