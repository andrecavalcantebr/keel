# T12 — `engine/emit/emit.c` em módulos (refactor, sem mudança de comportamento)

## Objetivo

O `emit.c` (≈700 linhas) vai ganhar o lowering de corpo de função (`array`,
`foreach`, índice verificado, `constexpr` de bloco). Pelo desenho (codegen
[E5]: um `.c` por funcionalidade) cada construção mora num arquivo. Esta
tarefa **só prepara o terreno**: parte o `emit.c`, cria os quatro arquivos de
construção **vazios** (corpos que não fazem nada) e os liga por ganchos. Depois
dela, quatro agentes preenchem um arquivo cada, em paralelo, sem tocar no
`emit.c`. **O comportamento não muda**: a saída de `cgen --stop-after=gen` é
byte a byte a de hoje, e a suíte inteira continua verde.

## Arquivos

Pode criar: `tools/cgen/src/engine/emit/emit_internal.h`, `emit_core.c`,
`array.c`, `foreach.c`, `constexpr_block.c`, `index.c` (todos em
`tools/cgen/src/engine/emit/`), e o relatório
`tools/agents/reports/T12-emit-split.md`.
Pode editar: `tools/cgen/src/engine/emit/emit.c`.
Nada mais: nem testes, nem golden, nem normativos, nem o `Makefile` (ele acha os
`.c` por `find`). Não commite.

Regras do motor (`make boundary`): nada de `<stdio.h>`, `<stdlib.h>`,
`<unistd.h>`, `<fcntl.h>`, `<sys/*.h>`; sem alocação. Nomes e comentários em
inglês, no estilo do `emit.c`.

## Antes de editar

```sh
sh tools/agents/verify.sh        # -> ALL OK (a linha de base)
mkdir -p /tmp/t12-before && for p in c23 c11; do
  for d in test/cgen/gen/hello test/cgen/gen/shapes test/cgen/gen/tags; do
    m=$(ls $d/*.k | head -1)
    tools/cgen/cgen -std=$p --base-dir base --stop-after=gen -I $d \
      --dest-dir /tmp/t12-before/$p/$(basename $d) $m >/dev/null 2>&1
  done; done
```

(É o seu instantâneo da saída; depois do refactor repita para `/tmp/t12-after`
e `diff -r` não pode achar diferença.)

## Contrato

### `emit_internal.h`

Move para cá, **sem mudar o conteúdo**, tudo que hoje é privado do `emit.c` e
que os arquivos de construção vão precisar:

- os `#define` (`NAME_CAP`, `LINE_CAP`, `MAX_EDITS`), `EditKind`, `Edit`, e a
  struct `Emit` (acrescente o campo `size_t block_ordinal;`, inicializado em
  `0` em `k_emit_file`: é o contador de `constexpr` de bloco, do módulo todo);
- as declarações das funções-ajuda, que saem de `static` e ganham o prefixo
  `k_em_`: `tok`, `tok_off`, `tok_end`, `first_off`, `last_end`, `text_is`,
  `line_start`, `line_end`, `line_at`, `put_bytes`, `put_mangled`, `push_edit`,
  `declared_at`, `is_keel`, `is_visibility`. Ex.: `k_em_tok(ast, i)`,
  `k_em_push_edit(e, ed, cnt, x)`. As definições ficam em `emit_core.c`.
  Funções que só o `emit.c` usa continuam `static` lá.
- `k_em_sort_edits(Edit *ed, size_t from, size_t cnt)`: a ordenação por
  inserção que hoje está no fim de `comment_edits` (ordena `ed[from..cnt)` para
  dentro do prefixo já ordenado). `comment_edits` passa a chamá-la.

### Os quatro arquivos de construção (vazios)

Cada um inclui `emit_internal.h` e define, com o corpo **vazio** ou a resposta
neutra abaixo, as funções (troque `X` por `array`, `foreach`, `constblock`,
`index`):

