# T6 — writer.c e mangle.c: relatório (escrito pelo orquestrador)

O agente Haiku escreveu `tools/cgen/src/engine/emit/writer.c` (70 linhas) e
`mangle.c` (91 linhas), às 09:21 de 2026-10-23, e encerrou sem entregar o
relatório nem uma mensagem final. Quando a tarefa foi verificada, 27 minutos
depois, não havia processo dele rodando. O orquestrador revisou o código e
rodou as aceitações.

## Revisão

- Os dois arquivos seguem os headers. A saída é contada como no `snprintf`
  (o `KOut` de `parser_dump.c`), não há include proibido em `engine/`, e o
  número do `#line` é convertido à mão.
- `k_emit_at`: o `mapped == 0 ||` da condição é redundante, porque
  `src_line ≥ 1`. Não muda o resultado.
- `k_mangle_decl`: o prelúdio sai sem prefixo, e um tipo com o nome igual ao
  último componente do módulo não se repete. O resto é concatenação.

## Aceitação

```
make -C tools/cgen check | grep emit_    -> ok unit/emit_mangle, ok unit/emit_writer
sh tools/agents/verify.sh                -> ALL OK (4 avisos, linha de base 4; golden 46 ok)
```

## Lição para o harness

Um agente pode encerrar sem entregar o relatório. Antes de supor que ele está
em laço, o orquestrador confere a árvore (o `mtime` dos arquivos e o `ps`), e
fecha a tarefa com as aceitações do cartão.
