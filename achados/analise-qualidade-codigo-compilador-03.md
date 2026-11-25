# Revisão de código — keel / cgen (3ª rodada)

> **Data:** 25 de novembro de 2026
> **Objeto:** `cgen` 0.4.0, commit `v0.4.0-1-g6e434bc1` (branch `0.4.0`) — `tools/cgen/src/`
> (70 unidades de tradução, ~25 mil linhas de C), a base que o sustenta (`base/keel/*.k` e a
> cópia *bootstrap* em `tools/transform/base/`), a camada de ferramentas (`Makefile`,
> `tools/transform`, scripts de teste e de golden, `editors/`, `gdb/`) e a higiene do
> repositório.
> **Relação com as rodadas anteriores:** [`design/analise-qualidade-codigo-compilador-01.md`](../design/analise-qualidade-codigo-compilador-01.md)
> e [`-02.md`](../design/analise-qualidade-codigo-compilador-02.md). Este relatório concentra o
> que é novo e o que foi **re-verificado nesta rodada**; onde um item já constava nelas, está
> marcado **[persiste-02]** ou **[fechado-02]**.
> **Não é normativo. Nenhum arquivo de código foi alterado.** Cada achado vem com a
> reprodução e a proposta, para seguir o fluxo do projeto (caso de teste antes do conserto).

---

## 1. Resumo

O código é de qualidade alta, e isso se mede: zero warnings com `-Wall -Wextra`, a suíte
oficial inteira verde (golden 180/0; `gen` 144/0; docs 45/0), onze formas de aninhamento
extremo sem uma queda de pilha, e **118 mil execuções de fuzzing com um único defeito**, que
apareceu três vezes na mesma forma.

A exceção é importante. Com um método que as rodadas anteriores não usaram — redzones
envenenadas **dentro** da arena — apareceu uma **leitura fora dos limites real**, alcançável
por uma linha: declarar algo com o nome de uma palavra de keel (`typedef i32 pub;`). Ela não
é visível para o ASan comum: o `cgen` aloca tudo num único `malloc` de 128 MiB, e um estouro
de uma tabela cai silenciosamente na tabela vizinha. Em 45 mil entradas nesse estrato foi o
único caso: o problema não é generalizado, mas existe, e a ferramenta de verificação atual não
o enxerga.

Do ponto de vista do usuário, três defeitos são visíveis:

```
module owner;
typedef i32 pub;                                     /* palavra de keel como nome: só um warning */
pub inline i32 value(void) { return 17; }            /* linha de 45 bytes */
```
```
owner.k:2:13: warning: `pub` is a keel word ... [keel-name-shadowed]
owner.k:3: error: a generated line needs more than LINE_CAP = 1024 bytes ... [implementation-limit]
```
A linha 3 tem 45 bytes; a mensagem manda quebrar uma linha curta, e no caminho o programa lê
memória que não lhe pertence (A1). A forma aparentada é pior porque **falha em silêncio**:
`constexpr i32 BIAS priv = 2;` sai com rc=0 e a constante não existe no C gerado (A2). E há
uma terceira, independente da primeira: um protótipo **escrito em várias linhas** cujo texto
passa de 1024 bytes é recusado com o mesmo `LINE_CAP` falso (A6).

A varredura final cobriu o que a primeira passada deixou de fora — `tools/transform`,
`editors/`, `gdb/` e os 62 scripts. São, no geral, cuidadosos (sem shell, `subprocess` com
lista de argumentos, `mktemp -d` na quase totalidade dos testes). Três pontos de baixa
severidade ficaram registrados: o `transform` (ferramenta de *bootstrap*) tem um estouro de
buffer e uma queda em linha de comando malformada; três scripts usam nomes previsíveis em
`/tmp`; e `install-zed.sh` faz `git add -A` num repositório que pode não ser seu.

