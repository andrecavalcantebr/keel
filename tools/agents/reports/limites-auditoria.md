# limites-auditoria: limites de implementação do cgen

Auditoria só de leitura, feita em 2026-11-23 sobre o commit `85a387d` (branch `0.4.0`). Nenhum código, documento normativo, golden ou teste foi alterado. Os programas de medição ficaram em `/tmp/limites/`, fora do repositório.

**Como se mediu.** Cada limite de A com valor numérico foi confirmado com um `.k` mínimo, rodando `tools/cgen/cgen -std=c23 --base-dir base -I . --stop-after=gen --dest-dir <tmp> <arquivo>.k` (ou `--stop-after=parse` quando o limite é de binders). Os nomes de módulo seguem o diretório, como exige o cgen (`module-path-mismatch`). Onde a medição não foi feita, a linha diz "não medido".

**Como ler a coluna "configurável".** `-D` quer dizer que a constante tem `#ifndef` e pode ser trocada na compilação do cgen. "fixo" quer dizer `#define` sem guarda, ou constante de tipo/tabela no código.

## A. Limites do usuário

Atingíveis por um programa keel razoável. Os de valor medido aparecem com o resultado.

| Limite | Valor | O que limita (do usuário) | Configurável | Diagnóstico quando atingido | Onde | Medição |
|---|---|---|---|---|---|---|
| Itens de um `tags` | 128 | constantes de um conjunto `pub tags` | fixo | `tags-limit-exceeded` | `engine/parser/parser.h:165` (`K_TAGS_MAX_ITEMS`), recusa em `engine/parser/parser_decls.c:284` | 128 itens em linhas separadas: ok. 129: `tags-limit-exceeded`. Ver Dúvida 1 |
| Bytes de C de **uma linha** do fonte que vira `tags` | 1024 (≈62 itens numa linha só) | a linha física do fonte que o cgen reescreve | fixo | `implementation-limit`: "a generated line needs more than LINE_CAP = 1024 bytes ... break that source line" | `engine/emit/emit_internal.h:14` (`LINE_CAP`) | `pub tags Op [T0, ..., T61];` em uma linha: ok. Com 63 itens: `implementation-limit`. Com 64, 80, 100, 120: idem |
| Binders de um módulo, por lista (`dim`, `tags`, `type`) | 8 cada | binders de `module m dim ... tags ... type ...;` | fixo | `implementation-limit` com texto genérico "insufficient storage while collecting module `m`" | `engine/parser/parser.h:43-45` (`KModuleHeader`) | `dim A, ..., H` (8): ok. `dim` com 9: recusado. `tags` com 9 e `type` com 9: recusados. 5 `dim` + 4 `type` (9 no total): ok, logo o limite é por lista, não somado. Ver Dúvida 2 |
| Aninhamento de delimitadores `( ) [ ] { }` | 256 | parênteses, colchetes e chaves aninhados num mesmo arquivo | fixo | `implementation-limit`: "delimiters nested deeper than 256" | `engine/parser/parser_delimiters.c:9` (`K_OPEN_MAX`), `:88` | 255 níveis de `(`: ok. 256: `implementation-limit` |
| Níveis de modificador aninhado (`instance-depth`) | 9 níveis usados (o décimo recusa) | `buffer buffer ... i32` (`keel.buffer`), instâncias aninhadas | fixo | `instance-depth` | `engine/parser/parser.h:296` (`K_SPEC_MAX_DEPTH 8`), `engine/parser/parser_declarator.c:196,256` | 9 `buffer`: ok. 10: `instance-depth` (o mesmo caso de `test/cgen/diag/instance-depth`) |
| Nome gerado de uma instância | 255 caracteres (teto do backend §2.4, D21) | nome de instância do modificador, que inclui o caminho do módulo de uso | fixo | `name-too-long` ("generated name exceeds the backend limit of 255 characters") | `engine/instances.c:98`, `engine/diag_messages.def:23` | Módulo `app.pbox` com `box(3)` sobre tipo `T` de L caracteres, em `app.use`: L=234, 236, 238 e 240 dão `name-too-long`. L=230 e 232 dão antes `implementation-limit` pelo `NAME_CAP` (ver Dúvida 3). L=200 é ok |
| Nome de arquivo ou símbolo gerado (inclui o que vai para o nome do arquivo) | 256 bytes | nome do arquivo `.c`/`.h` gerado para a instância, mais guardas | fixo | `implementation-limit`: "a generated name or path needs more than NAME_CAP = 256 bytes" | `engine/emit/emit_internal.h:13` (`NAME_CAP`) | ver a linha anterior; o limite de arquivo dispara antes do de 255 em nomes de ~230 a 233 caracteres |
| Identificadores de usuário fora de instância | sem teto próprio na prática | nomes de função, de tipo | não se aplica | nenhum | não se aplica | função com identificador de 300 caracteres: ok, compila e gera o `.h`. O teto de 255 vale só para nomes **gerados** |
| Memória do cgen (arena) | 64 MiB (`67108864` bytes) | o tamanho do programa: tokens, símbolos, instâncias; medido com um `pub array i32` de N elementos numa linha por dez | `-D CGEN_ARENA_CAPACITY=...` (build) | `implementation-limit`: "arena capacity exhausted allocating <T>; used ..., configured limit ... bytes" | `tool/memory.h:11` (`CGEN_ARENA_CAPACITY`, com `#ifndef`), `tool/memory.h:32` | array de 50 000 elementos: ok. 100 000: "arena capacity exhausted allocating KInstanceUse". 200 000: "allocating KSymbol" (e depois `module-load-failed`). Ver Dúvida 6 |
| Argumentos repassados ao cc (`-I`, `-D`, ...) | 256 | toda opção que não é do cgen é repassada ao cc e conta aqui | `-D CGEN_CLI_ARGS_CAP` (build) | `implementation-limit`: "forwarded arguments limit exceeded: requested N, configured limit 256; adjust CGEN_CLI_ARGS_CAP" | `tool/cli_limits.h:8`, `tool/main.c:31` (`require_room`), `tool/main.c:36` (`ARG_ROOM`) | 127 pares `-I .` (254 argumentos): ok. 128 (257 argumentos): recusado. Ver Dúvida 5 |
| Raízes de busca (`roots`) | 128 | caminhos de busca de módulos | `-D CGEN_CLI_ROOTS_CAP` (build) | `implementation-limit`: "search roots exceed configured limit 128; adjust CGEN_CLI_ROOTS_CAP" | `tool/cli_limits.h:5`, `tool/stop_gen.c:360,466`, `tool/tool.c:322` | não isolado: passar 128 raízes com `-I` esbarra antes no limite de argumentos repassados. Ver Dúvida 5 |
| Includes de um arquivo gerado | 32 | `#include` gerados num mesmo arquivo | fixo | `implementation-limit`: "a file needs more than MAX_INCLUDES = 32" | `engine/emit/emit_internal.h:222` (`MAX_INCLUDES`), `engine/emit/files.c:356`, `engine/emit/use_paths.c:213` | 33 `import_c <hN.h>` num módulo: ok (limite não atingido). Não medido o caminho de emissão que conta os includes |
| Parâmetros de função | 16 no analisador de ilhas | parâmetros de função no caminho de análise (genérico e fechamento) | fixo | nenhum medido | `engine/parser/parser_islands_internal.h:14` (`K_PARAMS_MAX`) | função não genérica com 17 parâmetros: ok, o `.h` sai com `p0..p16`. Caminho genérico/fechamento não medido. Ver Dúvida 7 |
| Declaradores de um `struct` | 8 | nomes numa mesma declaração de `struct` | fixo | `parser.h:237` diz "não casa" (recusa) | `engine/parser/parser.h:237` (`K_STRUCT_MAX_NAMES`) | 9 campos `int a0; ... a8;` numa `typedef struct`: ok, sem diagnóstico. Caminho não coberto pela constante. Não medido o caminho que a usa |
| Dimensões de um array (`MAX_RANK`) | 8 | índices de um uso e dimensões de um array | fixo | `index-rank` (desde 0.4.0, commit 85a387d) | `engine/emit/index.c:19` (`MAX_RANK`) | array `int a[2]...[2]` com 9 dimensões: ok. Não medido o uso `x[i, ...]` com 9 índices |
| Dimensões de `extent` | 4 | dimensões de `extent` | fixo | diagnóstico do parser (não medido) | `engine/parser/parser.h:200` (`K_EXTENT_MAX_DIMS`) | não medido |
| Grupos e colunas de `extent` | 4 grupos, 8 colunas | colunas de tabela de `extent` | fixo | `extent-*` (não medido) | `engine/emit/extent.c:28-29` (`MAX_GROUPS`, `MAX_COLUMNS`) | não medido |
| Itens de `protocol` | 16 itens, 8 parâmetros | itens de um protocolo | fixo | não medido | `engine/parser/parser.h:575-576` | não medido |
| Entradas de `defer [now ...]` | 8 | lista de um `[now ...]` | fixo | não medido | `engine/emit/defer.c:45` (`MAX_CAP`) | não medido |
| Rótulos de `match` | 32 | rótulos de um `match` | fixo | não medido | `engine/emit/match.c:44` (`MAX_LABELS`), `:64` (`BLOCK_CAP`) | não medido |
| Campos de `routine` | 16 | campos de uma `routine` | fixo | não medido | `engine/emit/routine.c:31` (`MAX_FIELDS`) | não medido |
| Capturas de bloco paralelo | 16 | capturas de `parallel` | fixo | não medido | `engine/emit/parallel.c:39` (`MAX_CAPS`) | não medido |
| Nomes de declarador (`typedef`, `array`) | 8 cada | nomes numa mesma declaração | fixo | não medido | `engine/parser/parser.h:417` (`K_TYPEDEF_MAX_NAMES`), `:505` (`K_ARRAY_MAX_NAMES`) | não medido |
| Profundidade de declarador | 16 | ponteiros e sufixos aninhados numa declaração | fixo | recusa do reconhecedor (não medido) | `engine/parser/parser.h:363` (`K_DECLARATOR_MAX_DEPTH`) | não medido |
| Módulos escritos numa chamada | 1024 | módulos cujo cabeçalho é escrito | fixo | `implementation-limit`: "imported modules exceed the writing limit 1024" | `tool/stop_gen.c:123` (`WRITTEN_CAP`), `:158` | não medido (ver B) |
| Máscara `mask` de `routine` | 64 slots | entradas de `routine.mask(s)` | fixo (64 bits de `u64`) | `mask-above-64-slots` (`debug`, em tempo de execução) | `keel-c-backend.md` §5.17, `:1839`, `:2150` | não medido (é verificação de tempo de execução) |

