# Auditoria e Análise da Qualidade de Código do Compilador (`cgen`)

> **Data da Análise:** 11 de Novembro de 2026  
> **Objeto de Análise:** Código-fonte em C da implementação do compilador (`tools/cgen/src/`)  
> **Escopo:** Exclusivamente a arquitetura, qualidade de código, segurança e robustez do **compilador em si** (não avalia as propriedades do código C gerado pelo compilador nem a linguagem Keel).  
> **Metodologia:** Inspeção estática de código, auditoria de segurança de processos e ponteiros, análise de acoplamento e rastreabilidade com o build system.

---

## Sumário Executivo

O compilador `cgen` é implementado em C (C2x/C11) com aproximadamente 13.000 linhas de código distribuídas entre o driver da ferramenta (`tool/`) e o motor de tradução em memória (`engine/`). 

A implementação destaca-se positivamente por adotar uma arquitetura de compilação pura em memória com **Arena de Alocação Linear**, eliminando classes inteiras de bugs de memória dinâmica (*Use-After-Free* e *Double Free*), por executar o compilador nativo via chamadas diretas de sistema (`fork+execvp`) sem intermediação de shell, e por possuir um mecanismo de build (`make boundary`) que valida mecanicamente o isolamento entre E/S e lógica de compilação.

Por outro lado, a base de código do compilador apresenta dívidas técnicas significativas:
1. **Limites estáticos rígidos** espalhados em macros (`LINE_CAP 1024`, `MAX_EDITS 32`, `WRITTEN_CAP 1024`), que causam abortos abruptos do compilador diante de fontes com linhas longas;
2. **Estilo de código hiperdenso** em arquivos cruciais (`tool.c`, `parser_islands.c`, `names.c`, `instances.c`), com comandos empilhados na mesma linha e sem espaçamento, dificultando depuração passo a passo com GDB e revisões de código;
3. **Vulnerabilidade de corrida/symlink** na criação de arquivos temporários em `output.c`;
4. **Re-lexing redundante:** O motor instancia repetidamente lexers descartáveis (`KLexer`) sobre substrings da memória durante o parse, por falta de iterador sobre os tokens já lidos.

### Matriz de Avaliação do Compilador

| Critério | Nota (1 a 5) | Diagnóstico da Implementação do Compilador |
| :--- | :---: | :--- |
| **1. Segurança** | **4.0** | Arena única previne UAF/vazamentos; `fork+execvp` previne shell injection; vulnerabilidade TOCTOU em temporários de `output.c`. |
| **2. Estabilidade** | **4.1** | Determinismo estrito; detecção de ciclos em imports; fragilidade por limites fixos de buffer (`LINE_CAP`) e dependência do Linux em `/proc/self/exe`. |
| **3. Manutenabilidade** | **3.9** | Rastreabilidade exemplar com as specs normativas e zero warnings (`-Wall -Wextra`); penalizada pela densidade extrema e legibilidade ruim em `tool.c` e `parser_islands.c`. |
| **4. Extensibilidade** | **4.3** | Emissor muito bem dividido por funcionalidade (`emit/*.c`); catálogo de erros sincronizado via Python; limitado por arrays estáticos de argumentos (`K_EMIT_INSTANCE_ARGS 4`). |
| **5. Modularidade** | **4.7** | Excelente barreira arquitetural `tool/` vs `engine/` verificada por Makefile; inversão de dependência via `KLoader`; alto acoplamento interno no subsistema de ilhas. |
| **6. Ergonomia** | **4.4** | CLI transparente compatível com GCC em Makefiles; depfiles `.d` integrados; falha ao silenciar `--pedantic-names` e mensagens opacas ao estourar buffers internos. |

---

## 1. Segurança (Memória, Uso, Invasão)