| # | Severidade | Achado | Onde |
| :-: | :-: | :-- | :-- |
| A1 | **alta** | Palavra de keel como nome → dois edits no mesmo offset → leitura fora dos limites + diagnóstico falso | `emit/lines.c:104-106`, `emit/decls.c:33-43` |
| A2 | **alta** | `constexpr` malformado aceito e **descartado em silêncio** (rc=0) | parser (`constexpr-name-missing` incompleto) |
| A6 | média | Protótipo multilinha com mais de 1024 bytes de texto é recusado com `LINE_CAP` falso | `emit/decls.c:181-195` |
| A3 | média | Tempo quadrático com splices `\`+nova linha numa mesma linha lógica | `emit/lines.c:23-47` |
| A4 | média | Tempo superquadrático com o número de declarações (buscas lineares) | `emit_core.c:98`, `instances.c`, `symtab.h` |
| A4b | média | Dentro de uma função, ~20 analisadores re-varrem o corpo inteiro **por linha emitida** | `emit/lines.c:197-210`, `emit/emit.c:14-42` |
| A5 | média | Reserva de ~536 B **por token** → teto de ~250 mil tokens, só ajustável recompilando | `tool/tool.c:190-294` |
| S1 | média | O cgen é compilado contra uma base **sem checagem de limites**; o ASan é cego dentro da arena | `tools/cgen/gen/keel/*.h`, 264 acessos indexados |
| S2 | baixa | Estado global mutável no motor **[persiste-02]** | `lines.c:97`, `emit_core.c:58`, `parser_islands.c:727,734` |
| X1 | baixa | `transform`: estouro de buffer em fonte > 3 MiB, SIGSEGV em opção sem valor, cabeçalho > 1 KiB descartado em silêncio | `tools/transform/src/main.c` |
| X2 | baixa | Nomes previsíveis em `/tmp` (`/tmp/<nome>.$$`) em 3 scripts | `test/cgen/lex_dump.sh`, `parse_dump.sh`, `tools/agents/verify.sh` |
| X3 | baixa | `install-zed.sh` faz `git add -A` + commit num destino que já pode ser repositório | `editors/install-zed.sh` |
| Q1–Q13 | baixa | Clareza e objetividade: saídas silenciosas, estado morto, mensagens duplicadas, enum e comentários defasados, warnings extras | §4 |
| M1–M4 | baixa | Modularidade: invariante implícita, testes presos ao binário, `stop_gen.c` **[persiste-02]**, base *bootstrap* de outro dialeto | §5 |
| H1 | baixa | Higiene do repositório | §6 |

**Prioridade sugerida:** A1 e A2 primeiro (defeito visível, conserto pequeno, caso de teste
barato), A6 logo atrás (código legal recusado); depois S1 como método (um alvo `asan` com
redzones impede a recaída); depois A3, A4b, A5 e A4. Os X são baratos e podem ir em qualquer
momento.

---

## 2. Método e evidências

Tudo foi feito fora da árvore: builds em diretório temporário, nenhum `obj/`, `gen/` ou
arquivo do projeto foi tocado. Os números abaixo são os medidos nesta rodada, nesta máquina.

| Verificação | Resultado |
| :-- | :-- |
| Build padrão do cgen (`-std=c2x -Wall -Wextra`, 70 `.c`) | 0 warnings, 0 erros |
| Warnings extras no cgen (`-Wshadow -Wconversion -Wsign-conversion -Wformat=2 -Wstrict-prototypes -Wmissing-prototypes -Wvla -Wcast-qual …`) | 185: `cast-qual` 129 (52 em `emit/emit.h`), `missing-prototypes` 37, `sign-conversion` 9, `shadow` 7, `conversion` 3. **Nenhum** de formato, nenhum de VLA |
| ASan + UBSan, 417 execuções válidas (golden, base, bootstrap × lex/parse/gen) | 0 ocorrências |
| O mesmo com **redzones intra-arena** (apêndice B), 417 + as 44 `--instance` do oráculo da base | 0 ocorrências |
| Fuzzing byte a byte (qualquer posição da fonte) | 3.000 execuções, 0 |
| Fuzzing byte a byte, cabeçalho `module` preservado | 20.000 (ASan) + 15.000 (redzones), 0 |
| Fuzzing de `--instance` (strings aleatórias) | 20.000 (ASan) + 15.000 (redzones), 0 |
| Fuzzing **em nível de token**, com redzones, duas sementes independentes | 15.000 + 30.000 execuções, **3** (todas A1) |
| **Total de fuzzing** | **118.000 execuções, 3 ocorrências, 1 defeito** |
| Aninhamento extremo (100 k parênteses em expressão e em declarador, 200 k chaves, 100 k colchetes, 50 k genéricos encaixados, 60 k dimensões, 100 k `#if`, 20 k `defer`, identificador de 1 MiB, string de 4 MiB, 50 k splices) | 11 formas: nenhuma queda, nenhum erro de sanitizer; todas recusadas por um teto ou concluídas |
| CLI adversarial (fonte ausente, diretório, vazia, sem permissão, BOM, binária, com NUL, `--dest-dir` em arquivo, `--cc` inexistente, `-x c`, `-U FOO`, `-MF /dev/stdout`, opção sem argumento, dois `.k`) | comportamento correto ou recusa clara |
| Suíte oficial `sh test/run.sh` por área | **verde**: `unit` 153 s, `cli` 54 s, `lex` 15 s, `parse` 10 s, `diag` 3 s, `gen` 28 s (144 ok, 0 fail), `docs` 1 s (45 ok), `golden` 30 s (180 ok, 0 fail) — ~294 s |
| Perfil (`valgrind --tool=callgrind` e `gprof`, build `-O2`) | A3, A4 e A4b, abaixo |
| **Varredura final**: 62 scripts (`tools/`, `golden/`, `test/`, `editors/`, `gdb/`, `Makefile`) por leitura dirigida e padrões perigosos; `tools/transform` sob ASan | sem `shell=True`, `eval`, `system`, `mktemp -u` ou `curl \| sh`; 3 achados baixos (X1–X3) |

**Duas hipóteses desta rodada foram testadas e descartadas**, para que ninguém as investigue
de novo: diagnósticos além de 4096 **não** são truncados em silêncio (6000 erros do lexer, 6000
linhas impressas); a tabela local de 256 símbolos da passagem de cabeçalho (`ast.c`) **não** é
um teto na prática, porque nessa passagem tudo que não é `import` cai no ramo opaco antes de
chegar aos reconhecedores que registram símbolos. Uma terceira também caiu: um parâmetro ou
local homônimo de um nome do módulo **não** é renomeado (conferido no C gerado e na
compilação); o que está errado ali é só o comentário (Q12).

**O que não foi coberto.** Não houve leitura linha a linha de todo o motor: o núcleo de ilhas
(`parser_islands_*.c`, ~4,9 mil linhas) e os emissores por construção (`match.c`,
`foreach.c`, `defer.c`, `parallel.c` etc.) foram cobertos só pela análise dinâmica, que os
exercita por inteiro via golden. Da base keel, li `arena`, `buffer`, `slice` e `outcome`; os
demais módulos não foram lidos. Os scripts foram lidos por onde escrevem em disco, falam com
processos ou com a rede, não linha a linha.

**Por que a rodada 2 não viu o A1.** A conclusão dela — "o cgen é disciplinado no uso" (S3) —
era correta para o que o ASan consegue ver. Dentro de uma arena não há redzone entre
alocações: um índice a mais numa tabela de tokens lê a tabela seguinte sem nenhum aviso. O
apêndice B fecha essa lacuna em ~25 linhas.

---

## 3. Achados

Cada item traz a reprodução com o binário **real**, o observado, o esperado, a causa e a
proposta.

### 3.1 A1 — Palavra de keel como nome: leitura fora dos limites e diagnóstico falso

**Severidade: alta** (memória + diagnóstico enganoso; alcance: qualquer projeto que declare
algo com o nome de uma palavra de keel).

**Reprodução** (release ou dev, tanto faz):

```sh
printf 'module owner;\ntypedef i32 pub;\npub inline i32 value(void) { return 17; }\n' > owner.k
cgen -I . --base-dir base --stop-after=gen --dest-dir gen owner.k; echo rc=$?
```
```
owner.k:2:13: warning: `pub` is a keel word or the qualifier of an import: where it stands as a construction, keel still reads the construction [keel-name-shadowed]
owner.k:3: error: a generated line needs more than LINE_CAP = 1024 bytes (engine/emit/emit_internal.h); break that source line into several lines (the limit is per line, not per file) [implementation-limit]
rc=2
```

**Observado.** A linha 3 tem 45 bytes. O usuário é mandado quebrar uma linha curta, e o erro
aponta para a declaração seguinte, não para a que deu origem ao problema. Com `i32 pub = 1;`
no lugar do `typedef` o resultado é o mesmo (rc=2); com a função sem `pub` no cabeçalho,
não acontece (rc=0).

Com as redzones do apêndice B, o ASan mostra o que acontece antes do diagnóstico (aqui com
a variante `constexpr i32 BIAS pub = 2;`, achada pelo fuzzing):

```
==ERROR: AddressSanitizer: use-after-poison ... READ of size 1
    #0 k_em_put_bytes      emit/emit_core.c:70
    #1 k_em_copy_edited    emit/lines.c:106
    #2 k_em_emit_proto     emit/decls.c:191
    #3 h_body              emit/files.c:109
    #4 emit_header         emit/files.c:224
```
O endereço lido é exatamente o primeiro byte após o fim do buffer da fonte (conferido pela
aritmética de offsets da arena).

**Causa.** Uma cadeia de três elos, cada um inocente sozinho:

1. A linguagem permite palavras de keel como nomes e avisa (`keel-name-shadowed`). O aviso
   promete: *"where it stands as a construction, keel still reads the construction"*.
   Então `typedef i32 pub;` é legal e cria um símbolo **chamado `pub`**.
2. No protótipo de `pub inline i32 value(void)`, `k_em_decl_edits` (`decls.c:33-43`) empilha
   o edit que troca a visibilidade: `at` = offset do `pub`, `skip = 3`. Mais adiante, no mesmo
   laço de renomeações, o token `pub` "resolve" para o símbolo homônimo e recebe um
   **segundo** edit, um `EDIT_RENAME` no mesmo `at`. O emissor quebra a promessa do aviso:
   lê o mesmo `pub` como construção e como referência.
3. `k_em_copy_edited` (`lines.c:104-106`) avança `pos` três bytes com o primeiro edit e então
   calcula `ed[1].at - pos` = −3. Como `size_t`, isso é SIZE_MAX − 2. `k_em_put_bytes` passa
   a copiar da fonte até bater em `LINE_CAP` (1024 bytes): lê além do fim da fonte, dentro da
   arena. Só então `k_em_limit` recusa — e o diagnóstico sai na linha seguinte.

**Alcance.** Não é do `constexpr` nem do `typedef`: basta **qualquer declaração de escopo de
arquivo chamada `pub`** seguida de uma função `pub` cujo protótipo ou corpo seja emitido. As
três ocorrências do fuzzing são todas essa forma (um `typedef ... pub;` e uma função
`pub(void)`). Hoje o estouro é contido por acaso: `LINE_CAP` corta a cópia antes de qualquer
coisa vazar para o arquivo gerado. Um outro consumidor da mesma subtração que não tenha esse
teto escreveria memória da arena no C de saída.

**Esperado.** Se a linguagem mantém "palavra de keel pode ser nome" (decisão dela, com o
warning), o programa acima é legal e deve gerar `typedef i32 owner_pub;` e
`static inline i32 owner_value(void) { … }`. Se decidir o contrário, deve ser **erro**, com a
posição. Em nenhum caso é este diagnóstico.

**Proposta** (duas mudanças; a segunda é defesa em profundidade e vale por si):

- **Não renomear um token em posição de construção.** Em `k_em_decl_edits`, o token de
  visibilidade (`n->first`, quando `k_em_is_visibility`) não entra no laço de renomeações.
  Regra geral para o laço: um token que o parser leu como construção não é referência a
  declaração. Vale conferir as outras construções que também podem ser nomes (`as`,
  `inline`, `byref`…): nenhuma gera edit hoje, mas é o mesmo princípio.
- **Validar a invariante dos edits.** `k_em_copy_edited` recebe os edits ordenados e sem
  sobreposição; é o contrato de `k_em_push_edit`/`k_em_sort_edits`, e nada o verifica. Um
  `if (ed[k].at < pos) { k_em_refuse(...); return; }` transforma qualquer reincidência em
  recusa limpa, em vez de leitura fora dos limites. Barato: uma comparação por edit.

### 3.2 A2 — `constexpr` malformado é aceito e some em silêncio

**Severidade: alta** (código que o usuário escreveu não chega ao C, sem erro).

**Reprodução:**

```sh
printf 'module owner;\nconstexpr i32 BIAS priv = 2;\n' > owner.k
cgen -I . --base-dir base --stop-after=gen --dest-dir gen owner.k; echo rc=$?
```
```
owner.k:2:20: warning: `priv` is a keel word ... [keel-name-shadowed]
rc=0
```
`gen/owner.c` fica com o `#include` e nada mais (102 bytes); `owner.h` só com as guardas. A
constante não existe.

**A matriz** (mesma linha, só muda o token antes do `=`):

| Token antes do `=` | rc | Diagnóstico | C gerado |
| :-- | :-: | :-- | :-- |
| `priv`, `as`, `pub` | **0** | só o warning | **nada** |
| `inline`, `5`, `+` | 1 | `constexpr-name-missing` (correto) | — |
| nada (`constexpr = 2;`) | 1 | `constexpr-name-missing` (correto) | — |
| `BIAS = ;` (valor ausente) | 2 | `implementation-limit` "does not cover this construction" | — |

**Causa.** `constexpr-name-missing` exige que o token antes do `=` seja `k_token_is_ident`.
Esse predicado exclui as palavras reservadas de **C** (`inline`), mas não as palavras
contextuais de **keel** (`pub`, `priv`, `as`), que para ele são identificadores comuns. O
parser aceita então `i32 BIAS` como tipo e `priv` como nome; o emissor não escreve a
constante. (A causa raiz está na aceitação; o ponto exato da omissão no emissor não foi
perseguido.)

De novo há contradição com o próprio aviso do lexer: ele diz que keel lê a construção onde
ela está — mas o parser leu `priv` como nome.

**Esperado.** Um erro com a posição. O `constexpr` tem forma fixa (`constexpr T NAME =
value;`, como a própria mensagem de `constexpr-name-missing` diz); `i32 BIAS` não é um tipo
registrado e `priv` não é um nome.

**Proposta.** Em `constexpr-name-missing`, recusar também token antes do `=` que seja palavra
de keel (`k_token_is_keel_word`), e o tipo de mais de um token que não seja uma frase de tipo
de C (`k_c_words_primitive`). **Ponto de spec:** se a regra for "palavra de keel nunca nomeia
um `constexpr`", é texto normativo — pela regra do projeto, discutir antes. Se a decisão for
aceitar, então o emissor tem de escrever a constante.

Observação menor, mesma tabela: `constexpr i32 BIAS = ;` sai como *emitter does not cover
this construction yet*, quando é erro de sintaxe (`unexpected-token`).

### 3.3 A6 — Protótipo em várias linhas é recusado com `LINE_CAP` falso

**Severidade: média** (código legal recusado; mensagem enganosa; sobe se funções de 20 ou
mais parâmetros forem comuns no projeto).

**Reprodução:**

```sh
{ printf 'module owner;\npub inline bool validate(\n'
  for i in $(seq 0 29); do printf '    i32 requested_token_count_%02d%s\n' $i $([ $i -lt 29 ] && echo ,); done
  printf ') { return true; }\n'; } > owner.k
cgen -I . --base-dir base --stop-after=gen --dest-dir gen owner.k; echo rc=$?
```
```
owner.k:2:1: error: a generated line needs more than LINE_CAP = 1024 bytes (engine/emit/emit_internal.h); break that source line into several lines (the limit is per line, not per file) [implementation-limit]
rc=2
```

**Observado.** A linha mais longa do arquivo tem 33 bytes. A mensagem diz que o limite é por
linha e manda quebrar a linha — o usuário já a quebrou, em 30. O limiar é o tamanho **total**
do protótipo, da visibilidade ao `)`:

| Protótipo com um parâmetro por linha | texto da declaração | linha mais longa | rc |
| :-- | :-: | :-: | :-: |
| 24 parâmetros (`requested_token_count_NN`) | 844 B | 33 B | 0 |
| 30 parâmetros (idem) | 1048 B | 33 B | **2** (`LINE_CAP`) |
| 12 parâmetros de tipo qualificado longo | 706 B | 52 B | 0 |
| 80 parâmetros curtos (`i32 aN`) | 1058 B | 13 B | **2** (`LINE_CAP`) |
| 160 parâmetros curtos | 2158 B | 13 B | 1 (`more than 128 variables visible at once` — correto) |

Vale para qualquer visibilidade: `pub inline`, `pub`, `inline` e `priv inline` dão o mesmo
resultado nas três faixas.

**Causa.** `k_em_emit_proto` (`decls.c:181-195`) copia a declaração inteira, do início ao
`)` que fecha os parâmetros, **num texto só** (`k_em_copy_edited` → um `k_emit_at`). O buffer
desse texto é `e->buf`, de `LINE_CAP` bytes. O limite, que a mensagem declara por linha,
passa a ser por declaração. Abaixo do teto o protótipo sai com as quebras do autor (24
parâmetros: 25 linhas no `.h`, um `#line` no topo), o que mostra que a emissão por linha já
funciona.

**Esperado.** Compilar. Ou, se o teto por declaração for desejado, que a mensagem o diga.

**Proposta.** Emitir o protótipo como o corpo é emitido: linha fonte por linha fonte
(`k_em_emit_range` com os edits), de modo que `LINE_CAP` meça o que a mensagem promete. Em
qualquer hipótese, a mensagem não deve mandar quebrar uma linha que já está quebrada.

### 3.4 A3 — Tempo quadrático com splices numa mesma linha lógica

**Severidade: média** (não é falha de correção; é negação de serviço barata: 78 KB custam
1,7 s de CPU).

**Reprodução** (release, `--stop-after=gen`; gerador no apêndice C):

```sh
gen file splice 40000 owner.k      # `int x = 1` + 40 000 × `\⏎` + ` + 2;`
time cgen -I . --base-dir base --stop-after=gen --dest-dir gen owner.k
```

| splices na mesma linha lógica | 2 500 | 5 000 | 10 000 | 20 000 | 40 000 |
| :-- | :-: | :-: | :-: | :-: | :-: |
| escopo de arquivo | 21 ms | 60 ms | 205 ms | 667 ms | **1,69 s** |
| dentro de uma função `pub inline` | 29 ms | 75 ms | 229 ms | 820 ms | **3,13 s** |

Cresce ×2,5 a ×3,8 a cada duplicação: quadrático. Todos concluem (rc=0). **Controles, também
com 40 mil:** espaços na mesma linha → 5 ms; comentários `/**/` → 4 ms; splices dentro de uma
diretiva (`#define` continuado) → 11 ms, linear. O problema é específico de splices **entre
tokens numa linha comum**.

