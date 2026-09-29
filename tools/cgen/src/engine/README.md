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
| `lexer.h`, `lexer.c`, `lexer_*.c` | [lexer-design.md](../../../../design/lexer-design.md) |
| `parser_keel.c` | ponto de entrada, `k_parser_keel(input, output, diagnostics)`; por ora, o despejo de tokens do `--stop-after=lex` ([desenho do cgen §5.1](../../../../design/cgen-tool.md#51---stop-afterlex)) |
| `ast.h`, `ast.c` | AST de nível de arquivo, com spans de tokens e fonte persistente; coleta de `module`, imports e declarações para M2 |
| `parser_dump.c` | projeta essa AST no formato `--stop-after=parse` (níveis `header`, `decl`, `inst` e `ilha`) |
| `parser_islands.c`, `islands.h` | passagem 3, etapa 4a: as ilhas `type`, `name`, `call`, `from-stack`, `ref` e `implicit-init` ([parser-design §3.2](../../../../design/parser-design.md)) |
| `parser.c`, `symtab.c` | [parser-design.md](../../../../design/parser-design.md) |
| `emit/` | [codegen-design.md](../../../../design/codegen-design.md) |
| `diag.h`, `diag.c` | [diag-design.md](../../../../design/diag-design.md): o sink, que acumula na memória dada por quem chama; a tabela tem por ora só os diagnósticos do lexer |

## Estado da integração (2026-09-28)

`--stop-after=parse` usa o carregador real, com a base por último nas raízes.
O fluxo é `k_parse_headers` → `k_resolve_imports` → `k_collect_ast` →
`k_collect_instances`. A primeira passagem guarda as regiões de declaração
sem exigir símbolos importados; a coleta seguinte recebe a tabela resolvida.

`KModule` conserva a AST e os exports; `KSymbol.origin` conserva a identidade
original e o destino de um qualificador/alias. A ferramenta é dona dos fontes,
tokens, nós, tabelas e instâncias durante toda a invocação. Sua destruição ocorre
somente depois do dump e dos diagnósticos, quando nenhuma fatia será usada.

O dump `inst` registra os usos concretos escritos no arquivo, inclusive em
campos, parâmetros e corpos, em ordem de primeira ocorrência e sem repetir a
mesma identidade. Não é ainda o fechamento transitivo das instanciações de
módulos genéricos, nem a passagem de resolução das ilhas e dos escopos locais.
A nomeação dos usos aninhados conserva as identidades completas conforme os
oráculos atuais de `test/parse`; a divergência com a redação de “prefixo só na
raiz” do backend §2.1 permanece uma questão de especificação a resolver.

Regressões adicionais: `test/cli/imports.sh` cobre o executável e
`test/unit/loader_regressions.sh` cobre estado de carga, fecho de timestamps,
cache de falhas e mais de 256 exports. Os quatro oráculos `.parse` existentes
não foram alterados. `PARSE_LEVEL=inst make check` verifica esse marco.

## Ilhas, etapa 4a (2026-09-29)

`k_collect_islands` (`parser_islands.c`) roda depois de `k_collect_instances`,
sobre as declarações `func` e `var` do próprio arquivo, em assinatura e em
corpo, e preenche `KAst.islands` (ordenado pela âncora) e `KAst.island_text`
(a coluna de detalhe, já formatada). O módulo genérico não tem ilhas concretas,
como não tem instâncias. O dump imprime as linhas `ilha` depois das `inst`.

Espécies desta etapa: `type`, `name`, `call`, `from-stack`, `ref` e
`implicit-init`. O que a chamada resolve vem da assinatura declarada no módulo
chamado (spec §4.4), nunca do ponto de chamada; por isso a passagem lê os
parâmetros da AST dos módulos importados. A única tipagem de expressão é a forma
mais simples de `container`: um argumento que é um identificador só, declarado
antes com tipo keel. A origem da instância segue a tabela do §4.4: o objeto num
argumento, o tipo escrito num parâmetro `type` de seleção, ou o tipo declarado
do objeto que a chamada inicializa. O sufixo de aridade vem do backend §2.1.1.

**O que fica sem ilha, de propósito:** chamada cujo contêiner não é um
identificador declarado (`slice.of(steps)` sobre um `array`, `x.f`, resultado de
outro verbo), chamada C desconhecida, e `alias.CONSTANTE`. A ausência é o
"emite a chamada para o C validar" do §4.4; o diagnóstico
`not-a-container-expression` ainda não existe. A marca de adaptação `*k`
(parâmetro por valor, argumento ponteiro) não está no §5.2 e não é impressa.

`PARSE_LEVEL=ilha:type,name,call,from-stack,ref,implicit-init` é o padrão do
`make check`: compara só essas espécies nos `.parse`, que continuam inteiros
(001, 009 e 013 passam; 021 segue `wip`). `test/cli/islands.sh` cobre o que os
três oráculos não alcançam.

## Memória da ferramenta

`tool/memory.h` fornece a arena por invocação e os helpers de string.
`CGEN_ARENA_CAPACITY` configura o bloco na compilação (padrão: 64 MiB).
Somente a aquisição/liberação desse bloco usa `malloc`/`free`; a arena não
cresce nem move endereços. Falhas informam `implementation-limit` com o uso,
a capacidade e a configuração. Os vetores e acessos por índice são preservados.

`string` é `keel_buffer_char`: `len` exclui o terminador e `cap` inclui seu
espaço. `str_str` empresta uma string C mutável; `str_cstr` devolve o ponteiro.
`str_dup` copia uma região delimitada por comprimento para a arena e acrescenta
zero, inclusive para entrada vazia. Falha retorna descritor com `ptr == NULL`.
Fontes e tokens continuam sendo regiões delimitadas por comprimento; não se
exige terminador nelas. Nomes de instância reservam 256 bytes e recusam nomes
gerados acima dos 255 caracteres permitidos pelo backend.

## Buffers de armazenamento

`KAst.tokens`, `KAst.nodes` e `KAst.instances` são instâncias geradas de
`buffer KLexeme`, `buffer KAstNode` e `buffer KInstanceUse`. `KSymbolTable`
é um alias de `buffer KSymbol`, preservando as operações de busca/inserção.
Os elementos são declarados em `storage_types.h`, sem dependência dos buffers,
e instanciados por `tools/transform/base/Makefile`.

O acesso indexado usa `keel_buffer_T_ptr(&b, i)`; `data(&b)` fornece o endereço
inicial. A base de bootstrap adotou esses nomes no lugar de `ptr_1` e do
antigo `ptr` sem índice. Não há crescimento automático: a ferramenta continua
alocando as mesmas capacidades na arena. Arrays embutidos permanecem arrays.
As funções `k_parse_ast`, `k_parse_headers`, `k_collect_ast` e
`k_collect_instances` usam o armazenamento do próprio `KAst`, sem repetir
ponteiro e capacidade nos argumentos. A operação de contagem `k_lexemes`
continua aceitando saída nula antes da alocação dos tokens.
