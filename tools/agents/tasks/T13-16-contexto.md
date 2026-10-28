# T13–T16 — contexto comum: o lowering de corpo no emissor

Leia isto antes do seu cartão (T13 `array`, T14 `constexpr` de bloco, T15
`foreach`, T16 índice verificado). Quatro agentes trabalham ao mesmo tempo, cada
um num worktree próprio e **num arquivo seu**; quem integra é o orquestrador.

## O que o emissor faz hoje

`k_emit_file` (`tools/cgen/src/engine/emit/emit.c`) escreve um arquivo gerado por
chamada. O `.c` é o fonte linha a linha: cada linha passa por `emit_line`, que
junta as **edições** dos nós que a tocam, copia o texto da linha aplicando-as
(`copy_edited`) e entrega ao `KWriter`, o único que escreve `#line`. Uma
edição troca os bytes `[at, at+skip)` do fonte por outra coisa. Os nós de
função empilham as suas em `node_edits`, e depois chamam os quatro ganchos de
construção, **na ordem** array, foreach, constblock, index (cada um acrescenta
ao vetor `ed`; o emissor ordena por posição, e numa mesma posição uma inserção,
`skip == 0`, vem antes de uma substituição).

Os ganchos vazios que você vai preencher estão em
`tools/cgen/src/engine/emit/{array,foreach,constexpr_block,index}.c`. A API que
você usa está em `emit_internal.h` (leia-o inteiro). Em resumo:

```c
typedef struct {            /* uma edição: troca os bytes [at, at+skip) do fonte */
    EditKind kind; size_t at, skip; size_t name_tok; bool is_type;
    const char *text; size_t text_len;   /* EDIT_TEXT: texto literal */
    size_t tok_a, tok_b;                 /* EDIT_TOKS: tokens [tok_a, tok_b) copiados, com os renomes */
} Edit;
/* EDIT_DROP apaga; EDIT_TEXT escreve `text`; EDIT_TOKS copia tokens do fonte
   (com os nomes renomeados como no C gerado); os outros são dos nós de tipo/tags */

void k_em_push_edit(Emit *e, Edit *ed, size_t *cnt, Edit x);   /* empilha (e->ok = false se encher) */
char *k_em_scratch(Emit *e, size_t n);  /* n bytes de texto para uma edição; vale até a linha ser copiada */
KToken k_em_tok(const KAst *ast, size_t i);                    /* token i: ptr/len no fonte */
size_t k_em_tok_off(const KAst *ast, size_t i), k_em_tok_end(const KAst *ast, size_t i);
bool k_em_text_is(KToken t, const char *s);
```

`node_edits` chama o seu gancho assim, para cada nó **função**, com `[a, b)` o
intervalo de bytes **da linha que está sendo escrita** (ou da assinatura, para o
protótipo do `.h`):

```c
void k_em_edits_X(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
```

Você percorre os **tokens** de `fn` (`fn->first` até `fn->end`; o corpo começa em
`fn->body_first`, o `{` é `body_first - 1`, o `}` final é `fn->body_end`) e
empilha só as edições cujo `at` caia em `[a, b)`. O gancho roda **uma vez por
linha**, então calcule o que precisar a partir dos tokens, sem estado entre
chamadas (se precisar de um contador, derive-o contando tokens desde o começo do
módulo). O `KAst` (`ast->tokens`, `ast->nodes`, `ast->islands`) está em
`engine/ast.h` e `engine/storage_types.h`. **Ilhas** são o que a passagem 3 do
parser reconhece: `ast->islands`, cada uma com `kind`, `anchor` (índice de token)
e `text` (o detalhe, em `ast->island_text`). Para ver o que o parser entrega para
um `.k`: `tools/cgen/cgen --stop-after=parse -I <dir> --base-dir base <arquivo.k>`.

`k_em_handles_X(ast, ilha)` diz ao emissor que a sua construção cobre aquela
ilha (se nenhuma construção cobre, o módulo é recusado com
`implementation-limit`, em vez de sair errado). Responda `true` **só** para as
formas que o seu lowering de fato trata; as outras devem continuar recusadas.

## Regras de ouro

- **Edição mínima de token.** O texto do autor passa como foi escrito (espaços,
  comentários saem pelo mecanismo próprio); você troca só as palavras que o
  backend manda trocar. Não reconstrua a linha.
- O oráculo do seu cartão é **escrito à mão** e fica em `test/cgen/gen/<dir>/`;
  você **não o altera**. Se achar que ele está errado, pare e diga, com a
  evidência (o backend §…, a linha do golden).
- `sh test/cgen/gen_golden.sh` imprime `ok`/`wip`/`FAIL` por caso; o seu caso
  tem de sair `ok`, e os que já eram `ok` continuam `ok`.
- Você toca **no seu arquivo** (e no relatório). Não edite `emit.c`,
  `emit_core.c`, `emit_internal.h`: se faltar algo na API, registre no relatório
  e contorne no seu arquivo. Nada de normativo, golden, outros oráculos.
- Motor (`make boundary`): sem `<stdio.h>`, `<stdlib.h>`, `<unistd.h>`,
  `<fcntl.h>`, `<sys/*.h>`; sem alocação. Inglês no código; pt-BR no relatório.
- Não commite. Termine com `sh tools/agents/verify.sh` → `ALL OK` (sem aviso novo)
  e **escreva o relatório** `tools/agents/reports/<id>.md`: arquivo por arquivo,
  a saída colada do `gen_golden.sh` e do `verify.sh`, e toda dúvida.
- Meça antes de supor: imprima o que o parser dá (tokens, ilhas, intervalos) num
  programa de três linhas em `/tmp` ou pelo `--stop-after=parse`.

## Backend, o que importa

`keel-c-backend.md` §5.1 (declarações e `array`), §5.3 (indexação), §5.7
(`foreach`), §9.2 (`constexpr` sob C11). O golden `golden/cases/008-block-constexpr`
(`app/cx.k` e `expected/<perfil>/app/app_cx.c`) usa as quatro construções juntas,
e é o teste de integração que o orquestrador roda depois.