**Causa** (callgrind, n = 5 000: 92% das instruções em `k_em_comment_edits`, chamado duas
vezes por linha física). O arquivo é escrito linha física por linha física, e para cada uma o
emissor procura comentários nos intervalos entre tokens. A busca (`lines.c:23-47`)
**recomeça no começo do intervalo** (`g0`), nunca no começo da linha corrente — de propósito,
diz o comentário, porque um comentário de bloco aberto antes pode cobrir o início desta
linha. Com splices, o intervalo entre `1` e `+` cobre as 40 mil linhas físicas de uma vez: a
k-ésima linha custa O(k), e o total é O(n²).

**Proposta.** Guardar, por intervalo, o que é estático no arquivo: se há `/*` não fechado
antes da linha e onde ele termina. Uma passada linear no começo da emissão (ou memorização
por intervalo) e cada linha recomeça em `max(g0, a)`. O caso que o comentário protege continua
coberto.

### 3.5 A4 — Escala com o número de declarações, e com o tamanho da função

**Severidade: média.** O `achados/E03` já registra "tempo quadrático: aberto" para `extent`;
a medição mostra que é geral.

**A4 — muitas declarações.** `module owner;` + N funções
`pub inline i32 gN(void) { i32 v = N; return v + 1; }` (release):

| N | 500 | 1 000 | 2 000 | 4 000 | 8 000 | 16 000 |
| :-: | :-: | :-: | :-: | :-: | :-: | :-: |
| fonte | 27 KB | 55 KB | 113 KB | 228 KB | 458 KB | 931 KB |
| tempo | 168 ms | 439 ms | 1,44 s | 5,0 s | **11,2 s** | arena esgotada (A5) |

