<!--
   FILE: T44-type-arg-foreign.md
   DESC: cartão-registro — novo diagnóstico type-arg-foreign (spec §4.4,
         regra 20), no lugar do implementation-limit genérico.
   AUTHOR: Claude Opus 5.5 (orquestrador)
   DATE: Nov, 2026
-->

# T44 — `type-arg-foreign`: diagnóstico próprio para argumento de tipo apagado de outro módulo

## Origem

Continuação da investigação do T43 (`../keel_maths`). O André apontou duas
coisas: (1) a linguagem já tem uma funcionalidade planejada — o **molde**
(`keel_code`, `design/mold-impacto.md`/`design/possibilidades.md`) — para o
que o `keel_maths` tentava fazer com `zeros(matrix(2,3))`; (2) o
`implementation-limit` que a ferramenta dava para esse caso estava
semanticamente errado: `cgen-tool-spec.md` diz que esse diagnóstico "não
implica erro de sintaxe do programa" — mas aqui implica. Pediu diagnóstico
dedicado e documentação objetiva no `keel-llm-context.md`, para não perder
essas distinções de novo.

## O que já estava certo, só sem nome

A regra 12 da §4.4 (preexistente) já proíbe seleção num módulo `dim`/`tags`
sem `type`: `type M` nesse caso é sempre **apagado** (regra 13), nunca
seleciona dimensão. `from-without-target` já cobre "sem alvo nenhum". O que
faltava: quando HÁ alvo (a dimensão resolve certo) e o argumento apagado é
de outro módulo — caía no `implementation-limit` da ferramenta
(`uses.c:foreign_in`, dentro de `k_em_handles_use`), sem posição própria,
misturado com a lógica geral de uso.

## O conserto

1. **Normativo** (`keel-spec.md` §4.4): regra 20 — "O tipo escrito num
   argumento de parâmetro apagado (regra 13) não é de outro módulo. A
   violação é `type-arg-foreign`." Entrada no catálogo (§6.2).
2. **`tools/cgen/diag-messages.md`**: mensagem em inglês; `gen-diags.py`
   regenerou `diag_catalog.def`/`diag_messages.def`.
3. **`design/keel-llm-context.md`**: parágrafo novo em "Regras que a
   gramática não mostra", distinguindo seleção (regra 11, nome bate o
   binder `type` da linha `module`) de apagamento (regra 13, resto),
   destacando a regra 12 (é a causa raiz do que o `keel_maths` tentava), e
   apontando o molde como o mecanismo planejado — não implementado — para
   especialização por `dim`/`type` como argumento.
4. **`parser_islands_types.c`**: `erased_type_arg_foreign` (nova), chamada
   no laço de marcas de adaptação (§4.4, item 5), exatamente onde `type:`
   seria marcado como apagado. Dispara `K_DIAG_TYPE_ARG_FOREIGN` com a
   posição e o texto do argumento, e `return -1` (aborta a chamada, como os
   demais erros de §4.4 ali).

   A checagem: argumento qualificado (`pt.point`) → sempre estrangeiro;
   senão, não é estrangeiro se for primitivo, se for um binder `type` da
   PRÓPRIA linha `module` do arquivo (`isl_binder(a, ...)` — é o que
   `slice.k`/`buffer.k` usam o tempo todo, `arena.alloc(a, T, ...)` com `T`
   da própria `module keel.slice type T;`), ou se estiver declarado no
   próprio arquivo.

## Erro cometido e pego antes do commit

A primeira versão não tinha a checagem do binder do PRÓPRIO arquivo —
regrediu `golden`/`base` pesado (49 ok, 27 fail no `gen_golden`; 110 ok, 14
fail no `base`): toda função da base que usa `arena.alloc(a, T, ...)` com o
`T` do seu próprio módulo genérico (slice, buffer) passou a disparar o
diagnóstico à toa. `test/run.sh` pegou antes do commit; corrigido
acrescentando `isl_binder(a, ...)` à lista de "não é estrangeiro".

## Oráculo

`test/cgen/diag/type-arg-foreign/` (`pt.k` + `tf.k`, convenção de
`test/cgen/diag.py`, marca `/* DIAG: ... */`): primitivo e tipo do mesmo
arquivo passam sem diagnóstico; tipo de outro módulo dispara
`type-arg-foreign` na linha certa. Cobertura do catálogo: 137/149.

## Verificação

`test/run.sh` verde: 132 ok golden, 76 ok gen_golden, 124 ok base, 45 ok
docs, 48 casos de diagnóstico.

## Pendência, sem cartão aberto

O `cgen` ainda não implementa o molde (`keel_code`) — é o mecanismo correto
para o que o `keel_maths` precisa (especialização por `dim`/`type` como
argumento, por substituição de fichas). Ver `design/mold-impacto.md` para o
estudo de impacto; nada lá foi tocado por este cartão.
