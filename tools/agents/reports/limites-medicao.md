# limites-medicao: medição dos limites do cgen sem medição

Medição só de leitura, feita sobre o `tools/cgen/cgen` do repositório (branch `0.4.0`, commit `20278a0`). Nenhum código, normativo, golden ou teste foi alterado, e nada foi commitado. Os programas ficam em `/tmp/limites2/`, um diretório por caso (o `.k` fica em `app/m.k`, com `module app.m;`). Os scripts de geração e o driver estão em `/tmp/limites2/lib.py`, `medir1.py`, `lines.py`, `dirs.py` e nos blocos de probe.

**Comando usado.** `tools/cgen/cgen -std=c23 --base-dir base -I <dir> --stop-after=gen --dest-dir <dir>/_out <dir>/app/m.k` (a partir da raiz do repositório). Para limites do parser, `--stop-after=parse`. Para o caso genérico, `--instance "gmod.box i32"`.

**Critério.** "Passa" é código de saída 0. "Silencioso" é saída 0 com conteúdo errado: o texto keel sobra no C gerado, ou um elemento some. Onde a conferência foi feita pelo texto gerado, está dito.

**Como ler os valores.** "Passa até" é o maior N, varrido de 1 em 1 nos casos pequenos. "Falha em" é o primeiro N recusado. O limite medido é o do programa (forma sintática e tamanho dos nomes), não uma constante isolada.

## Tabela (uma linha por limite)

