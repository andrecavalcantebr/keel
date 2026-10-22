# T2 — consolidar os `parser_*.c` pequenos em cinco arquivos

## Objetivo

Os `parser_*.c` de `tools/cgen/src/engine/` são 21 arquivos de 15 a 170
linhas, quase todos com uma função só. O corte é herança do harness local
(um arquivo por cartão de tarefa), não do desenho. Esta tarefa os junta por
fase, **sem mudar comportamento**: o corpo das funções fica igual, com as
exceções listadas em "Limpezas".

## Snapshot (antes de qualquer edição)

O orquestrador já tirou: `/tmp/keel-snap-base`. Se não existir, rode
`sh tools/agents/verify.sh snapshot /tmp/keel-snap-base` **antes** de editar.

## Arquivos

Dentro de `tools/cgen/src/engine/`:

| Novo arquivo | Recebe (na ordem) | Apaga |
| --- | --- | --- |
| `parser_scan.c` | `parser_opaque_until.c`, `parser_extern_c.c` só a `k_scan_braced_opaque`, `parser_scan_qualified_name.c`, `parser_ident_list.c` | `parser_opaque_until.c`, `parser_scan_qualified_name.c`, `parser_ident_list.c` |
| `parser_declarator.c` (já existe) | ele mesmo + `parser_known_type.c` | `parser_known_type.c`, `parser_declarator_head.c` (código morto, ver Limpezas) |
| `parser_header.c` (já existe) | ele mesmo + `parser_imports.c` | `parser_imports.c` |
| `parser_decls.c` | `parser_decl_keel.c`, `parser_decl_constexpr.c`, `parser_decl_typedef.c`, `parser_decl_array.c`, `parser_struct_decl.c`, `parser_tags_decl.c`, `parser_modifier_decl.c`, `parser_extent_decl.c`, `parser_protocol_decl.c`, `parser_extern_c.c` (o resto, `k_scan_extern_c`) | todos esses |
| ficam como estão | `parser_keel.c`, `parser_dump.c`, `parser_delimiters.c`, `parser_islands.c` | — |

Também podem ser editados: `parser.h` (só as limpezas abaixo e os comentários
que citam nome de arquivo), `ast.c` (só as chamadas afetadas pelas
limpezas), `tools/cgen/src/engine/README.md` (a tabela de arquivos), os
`tools/cgen/test/unit/*.sh` e `*_main.c` dos arquivos movidos, e
`tools/agents/warnings.baseline`. **Não toque em `parser_islands.c`**: ele é a
tarefa T3.

Ao juntar, preserve os comentários de cabeçalho de cada arquivo de origem
como comentário de seção (`/* ---- k_scan_struct_decl — <o comentário> ---- */`
ou parecido, no estilo das seções de `parser_islands.c`). Junte os `#include`
no topo, sem repetir. Funções `static` de mesmo nome vindas de arquivos
diferentes colidiriam: se houver, renomeie a segunda com um sufixo que diga
de onde veio e registre no relatório.

## Limpezas (as únicas mudanças de código permitidas)

1. **Apagar `k_scan_declarator_head`** (`parser_declarator_head.c`, a
   declaração em `parser.h:337` e o tipo `KDeclaratorHead`, se nada mais o
   usar) e os testes `test/unit/parser_declarator_head.sh` e
   `_main.c`. Hoje só o próprio teste a chama: confira com
   `grep -rn k_scan_declarator_head tools/cgen`. O comentário de
   `test/unit/parser_decl_keel_main.c:3-18` cita a função; ajuste o texto
   para `k_scan_declarator`, sem mudar as asserções.
2. **`k_token_is_qual_c` vira `static`** em `parser_declarator.c`, e sai do
   `parser.h`, se nenhum outro arquivo a usar (confira com grep).
3. **Retirar os parâmetros `*_kw` que não são usados.** O build limpo avisa:

   ```
   src/engine/parser_extern_c.c:46:44: warning: unused parameter ‘extern_c_kw’
   src/engine/parser_modifier_decl.c:3:49: warning: unused parameter ‘modifier_kw’
   src/engine/parser_header.c:3:47: warning: unused parameter ‘module_kw’
   src/engine/parser_header.c:28:42: warning: unused parameter ‘import_kw’
   src/engine/parser_header.c:61:44: warning: unused parameter ‘import_c_kw’
   ```

   É o mesmo feito no commit `8b0bdf4` para `k_scan_decl_constexpr`. Use
   aquele diff como modelo (`git show 8b0bdf4`): o parâmetro sai da definição
   e da declaração, a continuação da assinatura é realinhada sob o `(`, e o
   chamador para de passar o token. **Atenção em `ast.c`:** o chamador costuma
   ter `KToken kw = reenter(a, i, &lexer, &pp);`. A chamada `reenter` tem efeito
   (posiciona o lexer) e tem de ficar. Só a variável sai, e só quando nada mais
   a usa naquele bloco. Os testes de unidade que chamam essas funções passam a
   chamar sem o token (o `first_token(...)` fica, porque posiciona o lexer).
   Os quatro avisos restantes (`main.c`: `dest_dir`, `main_module`,
   `pedantic_names`, `f_flag`) são das opções do M3: **não mexa**.

Nada além disso: nenhuma renomeação de função pública, nenhuma mudança de
lógica, nenhuma reformatação de corpo.

## Os testes de unidade

Cada `test/unit/<x>.sh` tem `SRC=tools/cgen/src/engine/<x>.c` e liga todos
os outros `.c` do motor como `DEPS`. Depois da junção, `SRC` tem de apontar
para o arquivo novo que contém a função testada. Os nomes dos testes
(`parser_struct_decl.sh` etc.) ficam como estão: eles nomeiam a função,
não o arquivo.

## Aceitação

```sh
sh tools/agents/verify.sh compare /tmp/keel-snap-base
```

Tem de imprimir `ALL OK`. Isso inclui o build com no máximo
`warnings.baseline` avisos. Ao terminar, os 5 avisos `*_kw` já saíram, então
**baixe `tools/agents/warnings.baseline` para 4** e rode de novo: tem de passar
com 4. E:

```sh
ls tools/cgen/src/engine/parser_*.c   # exatamente 8 arquivos:
# parser_declarator.c parser_decls.c parser_delimiters.c parser_dump.c
# parser_header.c parser_islands.c parser_keel.c parser_scan.c
grep -rn 'k_scan_declarator_head\|KDeclaratorHead' tools/cgen   # nada
```

## Relatório

`tools/agents/reports/T2-parser-consolidate.md`, em pt-BR: a tabela de
origem → destino, as limpezas feitas com `arquivo:linha`, as colisões de
nome (se houver), a saída final do `verify.sh compare` e as dúvidas.
