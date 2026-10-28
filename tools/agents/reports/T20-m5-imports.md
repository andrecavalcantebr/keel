# T20 — relatório: o driver escreve os headers dos módulos importados

Estado: **não está ALL OK**. Os oráculos do caso `imports` passam, mas
`make -C tools/cgen check` falha em um teste unitário que afirma o contrato
antigo (um `import` tem de ser recusado). Esse teste não está na lista de
arquivos do cartão, e não o alterei. Decisão necessária do orquestrador; ver
"Pontos em aberto".

## Preparação

- O worktree estava no commit `f6aa1d5`, sem o oráculo. Recebi a mensagem do
  coordenador e fiz `git merge master` (fast-forward para `bbe5be1`). Depois
  disso `test/cgen/gen/imports` e `tools/agents/tasks/T20-m5-imports.md`
  existem.
- Nenhum commit. O diff está só no worktree.

## Edições (arquivo por arquivo)

### `tools/cgen/src/engine/emit/emit.c`

- Predicados novos, `static`, depois de `EM_DASH`:
  - `module_is_generic(ast)`: o nó `K_AST_MODULE` tem `dim_*`, `tags_*` ou
    `type_*` não vazios.
  - `names_binder_or_modifier(ast, t)`: o token `t` é um identificador que
    escreve um binder do módulo (intervalos `type_*`, `dim_*`, `tags_*`) ou o
    nome de um `K_AST_MODIFIER`.
  - `generic_out(ast, n)`: no genérico, o nó é `K_AST_MODIFIER`, ou algum
    token identificador de `[n->first, n->end)` cita binder ou modificador.
    `K_AST_MODULE` e `K_AST_IMPORT` nunca saem. Sem binders, nada sai.
  - `island_out(ast, is)`: a âncora da ilha cai num nó que sai.
- Filtro aplicado: `type_h_body`, `h_body`, `type_h_has_content`,
  `h_has_content` (a linha de conteúdo do cabeçalho depende do que sobra),
  `islands_supported` (ilhas dentro de nó que sai não são julgadas) e
  `supported()` (um nó que sai é pulado antes do `switch`).
- `supported()`: `case K_AST_IMPORT:` aceito, sem emitir nada.

### `tools/cgen/src/tool/stop_gen.c`

- `source_path(name, buf, cap)`: nome com `/` no lugar de `.` mais `.k`. O
  caminho do módulo compilado passou a usar este helper (antes era um loop
  inline, mesmo resultado).
- `emit_imports(arena, tool, dest, m, &written, opt)`: percorre os
  `K_AST_IMPORT` de `m->ast`, obtém cada `KModule` por `cgen_load` (o mesmo
  caminho de `tool.c:214`), pula os já escritos (lista `Written`, cap
  `WRITTEN_CAP` = 1024, com `implementation-limit` se estourar), escreve
  `.type.h` e `.h` e recursa. Nunca escreve `.c`.
- `Written` começa com o prelúdio e o módulo compilado, então `keel` e o
  próprio módulo nunca são reescritos.
- O bloco do módulo compilado chama `emit_imports` depois do `.c`.

Nenhum outro arquivo foi tocado. `emit_internal.h` não precisou mudar, porque
os predicados ficaram `static` em `emit.c`.

## Saídas

Build limpo (`make -s -C tools/cgen clean; make -C tools/cgen -j8`): rc 0,
2 avisos em `src/tool/main.c` (`f_flag`, `pedantic_names`, já existentes),
baseline 4. Aprovado pelo critério do `verify.sh`.

Oráculo do caso (saída do cgen, comparada por `cmp` com
`test/cgen/gen/imports/expected/`):

```
$ tools/cgen/cgen -std=c23 --base-dir base --stop-after=gen -I test/cgen/gen/imports --dest-dir /tmp/t20 test/cgen/gen/imports/use.k
rc=0
/tmp/t20/coll.h  coll.type.h  deep.h  deep.type.h  keel.h  keel.type.h  lib.h  lib.type.h  use.c  use.h  use.type.h
same c23 lib.type.h / lib.h / deep.type.h / deep.h / coll.type.h / coll.h
```

Em C11 (`-std=c11`), os seis também batem byte a byte.

`sh test/cgen/gen_golden.sh`:

```
wip    003-alignment
wip    006-dim
wip    007-instance
12 ok, 0 fail, 3 wip
```

`imports` não aparece como `wip`. A linha da tabela continua com o `wip`
(o orquestrador tira).

Casos fora do oráculo, em `/tmp/t20e/`:

- `use2.k` (importa `lib` e `deep`, e `lib` importa `deep`): rc 0, `deep.h` e
  `deep.type.h` escritos uma vez.
- `bad.k` (importa `nope`): `bad.k:2:1: error: module 'nope' was not found in
  the source roots [module-not-found]`, rc 1, como antes.

`golden/run.sh`, rodado a partir de `golden/`:

```
46 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
```

`make -C tools/cgen check`: 47 linhas `ok`, e uma falha:

```
FAIL   unit/emit/emit
       FAIL: an explicit import must be K_EMIT_UNSUPPORTED
       FAIL: timed out or crashed (exit 1)
make: *** [Makefile:43: check] Erro 1
```

`verify.sh` não foi rodado como script inteiro: o guarda do worktree recusou
`sh tools/agents/verify.sh` (o script é uma construção que ele não consegue
verificar). Rodei os passos dele separadamente, na mesma ordem: build, check,
golden. O resultado é o acima: falha só no `check`.

## Pontos em aberto

1. **Teste unitário com o contrato antigo** (decisão para o orquestrador).
   `test/cgen/unit/emit/emit_main.c:87-93` afirma que
   `k_emit_file(... "module m;\nimport foo.bar;\n" ..., K_EMIT_TYPE_H)` é
   `K_EMIT_UNSUPPORTED`. O cartão, passo 1, manda aceitar `K_AST_IMPORT`, então
   essa afirmação deixa de valer. Não a alterei: o arquivo não está na lista do
   cartão, e alterar um teste para passar é o que a regra proíbe. Opções: (a)
   trocar a afirmação por `!= K_EMIT_UNSUPPORTED` para o `import`, e manter a
   recusa com uma construção que continue fora da fatia; ou (b) autorizar a
   edição no cartão. Com a escolha feita, a aceitação fecha em ALL OK.
2. **Custo.** `generic_out` e `names_binder_or_modifier` varrem tokens por
   nó, e `island_out` varre nós por ilha. Isso só acontece com módulo genérico.
   É O(tokens × nós) por passada. Para os módulos de hoje é desprezível; não
   medi em módulo grande.
3. **Recorte léxico.** A partição do §4.4.1 é feita por nome: um nó que cita
   `T` ou `stack` sai, mesmo que a citação seja só de tipo. É a regra do
   backend; não há caso no oráculo que a contrarie.
4. **`use.*` do módulo compilado** continua sem `#include` de `lib` e `coll`
   (é o T23). A tarefa não o exige.
5. **Modificador de módulo não genérico** continua recusado (o `default` de
   `supported()`), como antes.
