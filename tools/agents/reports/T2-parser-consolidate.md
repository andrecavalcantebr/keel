# T2 — relatório: consolidação dos `parser_*.c` em oito arquivos

Refatoração sem mudança de comportamento. O snapshot de partida é
`/tmp/keel-snap-base` (existia; não foi retirado de novo). Nada foi commitado
nem enviado; o índice do git continua sem mudanças em estágio.

## Origem → destino

| Destino | Recebe (na ordem) | Situação |
| --- | --- | --- |
| `parser_scan.c` (novo) | `parser_opaque_until.c`; `k_scan_braced_opaque` (linhas 1–44 de `parser_extern_c.c`); `parser_scan_qualified_name.c`; `parser_ident_list.c` | apagados os três primeiros |
| `parser_declarator.c` (existia) | ele mesmo; `parser_known_type.c` | `parser_known_type.c` apagado |
| `parser_header.c` (existia) | ele mesmo; `parser_imports.c` | `parser_imports.c` apagado |
| `parser_decls.c` (novo) | `parser_decl_keel.c`, `parser_decl_constexpr.c`, `parser_decl_typedef.c`, `parser_decl_array.c`, `parser_struct_decl.c`, `parser_tags_decl.c`, `parser_modifier_decl.c`, `parser_extent_decl.c`, `parser_protocol_decl.c`, `k_scan_extern_c` (linhas 46–59 de `parser_extern_c.c`) | apagados os nove e `parser_extern_c.c` |
| ficam como estão | `parser_keel.c`, `parser_dump.c`, `parser_delimiters.c`, `parser_islands.c` | não tocados (`parser_islands.c` é da T3) |

Cada trecho entrou como seção `/* ---- <função> ---- */`, com os `#include`
juntados no topo (`parser.h`, `names.h`, `ast.h`, `loader.h`, `<string.h>`,
`<stdint.h>`, sem repetição). Os comentários de cabeçalho de cada origem
ficaram no corpo, sem alteração.

Arquivos apagados (15 de `.c`): `parser_opaque_until.c`,
`parser_scan_qualified_name.c`, `parser_ident_list.c`, `parser_known_type.c`,
`parser_imports.c`, `parser_decl_keel.c`, `parser_decl_constexpr.c`,
`parser_decl_typedef.c`, `parser_decl_array.c`, `parser_struct_decl.c`,
`parser_tags_decl.c`, `parser_modifier_decl.c`, `parser_extent_decl.c`,
`parser_protocol_decl.c`, `parser_extern_c.c`; e `parser_declarator_head.c`
(limpeza 1).

`ls tools/cgen/src/engine/parser_*.c` dá exatamente os 8 esperados.

## Colisões de nome

Nenhuma. Os `static` dos arquivos juntados (`skip_suffix`, `scan`,
`byref_tail`, `parses_as_declarator`, `is_qual_arg`, `scan_known_type`,
`scan_argument`, `spelled`, `inject_types`, `resolve_one`, `scan_dim`,
`scan_verb`) têm nomes distintos entre si. Nenhuma renomeação.

## Limpezas

1. **`k_scan_declarator_head` apagado.** Removidos: `parser_declarator_head.c`,
   `test/unit/parser_declarator_head.sh` e `_main.c`; em `parser.h`, o tipo
   `KDeclaratorHead` e a declaração de `k_scan_declarator_head` (era a bloco
   de ~318–338). O comentário seguinte deixou de citar a função. Em
   `test/unit/parser_decl_keel_main.c` (linhas 3, 7, 13, 18) o nome virou
   `k_scan_declarator`, sem tocar nas asserções. `grep -rn
   'k_scan_declarator_head\|KDeclaratorHead' tools/cgen` não acha nada.
2. **`k_token_is_qual_c` virou `static`** (`parser_declarator.c:8`). Não estava
   no `parser.h` com outro uso; a declaração em `parser.h` (era a linha 383)
   foi removida.