| # | Limite | Passa até | Falha em | Diagnóstico na falha | Silencioso? | Programa |
|---|---|---|---|---|---|---|
| 1a | `extent`: grupos `[k, n]` no cabeçalho (`MAX_GROUPS` 4) | 4 | 5 | nenhum: a declaração não é emitida e o texto `pub extent struct ...` vai para o `.type.h` | **SIM (bug)** | `/tmp/limites2/ext_dims-5/app/m.k` |
| 1b | `extent`: colunas `array` (`MAX_COLUMNS` 8) | 8 | 9 | `implementation-limit`: "the emitter does not cover this construction yet (at `pub`)" | não | `/tmp/limites2/ext_cols-9/app/m.k` |
| 1c | `extent`: campos (`x->fields < 32`); contagem `k` depois do 32º campo | k antes dos campos: 35 campos ok. k depois: 30 campos ok (k é o 32º) | k como 33º (31 campos) | `extent-count-not-field`: "the count `k` ... names no field of the struct" (mensagem enganosa: `k` é campo) | não (erro, mas errado) | `/tmp/limites2/ext_fields-31/app/m.k` |
| 2 | `match`: rótulos (`MAX_LABELS` 32) | 14 rótulos curtos (`A0`...); 9 com rótulos de 27 caracteres | 15 (curtos); 10 (longos) | `implementation-limit`: "the text of a line's edits needs more than SCRATCH_CAP = 2048 bytes ...; shorten the line". O limite real é o `SCRATCH_CAP`, não `MAX_LABELS`; 33 dá "does not cover" | não | `/tmp/limites2/match-14`, `match-15`, `/tmp/limites2/m_27_9`, `m_27_10` |
| 3 | `defer [now ...]`: entradas (`MAX_CAP` 8) | 8 | 9 | `implementation-limit`: "the emitter does not cover this construction yet (at `defer`)" | não | `/tmp/limites2/defer-8`, `defer-9` |
| 4 | `routine`: campos de um modifier com tabela (`MAX_FIELDS` 16) | 17 campos com preenchimento completo (`= {0}` em 16 campos) | 18 campos: rc 0, **sem** nenhum preenchimento (teto em 16 campos, mas o preenchimento some por inteiro a partir de 18) | nenhum | **SIM** (efeito em C nulo: o campo omitido já é zero, mas o gerado diverge do desenho) | `/tmp/limites2/rtc16`, `rtc17`, `rtc18`, `rtc20` |
| 5 | `parallel`: capturas (`MAX_CAPS` 16) | 16 | 17 | `implementation-limit`: "the emitter does not cover this construction yet (at `parallel`)" | não | `/tmp/limites2/par16`, `par17` |
| 6a | `protocol`: itens (`K_PROTOCOL_MAX_ITEMS` 16) | 16 | 17 | `unexpected-token`: "malformed or truncated declaration" (mensagem enganosa) | não | `/tmp/limites2/proto16`, `proto17` (`--stop-after=parse`) |
| 6b | `protocol`: parâmetros de um verbo, contando o receptor (`K_PROTOCOL_MAX_PARAMS` 8) | 8 | 9 | `unexpected-token`: "malformed or truncated declaration" (enganosa) | não | `/tmp/limites2/protoar8`, `protoar9` (`--stop-after=parse`) |
| 7 | `#include` por arquivo gerado (`MAX_INCLUDES` 32) | 32 imports de tipos, cada um usado numa assinatura com corpo | 33 imports | `implementation-limit`: "a file needs more than MAX_INCLUDES = 32 includes (...); split the module' includes in .h" | não | `/tmp/limites2/ii32`, `ii33` |
| 8a | parâmetros de função não genérica (`K_PARAMS_MAX` 16) | 18 (testado até 18; sem truncamento: `p17` presente no `.h`) | não encontrado até 18 | — | não | `/tmp/limites2/params-16` a `params-18` |
| 8b | parâmetros de função num módulo genérico com instância (`--instance "gmod.box i32"`), extern e inline | 30 (`f(box, a0..a29)` inteiro no `.h`/`.c`) | não encontrado até 30 | — | não | `/tmp/limites2/gp-False-30`, `gp-True-30` |
| 8c | função chamada pelo fechamento (`K_PARAMS_MAX` em `isl_params_of`) | **não medido** (ver Dúvidas) | — | — | — | — |
| 9a | declaradores de `typedef` (`K_TYPEDEF_MAX_NAMES` 8) | sem recusa até 30 nomes: a saída é o próprio `typedef` do fonte, com os 30 nomes | nenhuma até 30 | — | não (a saída é C válido) | `/tmp/limites2/td30chk` |
| 9b | declaradores de `array` no topo (`K_ARRAY_MAX_NAMES` 8) | 8 | 9 | `unexpected-token`: "malformed or truncated declaration" (enganosa) | não | `/tmp/limites2/topa9`, `topapub9` (topo) |
| 9b' | declaradores de `array` local (dentro de função) | sem recusa até 30; todos emitidos | nenhuma até 30 | — | não | `/tmp/limites2/arrx30` |
| 9c | declaradores de `struct` no topo (`K_STRUCT_MAX_NAMES` 8) | **nenhum**: `pub struct S {...} s0;` é recusado já com 1 declarador (o emissor não cobre a forma); com 9, `unexpected-token` | 1 (forma de topo) | 1..8: `implementation-limit` "the emitter does not cover this module's constructions yet"; 9: `unexpected-token` | não | `/tmp/limites2/st1`...`st9` |
| 9c' | `struct` com declaradores dentro de função | sem recusa até 9 (passa; trecho opaco, C válido) | nenhuma até 9 | — | não | `/tmp/limites2/stl9` |
| 10a | dimensões de `array` (declaração) | sem recusa até 11 dimensões; a saída C tem as 9 dimensões (`i32 a[2][2]...`) | nenhuma até 11 | — | não | `/tmp/limites2/decl9`, `decl11` |
| 10b | índices de `x[i, ...]` (`MAX_RANK` 8) | 8 índices | 9 índices (com declaração de 9 dimensões) | `implementation-limit`: "the emitter does not cover this construction yet (at `a`)" | não | `/tmp/limites2/full8`, `full9` |
| 11 | profundidade de declarador (`K_DECLARATOR_MAX_DEPTH` 16) | sem recusa até 300 estrelas (typedef e array locais; saída com as 300 estrelas) | nenhuma até 300 | — | não | `/tmp/limites2/deep300`, `/tmp/limites2/td30` |
| 12a | locais de função (`K_LOCALS_MAX` 128) | 128 locais | 129 | `implementation-limit`: "insufficient storage while collecting module `l128`" | não | `/tmp/limites2/loc-i32128`, `loc-i32129` |
| 12b | globais do arquivo (mesmo limite) | 127 globais | 128 | `implementation-limit`: "insufficient storage while collecting module `g0`" (aponta `g0`, não o 128º) | não (mas localização errada) | `/tmp/limites2/gnf127`, `gnf128` |
| 13a | importações diretas de módulos (`WRITTEN_CAP` 1024; a arena vem antes) | 400 | 450 | `implementation-limit`: "arena capacity exhausted allocating KClosure; used 67074568 ... CGEN_ARENA_CAPACITY" | não | `/tmp/limites2/dir400`, `dir450` |
| 13b | cadeia transitiva de importações (`CGEN_SOURCES_CAP` 1024; a arena vem antes) | 400 | 500 | `implementation-limit`: "arena capacity exhausted allocating char" | não | `/tmp/limites2/chain400`, `chain500` |
| 14a | linhas de um fonte (sem teto próprio; limite é a arena) | 10 000 linhas (`fn10000`, 377 794 bytes): 42 s, pico 64 MB | 100 000 (`lin100000`, 3,9 MB): 0,12 s, arena `KAstNode` | `implementation-limit`: "arena capacity exhausted allocating KAstNode; used 30279512 ..." | não | `/tmp/limites2/fn10000`, `lin100000` |
| 14b | 1 000 000 linhas (41,8 MB) | falha | — | `implementation-limit`: "arena capacity exhausted allocating KLexeme; used 41779368 ..." em 1,09 s, 42 MB | não | `/tmp/limites2/fn1000000` |
| 14c | bytes de comentários (sem tokens), tempo | 8 000 linhas (478 934 bytes): 3,98 s, 4 MB | — (100 000 linhas não concluiu em 15 min; tempo quadrático) | — | não | `/tmp/limites2/cmt8000` |
| 14d | comprimento de constante de texto numa linha **sem** edição keel | 10 000 bytes (`strp10000`) | nenhuma até 10 000 | — | não | `/tmp/limites2/strp1000`, `strp10000` |
| 14e | comprimento de constante de texto numa linha **com** edição keel (`defer` na mesma linha) | linha-fonte de 1014 bytes (string de 955 bytes) | linha-fonte de 1015 bytes (string de 956 bytes) | `implementation-limit`: "a generated line needs more than LINE_CAP = 1024 bytes ...; break that source line into several lines (the limit is per line, not per file)" | não | `/tmp/limites2/strb` (último da bisseção), `strl1000` |
| 15 | funções por módulo (sem limite próprio; limite é a arena) | 10 000 funções (`fn10000`): 42 s, 64 MB | 11 000 (`fn11000`): arena `char`, 2,7 s; 12 000 (`fn12000`): arena `KIsland` | `implementation-limit`: "arena capacity exhausted allocating char; used 64346768 ..." | não | `/tmp/limites2/fn11000`, `fn12000` |

