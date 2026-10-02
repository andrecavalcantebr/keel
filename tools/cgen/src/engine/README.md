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
| `parser_islands.c`, `islands.h` | passagem 3, etapa 4a: as ilhas `type`, `name`, `call`, `ref` e `implicit-init` ([parser-design §3.2](../../../../design/parser-design.md)) |
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

## Ilhas, etapas 4a e 4b (2026-09-29)

`k_collect_islands` (`parser_islands.c`) roda depois de `k_collect_instances`,
sobre as declarações `func` e `var` do próprio arquivo, em assinatura e em
corpo, e preenche `KAst.islands` (ordenado pela âncora) e `KAst.island_text`
(a coluna de detalhe, já formatada). O módulo genérico não tem ilhas concretas,
como não tem instâncias. O dump imprime as linhas `ilha` depois das `inst`.

Espécies desta etapa: `type`, `name`, `call`, `ref` e
`implicit-init`. O que a chamada resolve vem da assinatura declarada no módulo
chamado (spec §4.4), nunca do ponto de chamada; por isso a passagem lê os
parâmetros da AST dos módulos importados. A única tipagem de expressão é a forma
mais simples de `container`: um argumento que é um identificador só, declarado
antes com tipo keel. A origem da instância segue a tabela do §4.4: o objeto num
argumento, o tipo escrito num parâmetro `type` de seleção, ou o tipo declarado
do objeto que a chamada inicializa. O sufixo de aridade vem do backend §2.1.1.

A etapa 4b acrescenta `array`, `array-index`, `index` e `range-index`. `array`
sai por nome declarado, com as dimensões como escritas, em corpo, parâmetro e
arquivo; o nome passa a ser um `array` de certo rank. `array-index` só sai
para vários índices (`v[1,2,3]`, `v rank 3`): `v[1][2][3]` e `v[i]` são C.
`index` e `range-index` valem para um identificador declarado com modificador:
`x[a..b]` é `as_slice(x, a, b)` do módulo do tipo do contêiner, e `x[a..]`,
`x[..]` pedem também o `length` dele: é o protocolo da spec (§4.5, §5.1), o
mesmo para qualquer módulo, da base ou do programa. Sobre `array` o verbo é o
de `keel.array`, que o arquivo tem de importar (`protocol-not-satisfied`); o
limite aberto é do núcleo (`core`). `x[i,j]` sobre modificador não tem `ptr`
por aridade (§4.5, item 1): sem `dim` é `no-ptr-for-arity`, e com `dim` (o
acessor de vetor, provisório) ainda não recebe ilha, à espera do oráculo 006.
A marca `&1` segue a regra da chamada (objeto por valor, parâmetro ponteiro).

**A regra (spec §2.3):** o símbolo conhecido é o que faz a ilha. `Q.IDENT` com `Q`
módulo ou alias ativo é ilha, resolva ou não: o que o módulo não declara sai
como a chamada (ou o nome) qualificada, para o C validar (§4.4, item 3), e o que
o módulo declara mas o objeto não serve vira diagnóstico (`wrong-qualifier`,
`not-a-container-expression`, `address-in-object-position`,
`flat-view-of-n-dim-array`). `x[…]` sobre `array` ou instância de modificador é
sempre ilha, mesmo quando a emissão é o próprio texto (`y[1][5]`); um índice sem
`ptr` daquela aridade é `no-ptr-for-arity`, e um intervalo sobre tipo cujo módulo
não declara `length` e `as_slice` de três parâmetros é `protocol-not-satisfied`. Símbolos de arquivo
(`buffer i32 gbuf;`, `array i32 v[6]`) valem em todas as funções, onde quer que
estejam no arquivo.

Um verbo sem qualificador (`length(s)`, `push(s, 7)`) é o verbo do tipo do
contêiner `s`, quando o primeiro argumento é um nome declarado com modificador e o
módulo dele declara o verbo (§4.4, passo 1); antes de uma função do próprio arquivo
com o mesmo nome.

`of`, `from` e `clone` são produtores, qualificados pelo módulo do produto
(§4.4, regra 1). `slice.of(x…)` é função sobre `Sliceable` (spec §4.14, backend
§5.19): a ilha resolve o `as_slice` de três parâmetros do tipo de `x` para achar a
instância e o tipo do resultado, e escreve `keel_slice_of<aridade>_<tipo>`, que não
é `wrong-qualifier`. `buffer.of(v)` baixa para o `of` do buffer, e os verbos de
`keel.array` são `keel_array_<T>_<verbo>`, todos com a marca `dim:1`.

**Tipagem de contêiner.** A posição de contêiner é a produção `container` da §2.2
(`IDENT`, `x[…]`, `x.f`, `x->f`, `verb(x…)`, `*x`, `&x`, `(x)`), e a passagem
tipa cada uma delas com o que viu declarado: um nome (local, parâmetro, variável
de arquivo), o elemento de um `array` ou o que o `ptr` de um modificador aponta,
um campo de `struct` (do arquivo ou de um import), e o retorno declarado de um
verbo com os parâmetros de tipo do módulo trocados pelos argumentos da instância.
O tipo é estrutural (`KType`): módulo, argumentos, símbolo C, quantidade de `*`,
e se tem endereço (é dele que sai a marca `&k`). Chamada com objeto que não é o
próprio modificador (`routine.seq(slice.of(steps))`, onde `seq` recebe `slice slot`)
acha a instância do módulo dentro do tipo do argumento. O prefixo do símbolo segue
o objeto: o próprio modificador leva o símbolo do tipo (`keel_routine_slot_ag2_Ag_code`),
o resto leva o módulo mais os argumentos (`keel_routine_ag2_Ag_seq`).