3. **Os cinco parâmetros `*_kw` não usados saíram**, com o mesmo padrão do
   commit `8b0bdf4`:
   - `k_scan_module_decl(lexer, out, next_out, next_pp)` — `parser_header.c:8`
   - `k_scan_import(lexer, out, ...)` — `parser_header.c`
   - `k_scan_import_c(lexer, out, ...)` — `parser_header.c`
   - `k_scan_extern_c(lexer, out, ...)` — `parser_decls.c:528`
   - `k_scan_modifier_decl(lexer, module_arity, symtab, out, ...)` — `parser_decls.c`

   Chamadores em `ast.c` (linhas 114, 173, 196, 204 e as chamadas
   correspondentes): o `KToken kw = reenter(...)` ficou como
   `reenter(a, i, &lexer, &pp);`, porque posiciona o lexer. Só a variável saiu.
   Os testes `parser_header_main.c`, `parser_extern_c_main.c` e
   `parser_modifier_decl_main.c` passaram a chamar sem o token; o
   `first_token(...)` ficou. Os comentários de `parser.h` que citavam
   `module_kw`/`import_kw`/`import_c_kw`/`extern_c_kw`/`modifier_kw` foram
   reescritos para "o token já foi consumido pelo chamador". Os outros
   `*_kw` (struct, tags, array, extent, typedef) não foram mexidos.

Não foi feito mais nada: nenhuma renomeação pública, nenhuma mudança de
lógica, nenhuma reformatação de corpo. Os quatro avisos restantes (`main.c`:
`dest_dir`, `main_module`, `pedantic_names`, `f_flag`) são das opções do M3 e
ficaram.

## Testes de unidade

`SRC` repontado em `test/unit/*.sh`:

- `parser_scan.c`: `parser_opaque_until.sh`, `parser_scan_qualified_name.sh`, `parser_ident_list.sh`
- `parser_decls.c`: `parser_extern_c.sh`, `parser_decl_keel.sh`, `parser_decl_constexpr.sh`, `parser_decl_typedef.sh`, `parser_decl_array.sh`, `parser_struct_decl.sh`, `parser_tags_decl.sh`, `parser_modifier_decl.sh`, `parser_extent_decl.sh`
- `parser_declarator.c`: `parser_known_type.sh`
- `parser_header.c`: `parser_imports.sh`

Os nomes dos testes ficaram. `parser_decl_keel.sh` teve o cabeçalho de
dependências reescrito (antes citava `parser_declarator_head.c` e as tarefas
`m2-*`).

Outros ajustes de texto: `src/engine/README.md`, três menções em prosa
(`parser_protocol_decl.c` → `parser_decls.c`; `parser_imports.c` →
`parser_header.c`, duas vezes). A tabela do README não listava esses
arquivos e não ganhou linhas novas.

## Build e aceitação

`warnings.baseline`: 9 → 4 (os cinco `*_kw` saíram). `make clean && make -j8`
dá 4 avisos, todos de `src/tool/main.c`.

Saída final de `sh tools/agents/verify.sh compare /tmp/keel-snap-base`, com o
baseline já em 4:

```
ok   build (4 warnings, baseline 4)
ok   make check (41 lines ok)
ok   golden: 44 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ok   behaviour identical to /tmp/keel-snap-base
ALL OK
```

Em `sh tools/cgen/test/run.sh`, todos os `unit/parser_*` saíram `ok`
(`parser_declarator_head` deixou de existir).

## Dúvidas e pontos abertos

- **Comentário solto em `parser_declarator.c`** (antes em `parser_declarator.c`
  linhas 93–96, agora ~linhas 93–96 do arquivo novo): "Whether a `declarator`
  starting at `from` parses…" está acima de `byref_tail`, mas descreve
  `parses_as_declarator`. Já estava assim; não corrigi, porque o card não pede
  e a mudança seria só de texto.
- **Documentos e cartões antigos citam os nomes antigos**: `tools/harness/`
  (tarefas `m2-*` e o README), `.claude/skills/` e `.agents/skills/`. São
  históricos ou fora do escopo do card; não foram tocados.
- **Reflow em `parser.h`**: as reescritas dos comentários de `extern_c` e
  `modifier` deixaram uma linha um pouco mais larga que 100 colunas. Cosmético.
- **Árvore de trabalho**: `git status` mostra também `golden/line-check.py` e
  `tools/agents/reports/T1-line-audit.md`, que não são meus (auditoria
  concorrente). Não toquei neles.