## Tempo de geração (quadrático em linhas de função)

Funções `pub i32 fnN(void) { return N; }`, uma por linha, `--stop-after=gen`:

| funções | tempo | pico de memória |
|---|---|---|
| 1 000 | 0,53 s | 9 MB |
| 2 000 | 1,86 s | 15 MB |
| 4 000 | 6,83 s | 28 MB |
| 8 000 | 26,07 s | 52 MB |
| 10 000 | 42,10 s | 64 MB |

A razão é cerca de 3,6 a 3,8 a cada dobra de N: o tempo cresce com o quadrado. Os comentários têm a mesma forma (2 000 linhas: 0,25 s; 8 000: 3,98 s). Vale a pena um olhar de desempenho, mesmo sem limite.

## Dúvidas (recusas silenciosas e mensagens enganosas)

1. **Silencioso (bug): `extent` com 5 ou mais grupos.** `pub extent struct s [d0, 2][d1, 3][d2, 4][d3, 5][d4, 6] {...}` retorna rc 0, e o `.type.h` gerado contém o texto keel `pub extent struct s [d0, 2]...` sem tradução. O emissor não reconhece a forma (`k_em_extent_at` recusa com `x->groups >= MAX_GROUPS`, `engine/emit/extent.c:96`), mas o caminho de recusa não emite diagnóstico, e o trecho escapa para o C. Com 4 grupos a extent sai correta. Repro: `/tmp/limites2/ext_dims-5/app/m.k`.
2. **Mensagem enganosa: contagem de `extent` além do 32º campo.** Com `k` como 33º campo, o diagnóstico diz que `k` "names no field of the struct", embora `k` seja campo. O causador é `if (x->fields < 32)` em `engine/emit/extent.c` (`x->field[]` tem 32 posições). Repro: `/tmp/limites2/ext_fields-31/app/m.k`. Com `k` antes dos campos, 35 campos passam (`/tmp/limites2/ext_fields_k2-35`).
3. **Mensagem enganosa: `match` pelo `SCRATCH_CAP`.** O limite que dispara com 15 rótulos é o texto de edição da linha do `match` (2048 bytes), e a mensagem manda "shorten the line", apontando a linha `match (s) {`. Quem escreve não consegue encurtá-la: o que cresce é o número de rótulos. Os valores dependem do tamanho dos nomes (14 rótulos curtos; 9 com nomes de 27 caracteres). O `MAX_LABELS` 32 nunca é atingido.
4. **Mensagem genérica em quase todos os limites de contagem do emissor.** `MAX_CAP` (defer), `MAX_CAPS` (parallel), `MAX_COLUMNS` (extent) e `MAX_RANK` (índices com 9 dimensões) saem como "the emitter does not cover this construction yet", sem dizer qual limite nem o valor. Compare com `MAX_INCLUDES`, que diz "a file needs more than MAX_INCLUDES = 32".
5. **Mensagem enganosa: `unexpected-token` em limites do parser.** Protocolo com 17 itens, verbo com 9 parâmetros e `array` do topo com 9 declaradores saem como "malformed or truncated declaration", sem indicar o limite. O programa está bem formado.
6. **Localização errada: globais.** Com 128 globais, o diagnóstico aponta `g0` (a primeira), e não a 128ª. O limite real de globais é 127 (com 128 locais dentro de função, o limite é 128).
7. **Forma de topo não coberta pelo emissor: `struct` com declaradores.** `pub struct S { int x; } s0;` é recusado com "the emitter does not cover this module's constructions yet", já com 1 declarador. A gramática (`keel-llm-context.md` §1, `decl-struct`) admite declaradores. Não é limite de contagem; é cobertura. Decisão para o orquestrador: lacuna da spec ou do emissor? Dentro de função, o mesmo código passa (trecho opaco).
8. **Protótipo sem corpo não coberto.** `pub i32 h(void);` (e `i32 g(void);`) sai com `implementation-limit` ("does not cover ... (at `pub`)"). A gramática admite `';'` em `decl-function`. Também é cobertura, e não limite de contagem; medido só para registrar, sem investigar a intenção.
9. **Rotulagem: o limite de `routine` silencia o preenchimento.** Com 18 ou mais campos, o gerado não tem nenhum `= {0}`, sem diagnóstico. Em C, o campo omitido num inicializador designado é zero, então o efeito é nulo, mas o desenho da tabela (zerar explicitamente) não se cumpre. Baixa gravidade; registrar.
10. **Não medido: função chamada pelo fechamento (8c).** O `K_PARAMS_MAX` do analisador de ilhas aparece em `isl_params_of`, e o caminho que o usa exige um fechamento de instância com mais de 16 parâmetros, que não montei. Os dois caminhos que medi (função não genérica e função de módulo genérico instanciado) não truncaram até 18 e 30 parâmetros, respectivamente.
11. **O limite de `#include` mede imports, não includes.** Com 32 imports, o `.h` tem 33 linhas `#include` (inclui `keel.type.h`). O diagnóstico conta por arquivo, e o teste passa com 32 imports. Não é divergência, só fica registrado para o manual.
12. **Limites de módulos e de fonte: a arena vem antes dos caps.** `WRITTEN_CAP` (1024) e `CGEN_SOURCES_CAP` (1024) não são atingidos: a arena de 64 MiB esgota com cerca de 400 a 450 módulos importados (direto ou em cadeia). Para o manual, o número útil é o da arena, não o de `WRITTEN_CAP`.
13. **Limite de linha: a mensagem fala em "generated line".** A linha que estoura é a do fonte, com edições keel, e a mensagem diz para quebrar "that source line", o que está certo. O valor 1014 bytes de linha-fonte é o medido com o `defer` deste caso, e pode mudar com o tamanho das edições.
14. **Desempenho: tempo quadrático em linhas.** Ver a tabela de tempo. Não é limite, mas o programa de 10 mil funções leva 42 s, e 100 mil linhas de comentário passam de 15 minutos sem concluir (interrompido).
