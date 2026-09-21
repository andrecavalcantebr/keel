# engine — depende só do fonte

Nada aqui abre arquivo, escreve arquivo ou termina o processo. O motor recebe
bytes e devolve buffers e diagnósticos; quem toca no sistema de arquivos é
`../tool/`.

A regra vem da [spec da ferramenta §3](../../../../cgen-tool-spec.md) —
"separação de responsabilidade, não de empacotamento" — e o layout que a torna
visível é a decisão D8 do [desenho do cgen](../../../../design/cgen-tool.md).

**Ela é verificável, e o build a verifica:** nenhum fonte deste diretório inclui
`<stdio.h>`, `<stdlib.h>`, `<unistd.h>`, `<fcntl.h>` ou `<sys/*.h>`. O alvo é
`make boundary`, e ele roda junto com `all`.

O que a separação compra, além da disciplina: o motor é testável **sem sistema
de arquivos**. Um teste entrega bytes e um `KLoader` de mentira que devolve
módulos de um vetor em memória, e confere os buffers de saída — sem diretório
temporário, sem `mkstemp`, sem limpeza.

| Arquivo | Desenho |
| --- | --- |
| `lexer.c` | [lexer-design.md](../../../../design/lexer-design.md) |
| `parser.c`, `symtab.c` | [parser-design.md](../../../../design/parser-design.md) |
| `emit/` | [codegen-design.md](../../../../design/codegen-design.md) |
| `diag.c` | [diag-design.md](../../../../design/diag-design.md) |