×2,6 a ×3,5 por duplicação. **Pontos quentes (gprof, N = 3 000)** — todos buscas lineares
repetidas por linha ou por token:

- **`k_em_declared_at` (`emit_core.c:98`): 51% do tempo.** Para cada identificador
  candidato, a cada linha emitida, varre **todas** as declarações do módulo à procura do nome
  → O(linhas × declarações);
- `k_token_spelled` 9,5% (50 M chamadas) e `k_em_overlaps` 9,5% (36 M chamadas = linhas ×
  nós: `emit_line` percorre todos os nós a cada linha);
- `k_collect_instances` (`instances.c`) 4%: para cada token, varre todos os nós para saber se
  está numa região opaca → O(tokens × nós);
- `k_symbol_resolve` / `k_symtab_lookup` (`symtab.h`) 3,2%: linear por nome — decisão
  documentada em parser-design §2.1 ("nunca se aproxima de onde importaria"); a tabela mostra
  onde passa a importar: ~2 mil declarações;
- `body_of` (`constexpr_block.c:25`) 3,2%: para cada token, varre todos os nós à procura da
  função que o contém.

**A4b — uma função grande.** O custo também cresce com o tamanho de **uma** função, por um
caminho diferente:

| `pub inline i32 f(void) { int a[E] = { … }; return a[0]; }`, um elemento por linha | 100 | 200 | 400 | 800 | 1 600 |
| :-: | :-: | :-: | :-: | :-: | :-: |
| tempo (mínimo de 3) | 6 ms | 11 ms | 32 ms | 116 ms | **448 ms** |

×1,8 a ×3,9 por duplicação. Um enunciado com N linhas em branco no corpo dá o mesmo
(11 / 26 / 71 / 235 / 862 ms para 2 500 a 40 000 linhas). Não há splice aqui: é só uma função
comprida.

**Causa** (gprof, E = 1 600): `k_em_node_edits` é chamado 3 240 vezes — uma por linha emitida,
no `.h` e no `.c` — e cada chamada dispara **os ~20 analisadores** de construção
(`k_em_edits_defer`, `_constblock`, `_foreach`, `_rangelit`, `_uses`, `_roles`…, `emit.c:14-42`),
cada um varrendo **todos os tokens da função** (`for (i = fn->body_first; i < fn->body_end; i++)`).
Resultado: 51,9 milhões de chamadas de `k_em_text_is` e 62,2 milhões de `k_em_tok` para uma
função de ~3 mil tokens. É O(linhas × tokens) **por função**, vezes o número de analisadores.
Em callgrind: `k_token_is_punct` 21%, `k_em_text_is` 18%, `strlen` 12%, `k_em_tok` 7%.

**Proposta.** Para A4b: cada analisador calcula os edits da função **uma vez** (em ordem de
offset) e a emissão de cada linha só consome a faixa `[a, b)` — um cursor por analisador, ou
um índice ordenado com busca binária, como `k_em_comment_edits` já faz (`first_tok_from`).
Para A4: um índice nome → declaração (tabela de dispersão, ou `nodes` ordenado por nome à
parte) elimina a metade do tempo que `k_em_declared_at` consome; para os demais,
pré-calcular por token, em O(n), o que hoje é recalculado — `função_do_token[i]`,
`em_região_opaca[i]` — e, para nós e ilhas, um índice ordenado por offset. A emissão em ordem
de declaração continua vindo de `nodes`, não do índice.

### 3.6 A5 — Reserva por token: ~536 bytes por token, teto de ~250 mil tokens

**Severidade: média.**

Com N = 16 000 (931 KB de fonte), o `cgen` recusa com `arena capacity exhausted allocating
KInstanceUse; used 70616208, configured limit 134217728`. A causa está no dimensionamento, não
no uso: `tool/tool.c` reserva **uma entrada por token** em cinco tabelas, quando quatro delas
têm uma entrada por *declaração*, por *ilha* ou por *instância*:

| tabela | `tool.c` | bytes/token | entrada real por |
| :-- | :-- | :-: | :-- |
| `KLexeme` | :190 | 24 | token (correto) |
| `KAstNode` | :191 | 128 | declaração (≈ 1 a cada 19 tokens no teste) |
| `KSymbol` | :238 (`capacity = count + 1`) | 72 | declaração |
| `KInstanceUse` | :288 | **280** | instância distinta (poucas por módulo) |
| `KIsland` | :294 | 32 | ilha |
| | | **536** | |