**As operações do núcleo** (`keel.length`, `keel.capacity`, `keel.dim`) são ilhas de
qualquer módulo, mas não lançam função: o detalhe diz `core`, e a marca `dim:1`
diz que a dimensão vem da tabela.

**`x[a..b]` sobre `array`** é `array.as_slice(x, a, b)`: o elemento dá a instância e a
dimensão 0 vem da tabela (`fx 2..5 → keel_array_f32_as_slice2 dim:1`; ponta aberta à
direita acrescenta `core`, e `fx[..]` tem as duas pontas: `… as_slice2 core dim:1`).

**O que ainda fica sem ilha:** chamada cujo objeto é uma expressão fora do que a
passagem tipa (variável que `foreach` ou `parallel` declaram, até as
etapas 4d e 4e; o resultado de `x[a..b]`); e o acessor de vários índices sobre
modificador com `dim` (§4.5, item 1; oráculo 006). Também **não há ainda**
`from-without-target` (falta o alvo de atribuição e de `return`),
`verb-not-in-instance`, e o `not-a-container-expression` para um nome que a
passagem não viu declarado. A marca `*k` (parâmetro por valor, argumento
ponteiro) não está no §5.2 e não é impressa. A nomeação de instância aninhada
segue o C esperado do golden (`keel_buffer_keel_buffer_i32`), não o §2.1 do
backend (`keel_buffer_buffer_i32`).

`PARSE_LEVEL=ilha:<espécies>` (padrão do `make check`: as das etapas 4a e 4b)
compara só essas espécies nos `.parse`, que continuam inteiros (001, 009 e 013
passam; 021 segue `wip`). `test/cli/islands.sh` cobre o que os
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

## Diagnósticos (2026-09-29)

`diag_catalog.def` é gerado por `tools/cgen/gen-diags.py` a partir do catálogo do
`keel-spec.md` §6.2 ([G1]): 140 identificadores de tradução, sem os 9 `debug`,
que são verificações do programa gerado. Os identificadores da ferramenta
(`module-not-found`, `unexpected-token`, `module-load-failed`,
`implementation-limit`) ficam em `diag.h`. Só as mensagens são escritas à mão,
em `diag.c`; um diagnóstico sem mensagem é um que o motor ainda não emite.
`gen-diags.py --check` roda no `make check` e falha se o `.def` divergir da spec.

Os casos de falha ficam em `test/diag/` ([G3], `diag-design.md` §7): cada fonte
marca a linha que deve ser recusada com `/* DIAG: <identificador> */`, e
`test/diag.py` compara o conjunto (arquivo, linha, identificador) impresso com o
marcado. A última linha do teste é a cobertura do catálogo;
`python3 tools/cgen/test/diag.py --coverage -v` lista o que falta.

## Protocolos, papéis e função sobre protocolo (2026-10-02)

A gramática acompanha a revisão de 2026-10-02 da spec (§§2.2, 4.12, 4.14, 5.1).

- **`decl-protocol`** (`parser_protocol_decl.c`): a forma com chaves lê o
  implementador, os tipos associados e, de cada protótipo, nome, aridade,
  posição do receptor, papéis e o nome do retorno; a de composição lê os
  componentes. O nó é `K_AST_PROTOCOL`, o símbolo `K_SYM_PROTOCOL`, exportado e
  injetado por `types`. No dump, `decl pub protocol <nome> -`: o protocolo não
  emite C. `protocol` é contextual: só `protocol NOME type` ou `protocol NOME [`
  abrem a declaração.
- **Assinaturas**: um papel (`parent`, `child`, `invalidates`, `consumes`) antes
  do tipo de um parâmetro não é o seu tipo; `child` antes do retorno também é
  pulado. Fora dessas posições, `role-position`. `keel_code` e o `dim N` de
  molde são reconhecidos e recusados com `not-in-v0`. `typedef … nome byref;`
  registra `nome`.
- **Função sobre protocolo** (spec §4.14, backend §5.19): uma chamada cuja
  função tem o primeiro parâmetro de protocolo escreve
  `<prefixo>_<nome><aridade>_<tipo>`. Os verbos exigidos vêm da declaração do
  protocolo (com os componentes); o tipo concreto atende quando o seu módulo
  declara cada um, com esse tipo como receptor, e senão é
  `protocol-not-satisfied`, com os verbos ausentes. Papéis diferentes dos do
  protótipo são `protocol-role-mismatch`. A ilha resolve a chamada como o verbo
  do tipo concreto que devolve o tipo associado de retorno da função (ou o
  primeiro que recebe o receptor), para achar a instância, a adaptação e o tipo
  do resultado. `slice.of` é o primeiro uso, e deixou de ser caso particular.

Ainda não: análise de papéis (§4.12: `region-escape`,
`child-region-after-invalidation`), parâmetro de protocolo fora da primeira
posição. `foreach`, `walk`, `parallel`, `match` e `else` ainda não são ilhas (as
espécies existem em `KIslandKind`, sem reconhecedor): quando entrarem, pedem os
verbos pela mesma tabela, como já fazem o `x[a..b]` e as funções sobre protocolo.
