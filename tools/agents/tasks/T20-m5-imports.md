# T20 — M5.1: o driver escreve os headers dos módulos importados

Leia `design/cgen-tool.md` §5.3 (a tabela de arquivos), `keel-c-backend.md` §4.3.2
(os três artefatos) e §4.4.1, e `tools/agents/README.md`.

## Objetivo

Hoje `cgen --stop-after=gen` escreve só o prelúdio e o módulo compilado, e o emissor
recusa (`implementation-limit`) qualquer módulo com `import`. Esta tarefa cobre o
primeiro degrau do M5 (base):

1. o emissor aceita `K_AST_IMPORT` (um `import` não emite nada: o `#include` de um
   módulo importado vem do **uso**, backend §4.3.2 regra 4, e isso é o T23);
2. o driver escreve `.type.h` e `.h` de **cada módulo importado**, transitivamente
   (`use` importa `lib`, que importa `deep`: os três), sem repetir um módulo;
3. um módulo **genérico** importado (`module coll type T;`) escreve só o que não
   menciona o parâmetro nem o modificador (backend §4.4.1): aqui, `coll.type.h` com o
   `constexpr` e `coll.h` que só inclui o `.type.h`. O modificador, os verbos
   e tudo o que o cita ficam de fora (são da instância, T21).

Os módulos importados **não** escrevem `.c`.

## Arquivos

Edita `tools/cgen/src/tool/stop_gen.c` e `tools/cgen/src/engine/emit/emit.c` (e
`emit_internal.h` se precisar). Cria `tools/agents/reports/T20-m5-imports.md`. Não
edite o oráculo nem a tabela do `gen_golden.sh`: o orquestrador tira o `wip`.

## Oráculo (escrito à mão; não altere)

`test/cgen/gen/imports/`: `use.k` (importa `lib` e `coll`, não usa nenhum),
`lib.k` (importa `deep`), `deep.k`, `coll.k` (o genérico do caso 007) e
`expected/{c23,c11}/{lib,deep,coll}.{h,type.h}`. Na tabela:
`test/cgen/gen/imports|use.k|-|wip`. Só os arquivos de `expected/` são julgados; o
`use.*` do módulo compilado não é (o que ele inclui é o T23).

## Contrato

- Como o driver já carrega os imports (`cgen_load`, `tool.c:212`: percorre os nós
  `K_AST_IMPORT` e chama `cgen_load(t, span(&e->ast, n->name_first, n->name_end), &m)`),
  use o mesmo caminho para obter o `KModule` (`loader.h`: `->ast`, `->name`) de cada
  import e recursar nos imports dele. O `KModule` do prelúdio `keel` já sai.
- O nome do `.k` para o `#line` e o cabeçalho é o do módulo com `/` no lugar de `.`
  mais `.k` (como o `stop_gen.c` já faz para o módulo compilado); a raiz não entra.
- Um módulo é genérico quando o nó `K_AST_MODULE` tem binders (`type_first !=
  type_end`, ou `dim_*`, ou `tags_*` não vazios; confirme em `parser_header.c`).
  No genérico, o emissor pula toda declaração que cita um binder ou um modificador
  do módulo; na dúvida, recuse (`K_EMIT_UNSUPPORTED`), nunca emita errado.
- Um `import` entre dois módulos não escreve `#include` em lugar nenhum.
- O resto do contrato (nomes, `#line`, guardas) é o do módulo compilado: o
  `lib.h` esperado é o que o cgen já gera para `lib.k` compilado sozinho.

## Passos sugeridos

1. Rode `tools/cgen/cgen -std=c23 --base-dir base --stop-after=gen -I test/cgen/gen/imports --dest-dir /tmp/t20 test/cgen/gen/imports/use.k` e veja a recusa.
2. `supported()` aceita `K_AST_IMPORT`; confira que `emit_c`/`emit_header` ignoram o nó.
3. Função no `stop_gen.c` que escreve `.type.h` e `.h` de um `KModule` e recursa, com
   uma lista de módulos já escritos (nomes) no frame.
4. O genérico: filtro no emissor.
5. `sh test/cgen/gen_golden.sh` a cada passo (o caso `imports` imprime `wip` enquanto
   difere, e nada quando bate).

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # -> imports sem wip (c23 e c11); os demais como estavam
sh tools/agents/verify.sh       # -> ALL OK
```

Fora do oráculo, em `/tmp`: `import` de um módulo que não existe (o diagnóstico
atual continua); dois módulos que importam o mesmo (escrito uma vez).

## Relatório

`tools/agents/reports/T20-m5-imports.md`: edições, saídas coladas, dúvidas.