Os tamanhos batem com a tabela que o próprio erro imprime: 38,97 MB de `KAstNode` e 23,08 MB
de `KSymbol` para ~304 mil tokens. 128 MiB ÷ 536 ≈ 250 mil tokens ≈ **0,8 a 1,2 MB de
fonte**, conforme a densidade. E o limite só se muda **recompilando**: `CGEN_ARENA_CAPACITY`
é constante de build, e a mensagem manda "adjust CGEN_ARENA_CAPACITY and rebuild the cgen".

**Proposta.** (i) Curto prazo: aceitar o limite também por ambiente (`CGEN_ARENA_CAPACITY`
em tempo de execução, com o mesmo aviso), para quem bate no teto sem poder recompilar.
(ii) Dimensionar `KSymbol`, `KInstanceUse` e `KIsland` pelo que o parser encontra (duas
passadas: a primeira já conta tokens) e manter só `KLexeme` por token. `KInstanceUse` sozinho
é 52% da reserva.

### 3.7 S1 — O compilador usa uma base sem checagem, e o ASan não vê dentro da arena

**Severidade: média** (é o que permitiu A1 passar por duas rodadas de revisão com ASan).

- A base real (`base/keel/buffer.k`) chega a checar: `get`/`ptr`/`set` levam
  `KEEL_CHECK(i < b->len, …)`. Mas o `cgen` **não é compilado contra ela**: usa a cópia
  *bootstrap* em `tools/transform/base/`, que não tem nenhuma checagem — nem no fonte
  (`tools/transform/base/keel/buffer.k`: 0 ocorrências de `KEEL_CHECK`) nem nos cabeçalhos
  gerados (`tools/cgen/gen/keel/keel_buffer_KLexeme.h`: `return &b->ptr[index];`). São **264**
  acessos `keel_buffer_*_ptr(&buf, i)` no motor, todos dependentes de o chamador acertar o
  índice, sem rede de segurança nenhuma — nem no build `-DDEBUG`.
- Com a arena inteira num `malloc` só, o ASan não tem redzone entre alocações: estouro de
  tabela é invisível. A rodada 2 concluiu "disciplinado" a partir de evidência que não podia
  mostrar o contrário. Com redzones (apêndice B), 118 mil execuções mostraram **um** defeito:
  a disciplina é real, mas não absoluta.

Isso reclassifica o S3 da rodada 2: **não é só "risco de uso"** — há um bug presente.

**Proposta.**

1. **Um alvo `make -C tools/cgen asan`** (ou `check-asan`): ASan + UBSan + o patch de redzone
   do apêndice B (atrás de `#ifdef CGEN_ASAN_REDZONE` em `memory.h`), rodando a suíte. Para
   isso os scripts de teste precisam aceitar outro binário (M2).
2. **No build de desenvolvimento, checar.** Regenerar `tools/cgen/gen/` com os acessores
   checados da base atual (ou, se o bootstrap não dá conta, um `#ifdef DEBUG` nos acessores
   gerados). O build `release` fica como está.
3. **Defesa em `k_em_copy_edited`** (segunda parte da proposta de A1): a invariante dos edits
   passa a ser verificada onde é consumida.

### 3.8 S2 — Estado global mutável no motor **[persiste-02]**

Quatro variáveis de arquivo, fora de qualquer contexto: `lines.c:97` (`spanned_end`),
`emit_core.c:58` (`pending_cap`), `parser_islands.c:727` e `:734` (`seed_node_count`,
`seed_count`). Continua sem ser bug no processo de tiro único de hoje, e continua sendo o
impedimento para paralelizar o carregamento de módulos ou reusar o motor como biblioteca.
A recomendação da rodada 2 vale: mover para `KAst`/`Emit`.

### 3.9 O que está sólido (com a evidência)

Para o balanço ficar claro — e porque é o que deve ser preservado em qualquer conserto:

- **Fronteira `engine/` ↔ `tool/` verificada pelo build** (`make boundary`): o motor não
  inclui `stdio`/`stdlib`; E/S só por callback (`KLoader`) e por slices.
- **Escrita limitada por convenção uniforme**: `writer.c`, `mangle.c`, `instances.c`
  (`Name`), todos "medir, depois preencher", com bytes além do fim contados e descartados.
  Os três `strcpy`/`strcat` do `tool/` estão em buffer dimensionado na linha anterior.
- **Sem shell**: nenhum `system`/`popen`; `cc` por `fork` + `execvp` com `argv` em array.
  `--cc` é um nome de programa, não uma linha de comando.
- **Escrita atômica e segura** (`output.c`): temporário `O_EXCL` no mesmo diretório +
  `rename` **[fechado-02]**; arquivo idêntico não mexe no mtime. (A sobra de um crash é o
  Q11, baixa.)
- **Caminhos**: `cgen_path_normalize` é lexical e correta nos casos de `..` conferidos
  (`/..`, `a/../..`, `a/b/../../..`, `../../a`); um nome de módulo só contém identificadores
  e pontos, logo `import` e os arquivos gerados nunca escapam de uma raiz ou de `--dest-dir`.
- **Reordenação da linha do `cc`** (`run_cc.c`): `-x c`, `-U FOO`, `-MF /dev/stdout` e as
  bibliotecas separadas (`-l m`, `-L dir`) chegam ao compilador na ordem certa.
- **Escopo nos renomes**: parâmetros e locais homônimos de nomes do módulo saem como
  escritos; só os nomes do módulo são renomeados (conferido no C gerado e na compilação).
- **Tetos de profundidade**: nenhuma das onze formas de aninhamento extremo derrubou a pilha.
- **Determinismo**: golden byte a byte em dois perfis, 180/180.
- **Rastreabilidade normativa**: cada função cita a regra que implementa; o catálogo de
  diagnósticos é gerado e travado (`diag-messages.lock.json`), e a suíte confere a sync.

### 3.10 Varredura final — as ferramentas ao redor do compilador (baixa severidade)

**X1 — `tools/transform` (gerador de *bootstrap*), três defeitos confirmados sob ASan.**

| | Entrada | Observado |
| :-: | :-- | :-- |
| T1 | fonte maior que 3 MiB | **estouro do vetor global** `memory` em `load_file` (`main.c:305-317`): `fread` de `ftell` bytes no buffer fixo de 1 MiB, sem checar tamanho nem retorno. Entre 1 e 3 MiB a leitura sobrescreve em silêncio a arena vizinha (o mesmo vetor), sem nenhum aviso |
| T2 | `transform -m` (opção sem valor) | **SIGSEGV**: o laço de opções (`main.c:415-444`) faz `i++` e cai no `if` seguinte com `argv[argc]`, que é `NULL` → `strcmp(NULL, …)`. Um valor que começa com `-` é lido como opção |
| T3 | comentário de cabeçalho de 9 000 bytes | o cabeçalho **some inteiro** do `.h` (inclusive a licença), rc=0: `head` tem 1024 bytes e `strbuf_append_strbuf` descarta o bloco quando não cabe |

Além disso: é o único `.c` do projeto que compila com warning no build padrão (`fread` sem
uso do retorno, `main.c:315`); `fwrite`/`fclose` sem verificação (disco cheio → header
truncado com rc=0, e o alvo `base` do Makefile copia justamente o que difere); erros vão para
stdout; `mkdir_p` tem o retorno ignorado; nenhum limite é diagnosticado. Alcance pequeno — a
linha de comando é fixa no Makefile e as entradas são os `.k` pequenos do bootstrap —, mas é
a mesma família de defeitos do resto. **Proposta:** checar `len <= cap` em `load_file`, `else`
no laço de opções (e recusar valor ausente), e `strbuf_cstr` em `head` antes de escrever.