## B. Limites internos

Só atingíveis por entrada absurda ou gerada, ou por capacidade de buffer do emissor. Ficam com o diagnóstico que mostram quando atingidos, mas não são de uso normal.

| Limite | Valor | O que limita | Configurável | Diagnóstico | Onde |
|---|---|---|---|---|---|
| `MAX_EDITS` | `LINE_CAP / 2` = 512 | edições de uma linha do fonte | fixo | `implementation-limit` (`K_EMIT_LIMIT_EDITS`) | `engine/emit/emit_internal.h:15` |
| `SCRATCH_CAP` | 2048 | texto apontado pelas edições de uma linha | fixo | `implementation-limit` (`K_EMIT_LIMIT_SCRATCH`) | `engine/emit/emit_internal.h:16` |
| `PAR_HEADER_CAP` | 1000 | cabeçalho de um bloco `parallel` | fixo | `implementation-limit` (`K_EMIT_LIMIT_HEADER`) | `engine/emit/emit_internal.h:17` |
| `TEXT_CAP` | 128 | texto de um verbo (produto do binder) | fixo | interno | `engine/emit/core.c:54` |
| `DIGITS_MAX` | 9 | dígitos de decimal, como o `isl_digits` do parser | fixo | interno | `engine/emit/index.c:20` |
| `MAX_WORDS` | 3 | palavras do detalhe após a seta | fixo | interno | `engine/emit/subscript.c:28` |
| `CB_NAME_MAX` | 200 | nome-fonte de `constexpr_block` (o gerado cabe em `NAME_CAP`) | fixo | interno | `engine/emit/constexpr_block.c:13` |
| `CGEN_SOURCES_CAP` | 1024 | arquivos `.k` que a unidade alcança por import | fixo | não medido | `tool/tool.h:18` |
| `CGEN_CLOSURE_MAX` | 128 | entradas do fechamento de instâncias e verbos perdidos, por módulo | `-D CGEN_CLOSURE_MAX` | não medido | `tool/cli_limits.h:22`, `tool/tool.c:276` |
| `CGEN_CLOSURE_TEXT` | 65536 | texto das linhas do dump de fechamento | `-D CGEN_CLOSURE_TEXT` | não medido | `tool/cli_limits.h:17-18` |
| `CGEN_UNAVAILABLE_MAX` | 512 | entradas de `unavailable` | `-D CGEN_UNAVAILABLE_MAX` | não medido | `tool/cli_limits.h:25`, `tool/tool.c:277` |
| `CGEN_ISLAND_TEXT_PER_TOKEN`, `CGEN_ISLAND_TEXT_BASE` | 32, 1024 | texto do dump das ilhas: bytes por token, mais base | `-D` | não medido | `tool/cli_limits.h:13-14,27-28`, `tool/tool.c:272` |
| `USE_CAP` | 4096 | buffer de caminhos e de uso do `--instance` (`put` recusa quando não cabe) | fixo | interno (`put` retorna falso) | `tool/stop_gen.c:449` |
| `K_DIAG_MAX_ARGS` | 3 | argumentos de um diagnóstico | fixo | interno | `engine/diag.h:49` |
| `K_INSTANCE_NAME_CAP` | 256 | nome de instância em `KInstanceUse` (255 + zero) | fixo | interno (`name-too-long` antes) | `engine/storage_types.h:61` |
| `K_SYMBOL_MAX` | 256 | símbolo C de um tipo nos resolvedores de ilha; nomes canônicos de `tags` | fixo | pode truncar em silêncio (`parser_islands_checks.c:29` pula o nome) | `engine/parser/parser_islands_internal.h:47` |
| `K_DETAIL_MAX` | 1024 | texto de detalhe de uma ilha | fixo | interno | `engine/parser/parser_islands_internal.h:17` |
| `K_TYPES_MAX` | 256 | pool de tipos de uma resolução | fixo | interno | `engine/parser/parser_islands_internal.h:46` |
| `K_LOCALS_MAX` | 128 | locais e globais do arquivo, no analisador de ilhas | fixo | interno | `engine/parser/parser_islands_internal.h:13` |
| `K_SURFACE_MAX` | 192 | superfície do fechamento | fixo | `isl_*` (interno) | `engine/parser/parser_islands_internal.h:126` |
| `K_CLOSURE_MAX` | 64 | fechamento de ilhas | fixo | interno | `engine/parser/parser_islands_internal.h:160` |
| `K_IMPORT_CLOSURE_MAX` | 64 | módulos do fechamento de import | fixo | interno | `engine/parser/parser_islands.c:418` |
| `K_SEED_MAX` | 48 | sementes do fechamento | fixo | interno | `engine/parser/parser_islands.c:701` |
| `K_BRACE_STACK_MAX` | 256 | pilha de chaves | fixo | interno | `engine/parser/parser_islands_flow.c:5` |
| `K_EXTENT_MAX` | 16 | extents no analisador de ilhas | fixo | interno | `engine/parser/parser_islands_flow.c:625` |
| `ROLE_UNION_NAMES_MAX` | 8 | nomes de união de papéis | fixo | interno | `engine/parser/parser_islands_closure.c` (perto de `:323`) |
| `K_GROUP_MAX` | 64 | grupos condicionais do analisador de delimitadores (C requer 63 níveis, C11 5.2.4.1) | fixo | interno | `engine/parser/parser_delimiters.c:10` |
| `K_GROUP_DEPTH_MAX` | 64 | grupos `#if` do lexer; além disso não são seguidos | fixo | interno | `engine/lexer/lexer.h:71`, `engine/lexer/group_depth.c:41` |
| `K_DEFER_BRACE_STACK_MAX` | 256 | pilha de chaves do escopo de `defer` | fixo | `implementation-limit` quando `defer` cruza um bloco | `engine/emit/defer_scope.c:63` |

