# T30-defer-return-impl — relatório

## O que mudou

Arquivos tocados (nada mais; sem `git add`, sem commit):

1. `tools/cgen/src/engine/emit/defer.c`
   - `block_kind`: o `{` de um `foreach (...)` passa a contar como corpo de laço (kind 1). Antes, `break` dentro de `foreach` não achava o laço (`exit_target` devolvia NO_TOKEN) e `plan_of` recusava o módulo com `implementation-limit`.
   - `plan_of`: saiu a recusa de `[` na expressão de um `return` inline. A recusa de comentário e de linha quebrada continua.
   - Novas `put_span` e `put_expr`: reconstroem a expressão de um `return` inline (`tokens [from, to)`) com os índices verificados. `put_expr` calcula as arestas de índice com `k_em_edits_index` sobre o intervalo da expressão, ordena com `k_em_sort_edits` e aplica cada aresta na fronteira de token; os trechos entre arestas saem por `k_em_put_range` (renomeações como antes) e por `k_em_put_source` (espaços). `inline_exit` usa `put_expr` onde antes usava `k_em_put_range(e, r + 1, semi)`.
   - Comentários do cabeçalho e de `inline_exit` atualizados para refletir isso.
2. `test/cgen/gen/golden-cases.txt`: só o `|wip` da linha `055-defer-return-index` foi apagado.

Nenhum teste de `unit/` ou `cli/` contradisse o cartão; não houve patch a guardar.

## Oráculo (conferido antes de editar)

Li `golden/cases/055-defer-return-index` contra `keel-c-backend.md` §5.5 (regras 4, 5 e 6) e §5.5.2 (regras 1, 2 e 6). O esperado está de acordo com a spec: o valor vai para `keel__rv0` antes do cleanup; o índice variável passa por `keel_index(i, 3)`; `break` sai com `{ <cleanup> break; }`; o cleanup de bloco interno sai no `}`; os `#line` seguem o contador. Não encontrei divergência. Não rodei `tools/cgen` sobre o caso para escrevê-lo, e o oráculo não foi alterado.

Conferência byte a byte: a saída do cgen para `app/dr2.k` (scratchpad) bate com `expected/c23/app/` e `expected/c11/app/` (`diff -r`, sem diferença).

## Saída dos comandos de aceitação

- `sh test/cgen/gen_golden.sh` (último trecho): `62 ok, 0 fail, 1 wip` (o wip é `056-length-fields`, de outro cartão).
- `golden/run.sh`: `106 ok, 0 fail, 0 xfail, 0 xpass, 3 wip`
- `sh tools/agents/verify.sh`: `ok   build (0 warnings, baseline 0)`, `ok   make check (53 lines ok)`, `ok   golden: 106 ok, 0 fail, 0 xfail, 0 xpass, 3 wip`, `ALL OK`.

Depois do `verify.sh`, fiz só uma reflowagem de dois comentários em `defer.c`. Refiz o build (0 warnings) e o `gen_golden.sh` (`62 ok, 0 fail, 1 wip`), mas não rodei o `verify.sh` de novo. A mudança é só de comentário.

## Dúvidas e achados

1. **Pré-existente, fora do cartão: o cleanup de `defer` não passa por `keel_index`.** Sonda em scratchpad (`defer v[i] = 0;` com `size_t i`) sai como `v[i] = 0;`, sem verificação. `put_body` copia o corpo com `k_em_put_range`, sem as arestas de índice. Viola `keel-c-backend.md` §5.3, regra 4. A correção usaria o mesmo mecanismo de `put_expr` em `put_body`, mas o `put_body` escreve por registro (uma linha por linha do corpo). Não mexi nisso. Precisa de decisão sua e de um oráculo próprio.
2. `put_expr` também passa a valer no caminho `to_ladder` com `inner_pending` (um `return` que também sai de um `defer` de bloco interno numa função em escada). Ele não tem caso de golden próprio. Só o caso 055 cobre o inline; a escada com índice segue como estava.
3. A mensagem `implementation-limit` de `plan_of` sai genérica (`stop_gen.c:78`, "the emitter does not cover this module's constructions yet"), sem dizer qual construção recusou. Não é deste cartão, mas dificulta achar a causa.