**X2 — nomes previsíveis em `/tmp`.** `test/cgen/lex_dump.sh` (`/tmp/lexdiff.$$`, `lexref.$$`,
`lexdiag.$$`), `test/cgen/parse_dump.sh` (`/tmp/parsediff.$$`) e `tools/agents/verify.sh`
(`/tmp/agents_build.$$`, `agents_check.$$`, `agents_diff.$$`) gravam com `>` num nome derivado
do pid. Numa máquina multiusuário, outro usuário pode plantar um link simbólico ali e ter um
arquivo seu sobrescrito com a saída do diff (CWE-377). Os outros trinta scripts de teste já
usam `mktemp -d` com `trap`. **Proposta:** o mesmo padrão nesses três. (Nota: `verify.sh` também
roda `make clean` em `tools/cgen`, o que apaga `cgen-release` e `obj-release`.)

**X3 — `editors/install-zed.sh`.** Num destino que já é repositório Git, o script copia os
arquivos e roda `git add -A` + `commit`: tudo que estiver pendente naquele repositório entra
no commit "keel: Zed extension". **Proposta:** adicionar só os arquivos copiados (`git add --
"$f"` por arquivo). O irmão `install-vscode.sh` está correto: recusa destino que não seja
link e cita todas as variáveis.

**X4 — `gdb/keel_gdb.py`.** `line.split("\t")` exige exatamente três campos: uma linha
malformada num `.keelsyms` levanta exceção dentro do comando do gdb. Esqueleto declarado.
**Proposta:** `split("\t", 2)` e pular a linha com aviso.

**O que está sólido nessa camada.** Nenhum `shell=True`, `eval`, `os.system`, `mktemp -u` ou
`curl | sh` em 62 scripts; `subprocess` sempre com lista de argumentos; `diag-sync.py` guarda a
chave só no ambiente e valida a resposta do modelo antes de ela virar literal C (esquema JSON,
mesmo número de `%s`, nenhuma outra conversão, sem quebra de linha), e `gen-diags.py` escapa
`\` e `"`; `generate.py` monta as regexes por `re.escape` e confere que toda palavra lida da
spec chegou aos dois artefatos; `layout-check.py` só reescreve com `--fix`; `blank-check.py`
trabalha numa cópia nova sob `/tmp`.

---

## 4. Clareza, objetividade e qualidade (baixa severidade, baratas)

| # | Achado | Onde | Proposta |
| :-- | :-- | :-- | :-- |
| Q1 | **Saídas sem mensagem.** 43 linhas de `return 2;` na camada `tool/` sem nenhuma linha no stderr. Confirmado: um `--instance` com 1 500 argumentos sai com rc=2 e **zero linhas** (o texto sintetizado não cabe em `USE_CAP`) | `stop_gen.c`, `run_cc.c` | uma linha por caminho, com o id do diagnóstico |
| Q2 | **`closure_mtime` é estado morto.** Calculado (`tool.c:240,324-325`), nunca lido para decisão. O help do `-f` promete "ignoring timestamps", mas não há lógica de timestamps: o `-f` só pula a comparação de conteúdo | `tool.c`, `main.c` (help) | remover o campo, ou implementar a regra de *staleness* da spec §5; corrigir o help |
| Q3 | **Três mensagens para "não consigo ler a fonte"**, duas delas em duplicata: `cannot read source` (stat falha); `cannot open file` **+** `cannot load module` (fopen falha); `short read` **+** `cannot load module` | `tool.c`, `read_source.c` | uma mensagem, na camada que souber do problema |
| Q4 | **A tabela de uso da arena vai para o usuário em release.** `cgen_memory_error` imprime "arena usage by label" em toda exaustão (15 linhas no caso do A5, no `dist/bin/cgen`), embora o próprio cabeçalho diga ser contabilidade de desenvolvimento, "not documented anywhere a user reads" | `tool/memory.h:66-76` | só com `CGEN_ARENA_STATS` ou em `DEBUG` |
| Q5 | **`enum TKPpKind` definido duas vezes** (em `lexer.h` e de novo em `lexer_scan_directive.c:7-13`). Se a ordem mudar num, o outro discorda em silêncio | `lexer_scan_directive.c` | incluir `lexer.h` |
| Q6 | **`#line` não escapa `\`** no caminho. Um módulo com UCN (`a\u00e9b`) gera `#line 2 "a\u00e9b.k"`; dentro da literal, `\u00e9` é `é`, e o depurador procura um arquivo que não existe | `emit/writer.c:60-62` | dobrar a barra no caminho |
| Q7 | **Depfile truncado em silêncio** além de `CGEN_SOURCES_CAP` (1024 fontes): `cgen_loader_sources` para em `n < cap` sem avisar | `tool.c:165`, `run_cc.c` | `implementation-limit` nomeando o teto |
| Q8 | **`ftell` sem verificação** no depfile: −1 vira `malloc(0)` + `fread(…, SIZE_MAX)`. Hoje inofensivo só porque a leitura falha antes de escrever | `run_cc.c:73-75` | checar `sz < 0` |
| Q9 | **Warnings extras no build** (185). Os que valem: `shadow` 7 (`parallel.c:190,227`, `subscript.c:297`, `instance.c:585,593`, `parser_islands_types.c:1150,1218`); `missing-prototypes` 37 (funções de ligação externa sem cabeçalho: `cgen_stop_after_*`, `cgen_run_cc`, declaradas à mão em `main.c`); `cast-qual` 129 (52 em `emit.h`, descartando `const` — a "mutação de AST" M3 da rodada 2, **[persiste-02]**) | vários | acrescentar `-Wshadow -Wmissing-prototypes` ao `CFLAGS`; mover protótipos para `tool.h` |
| Q10 | **`base` resolvida antes da fonte existir.** Sem `--base-dir`, o binário de desenvolvimento (`tools/cgen/cgen`, sem `../lib/base`) responde `base-not-found` mesmo para `cgen nope.k`; o erro útil fica mascarado. No `dist/` não acontece | `main.c:293-300` | checar a fonte antes de resolver a base |
| Q11 | **Sobra do temporário de escrita.** O nome é previsível (`.keel.<pid>.tmp`, `output.c`). Um crash entre `open` e `rename` deixa a sobra; um `cgen` posterior com o mesmo pid (reuso, contêineres com namespace pequeno) falha com `cannot write 'gen/keel.type.h' [write-failure]`, sem pistas — reproduzido plantando o temporário | `output.c:48-56` | em `EEXIST`, remover a sobra (arquivo regular, sem dono estranho) e tentar de novo; ou sufixo aleatório. `O_EXCL` está certo e fica |
| Q12 | **Comentário defasado** em `k_em_declared_at`: diz que o escopo não é rastreado e que um local homônimo de uma declaração do módulo também é renomeado. Não é o que o código faz: `decls.c:158` só renomeia onde o parser deixou ilha | `emit_core.c:92-97` | reescrever o comentário |
| Q13 | **`tools/transform/README.md` desatualizado**: descreve a detecção de "linha module" (não existe: o nome vem de `-m`), não lista a opção `-a` nem a marca `$A`, e tem erros de digitação ("copiador", "subistitui") | `tools/transform/README.md` | alinhar ao cabeçalho de `main.c`, que está correto |

**Observações que não são bugs** (registro, para não investigar duas vezes):