## Dúvidas

Tudo em que o código, os documentos e o comportamento medido discordam ou ficam em aberto. Nenhuma foi corrigida.

1. **`tags` numa linha só recusa muito antes de 128.** A spec e a memória dizem `tags` de até 128 itens. Mas uma linha do fonte com mais de 62 itens estoura `LINE_CAP` (1024) e cai em `implementation-limit`, que manda "quebrar a linha". Com a lista em linhas separadas, 128 passa. Para o usuário o limite efetivo, numa linha, é ~62 itens, e a mensagem diz "a generated line", embora a linha seja a do fonte. Candidato a bug de geração de linha. Repro: `module m; pub tags Op [T0, ..., T63];` (64 itens numa linha).
2. **Binders: o limite é por lista, e a mensagem não diz qual.** O comentário de `parser.h:46-49` fala em "the 8 binders KModuleHeader stores", que soa como total. Medido: 8 `dim` ok, 9 `dim` recusado; 5 `dim` + 4 `type` ok. A mensagem é a genérica "insufficient storage while collecting module", sem o nome da lista nem o limite de 8. Sugestão: o diagnóstico dizer "more than 8 dim binders" (ou o equivalente) em vez do texto genérico.
3. **Nome gerado: duas recusas, duas mensagens.** Para um nome de instância, a spec (§2.4) fixa 255 como o único teto. Mas o arquivo gerado (`NAME_CAP` 256, com o caminho do módulo de uso) estoura antes: com o probe do `pbox`, L=230 e L=232 dão `implementation-limit` ("needs more than NAME_CAP = 256 bytes"), e L=234 em diante dá `name-too-long`. Ou seja, nomes que a spec aceita são recusados com outro diagnóstico. Candidato a bug (ou a escrever a regra do arquivo na spec). Repro: `/tmp/limites/nm232` (módulo `app.use` com `box(3)` sobre `typedef int` de nome com 232 caracteres, importando `app.pbox`).
4. **`instance-depth`: o limite visto é 9 níveis, e a constante é 8.** Conferido: `K_SPEC_MAX_DEPTH 8` com a checagem `depth > K_SPEC_MAX_DEPTH` (`parser_declarator.c:196`) e o argumento que não cobra nível (comentário em `parser_declarator.c:312-318`) dão 9 níveis úteis. Coerente com o código; registrado para o manual não dizer "8".
5. **`-I` esbarra no limite de argumentos repassados, não no de raízes.** Cada `-I` vira dois argumentos (`-I`, caminho) e o cc repassa tudo; 128 pares dão 257 argumentos e o limite de 256 dispara antes de `CGEN_CLI_ROOTS_CAP` (128). Assim o teto de raízes não é alcançável só com `-I`. Medido com 127 pares (ok) e 128 (`forwarded arguments limit exceeded`). Vale decidir se as raízes têm outra via de entrada, ou se a tabela documenta o efeito.
6. **Arena de 64 MiB: o limite é baixo para um array literal grande.** Um `pub array i32` de 50 000 elementos passa e 100 000 não (`arena capacity exhausted allocating KInstanceUse`). A mensagem é clara e aponta `CGEN_ARENA_CAPACITY`, mas o limite é só de build. Vale a tabela de A dizer o tamanho em elementos, ou subir o padrão. Fica para o orquestrador decidir.
7. **`K_PARAMS_MAX` 16 no analisador de ilhas.** Função não genérica com 17 parâmetros sai correta. O caminho genérico e o de fechamento usam `isl_params_of(..., K_PARAMS_MAX)`; não medi se, com 17 parâmetros, a análise trunca em silêncio. Não medido.
8. **Limites sem medição.** Os de A marcados "não medido" (`extent`, `protocol`, `defer [now]`, `match`, `routine`, `parallel`, declaradores, profundidade de declarador, `mask`) e os de B de fechamento e de ilha precisam de um probe próprio. O `mask-above-64-slots` é de tempo de execução e exige rodar o binário gerado com `debug`.
9. **Identificadores de usuário sem teto.** Um nome de função de 300 caracteres passou e foi emitido. O teto de 255 vale só para nomes gerados (spec §2.4 e `name-too-long`). Não é divergência, mas a tabela deve dizer isso.
10. **Mensagens de capacidade genéricas.** Vários limites de A caem em "insufficient storage while collecting module" (binders, delimitadores, `tags` do parser). O texto que diz o limite existe só em alguns casos (delimitadores: "nested deeper than 256"). Sugestão de padronizar.
11. **Cobertura de teste.** `test/cgen/cli/tags_many.sh` cobre `tags-limit-exceeded` (não li o script). `golden/cases/081-large-table` cobre um array de 120 linhas, não o limite de arena nem o de linha.
