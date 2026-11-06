<!--
   FILE: T43-dim-target-instance.md
   DESC: cartão-registro — chamada resolvida pelo alvo (spec §4.4, item 4)
         perdia a dimensão de um módulo dim-genérico no nome da instância.
         Integração cirúrgica: escrito direto por Opus (sem agente Haiku).
   AUTHOR: Claude Opus 5.5 (orquestrador)
   DATE: Nov, 2026
-->

# T43 — chamada por alvo, num módulo `dim`-genérico, perdia a dimensão no nome

## Origem

André pediu para analisar `../keel_maths` (projeto separado, biblioteca de
matrizes para keel, fora de desenvolvimento). Os probes de lá
(`tests/probes/model.k`/`construct.k`, uma proposta de `zeros(matrix(3,4))`)
bateram em `implementation-limit`, mas combinavam `type M`, `outcome`, dois
modificadores e `child`/`parent` de uma vez — não dava para atribuir a causa
a nenhuma peça isolada. Reduzi ao mínimo aqui no `keel` (fora do
`keel_maths`, a pedido — "lá eu quero limpo", projeto para pegar casos de
uso real, não para depurar o compilador) e achei um bug de verdade, bem mais
simples do que o gap original parecia.

## O bug

```keel
module mx dim ROWS, COLS;
pub modifier box { double data[ROWS][COLS]; }
pub inline box zeros(void) { box v = {0}; return v; }
```
```keel
mx.box(2, 3) v = mx.zeros();
```

A instância `mx_box_2_3.h` saía certa (com `mx_box_2_3_zeros`), mas a
CHAMADA, em `app_mx_use.c`, saía `mx_zeros()` — símbolo inexistente. Nem
compilava (`implicit declaration`, depois `invalid initializer`).

## A causa

`module_instance_prefix` (`parser_islands_types.c`) monta o nome da
instância a partir do módulo mais os ARGUMENTOS DE TIPO do `KType`
(`arg[]`/`argc`) — nunca soube de um valor de binder de dimensão, que não é
um `arg`. Para uma chamada resolvida pelo RECEPTOR (`length(g)`, o objeto É
a instância — caso 006, `direct=true`), o código já usa `isl_kt(c,
inst)->symbol` direto, que `k_spec_symbol`/`isl_kt_from_text` calculam
certo (dimensão incluída). Para uma chamada resolvida pela **seleção** ou
pelo **alvo** (`sig.found && c->has_target`, ou a atribuição/`return` da
regra do spec §4.4 item 4) — sem receptor —, o código caía direto em
`module_instance_prefix`, sem olhar se `symbol` já estava certo.

## O conserto, e por que não foi só "usar symbol sempre"

Primeira tentativa: preferir `isl_kt(c,inst)->symbol` sempre que não vazio,
antes de `module_instance_prefix`. Quebrou o golden 010 (`gen_golden.sh`,
`010-routine c11`): `routine.seq` saiu `keel_routine_slot_ag2_Ag_seq` em vez
de `keel_routine_ag2_Ag_seq`. Essa chamada é resolvida pelo caminho
`nested` (`inst = kt_find(c, tk, module, 0)`, linha ~834): o `symbol` que
`kt_find` deixa no `KType` devolvido não é o da instância inteira (é do
`slot` aninhado) — preferir `symbol` ali piora, não corrige.

O conserto final restringe a preferência por `symbol` às três chamadas de
`isl_kt_from_text` (seleção, e os dois ramos de alvo) — marcadas por uma
variável nova, `named`, inicializada `false` ao lado de `direct`. O caminho
`nested` (`kt_find`) continua em `module_instance_prefix`, intocado.

## Arquivo tocado

- `tools/cgen/src/engine/parser/parser_islands_types.c` — variável `named`;
  marcada nos três ramos de `isl_kt_from_text` (seleção, `has_target`, alvo
  por atribuição/`return`); o ramo final da construção do nome da chamada
  usa `isl_kt(c, inst)->symbol` quando `named && *symbol`, só então cai em
  `module_instance_prefix`.

## Oráculo

`golden/cases/066-dim-target/` — `mx.k` (o módulo `dim`-genérico) e
`app/mx_use.k` (o consumidor, com `main`). A prova é aritmética: `mx.set`/
`mx.get` sobre índices que só cabem em `box(2,3)` — se a instância fosse
outra, a conta erraria ou acessaria fora dos limites.

## Verificação

`test/run.sh` inteiro verde: `test/cgen/run.sh` (incluindo `parse_dump/ilha`
e `gen_golden`, que pegaram a primeira tentativa quebrando o caso 010),
`golden/run.sh` (132 ok, 0 fail — 066 incluído), `test/docs.sh` (45 ok).

## Relação com `keel_maths`

Este conserto é necessário mas não suficiente para os probes originais: o
caso com `type M` (seleção de modificador por um parâmetro de tipo, em vez
de pelo alvo) ainda bate num `implementation-limit` diferente, mais adiante
no emissor — não investigado aqui. O design do `keel_maths`
(`zeros(matrix(3,4))`, o tipo como argumento da chamada) também não
corresponde a nenhuma forma documentada: o precedente real (`grids`, caso
006) escreve o tipo instanciado como anotação (`grid(3) f32 *a`, ou como
alvo de uma declaração `grid(DIM) f32 g;`), nunca como argumento de uma
chamada. Vale levar essa observação de volta ao `keel_maths`, não a este
cartão.