- Um arquivo com BOM UTF-8 recebe `missing-module`: o `cgen` não salta a marca. É decisão de
  spec (o arquivo "começa com `module`"); se o público usa editores que a inserem, vale uma
  linha na mensagem.
- A suíte informa `the catalog is covered 145/157`: 12 diagnósticos do catálogo não têm caso
  de falha. O próprio `diag.sh` já é o medidor.
- A suíte leva ~5 min nesta máquina, e `unit` é 153 s deles.

---

## 5. Modularidade e estrutura

| # | Achado | Proposta |
| :-- | :-- | :-- |
| M1 | **Invariante implícita entre módulos.** `cgen_run_cc` lê `late[i]` para `i < pass_n` sem bornar; só é seguro porque `main.c` limita `pass_n` a `CGEN_CLI_ARGS_CAP`. Quem subir a constante, ou chamar a função de outro lugar, estoura a pilha sem aviso | bornar no consumo (`i < CGEN_CLI_ARGS_CAP + 8`), ou `_Static_assert` + parâmetro `cap` |
| M2 | **Testes presos ao binário.** 29 scripts fixam `BIN=$PWD/tools/cgen/cgen` (23) ou o caminho direto (6). Não há como rodar a suíte contra um build ASan/redzone sem mexer na árvore | `BIN=${CGEN:-$PWD/tools/cgen/cgen}` em `test/cgen/base.sh` e nos demais |
| M3 | **`stop_gen.c` concentra responsabilidades** (721 linhas: caminhos, perfil, lowering, `mkdir -p`, emissão de módulo, de imports, de instâncias, de funções sobre protocolo, da *entry* e o parser manual de `--instance`) **[persiste-02]** | separar `--instance` (parser de texto) da escrita; os demais em ocasiões futuras de edição |
| M4 | **Base *bootstrap* é outro dialeto, e mais antiga.** `tools/transform/base/` não é uma cópia da base real: é o "pseudo keel" de marcas (`%type`, `%h`, `%impl`, `$T`), sem `generation` e sem `KEEL_CHECK` (S1), e os cabeçalhos gerados dizem "LICENSE: GPL v3" onde a base diz "GPL v3 + Keel Base Library Exception". Enquanto o cgen não se compilar a partir da base real (a meta do próprio README), as duas convivem | registrar a diferença em `tools/transform/README.md`; ao menos com os acessores checados (S1) e a licença da base |

---

## 6. Higiene do repositório

| # | Achado | Proposta |
| :-- | :-- | :-- |
| H1 | **Não rastreados na raiz:** `gen/` (sondas: `p1.c`, `probe*.h`…), `skills/`, `achados/Análise.md`, `a.out` (ignorado por `*.out`, mas é sobra) e `gdb/__pycache__/` (não ignorado) | remover ou ignorar; `__pycache__/` e `*.pyc` no `.gitignore` |
| H2 | **A árvore versionada está limpa de artefatos**: nenhum `.o`, `.out`, `obj/` ou binário rastreado; `.venv/`, `compile_commands.json` e `.claude/settings.local.json` já ignorados | — |

---

## 7. Propostas priorizadas

| Ordem | Item | Esforço | O que resolve |
| :-: | :-- | :-: | :-- |
| 1 | **Casos de teste antes do conserto** (regra do `AGENTS.md`): um teste de CLI com A1, A2 e A6, com o esperado escrito à mão, e um com splices (A3) | pequeno | impede recaída dos três bugs |
| 2 | **A1**: não renomear token em posição de construção + verificar `ed[k].at >= pos` em `k_em_copy_edited` | pequeno | leitura fora dos limites e diagnóstico falso |
| 3 | **A2**: recusar palavra de keel antes do `=` em `constexpr` (**discutir o texto da spec antes**) | pequeno | código que some em silêncio |
| 4 | **A6**: emitir o protótipo linha por linha | pequeno | código legal recusado |
| 5 | **M2 + alvo `asan`** com redzones (apêndice B), rodando a suíte no pre-push ou num CI noturno | médio | passa a enxergar o que o ASan comum não vê |
| 6 | **A3**: `k_em_comment_edits` recomeçar em `a`, com a informação do bloco aberto memorizada por intervalo | pequeno | quadrático barato de disparar |
| 7 | **A4b**: os analisadores calculam os edits da função uma vez; a linha consome a faixa | médio | função de 1600 linhas em 0,45 s |
| 8 | **A4**: índice nome → declaração; índices por token e ordenados; dispersão para símbolos | médio | 11 s para 8 mil declarações |
| 9 | **A5**: limite por ambiente e dimensionamento de `KSymbol`/`KInstanceUse`/`KIsland` por declaração/instância | médio | teto de ~1 MB de fonte |
| 10 | **X1** (`transform`: limite em `load_file`, `else` nas opções, `head` verificado), **X2** (`mktemp -d` nos três scripts), **X3** (`git add` só do copiado) | pequeno | três defeitos baixos, cada um de poucas linhas |
| 11 | Q1–Q8, Q10–Q13 (um a um, cada um pequeno) | pequeno | clareza das mensagens e dos textos |
| 12 | S2 (estáticos), Q9 (warnings extras), M1, M3, M4 | médio | dívida estrutural |

---

## Apêndice A — Reproduções mínimas

Caso do cgen com o binário real, todos com
`cgen -I . --base-dir base --stop-after=gen --dest-dir gen owner.k`, no diretório do `owner.k`;
os do `transform` com um build ASan do `tools/transform/src/main.c`.

| Caso | Entrada | Observado | Esperado |
| :-- | :-- | :-- | :-- |
| A1 | `module owner;`<br>`typedef i32 pub;`<br>`pub inline i32 value(void) { return 17; }` | rc=2, `LINE_CAP` na linha 3 | gera `owner_pub` e `owner_value` |
| A1 (variante) | `module owner;`<br>`i32 pub = 1;`<br>`pub inline i32 value(void) { return 17; }` | idem | idem |
| A1 (não cai) | o mesmo com a função sem `pub` | rc=0 | ok |
| A2 | `module owner;`<br>`constexpr i32 BIAS priv = 2;` | **rc=0**, C vazio | erro com a posição |
| A2 (não cai) | `constexpr i32 BIAS inline = 2;` | rc=1, `constexpr-name-missing` | ok |
| A6 | `module owner;` + `pub inline i32 f(` + 30 × `    i32 requested_token_count_NN,` + `) { return a0; }` | rc=2, `LINE_CAP` (linhas de 33 bytes) | compila |
| A6 (não cai) | idem com 24 parâmetros | rc=0 | ok |
| A3 | `module owner;` + `int x = 1` + 40 000 × `\⏎` + ` + 2;` | 1,69 s (release) | linear (~15 ms) |
| A4b | `module owner;` + função com `int a[1600] = {` + um elemento por linha | 448 ms | linear |
| A5 | 16 000 funções | arena esgotada (70,6 MB usados) | ou compila, ou limite ajustável |
| Q11 | `printf sobra > gen/.keel.$(pid do cgen).tmp` antes da escrita | `cannot write 'gen/keel.type.h' [write-failure]` | remove a sobra e escreve |
| X1-T1 | `.k` de 4 MiB para o `transform` | ASan: `global-buffer-overflow` em `load_file` | recusa nomeando o teto |
| X1-T2 | `transform -m` (último argumento) | SIGSEGV (`strcmp(NULL, …)`) | mensagem de uso |
| X1-T3 | `.k` com comentário inicial de 9 000 bytes | `.h` sem o comentário, rc=0 | o comentário inteiro, ou erro |

