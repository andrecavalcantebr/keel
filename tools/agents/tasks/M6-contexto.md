# M6 — contexto comum dos cartões M6a, M6b e M6c

O emissor do cgen (`tools/cgen/src/engine/emit/`) baixa cada construção da linguagem num
arquivo próprio (codegen design E5). Cada arquivo exporta duas funções, já declaradas em
`emit_internal.h` e já chamadas pelo `emit.c`:

```c
/* the edits this construction makes inside the function `fn`, in the source bytes [a, b)
   (one source line), appended to `ed`; cnt counts them */
void k_em_edits_X(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
/* whether this construction lowers this island (its kind AND its detail) */
bool k_em_handles_X(const KAst *ast, const KIsland *is);
```

`emit.c` aceita o módulo só se **toda** ilha for aceita por algum `k_em_handles_*`
(`islands_supported`, emit.c). Uma ilha recusada vira `implementation-limit` e o caso
fica `wip`. Leia `emit_internal.h` inteiro: o `Edit` (DROP, TEXT, TOKS, LINES…), o
`k_em_push_edit`, o `k_em_scratch` (texto das edições), o `k_em_put_range` (cópia de
tokens com renomes, usada pelo `EDIT_TOKS`).

**Arquivos que servem de modelo** (leia antes de escrever): `index.c` (índice sobre
`array`, com `keel_index`), `foreach.c` (o `foreach` já baixado), `defer.c` (o maior, com
`EDIT_LINES`), `array.c` (o menor).

**As ilhas** vêm do parser, já resolvidas: `cgen --stop-after=parse` imprime uma por
linha (`ilha <espécie> <detalhe> <posição>`). O detalhe traz os símbolos C prontos — você
não resolve nome nenhum, só usa o que a ilha diz. Para ver as ilhas de um caso:

```sh
cd golden/cases/<caso> && ../../../tools/cgen/cgen -std=c23 --base-dir ../../../base -I . --stop-after=parse <fonte.k> | grep '^ilha'
```

**O oráculo** são os `expected/{c23,c11}/` do caso golden (escritos à mão; a emissão segue
o `.k`). Você **não edita** nada em `golden/`, nem a tabela `test/cgen/gen/golden-cases.txt`.
O caso está na tabela com o quarto campo `wip`: `sh test/cgen/gen_golden.sh` imprime `wip`
enquanto a saída difere e nada quando bate. Para ver a diferença de um caso:

```sh
cd golden/cases/<caso> && rm -rf /tmp/o && ../../../tools/cgen/cgen -std=c23 --base-dir ../../../base -I . --stop-after=gen --dest-dir /tmp/o <fonte.k>; diff -r expected/c23 /tmp/o | head -40
```

(arquivos que o cgen escreve além dos do `expected/` não contam). Repita com `-std=c11`
e `expected/c11`.

Se você achar que o esperado está errado (não segue o `.k`, ou contradiz o backend), **pare
e diga no relatório**, com a evidência: o golden às vezes tem erro escrito à mão.

**Aceitação comum:** `sh tools/agents/verify.sh` termina em `ALL OK`, e o seu caso some da
lista `wip` do `gen_golden.sh` nos dois perfis. Nenhum outro caso pode passar a falhar.
