# E02 — o `implementation-limit` fala sempre a mesma frase

Estudo aberto em 2026-11-23, cgen 0.4.0-dev. Origem: auditoria de limites
(dúvidas 2 e 10).

## O que a spec exige

Spec §6.4 e cgen-tool-spec §7.1: o diagnóstico de limite "informa qual limite,
seu valor, a demanda quando conhecida e a configuração a ajustar".

## O que acontece

A mensagem do catálogo é uma só (`engine/diag_messages.def:150`):

    insufficient storage while collecting module `%s`

e o argumento varia por ponto de emissão. Alguns pontos passam um texto livre
(`parser_delimiters.c:88`: "delimiters nested deeper than 256"), o que sai
dentro de "module `…`". Outros passam o nome de um módulo, de um local ou de um
token qualquer (`parser_islands.c:136`, `:218`, `:770`, `:895`; `tool.c:163-279`;
`instances.c:108`). Nenhum diz o valor do limite nem a configuração.

Reprodução do caso dos binders (`/tmp/limites/bind9`):

    module bind9 dim A, B, C, D, E, F, G, H, I;
    bind9.k:1:1: error: insufficient storage while collecting module `bind9` [implementation-limit]

O limite é 8 **por lista** (`dim`, `tags`, `type`; 5 `dim` mais 4 `type` passam),
e nem a lista nem o 8 aparecem.

Os limites com mensagem própria já são assim e servem de modelo:
`tags-limit-exceeded`, o `LINE_CAP` ("a generated line needs more than
LINE_CAP = 1024 bytes … break that source line"), a arena ("arena capacity
exhausted allocating … configured limit … adjust CGEN_ARENA_CAPACITY"),
`forwarded arguments limit exceeded: requested N, configured limit 256; adjust …`.

## Análise

1. É conserto de mensagem, não de linguagem: a spec já pede o conteúdo. Não muda
   normativo.
2. Cada ponto de emissão conhece o que estourou (a lista, o limite). Falta o
   catálogo aceitar o texto.
3. Alguns limites são só de compilação (`-D CGEN_…`), e os demais são
   constantes sem opção; a mensagem precisa dizer qual é qual ("fixo nesta
   compilação; recompile o cgen com …" ou "não ajustável").

## Caminho proposto

- Trocar o texto do catálogo para `implementation limit: %s`, e fazer cada ponto
  passar uma frase completa no molde: *<o que> passa de <valor> (<onde>);
  <como sair>*. Exemplos:
  - binders: ``module `m` has more than 8 `dim` binders: the cgen holds 8 per
    list (`dim`, `tags`, `type`); split the module``;
  - locais: ``more than 128 names declared in one file …``.
- Um teste de mensagem por ponto, com o esperado escrito à mão
  (`test/cgen/cli/`), e a linha do `diag-messages.md` ajustada.
- Pontos a cobrir (levantados pela auditoria): binders, delimitadores,
  `K_LOCALS_MAX`, fechamento (`pool`, `closure`), ilhas (`isl_emit`), instâncias
  (`instances.c:108`), `tool.c` (as fases de carga).

## Decisão pendente

Se o trabalho entra na 0.4.0 (é pequeno e mexe só em mensagens) ou fica para
depois do `name-too-long` (E01), que pede texto normativo.