### 1.1 Segurança de Memória e Gestão de Recursos
A gestão de memória do compilador foi desenhada em torno do conceito de **Arena Monolítica por Invocação** (`keel_arena`), implementada em [`tools/cgen/src/tool/memory.h`](file:///home/andre/code/cprojects/keel/tools/cgen/src/tool/memory.h).

* **Ausência de Desalocações Individuais:**
  Durante toda a análise léxica, sintática, semântica e emissão, o compilador **nunca invoca `free()`**. Toda a memória alocada por `cgen_alloc` ([`memory.h:35-41`](file:///home/andre/code/cprojects/keel/tools/cgen/src/tool/memory.h#L35-L41)) é mantida viva até o término do processo em [`main.c:334`](file:///home/andre/code/cprojects/keel/tools/cgen/src/tool/main.c#L334), onde `cgen_memory_destroy(&arena)` descarta todo o bloco de uma só vez.
  - **Benefício:** Elimina estruturalmente *Use-After-Free* (UAF), *Double Free*, ponteiros pendentes entre passos de compilação e fragmentação de heap.
* **Proteção contra Overflow Aritmético de Alocação:**
  O compilador checa explicitamente limites aritméticos antes de alocar tabelas dimensionadas por contagens de tokens ou símbolos:
  ```c
  // tool.c:175-176
  if (count > SIZE_MAX / sizeof(KLexeme) - 1 || count > SIZE_MAX / sizeof(KAstNode) - 1)
      return failure(t, e, K_DIAG_CAPACITY, before);
  // tool.c:231
  if (capacity > SIZE_MAX / sizeof(KSymbol)) {
      t->active = parent;
      return failure(t, e, K_DIAG_CAPACITY, before);
  }
  ```
* **Controle de Limites da Arena:**
  A arena é pré-alocada com capacidade padrão de 64 MiB (`CGEN_ARENA_CAPACITY`). Se a capacidade for ultrapassada, `cgen_alloc` emite `[implementation-limit]` e retorna `NULL`. Os pontos críticos de inicialização de módulos verificam o retorno nulo e abortam com segurança.
* **Strings e Fatias Sem Terminador Nulo:**
  A estrutura base de strings no motor é a fatia delimitada por comprimento (`keel_slice_char { size_t len; char *ptr; }`). Isso evita funções inseguras da libc como `strcpy`/`strcat` na manipulação de identificadores e expressões, reduzindo o risco de *buffer overflows* por falta de byte nulo.

### 1.2 Segurança contra Invasão e Execução de Processos (Subprocess Hijacking)
* **Execução Segura do Compilador C:**
  Ao atuar como driver, o `cgen` precisa invocar o compilador nativo (`gcc` ou `clang`). O arquivo [`tools/cgen/src/tool/run_cc.c:116-128`](file:///home/andre/code/cprojects/keel/tools/cgen/src/tool/run_cc.c#L116-L128) implementa:
  ```c
  static int spawn(const char *cc, char *const *argv) {
      fflush(NULL);
      pid_t pid = fork();
      if (pid < 0) { perror("fork"); return 2; }
      if (pid == 0) {
          execvp(cc, argv);
          fprintf(stderr, "cgen: error: cannot run the C compiler '%s' [cc-not-found]\n", cc);
          _exit(127);
      }
      ...
  }
  ```
  **Avaliação:** O compilador não usa `system()`, nem `popen()`, nem `/bin/sh -c`. Todos os argumentos passados na linha de comando são repassados verbatim em um vetor de argumentos nativo (`argv`). Isso garante imunidade total contra *Command Injection* por nomes de arquivos contendo `;`, `|`, `&&`, `$()`, etc.

* **Vulnerabilidade Identificada: Escrita Insegura de Arquivos Temporários (TOCTOU / Symlink Attack):**
  Ao gravar arquivos gerados com garantia de escrita atômica, o arquivo [`tools/cgen/src/tool/output.c:40-48`](file:///home/andre/code/cprojects/keel/tools/cgen/src/tool/output.c#L40-L48) faz:
  ```c
  size_t len = strlen(path);
  char *tmp = malloc(len + 32);
  if (!tmp) goto failed;
  snprintf(tmp, len + 32, "%s.tmp.%ld", path, (long)getpid());
  FILE *f = fopen(tmp, "wb");
  if (!f) { free(tmp); goto failed; }
  bool ok = (n == 0 || fwrite(data, 1, n, f) == n);
  ok = (fclose(f) == 0) && ok;
  if (!ok || rename(tmp, path) != 0) { remove(tmp); free(tmp); goto failed; }
  ```
  > [!CAUTION]
  > **Risco de Segurança (CWE-377 / CWE-59):**  
  > 1. O nome do arquivo temporário `%s.tmp.%ld` é completamente previsível a partir do PID do processo.  
  > 2. `fopen(tmp, "wb")` **trunca ou segue symlinks existentes**, sem utilizar flags atômicas exclusivas (`O_CREAT | O_EXCL`).  
  > Se o compilador for executado em um diretório com permissão de escrita compartilhada (ex: `/tmp`, diretórios de CI/CD compartilhados ou nós de build multiusuário), um usuário malicioso local pode antecipar o PID, criar um link simbólico apontando para um arquivo crítico do sistema e fazer com que o `cgen` sobrescreva esse arquivo.  
  > **Correção recomendada:** Usar `open(tmp, O_WRONLY | O_CREAT | O_EXCL, 0600)` ou a chamada padrão POSIX `mkstemp()`.

### 1.3 Segurança de Uso e Sanitização de Entradas
* **Normalização Léxica de Caminhos:**
  A função `cgen_path_normalize` em [`tools/cgen/src/tool/paths.c:14-60`](file:///home/andre/code/cprojects/keel/tools/cgen/src/tool/paths.c#L14-L60) resolve segmentos `..` e `.` de forma estritamente léxica. Caminhos absolutos como `/../etc/passwd` são normalizados com segurança para `/etc/passwd` sem escapar da raiz léxica.
* **Validação do Stem de Arquivos:**
  Em [`tools/cgen/src/tool/tool.c:116-125`](file:///home/andre/code/cprojects/keel/tools/cgen/src/tool/tool.c#L116-L125), o compilador valida se o nome base do arquivo corresponde à gramática de identificador C (`k_token_is_ident`). Qualquer caractere estranho ou tentativa de injeção em nomes de módulos é barrada com o erro `[invalid-stem]`.

---

## 2. Estabilidade

### 2.1 Determinismo e Idempotência
O compilador foi projetado com foco em compilações reproduzíveis:
- **Ordenação em Depfiles:** A geração de depfiles para Make/Ninja classifica todos os caminhos do fecho de dependências através de `qsort` com `by_path` ([`run_cc.c:19-20, 85`](file:///home/andre/code/cprojects/keel/tools/cgen/src/tool/run_cc.c#L19-L20)), garantindo que a ordem de leitura do sistema de arquivos não altere o arquivo `.d` gerado.
- **Prevenção de Escrita Redundante:** O compilador lê o arquivo de destino antes de gravar (`same_content` em [`output.c:18-30`](file:///home/andre/code/cprojects/keel/tools/cgen/src/tool/output.c#L18-L30)). Se os bytes gerados forem idênticos aos existentes no disco, o arquivo **não** é tocado. Isso estabiliza os timestamps (`mtime`) e evita loops de recompilação desnecessários em ferramentas como Make e Ninja.

### 2.2 Prevenção de Falhas Críticas e Recuperação de Erros
* **Detecção de Ciclos no Grafo de Dependências:**
  O compilador rastreia o ciclo de vida de cada módulo através de estados finitos (`CGEN_PENDING`, `CGEN_DONE`, `CGEN_FAILED` em [`tool.c:20`](file:///home/andre/code/cprojects/keel/tools/cgen/src/tool/tool.c#L20)). Quando um módulo tenta importar a si mesmo ou a um importador ancestral, a função `cgen_load` identifica o estado pendente e retorna `K_LOAD_CYCLE`, que dispara o diagnóstico `circular-import` em vez de entrar em recursão infinita e causar estouro de pilha (*stack overflow*).
* **Robustez Léxica:**
  O analisador léxico ([`lexer_scan_quoted.c`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/lexer/lexer_scan_quoted.c) e [`lexer_skip_trivia.c`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/lexer/lexer_skip_trivia.c)) trata entradas malformadas (strings sem aspas de fechamento, comentários de bloco abertos no final do arquivo e barras invertidas no final do buffer) setando a flag `*unterminated_out = true` sem nunca ler além de `source.len`.

### 2.3 Fragilidades e Riscos de Estabilidade no Código do Compilador

1. **Limites Rígidos em Buffers Estáticos (*Hardcoded Limits*):**
   O compilador faz uso extensivo de capacidades estáticas que podem provocar falhas em código de produção real:
   - `LINE_CAP 1024` ([`emit_internal.h:14`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/emit/emit_internal.h#L14)): Limite máximo de 1.024 bytes por linha de código gerada. Uma única linha contendo um array literal grande, macro expandida ou assinatura de função extensa estoura o buffer (`e->n >= LINE_CAP`), ativando `e->ok = false` e abortando a compilação do arquivo.
   - `MAX_EDITS 32` ([`emit_internal.h:15`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/emit/emit_internal.h#L15)): Limite máximo de 32 transformações/edições em uma única linha.
   - `WRITTEN_CAP 1024` ([`stop_gen.c:107`](file:///home/andre/code/cprojects/keel/tools/cgen/src/tool/stop_gen.c#L107)): O compilador não consegue gerar código para projetos com mais de 1.024 módulos na mesma execução.
   - `CGEN_CLI_ROOTS_CAP 128` e `CGEN_CLI_ARGS_CAP 256` ([`cli_limits.h:5,8`](file:///home/andre/code/cprojects/keel/tools/cgen/src/tool/cli_limits.h#L5)): Limita o número de opções repassadas e raízes de busca `-I`.
2. **Buffer Estático Global em `parser_header.c` (Problema de Reentrância):**
   Em [`tools/cgen/src/engine/parser/parser_header.c:163-164`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/parser/parser_header.c#L163-L164), há o seguinte trecho:
   ```c
   static char store[32768];
   static size_t used;
   ```
   Variáveis estáticas locais a funções retêm estado entre invocações. Se o `cgen` for compilado como biblioteca estática ou invocado múltiplas vezes dentro do mesmo processo em testes unitários, esse buffer preservará lixo de execuções anteriores, quebrando o determinismo e a reentrância.
3. **Incompatibilidade de Portabilidade no `base_resolve.c`:**
   Em [`tools/cgen/src/tool/base_resolve.c:18`](file:///home/andre/code/cprojects/keel/tools/cgen/src/tool/base_resolve.c#L18):
   ```c
   char exe_path[PATH_MAX];
   ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
   ```
   O caminho `/proc/self/exe` é específico de Linux. Em sistemas macOS (`/proc` não existe) e FreeBSD/Windows, essa chamada falha invariavelmente com `ENOENT`. O compilador falha com `base-not-found` a menos que o usuário passe explicitamente `--base-dir`.

---

## 3. Manutenabilidade (Facilidade de Manutenção do Compilador)

### 3.1 Rastreabilidade Normativa
A documentação interna do compilador é de altíssimo padrão:
- Quase toda função relevante no motor (`engine/`) contém um comentário indicando a seção exata dos documentos normativos que dita seu comportamento (ex.: `cgen-tool-spec §4.10`, `backend §5.5, rule 4`, `keel-spec §4.4`);
- Decisões de implementação e interpretações de lacunas de especificação são identificadas com tags formais (ex.: `[D1]` a `[D8]`), documentadas em [`design/cgen-tool.md §12`](file:///home/andre/code/cprojects/keel/design/cgen-tool.md#12-decisões-deste-documento);
- A base compila com **zero warnings** sob `-Wall -Wextra`, fato verificado e registrado em `tools/agents/warnings.baseline`.

### 3.2 Validação Automatizada de Fronteiras no Build (`make boundary`)
Em [`tools/cgen/Makefile:49-59`](file:///home/andre/code/cprojects/keel/tools/cgen/Makefile#L49-L59), o comando `make boundary` roda antes da compilação:
```makefile
FORBIDDEN := stdio\.h|stdlib\.h|unistd\.h|fcntl\.h|sys/

boundary:
	@bad=$$(grep -rnE '^[[:space:]]*#[[:space:]]*include[[:space:]]*<($(FORBIDDEN))' \
	          $(SRCDIR)/engine 2>/dev/null); \
	if [ -n "$$bad" ]; then \
	  echo "engine/ não pode fazer E/S (desenho do cgen §3.1):"; \
	  echo "$$bad" | sed 's/^/  /'; \
	  exit 1; \
	fi; \
	echo "ok     engine boundary             (desenho do cgen §3.1)"
```
Essa regra automatizada garante que desenvolvedores futuros não violem acidentalmente o isolamento do motor do compilador inserindo chamadas de E/S de sistema operacional em `engine/`.

### 3.3 Problema Grave: Densidade Excessiva e Formatação Obscura
A maior fraqueza de manutenabilidade do compilador é a formatação de código adotada em arquivos como [`tool.c`](file:///home/andre/code/cprojects/keel/tools/cgen/src/tool/tool.c), [`parser_islands.c`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/parser/parser_islands.c), [`names.c`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/names.c) e [`instances.c`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/instances.c).

Exemplos reais de código do compilador:
```c
// tool.c:168
e->state=CGEN_PENDING;e->next=t->mods;t->mods=e;
if(!saved.ptr||!e->path.ptr)return failure(t,e,K_DIAG_CAPACITY,before);

// tool.c:241
for(size_t i=mod->type_first;i<mod->type_end;i++)
    if(k_token_is_ident(keel_buffer_KLexeme_ptr(&e->ast.tokens, i)->token))
        if(!k_symtab_insert(&e->symbols,keel_buffer_KLexeme_ptr(&e->ast.tokens, i)->token,K_SYM_TYPE,0))return failure(t,e,K_DIAG_CAPACITY,before);

// instances.c:10
static void text(Name *n,const char *s){append(n,k_diag_text(s));}

// names.c:57-61
if(!primitive)for(size_t i=0;i<module.len;i++){if(n<out.len)out.ptr[n]=module.ptr[i]=='.'?'_':module.ptr[i];n++;}
if(!shorten||!module.len||primitive){
    if(n){if(n<out.len)out.ptr[n]='_';n++;}
    for(size_t i=0;i<s->name.len;i++){if(n<out.len)out.ptr[n]=s->name.ptr[i]=='.'?'_':s->name.ptr[i];n++;}
}
```
**Impactos na Manutenabilidade:**
- **Depuração com GDB/LLDB:** Em instruções encadeadas na mesma linha (`a(); b(); c();`), não é possível colocar *breakpoints* ou executar *step-over* (`next`) em instruções individuais;
- **Revisão de Código / Diffs Git:** Um diff de uma linha altera múltiplos estados do compilador simultaneamente, mascarando mudanças sutis;
- **Leitura Cognitiva:** A supressão intencional de espaços em torno de operadores e instruções de controle (`if(...)stmt;`) viola as convenções clássicas de C e desacelera o entendimento de novos mantenedores.

### 3.4 Padrão Ineficiente: Re-lexing Repetido de Substrings
Em [`ast.c:87-101`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/ast.c#L87-L101) e [`parser_islands.c:37-41`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/parser/parser_islands.c#L37-L41), o compilador utiliza a função `reenter()` / `isl_enter()`:
```c
static KToken reenter(const KAst *a, size_t i, KLexer *lexer, TKPpKind *pp) {
    KToken t = keel_buffer_KLexeme_ptr(&a->tokens, i)->token;
    keel_slice_char rest = { (size_t)((a->source.ptr + a->source.len) - t.ptr), t.ptr };
    k_lexer_init(lexer, rest, NULL);
    return k_lexer_next(lexer, pp);
}
```
Em vez de alimentar o parser com a sequência de tokens já processada e armazenada em `a->tokens`, os reconhecedores de declaração recebem `KLexer *` e forçam o compilador a re-tokenizar fatias do código-fonte repetidas vezes. Para recuperar o índice do token após essa varredura, o compilador executa `index_at()` ([`ast.c:110-114`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/ast.c#L110-L114)), que realiza uma busca linear por comparação de ponteiros (`token.ptr < ptr`).

---

## 4. Extensibilidade (Facilidade de Inserção de Novas Funcionalidades)

### 4.1 Arquitetura Modular do Emissor (`engine/emit/`)
A adição de suporte a novas construções no compilador é muito simples no subsistema de emissão:
- O arquivo [`engine/emit/emit.c`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/emit/emit.c) atua como um despachante limpo:
  ```c
  void k_em_node_edits(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt) {
      ...
      if (n->kind == K_AST_FUNCTION) {
          k_em_edits_roles(e, n, a, b, ed, cnt);
          k_em_edits_range_check(e, n, a, b, ed, cnt);
          k_em_edits_array(e, n, a, b, ed, cnt);
          k_em_edits_foreach(e, n, a, b, ed, cnt);
          k_em_edits_constblock(e, n, a, b, ed, cnt);
          k_em_edits_defer(e, n, a, b, ed, cnt);
          k_em_edits_index(e, n, a, b, ed, cnt);
          k_em_edits_else(e, n, a, b, ed, cnt);
          k_em_edits_match(e, n, a, b, ed, cnt);
          k_em_edits_parallel(e, n, a, b, ed, cnt);
          ...
      }
  }
  ```
- Cada construção possui seu próprio arquivo `.c` isolado (`defer.c`, `parallel.c`, `match.c`, `foreach.c`). Desenvolver um novo comando consiste em criar `novo_recurso.c`, definir sua função de coleta de edições e registrá-la em `k_em_node_edits`. Não há necessidade de alterar a lógica dos outros comandos.

### 4.2 Sincronização Automática do Catálogo de Diagnósticos
A inserção de novos diagnósticos de compilação está automatizada:
- O desenvolvedor adiciona a linha com o código do diagnóstico em [`keel-spec.md §6.2`](file:///home/andre/code/cprojects/keel/keel-spec.md#62-catálogo);
- O script [`tools/cgen/gen-diags.py`](file:///home/andre/code/cprojects/keel/tools/cgen/gen-diags.py) reescreve `diag_catalog.def` e verifica sincronismo;
- O enum `KDiagId` e a tabela `k_diags` em [`diag.h`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/diag.h) e [`diag.c`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/diag.c) utilizam X-Macros (`K_DIAG_CATALOG(X)`), adaptando-se automaticamente sem edição manual de tabelas.

### 4.3 Gargalos de Extensibilidade no Compilador
* **Falta de Tabela de Tipos Completa:**  
  Como o compilador utiliza a abordagem de "parser de ilhas", ele não modela expressões arbitrárias de C nem possui um sistema unificado de inferência de tipos. O arquivo [`parser_islands_types.c`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/parser/parser_islands_types.c) implementa heurísticas sintáticas locais para descobrir se uma variável é contêiner. Caso a linguagem evolua para suportar expressões complexas de tipos (ex: encadeamento de métodos, chamadas funcionais de ordem superior), esse modelo ad-hoc de tipagem se tornará um grande gargalo para expansão.
* **Arrays Estáticos de Argumentos Genéricos:**  
  Em [`emit.h:33, 42`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/emit/emit.h#L33):
  ```c
  #define K_EMIT_INSTANCE_ARGS 4
  ...
  keel_slice_char arg[K_EMIT_INSTANCE_ARGS];
  ```
  O compilador está limitado a no máximo 4 argumentos de tipo para instâncias genéricas. Suportar 5 ou mais parâmetros requer alterar e recompilar múltiplas definições estruturais.

---

## 5. Modularidade

### 5.1 Arquitetura de Camadas: `tool/` vs `engine/`

A estrutura de código do compilador aplica uma divisão estrita de responsabilidades:

```
tools/cgen/src/
├── tool/       -> Depende do SO: arquivos, CLI, processos, caminhos reais, timestamps
│   ├── main.c           (Parse de CLI, orquestração e ponto de saída)
│   ├── run_cc.c         (Execução via fork/execvp e escrita de depfiles)
│   ├── paths.c          (Normalização e manipulação léxica de caminhos)
│   ├── tool.c           (Gerenciador de módulos, cache e memoização)
│   └── output.c         (Escrita atômica em disco)
│
└── engine/     -> Depende apenas de buffers em memória: NENHUM include de SO permitido
    ├── lexer/           (Tokenização e rastreamento de profundidade léxica)
    ├── parser/          (Reconhecimento sintático e extração de ilhas)
    ├── emit/            (Transformação de nós do AST para código C11/C23)
    ├── ast.c / ast.h    (Tabelas e nós do AST)
    ├── diag.c / diag.h  (Sink de diagnósticos em memória)
    └── names.c / symtab (Tabela de símbolos e mangling)
```

### 5.2 Inversão de Dependência via `KLoader`
O ponto mais elegante da modularidade do compilador é a interface `KLoader` em [`tools/cgen/src/engine/loader.h`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/loader.h):
```c
typedef enum {
    K_LOAD_OK,
    K_LOAD_ALREADY,
    K_LOAD_CYCLE,
    K_LOAD_NOT_FOUND,
    K_LOAD_ERROR
} KLoadResult;

typedef struct {
    KLoadResult (*load)(void *tool, keel_slice_char module_name, KModule **out);
    void *tool;
} KLoader;
```
O motor de análise sintática e semântica (`engine/`) não tem nenhuma dependência do sistema de arquivos. Quando o parser precisa carregar uma dependência `import a.b;`, ele invoca o ponteiro de função `loader->load(...)`.  
**Consequência Prática:** O motor pode ser executado e testado 100% em memória, passando buffers virtuais, sem criar um único arquivo temporário no disco.

### 5.3 Acoplamento Interno no Módulo de Ilhas (`parser_islands.c`)
Enquanto a separação externa (`tool` vs `engine`) é exemplar, a modularidade interna do subsistema de ilhas apresenta alto acoplamento:
- `parser_islands.c` possui 1.170 linhas e divide sua implementação em 5 arquivos auxiliares (`parser_islands_closure.c`, `parser_islands_flow.c`, `parser_islands_index.c`, `parser_islands_types.c`, `parser_islands_checks.c`);
- Todos esses arquivos compartilham uma estrutura de contexto monoliticamente acoplada (`Ctx`, em [`parser_islands_internal.h`](file:///home/andre/code/cprojects/keel/tools/cgen/src/engine/parser/parser_islands_internal.h)), com dezenas de campos mutáveis que transitam entre os submódulos, tornando as funções interdependentes.

---

## 6. Ergonomia (Facilidade de Uso do Compilador)

### 6.1 Usabilidade da Ferramenta de Linha de Comando (CLI)
* **Compatibilidade Transparente com GCC/Clang:**
  O compilador atua como um substituto direto de `gcc` em Makefiles (`CC = cgen`):
  - Consome apenas as opções próprias do `cgen` e repassa todas as demais opções intactas ao compilador nativo;
  - Se nenhum arquivo `.k` for fornecido, executa diretamente o `cc` com os parâmetros repassados ([`main.c:69-73, 262-273`](file:///home/andre/code/cprojects/keel/tools/cgen/src/tool/main.c#L69-L73));
  - Gera depfiles `.d` compatíveis com Make e Ninja através de `-MD` e `-MMD`.
* **Fases Intermediárias Acessíveis:**
  O compilador fornece a flag `--stop-after=<lex|parse|gen>`, permitindo que desenvolvedores e ferramentas de análise inspecionem o estado intermediário do AST e dos tokens sem acionar o compilador backend.

### 6.2 Mensagens de Erro e Diagnósticos
* **Formato Universal GNU:**
  As mensagens de erro seguem rigorosamente o padrão consumido por IDEs e editores:
  ```plain
  <arquivo>:<linha>:<coluna>: <severidade>: <mensagem> [<identificador>]
  ```
  Exemplo real: `src/main.k:12:5: error: circular import detected [circular-import]`
* **Rastreabilidade por Tags:**
  A inclusão da tag `[identificador]` no final de cada erro permite ao desenvolvedor buscar a regra exata na especificação da linguagem ou pesquisar na documentação com facilidade.

### 6.3 Deficiências Ergonômicas do Compilador
1. **Opção `--pedantic-names` Silenciada:**
   Em [`tools/cgen/src/tool/main.c:224-225`](file:///home/andre/code/cprojects/keel/tools/cgen/src/tool/main.c#L224-L225), a opção é consumida na linha de comando:
   ```c
   } else if (strcmp(w, "--pedantic-names") == 0) {
       pedantic_names = true;
   ```
   No entanto, na linha 286 de `main.c`:
   ```c
   (void)pedantic_names;
   ```
   A variável é simplesmente descartada e nunca enviada ao emissor ou ao mangler. O usuário passa uma flag válida, mas o compilador ignora a instrução silenciosamente sem qualquer aviso.
2. **Mensagem de Erro Enganosa para Estouro de Buffers Internos:**
   Quando uma linha excede 1.024 bytes ou atinge mais de 32 edições durante a emissão, o motor retorna `K_EMIT_UNSUPPORTED`. O compilador imprime em [`stop_gen.c:78`](file:///home/andre/code/cprojects/keel/tools/cgen/src/tool/stop_gen.c#L78):
   ```plain
   cgen: error: <arquivo>: the emitter does not cover this module's constructions yet [implementation-limit]
   ```
   Isso prejudica gravemente a experiência do desenvolvedor, pois ele presume que usou uma funcionalidade não suportada da linguagem, quando na verdade o compilador falhou por um limite interno de comprimento de linha.

---

## 7. Recomendações Priorizadas para o Código do Compilador

### Prioridade 1 (Correções Imediatas de Segurança e Robustez)
1. **Sanitizar a Escrita de Temporários em `output.c`:**
   Substituir a combinação insegura `snprintf(tmp, ...)` + `fopen(tmp, "wb")` por abertura exclusiva com permissões seguras:
   ```c
   int fd = open(tmp, O_WRONLY | O_CREAT | O_EXCL, 0600);
   ```
   ou utilizar `mkstemp()`, mitigando vulnerabilidades de symlink e race conditions em ambientes multiusuário.
2. **Diagnóstico Preciso para Estouro de Limites:**
   Distinguir códigos de retorno em `k_emit_file` para emitir mensagens claras (ex.: `cgen: error: line exceeds internal capacity of 1024 bytes; split the line or adjust LINE_CAP`).

### Prioridade 2 (Manutenabilidade e Portabilidade)
3. **Refatoração de Legibilidade e Estilo:**
   Normalizar os blocos ultracompactos em `tool.c`, `parser_islands.c`, `names.c` e `instances.c`, expandindo comandos em linhas separadas e aplicando espaçamento padrão em operadores. Isso aumentará a produtividade na depuração com GDB e a clareza em code reviews.
4. **Implementar Resolução Portável do Caminho do Executável:**
   Em `base_resolve.c`, incluir `#ifdef __APPLE__` com `_NSGetExecutablePath` e `#ifdef __FreeBSD__` com `sysctl(KERN_PROC_PATHNAME)` para eliminar a dependência estrita do `/proc/self/exe` do Linux.
5. **Eliminar o Estado Estático em `parser_header.c`:**
   Mover `store[32768]` e `used` para dentro da estrutura da sessão do compilador (`KAst` ou `CgenTool`), garantindo reentrância perfeita em testes e execuções repetidas.

### Prioridade 3 (Extensibilidade e Polimento)
6. **Eliminar o Re-lexing Redundante:**
   Refatorar os reconhecedores de declaração (`k_scan_decl_*`) para consumirem um iterador ou índice sobre o vetor de lexemas existente (`a->tokens`), removendo a necessidade de reinicializar instâncias temporárias de `KLexer` e fazer buscas lineares com `index_at()`.
7. **Ativar ou Alertar sobre `--pedantic-names`:**
   Conectar a flag ao limite de mangling ou emitir um aviso (`warning: --pedantic-names is not yet implemented`) para não induzir o usuário a falsas expectativas de conformidade C estrita.