## Apêndice B — Redzones intra-arena (para o alvo `asan`)

Numa cópia de `tool/memory.h`, toda alocação passa a ter uma zona envenenada antes e depois.
Com isso o ASan detecta estouro **entre** alocações da arena, que de outro modo ele não vê.
Foi o que trouxe A1 à luz (3 em 45 mil execuções em nível de token; 0 nas 73 mil byte a byte
e de `--instance`, e nas 461 válidas).

```c
#include <sanitizer/asan_interface.h>
static void *rz_alloc(keel_arena *a, size_t n, size_t size, size_t alignment) {
    if (size == 0 || n > (SIZE_MAX - 256) / size || alignment == 0 || (alignment & (alignment - 1))) return NULL;
    size_t bytes = n * size;
    void *raw = keel_arena_alloc_(a, 1, 16, 128 + 8 * alignment + bytes);
    if (!raw) return NULL;
    uintptr_t base = ((uintptr_t)raw + 64 + alignment - 1) & ~(uintptr_t)(alignment - 1);
    while (((base + bytes) & 7) != 0) base += alignment;   /* the end sits on an 8-byte granule */
    char *p = (char *)base;
    __asan_poison_memory_region(raw, (size_t)(p - (char *)raw));
    __asan_poison_memory_region(p + bytes, 64);
    return p;
}
```
`cgen_alloc` e `str_dup` chamam `rz_alloc` no lugar de `keel_arena_alloc_`. A granula alinhada
importa: sem ela, um estouro de 1 a 7 bytes passa entre as frestas da sombra.

## Apêndice C — Comandos

A partir da raiz do repositório; nada toca em `obj/`, `gen/` ou `dist/`. As fontes de teste
são geradas por um script Python em heredoc citado — uma linha de shell com aspas aninhadas
troca `\⏎` por outra coisa sem aviso, e foi assim que esta rodada mediu duas vezes o arquivo
errado.

```sh
W=$(mktemp -d); mkdir "$W/w"; cd "$W/w"
cat > gen.py <<'EOF'
import sys
where, kind, n, out = sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4]
sep = {'splice': chr(92) + chr(10), 'newline': chr(10)}[kind]
body = 'int x = 1' + sep*n + ' + 2;'
src = 'module owner;\n' + (body + '\n' if where == 'file' else 'pub inline i32 f(void) { ' + body + ' return x; }\n')
open(out, 'w').write(src)
EOF
BASE=$OLDPWD/base; CG=$OLDPWD/dist/bin/cgen
ms() { s=$(date +%s%N); "$@" >/dev/null 2>&1; echo "$(( ($(date +%s%N)-s)/1000000 )) ms"; }

# splices (A3): a mesma linha lógica com n continuações
for n in 2500 5000 10000 20000 40000; do python3 gen.py file splice $n owner.k; echo "$n $(ms $CG -I . --base-dir $BASE --stop-after=gen --dest-dir gen owner.k)"; done

# escala (A4): N declarações
for n in 1000 2000 4000 8000; do
  { printf 'module owner;\n'; awk -v n=$n 'BEGIN{for(i=0;i<n;i++) printf "pub inline i32 g%d(void) { i32 v = %d; return v + 1; }\n", i, i}'; } > owner.k
  echo "$n $(ms $CG -I . --base-dir $BASE --stop-after=gen --dest-dir gen owner.k)"
done

# uma função grande (A4b): E elementos, um por linha
E=1600; { printf 'module owner;\npub inline i32 f(void) {\n    int a[%d] = {\n' $E
  awk -v n=$E 'BEGIN{for(i=0;i<n;i++) printf "        %d%s\n", i, (i<n-1?",":"")}'; printf '    };\n    return a[0];\n}\n'; } > owner.k
echo "$E $(ms $CG -I . --base-dir $BASE --stop-after=gen --dest-dir gen owner.k)"

# protótipo multilinha (A6)
{ printf 'module owner;\npub inline i32 f(\n'; awk 'BEGIN{for(i=0;i<30;i++) printf "    i32 requested_token_count_%02d%s\n", i, (i<29?",":"")}'; printf ') { return a0; }\n'; } > owner.k
$CG -I . --base-dir $BASE --stop-after=gen --dest-dir gen owner.k; echo rc=$?

# warnings extras do cgen (conta 185; o build padrão conta 0)
ROOT=$OLDPWD; mkdir "$W/o"
(cd "$W/o" && gcc -std=c2x -I"$ROOT/tools/cgen/src" -I"$ROOT/tools/cgen/gen" -O1 -Wall -Wextra -Wshadow \
   -Wconversion -Wsign-conversion -Wformat=2 -Wstrict-prototypes -Wmissing-prototypes \
   -Wold-style-definition -Wvla -Wdouble-promotion -Wwrite-strings -Wundef -Wcast-qual \
   -c $(find "$ROOT/tools/cgen/src" -name '*.c') 2>&1 | grep -c 'warning:')

# perfil: quem domina A3, A4 e A4b
gcc -std=c2x -I"$ROOT/tools/cgen/src" -I"$ROOT/tools/cgen/gen" -O2 -g -DNDEBUG -o "$W/cgenp" $(find "$ROOT/tools/cgen/src" -name '*.c') -lm
valgrind --tool=callgrind --callgrind-out-file="$W/cg.out" --quiet "$W/cgenp" \
   -I . --base-dir "$BASE" --stop-after=gen --dest-dir gen owner.k >/dev/null 2>&1
callgrind_annotate --threshold=92 "$W/cg.out" | grep -A8 'Ir *file:function'
gcc -std=c2x -I"$ROOT/tools/cgen/src" -I"$ROOT/tools/cgen/gen" -O2 -pg -DNDEBUG -o "$W/cgpg" $(find "$ROOT/tools/cgen/src" -name '*.c') -lm
"$W/cgpg" -I . --base-dir "$BASE" --stop-after=gen --dest-dir gen owner.k >/dev/null 2>&1; gprof -b -p "$W/cgpg" gmon.out | sed -n '5,16p'

# o transform sob ASan (X1): caminho normal, opção sem valor, cabeçalho longo, fonte grande
gcc -std=c2x -O1 -g -Wall -Wextra -fsanitize=address,undefined -o "$W/transform" "$ROOT/tools/transform/src/main.c"
printf '/* demo */\n%%type\ntypedef int demo_t;\n%%h\nvoid demo_f(void);\n%%impl\nvoid demo_f(void) {}\n' > demo.k
"$W/transform" -m demo -i . -d out -t ""                      # caminho normal: rc=0
"$W/transform" -m; echo rc=$?                                 # opção sem valor: SIGSEGV
{ head -c 9000 /dev/zero | tr '\0' 'x' | sed 's/^/\/*/; s/$/*\//'; cat demo.k; } > big.k
"$W/transform" -m big -i . -d out -t ""; head -1 out/big.type.h   # começa em #ifndef: o cabeçalho sumiu
{ printf '%%type\n'; head -c 4000000 /dev/zero | tr '\0' ' '; printf '\n%%impl\nvoid huge_f(void) {}\n'; } > huge.k
ASAN_OPTIONS=detect_leaks=0 "$W/transform" -m huge -i . -d out -t "" 2>&1 | grep -m1 'ERROR: AddressSanitizer'
```