```c
/* edits this construction makes inside the function `fn`, in source [a, b) */
void k_em_edits_X(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
/* whether this construction handles the island kind (so supported() accepts it) */
bool k_em_handles_X(KIslandKind kind);                 /* neutra: return false; */
```

`constexpr_block.c` tem, além dessas, duas funções de linhas inteiras:

```c
/* whole lines written before / after the source line `ln` (bytes [a, b)) */
void k_em_pre_lines_constblock(Emit *e, size_t ln, size_t a, size_t b);    /* vazio */
void k_em_post_lines_constblock(Emit *e, size_t ln, size_t a, size_t b);   /* vazio */
/* false when `fn` has a block constexpr this profile cannot emit yet */
bool k_em_supported_constblock(const Emit *e, const KAstNode *fn);
/* neutra HOJE: return true for C23 (verbatim); for C11 return false if the
 * body of `fn` has a `constexpr` token, true otherwise */
```

Os protótipos vão no `emit_internal.h`.

### Os ganchos no `emit.c`

1. Em `node_edits`, no ramo `value` (declaração com `declares_value`), logo
   depois do laço que empilha os renomes, **só quando `n->kind ==
   K_AST_FUNCTION`**, chame, nesta ordem, `k_em_edits_array`,
   `k_em_edits_foreach`, `k_em_edits_constblock`, `k_em_edits_index` (sempre
   com `a`, `b`, `ed`, `cnt` do chamador) e então
   `k_em_sort_edits(ed, antes, *cnt)`, onde `antes` é o `*cnt` que havia
   **antes** das quatro chamadas (as edições novas entram ordenadas entre as
   antigas).
2. Em `emit_line`, antes de escrever o texto da linha chame
   `k_em_pre_lines_constblock(e, ln, a, b)` e depois
   `k_em_post_lines_constblock(e, ln, a, b)` (as duas só para linhas
   dentro de função; hoje `emit_line` serve a todas as linhas, e isso é
   inofensivo porque os corpos são vazios). Atenção: `emit_line` tem um
   atalho que escreve a linha direto quando não há edição; os dois ganchos
   envolvem **os dois caminhos**.
3. Em `islands_supported`: a lista de espécies aceitas hoje é
   `TYPE`, `NAME`, `CALL`. Passe a aceitar também a espécie `K` quando
   `k_em_handles_array(K) || k_em_handles_foreach(K) || k_em_handles_index(K)
   || k_em_handles_constblock(K)`. A verificação do detalhe `A → B` (que
   exige a seta) só vale para `TYPE`, `NAME` e `CALL`; para as espécies
   delegadas, pule-a (o arquivo da construção responde por ela).
4. Em `supported()`, no ramo `K_AST_FUNCTION`, acrescente
   `if (!k_em_supported_constblock(e, n)) return false;` (a função hoje só
   recebe `const KAst *ast`: passe o `Emit` ou o `KEmitOptions`, como achar
   mais simples; o que importa é que a resposta de C11 chegue lá).

## Passos sugeridos

1. Instantâneo (acima).
2. Crie `emit_internal.h` e `emit_core.c` movendo as funções; compile; rode o
   `diff -r` do instantâneo.
3. Crie os quatro arquivos vazios e os ganchos; compile; `diff -r`.
4. `sh tools/agents/verify.sh`.

## Aceitação

```sh
sh tools/agents/verify.sh            # -> ALL OK, sem aviso novo
# instantâneo depois, mesmo laço do "Antes de editar", para /tmp/t12-after
diff -r /tmp/t12-before /tmp/t12-after && echo identical      # -> identical
wc -l tools/cgen/src/engine/emit/*.c                          # emit.c bem menor que antes
```

## Relatório

`tools/agents/reports/T12-emit-split.md`, em pt-BR: arquivo por arquivo o que
saiu do `emit.c` e para onde; a saída colada de `verify.sh` e do `diff -r`; as
dúvidas (por exemplo, uma função que não decidiu se movia).
