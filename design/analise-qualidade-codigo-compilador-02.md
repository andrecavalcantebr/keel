# Análise de Qualidade de Código do Compilador — 2ª rodada

> **Data:** 12 de novembro de 2026
> **Objeto:** `cgen` — o transpilador keel→C em `tools/cgen/src/` (~23.000 linhas de C, divididas entre `tool/` e `engine/`), mais os contêineres gerados em `tools/cgen/gen/keel/` que o sustentam.
> **Escopo:** o compilador em si — não a linguagem, não o C que ele gera, não a base.
> **Metodologia:** leitura direta de 100% dos arquivos de `tool/` e dos módulos centrais de `engine/` (lexer completo, AST, delimitadores, loader, emissor: writer, edits, mangle, files, defer, parallel, roles, instances, supported); **compilação com ASan+UBSan** e execução de (i) toda a suíte golden (174 invocações = 87 casos × 2 perfis), (ii) **fuzzing por mutação** de 494 arquivos derivados de golden/base, (iii) casos adversariais dirigidos (NUL byte, CRLF, splice `\`+newline, unicode cru, aninhamento profundo, ciclos de import, nomes enormes, `--instance` malformado); verificação do pipeline completo com `cc` real (`-c`, `-E`, `-MMD -MF`, link); compilação com flags extras (`-Wpedantic -Wshadow -Wconversion -Wmissing-prototypes` …).
> **Resultado dos testes dinâmicos:** zero falha de sanitizador em todas as execuções; suíte oficial `make check` integralmente verde (80 golden, 144 base, 51 diag, 18 CLI, 58 unit); zero warnings com os flags do projeto (`-Wall -Wextra`).
> **Relação com a 1ª rodada** (`analise-qualidade-codigo-compilador-01.md`): os achados persistentes estão marcados **[persiste-01]**; os novos, **[novo]**.
> **Nota sobre o estado da árvore:** a análise foi feita sobre o estado *committed* + árvore de trabalho; ao fim da rodada, a árvile de trabalho já continha uma correção não commitada do item S1 (ver lá).

---

## Matriz de avaliação

| Critério | Nota | Síntese |
| :--- | :---: | :--- |
| Segurança | **4.5** | Arena única elimina UAF/double-free por construção; `fork+execvp` sem shell; argv sempre dimensionado com checagem; ASan/UBSan limpos em ~670 execuções. Descontos: temporário com nome previsível (TOCTOU/symlink — correção já presente na árvore, não commitada) e estado estático mutável no motor. |
| Estabilidade | **4.3** | Fail-closed consistente: tudo que a fatia não cobre é recusado, nunca mistraduzido; fuzzing sem um único crash. Descontos: diagnóstico **falso** em aninhamento balanceado >256 e limites internos que se apresentam com mensagens erradas. |
| Manutenabilidade | **4.0** | Rastreabilidade normativa excepcional (cada função cita spec/design), zero warnings, suíte densa. Descontos: estilo hiperdenso em `tool.c`/`instances.c`/`stop_*.c`, re-lexing redundante, mutação destrutiva de AST compartilhada. |
| Extensibilidade | **4.2** | Emissor com uma arquivo por construção e dispatcher central — acrescentar construção é localizado; catálogo de diagnósticos gerado e sincronizado por script. Descontos: constantes de capacidade espalhadas (4 args de instância, 8 capturas, 16/32 campos) e o núcleo de ilhas monolítico. |
| Modularidade | **4.7** | Fronteira `engine/`↔`tool/` verificada **mecanicamente** pelo build (`make boundary`); inversão de dependência via callback `KLoader` permite testar o motor sem disco; base de contêineres auto-hospedada. Desconto: `stop_gen.c` concentra muitas responsabilidades. |
| Ergonomia | **4.2** | CLI drop-in no lugar do `cc` (pass-through verbatim, depfile com escape make, `-c/-S/-E/-shared` testados e funcionando); diagnósticos no formato gcc com ID estável. Descontos: mensagens conflitadas (linha longa ≠ construção não suportada; nome esperado vazio), `--pedantic-names` aceito e inerte, help com frase cortada. |

---

## 1. Segurança

### 1.1 O que está sólido

- **Arena única por invocação** (`tool/memory.h`, `gen/keel/keel_arena.h`): um `malloc` de 64 MiB no início, um `free` no fim. Não existe `free` individual em nenhum caminho — use-after-free e double-free são estruturalmente impossíveis. O alocador `keel_arena_alloc_` valida `len > SIZE_MAX/datasize`, alinhamento potência de 2, `top > cap`, e padding+need dentro do disponível — aritmética de alocação inteira checada.
- **Strings por comprimento, nunca por NUL**: `keel_slice_char {len, ptr}` em todo o motor. Um fonte com byte NUL embutido (testado) é lexado normalmente; nenhum `strcpy`/`sprintf`/`strcat` fora de buffers dimensionados exatamente antes (`base_resolve.c` aloca `strlen(a)+strlen(b)+1` antes do `strcpy`+`strcat`; `run_cc.c` valida `snprintf` e reserva 3 bytes antes do `strcpy(dot, ".o")`).
- **Sem shell em nenhum lugar**: `grep` confirma — nenhum `system()`, nenhum `popen`. O `cc` é invocado por `fork`+`execvp` com `argv` montado em array; o link transparente também (`execvp(cc, new_argv)`). Injeção por metacaracteres não existe por construção.
- **argv e vetores de CLI sempre com checagem de capacidade**: `require_room`/`ROOT_ROOM`/`ARG_ROOM` em `main.c`; `argv[CGEN_CLI_ARGS_CAP+8]` dimensionado para o pior caso (verificado por contagem: 256 pass-through + cc + `-I` + checks + unit + entry + NULL = 262 ≤ 264); `new_argv[CAP+2]` do link transparente cabe exato.
- **Escrita de saída atômica** (`tool/output.c`): temporário no mesmo diretório + `rename`; arquivo idêntico não é tocado (comparação byte a byte com curto-circuito correto em `same_content`).
- **Caminhos sem injeção**: normalização lexical pura (`tool/paths.c`, sem tocar disco); nome de módulo deriva de caminho e o declarado tem que bater (`module-path-mismatch`); `module ..evil;` e afins são recusados (testado); `import` só produz `<raiz>/<ident>/<ident>.k` — não há como um `.k` alcançar fora da raiz por texto de módulo.
- **Leitura de fonte** (`tool/read_source.c`): `fstat`+`fread` com verificação de leitura curta; tamanho negativo recusado; buffer exato da arena.

### 1.2 Achados

**S1. Temporário com nome previsível (TOCTOU/symlink) — [persiste-01 no commit; correção já na árvore], severidade baixa.**
No estado *committed*, `tool/output.c` criava o temporário com `snprintf(…, "%s.tmp.%ld", path, getpid())` + `fopen(tmp, "wb")`: em diretório gravável por outro usuário, um atacante local que preveja o PID pode pré-criar `saida.h.tmp.<pid>` como symlink e fazer o cgen gravar por cima do alvo do link. Ferramenta de build de estação única tem risco prático pequeno. **Ao fim desta rodada, a árvore de trabalho já contém a correção** (não commitada): `open(tmp, O_WRONLY|O_CREAT|O_EXCL, 0666)` + `fdopen`, com `close/remove` no erro — exatamente a recomendação da 1ª análise. Recomenda-se apenas o commit.

**S2. Estado estático mutável dentro do `engine/` — [novo] (parcialmente apontado na 01), severidade baixa hoje, armadilha amanhã.**
Três ocorrências quebram a pureza que os cabeçalhos do motor anunciam ("no I/O, no allocation" — esta se cumpre; a reentrância não é prometida, mas o design convida a assumi-la):
- `engine/parser/parser_delimiters.c:45-46` — `static size_t open[256]; static Group groups[64];` (ver B1 abaixo);
- `engine/parser/parser_header.c:163-164` — `static char store[32768]; static size_t used;` acumulado **entre módulos** durante a invocação inteira; ao encher, o bloco `injected-names` (uma *info*) é descartado em silêncio (`used = start; break;`);
- `engine/emit/use_paths.c:158` — `static KModule self;` reescrito a cada chamada (uso local hoje; o ponteiro escapa para decisões de fechamento).

Nada disso é bug na execução atual (single-thread, uso sequencial), mas: (i) impede paralelizar o carregamento de módulos no futuro; (ii) qualquer reuso do motor como biblioteca (o formato de teste unitário já liga o motor em probes) herda o estado. Recomendação: mover para `KAst`/contexto de emissão, como a 01 já sugeria para `parser_header.c`.

**S3. API da base permite acesso sem checagem — [novo], severidade informativa.**
`gen/keel/keel_buffer_char.h`: `get`/`set`/`ptr` indexam sem validar (a alternativa segura `at` existe e devolve `outcome`). O cgen é disciplinado no uso (índices sempre bornados antes), mas a API oferece o caminho curto. É um risco de uso, não um bug presente.

---

## 2. Estabilidade

### 2.1 O que está sólido

- **Fail-closed é uma lei no emissor**: `k_em_put_bytes`/`k_em_put_mangled`/`k_em_scratch`/`k_em_push_edit` checam capacidade e degradam para `e.ok = false` → `k_emit_file` devolve `K_EMIT_UNSUPPORTED` → a ferramenta recusa com `[implementation-limit]`. **Nenhum caminho encontrado escreve fora de buffer.**
- **Determinismo**: golden compara byte a byte em dois perfis (80 casos) e a base inteira (144) — verde.
- **Robustez comprovada empiricamente**: 494 arquivos mutados (byte-flip, truncagem, embaralhamento de linhas, injeção de `#if/#else/#endif`, `defer`, `parallel`, NUL, UTF-8 quebrado, splices) + casos dirigidos — **zero** ocorrência de AddressSanitizer/UBSan, zero crash, sempre um diagnóstico ou recusa limpa.
- **Ciclos de import** detectados com precisão (`self-import` e A↔B testados: "`ca` is already being loaded: importing it here closes a cycle"), com o estado por-módulo (`never/pending/done/failed`) correto para memoização.
- **Delimitadores por alternativa condicional** (`group_depth.c`, `parser_delimiters.c`): a conta de `{}` por ramo de `#if` e a concordância entre ramos é um problema difícil tratado com cuidado e testado.

### 2.2 Bugs e defeitos

**B1. Diagnóstico falso para aninhamento balanceado profundo — [novo], severidade média.**
`engine/parser/parser_delimiters.c`: `K_OPEN_MAX` é 256 e a pilha descarta aberturas além disso **silenciosamente** (`if (n < K_OPEN_MAX) open[n++] = i;`). Com 300 parênteses perfeitamente balanceados, os 44 fechamentos finais chegam com a pilha já vazia e o verificador emite ``error: `)` has no pair: it meets nothing [unmatched-delimiter]`` — ou seja, **acusa de desbalanceado um fonte que está balanceado**. Limiar medido por busca binária: passa em 256, falha em 257. O mesmo padrão de "capacidade estourada virando diagnóstico errado" está em `defer_scope.c` (`K_DEFER_BRACE_STACK_MAX 256`, blocos profundos saem da pilha sem aviso). Correção: ao ver `n == K_OPEN_MAX` num abre-delimitador, emitir `[implementation-limit]` nomeando o teto (o projeto já tem o hábito correto disso em `require_room`, `K_PROTOCOL_MAX_ITEMS`, `MAX_CAP`).

**B2. Nome de módulo esperado vazio quando o fonte não está sob nenhuma raiz — [novo], severidade média (ergonomia de erro).**
`tool/stop_parse.c` (e o caminho equivalente do gen): quando nenhum `-I`/cwd cobre o fonte, `name[0]='\0'` e o carregador compara o declarado contra a string vazia, produzindo ``expected module `` for this path, found `keel.arena` [module-path-mismatch]`` (reproduzido com `cgen --stop-after=parse ../../base/keel/arena.k` sem `-I`). O usuário lê "esperava módulo vazio". O diagnóstico correto é "o fonte não está sob nenhuma raiz de busca — adicione `-I`".

**B3. Linha longa reportada como construção não suportada — [persiste-01], severidade média.**
`engine/emit/emit_core.c` (`LINE_CAP 1024`): qualquer linha que o emissor **reconstrói** e que passa de 1024 bytes faz `e.ok=false`, que sobe como `K_EMIT_UNSUPPORTED` e vira `error: <arquivo>: the emitter does not cover this module's constructions yet [implementation-limit]`. Reproduzido: função com `defer` e `return` de expressão ~1600 bytes → recusa com a mensagem de "construções não cobertas", quando a construção está coberta e o que estourou foi o buffer interno da linha. (Linhas longas que só são **copiadas** atravessam sem problema — verificado com inicializador de 400 elementos.) A 01 já pedia a distinção; a mensagem única continua existindo. Correção barata: um bool distinto (`limit-hit`) e uma mensagem nomeando `LINE_CAP`.

**B4. Limites de lista viram "malformed declaration" — [novo], severidade baixa.**
`k_scan_ident_list` retorna `false` quando a lista excede `cap`, e o chamador trata qualquer `false` como erro sintático: `module g type A,…,I;` (9 binders, teto 8 em `KModuleHeader`) produz `malformed or truncated declaration [unexpected-token]` (reproduzido) em vez de um `[implementation-limit]`. Mesma família do B1/B3: **a recusa é segura, a mensagem aponta para o lugar errado**.

**B5. Frase cortada no help — [novo], severidade baixa.**
`tool/main.c`, `print_help`: `--main <module>  generate the C entry point, calling the module's` — a frase termina sem objeto (faltou "main" no fim, perdido em edição). Visível em `cgen --cgen-help`.

**B6. `--pedantic-names` aceito e inerte — [persiste-01], severidade baixa.**
`tool/main.c:286`: `(void)pedantic_names;` — a opção é validada, documentada no help ("name ceiling 31 (external) / 63 (other)") e não tem efeito algum. Silêncio aqui é pior que erro: o usuário acredita que garantiu portabilidade de nomes. Ou conecta ao mangler ou emite `warning: --pedantic-names is not yet implemented`.

**B7. Mensagem base-not-found com placeholder — [novo], severidade baixa.**
`tool/base_resolve.c`: no ramo em que `readlink("/proc/self/exe")` falha, a mensagem imprime o literal `<candidate>` (`no keel.k under '<candidate>'`) — placeholder de template vazou para a saída. (Também documenta a dependência Linux: ver N2.)

### 2.3 Notas

- **N1.** O critério de atualização "gera sempre e compara bytes" é **v1 por especificação** (`cgen-tool-spec.md` §5: "Estado da v1: o cgen gera sempre") — `closure_mtime` é carregado e propagado mas ainda não consulta nada. Não é dívida escondida; é preparação documentada.
- **N2.** `main.c` com `argc==1` faz `execvp("cc", argv)` mantendo `argv[0]="cgen"` — o cc roda com nome de programa errado (suas mensagens dirão "cgen:"). Escolha deliberada de transparência; efeito cosmético.
- **N3.** Diagnósticos podem carregar bytes crus do fonte para o stderr (ex.: NUL byte de um literal inválido é impresso como byte). Inofensivo.
- **N4.** `extent_scan` trunca silenciosamente >16 colunas / >32 campos (`parser_islands_flow.c`): o excedente atravessa como C e o erro aparece no compilador C, longe da causa. Falha segura, mensagem distante — mesma família do B3/B4.

---

## 3. Manutenabilidade

**Pontos fortes.** A rastreabilidade é a melhor que já vi num projeto deste porte: cada arquivo abre com a seção normativa que implementa e cada invariante de difícil leitura (o `#line` do `writer.c`, o ladder do `defer.c`, a pilha de blocos do `defer_scope.c`) tem um comentário que explica *por que* a forma ingênua está errada — inclusive com a data em que um comentário antigo mentia e bloqueou uma tarefa (`parser.h`, sobre `k_scan_braced_opaque`). Zero warnings com `-Wall -Wextra`. O catálogo de diagnósticos tem fonte única (`diag-messages.md` → `.def` via `diag-sync.py`) com teste de cobertura (140/148) — mudar mensagem não desincroniza.

**Dívidas.**
- **M1. Estilo hiperdenso — [persiste-01].** `tool.c`, `instances.c`, `names.c`, `stop_parse.c`, parte de `stop_gen.c` empilham três comandos por linha sem espaços. `instances.c` (112 linhas) contrasta com o padrão do resto do motor: é o único arquivo que parece de outro projeto. Custo real em diff/blame/GDB.
- **M2. Re-lexing redundante — [persiste-01].** `reenter()` (`ast.c`) e lexers descartáveis sobre a cauda do fonte (`instances.c`, `parser_islands_flow.c`) re-lexam o mesmo texto para reaproveitar reconhecedores escritos contra `KLexer` vivo. Funciona (o lexer é puro), mas é custo O(n) repetido e uma costura conceitual que todo novo reconhecedor precisa saber costurar.
- **M3. Mutação destrutiva de AST compartilhada — [novo].** `stop_gen.c` (`emit_instances`): após escrever os headers de cada instância, zera `generic->islands.len / island_text.len / closure.len / closure_text.len` (com cast fora de `const`) para a próxima `k_collect_instance_islands`. É invalidação manual de cache por mutação — funciona porque a emissão é sequencial, mas é exatamente o tipo de estado que quebra em silêncio quando a ordem muda.
- **M4. Contratos de comentário ≠ código — [novo], menor.** `k_scan_ident_list` promete "never writes past out[cap-1]" mas escreve `out[0]` antes de qualquer checagem (seguro hoje: todos os chamadores passam `cap=8`). `k_scan_qualified_name` assume "`.` sempre seguido de IDENT" e o degradado (EOF) devolve um slice válido mas que engole até o fim — correto por acidente, não por desenho.

---

## 4. Extensibilidade

- **X1. Emissor exemplar.** Uma construção = um arquivo (`defer.c`, `foreach.c`, `match.c`, `parallel.c`, …), todos atrás de um dispatcher único (`emit.c: k_em_node_edits`) e de um gate único (`supported.c: k_em_handles_*`). Acrescentar uma construção nova toca um arquivo novo, duas linhas no dispatcher e uma no gate. O modelo de *edits* sobre a linha (DROP/RENAME/LINES/…) é uma abstração boa e estável.
- **X2. Diagnósticos extensíveis.** Catálogo em `.def` + documento markdown + lock JSON; adicionar diagnóstico é um caminho só.
- **X3. Constantes de capacidade espalhadas** [persiste-01 em parte]: `K_EMIT_INSTANCE_ARGS 4` (argumentos de instância), `MAX_CAP 8` (capturas de `defer [now]`), `MAX_CAPS 16` (parallel), `K_EXTENT_MAX 16`/`field[32]`, `K_PROTOCOL_MAX_*`. Todas recusam com segurança quando estouram (a maioria com `[implementation-limit]` correto), mas são independentes entre si e não centralizadas — quem amplia a linguagem precisa caçá-las uma a uma.
- **X4. O núcleo de ilhas é o gargalo.** `parser_islands*.c` somam ~4.900 linhas em quatro arquivos com enumeração de estágios e varreduras que se repetem por construção. É a parte do sistema onde a extensão custa mais e onde o estilo denso mais pesa.

---

## 5. Modularidade

- **MO1. A fronteira é verificada pelo build** (`Makefile` alvo `boundary`): `engine/` não pode incluir `stdio/stdlib/unistd/…` — violação quebra o build. Invariante arquitetural que vira teste é diferencial raro.
- **MO2. Inversão correta** (`engine/loader.h`): o parser precisa de símbolos de outros módulos; quem abre arquivo é a tool, injetada por callback. É o que permite os unit tests atuais ligarem o motor puro em probes com sanitizadores, sem diretório temporário.
- **MO3. Base auto-hospedada**: os contêineres que o compilador usa (arena/buffer/slice/outcome) são gerados pelo próprio pipeline keel→C (`tools/transform` → `gen/`). Elegante e dogfooding real; o custo é que mexer em contêiner exige regenerar `gen/` (o Makefile já isola isso com o truque do `gen.new/` para não sujar mtimes).
- **MO4. `stop_gen.c` (681 linhas)** concentra: caminhos absolutos, perfil, lowering, mkdir-p, emissão de módulo, de imports, de instâncias, de funções sobre protocolo e da entry unit. É o único lugar onde a separação de responsabilidades da tool layer amolece.

---

## 6. Ergonomia

**Pontos fortes.**
- **Drop-in no lugar do `cc`** (testado de ponta a ponta com cc real): pass-through verbatim e em ordem, leitura dupla de `-I`/`-std=`/`-fopenmp`/`-M*` "pela mesma linha de comando que o cc lê — para os dois lados nunca discordarem" (spec), link transparente quando não há `.k`, `-c`/`-S`/`-E`/`-shared` tratados, **depfile** com a closure dos `.k` fundida à regra do compilador e escapes de make corretos (verificado o `.d` gerado).
- **Diagnósticos no formato gcc** (`arquivo:linha:col: severidade: mensagem [id-estável]`), com severidades contadas separadamente, infos de nomes injetados, e saída 0/1/2 coerente (0 ok, 1 fonte com erro, 2 falha da ferramenta).
- **Escrita que respeita o build**: arquivo idêntico não mexe no mtime; `-f` quando o usuário quer forçar.

**Defeitos** (detalhados em B2–B7): a mensagem de "construção não suportada" serve para quatro condições diferentes (não suportada de verdade, linha longa, lista cheia, aninhamento fundo); o nome esperado vazio; `--pedantic-names` inerte; help com frase cortada; placeholder `<candidate>` vazando. Nenhum impede uso — todos custam tempo de usuário exatamente na hora em que ele já tem um problema.

---

## 7. Correlação com a 1ª análise (01)

| Achado da 01 | Situação nesta rodada |
| :--- | :--- |
| TOCTOU em `output.c` | **Corrigido na árvore de trabalho** (O_EXCL), pendente de commit (S1) |
| `store[32768]` estático em `parser_header.c` | **Persiste** (S2) |
| `--pedantic-names` silencioso | **Persiste** (B6) |
| Mensagem opaca ao estourar buffer interno | **Persiste** — agora reproduzido e isolado: é a conflação do B3 |
| Densidade extrema (`tool.c`, `parser_islands.c`, `names.c`, `instances.c`) | **Persiste** (M1) |
| Re-lexing redundante | **Persiste** (M2) |
| `-Wall -Wextra` limpo | **Confirmado** (e com flags extras: só ruído pedantic do código *gerado*, `_Float16` etc.) |
| Arena, `fork+execvp`, `make boundary`, depfile | **Confirmados e re-testados dinamicamente** |
| — | **Novos nesta rodada:** B1 (delimitador falso >256), B2 (nome esperado vazio), B4 (limites como malformed), B5 (help cortado), B7 (placeholder), S2 (statics do engine catalogados por completo), M3 (mutação de AST), N1–N4 |

---

## 8. Recomendações priorizadas

1. **B1 + B4 + B3 (uma mudança de hábito, três bugs fechados):** capacidade estourada deve produzir `[implementation-limit]` com o nome do teto — nos abre-delimitadores de `parser_delimiters.c`, nas listas de `k_scan_ident_list`, e um canal distinto para `LINE_CAP` no `Emit` (não `K_EMIT_UNSUPPORTED` genérico). O projeto já tem o padrão pronto em `require_room`; é estendê-lo.
2. **S1:** commit da correção `O_EXCL` já presente na árvore.
3. **B2:** diagnóstico próprio para "fonte fora de toda raiz de busca", com a sugestão de `-I`.
4. **B6:** conectar `--pedantic-names` ao mangler ou avisar "not yet implemented".
5. **S2:** mover os três statics do motor para o contexto (`KAst`/`Emit`/`CgenTool`) — barato hoje, caro depois de qualquer paralelização.
6. **B5/B7:** help e mensagem do `base_resolve` (uma linha cada).
7. **M1/M3:** normalizar o estilo dos arquivos densos em ocasiões futuras de edição (não como refactor isolado); registrar no `stop_gen.c` por que a invalidação da AST por instância é segura *na ordem atual*.

---

## 9. Conclusão

O `cgen` é um compilador pequeno com disciplina de projeto incomum: memória por arena com aritmética checada, fronteira arquitetural verificada pelo build, fail-closed em tudo que não sabe traduzir, determinismo garantido por contrato de teste byte a byte, e documentação que rastreia cada função à sua norma. A evidência dinâmica acompanha a estática: ~670 execuções sob ASan+UBSan (golden, fuzzing por mutação e adversarial) sem uma única ocorrência de sanitizador, e toda entrada ruim produz diagnóstico — nunca crash.

Os defeitos reais encontrados não são de segurança nem de tradução errada: são **diagnósticos que mentem** (aninhamento balanceado acusado de desbalanceado; linha longa acusada de construção não suportada; nome esperado vazio; lista cheia acusada de malformada) e **opções que prometem e não entregam** (`--pedantic-names`). Numa ferramenta cuja proposta de valor é "quando não consegue decidir, recusa em vez de adivinhar", a mensagem errada na recusa é o próximo degrau de qualidade — e é também a correção mais barata da lista.
