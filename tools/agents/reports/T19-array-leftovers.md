# T19 — arrays: relatório (BLOQUEADO por um teste que afirma o contrário)

## Resultado

Não passou a aceitação. O código das cinco formas foi escrito e o oráculo
`046-array-leftovers` ficou verde nos dois perfis, mas `make check` falha numa
asserção de `test/cgen/unit/emit/emit_main.c`, que afirma o contrário do que o
cartão pede. Conforme o cartão, não editei esse teste. O patch foi guardado fora
do repositório e a árvore voltou ao HEAD (`git status` limpo).

Patch guardado (fora do repositório):
`/tmp/claude-1000/-home-andre-code-cprojects-keel/6df5c4e9-8d62-4717-9b96-cb209ab7b812/scratchpad/t19/T19-patch.diff`
(inclui o `golden-cases.txt`; sem ele: `.../scratchpad/t19/T19-patch-cgen-only.diff`).

## A asserção que bloqueia

`test/cgen/unit/emit/emit_main.c`, linhas 87-93:

    /* Outside the covered slice: refused, not mistranslated. */
    char *src = "module m;\npub typedef struct { array i32 a[2], b[3]; } T;\n";
    ... if (k_emit_file(...) != K_EMIT_UNSUPPORTED) fprintf(stderr,
        "FAIL: two declarators in one `array` field of a struct must be K_EMIT_UNSUPPORTED\n");

O cartão pede exatamente essa forma (`typedef struct { array i32 a[2], b[3]; } Pair;`
-> `typedef struct app_al_Pair { i32 a[2], b[3]; } app_al_Pair;`). O que o
oráculo 046 espera está em `golden/cases/046-array-leftovers/expected/*/app/app_al.type.h`,
e o cgen com o patch produz exatamente isso. Decisão para o André: trocar o
exemplo da unit por outra forma ainda recusada (o cartão autoriza isso), ou
manter a recusa e tirar a forma 2 do T19.

## O que o patch mudava (por arquivo)

- `engine/emit/subscript.c`: range-index aberto sobre `array` (`v[2..]`, `v[..]`).
  Lowering: `keel_array_i32_as_slice2(v, 6, 2, keel_array_i32_length(v, 6))`.
  O símbolo do length é o prefixo do `as_slice` mais `_length`, derivado no
  emissor. O detalhe do parser traz a palavra `core` (ver abaixo). Helpers
  `is_core`, `length_prefix`.
- `engine/emit/array.c`: removida a recusa de vários declaradores num campo de
  struct (`in_type`, e o `,` depois do declarador). Cada declarador tem as suas
  dimensões, com `][` nas vírgulas.
- `engine/emit/index.c`: `field_decl` e `declarator_named` aceitam o campo que
  é um declarador posterior (`p->b[1]`). Sem isso o `p->b[1]` era recusado.
- `engine/emit/core.c`: (a) `keel.capacity` sobre parâmetro com binder vira
  `keel__n`; (b) `keel.length` sobre parâmetro sem binder vira o produto literal
  (`2 * 3`), com `k_em_array_dim`; capacity e dim sobre ele continuam recusados;
  (c) elemento de tipo do módulo: a recusa `names_declaration` saiu (função
  removida) e o tipo entra como `EDIT_TOKS` depois do `)`, com o renomeio do
  `k_em_put_range`: `sizeof(bs)/sizeof(app_al_Box)`. Comentário do alto atualizado.
- `engine/emit/uses.c`: o include `keel/keel_array_i32.h` (o oráculo o pede).
  A loop de range-index recusava o detalhe `… core dim:1` porque `marks_of3`
  não conhece `core`. Agora a palavra `core` é pulada antes dos marcadores.
- `test/cgen/gen/golden-cases.txt`: apenas o `|wip` da linha 046 apagado.

Não toquei em `parser_islands_index.c`: a asserção de `test/cgen/cli/islands.py:133`
(`fx 3.. → keel_array_f32_as_slice2 core dim:1`) pina o detalhe do parser, e
por isso o emissor deriva o símbolo do length.

## Saída dos comandos de aceitação (com o patch aplicado)

`sh test/cgen/gen_golden.sh`:

    54 ok, 0 fail, 6 wip

(046 saiu de `wip`; os restantes 6 wip são 047-052.)

`golden/run.sh`:

    100 ok, 0 fail, 0 xfail, 0 xpass, 3 wip

(046 c23 e c11 `ok (run)`; os 3 wip são 022-024, como antes.)

`sh tools/agents/verify.sh` (primeira execução, em segundo plano):

    ok   build (0 warnings, baseline 0)
    FAIL make check
    FAIL   unit/emit/emit
    FAIL: two declarators in one `array` field of a struct must be K_EMIT_UNSUPPORTED
    FAIL: timed out or crashed (exit 1)

(A segunda execução foi interrompida, o `make check` falharia do mesmo jeito.)

`sh tools/agents/survey/run.sh` (com o patch):

    r01 [c23 OK] [c11 OK]
    r02 [c23 OK] [c11 OK]
    r03 [c23 OK] [c11 OK]
    r04 [c23 OK] [c11 OK]
    r05 [c23 OK] [c11 OK]
    r06 [c23 OK] [c11 OK]
    (r07-r09 continuam OK)

## Dúvidas e incoerências

1. A asserção da unit contradiz o cartão (acima). É a única falha.
2. O cartão diz que o `#include "keel/keel_array_i32.h"` "já sai". O oráculo o
   espera no `app_al.c`, e foi preciso o ajuste em `uses.c` para ele aparecer.
   Leia isso como "sai do emissor", não como "não deve estar".
3. `keel.capacity` e `keel.dim` sobre parâmetro sem binder continuam recusados:
   o cartão só pede `length`. Não estendi.
4. Em `engine/emit/uses.c`, o `core` pulado é um detalhe textual do parser; se o
   parser mudar o detalhe de `core` para outra palavra, este ajuste precisa mudar junto.
5. Nenhum commit, nenhum `git add`. Árvore em HEAD após a parada.
