# T11 — WP1: formas de declaração de nível de arquivo

## Objetivo

O emissor recusa (`implementation-limit`) declarações válidas de nível de arquivo. Faça-o aceitar:

1. `[pub|priv] struct NOME { … };` e `[pub|priv] union NOME { … };` (a produção `decl-struct` da spec, sem `typedef`);
2. `typedef struct NOME NOME;` (declaração adiantada) seguida de `struct NOME { … };`;
3. `typedef struct { … } P, *PP;` — **vários declaradores** num `typedef`;
4. `import_c "arquivo.h";` — a forma com aspas (hoje só `<…>`);
5. `pub int x = 1, y = 2;` — **vários declaradores** numa variável.

O oráculo são dois casos escritos à mão, que **não** são para você editar:

- `golden/cases/038-decl-forms` (formas 1, 2, 3 e 5; leia `app/dc.k` e todo `expected/c23/app/`);
- `test/cgen/gen/quoted` (forma 4; `q.k` e `expected/c23/`, `expected/c11/`).

`golden/run.sh` já passa neles (o esperado compila e roda); falta o cgen produzi-los, e
`sh test/cgen/gen_golden.sh` os mostra como `wip`.

## Arquivos

Pode editar: `tools/cgen/src/engine/emit/emit.c` e `emit_internal.h`, e, só se for inevitável, outros
`.c` de `tools/cgen/src/engine/emit/`. Se o parser recusar algo, pare e reporte (o `--stop-after=parse`
já reconhece todas essas formas). Pode apagar o campo `|wip` das linhas `038-decl-forms` e
`test/cgen/gen/quoted` de `test/cgen/gen/golden-cases.txt`, quando fecharem. Pode criar
`tools/agents/reports/T11-declaracoes.md`.

Não toque: normativos (`keel-*.md`, `cgen-tool-spec.md`), `golden/**/expected/**`,
`test/cgen/gen/quoted/expected/**`, nenhum `proof.c`, nenhum outro caso. Não faça commit.

## Contrato

A saída por forma (os números de linha são do `dc.k`; o esperado vale mais que esta descrição):

- `pub struct Box { int a; };` → `struct app_dc_Box { int a; };` no `.type.h`; a tag é o nome do
  módulo mais o nome (`M_NOME`, backend §2.3), e os usos `struct Box b` → `struct app_dc_Box b` (a
  ilha de tipo `Box → app_dc_Box` já existe no parser: veja `--stop-after=parse`). `union` igual.
  `priv` vai ao `.type.h` do mesmo jeito (D29).
- `typedef struct Node Node;` → `typedef struct app_dc_Node app_dc_Node;` (os dois nomes renomeados).
- `typedef struct { int a; } P, *PP;` → `typedef struct app_dc_P { int a; } app_dc_P, *app_dc_PP;` — a
  tag é a do **primeiro** declarador (como `typedef struct {…} cursor;` de 019 recebe `lst_cursor`);
  cada declarador renomeado.
- `import_c "string.h";` → `#include "string.h"` no `.type.h`, como `<…>` hoje, com o `#line`.
- `pub int x = 1, y = 2;` → `.h`: `extern int app_dc_x, app_dc_y;` (cada declarador sem o inicializador);
  `.c`: `int app_dc_x = 1, app_dc_y = 2;`. Os usos `x` e `y` no corpo são renomeados como os de
  qualquer variável.

Código existente que você vai tocar (leia no arquivo; as linhas mudam):

- `supported()` em `emit.c`: os `case K_AST_TYPE`, `K_AST_IMPORT_C` e `K_AST_VARIABLE` são as recusas.
  O `K_AST_TYPE` exige `typedef` e um declarador por nó (`first == first do nó anterior`); o
  `K_AST_IMPORT_C` exige o `<`; o `K_AST_VARIABLE` recusa a vírgula de nível 0.
- `node_edits()`: o ramo `K_AST_TYPE` (renome da tag e dos declaradores, `EDIT_TAG`).
- `emit_extern()`: corta o inicializador no primeiro `=` de nível 0; com vários declaradores, é preciso
  cortar o de cada um.
- `emit_import()`: o `#include`.
- O parser já cria um nó (`decl pub type Box`) por declarador (`P` e `PP` são dois nós que dividem o
  mesmo `first`): use isso para renomear cada nome e para escrever a linha uma vez só.

## Passos sugeridos

1. `sh test/cgen/gen_golden.sh`: veja os dois `wip`. Gere a saída do cgen numa pasta temporária e compare
   com o esperado — a diferença é o seu trabalho. **Não copie a saída para o esperado.**
2. Uma forma por vez; a cada passo `make -C tools/cgen -j8` e `sh test/cgen/gen_golden.sh` inteiro (uma
   mudança em `supported()` ou em `node_edits()` mexe em todos os casos).
3. Rode `sh tools/agents/survey/run.sh`: d01–d07 devem ir a `OK` (d08 e d09 ficam `LIMIT`: o
   `extern_c` plano e o `priv` esperam uma regra do André).

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # "46 ok, 0 fail, 0 wip"
golden/run.sh                   # "72 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
sh tools/agents/survey/run.sh   # d01..d07 com OK nos dois perfis
```

Se algum caso que passava deixar de passar, o defeito é seu; não altere oráculo nem baseline. Se
você achar que o esperado está errado, pare e diga no relatório, com a evidência.

## Relatório

`tools/agents/reports/T11-declaracoes.md`, em pt-BR: o que mudou, arquivo por arquivo; a saída final
exata dos quatro comandos; toda dúvida ou incoerência (cartão × código × spec).
