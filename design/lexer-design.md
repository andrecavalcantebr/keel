# keel — Design e Plano de Implementação do Lexer

Este documento define o desenho e o plano de implementação do **lexer** de keel. O contrato normativo está em [keel-spec.md §2](../keel-spec.md#2-léxico-vocabulário-e-gramática); as justificativas estão em [keel-rationale.md](../keel-rationale.md). A preservação do fonte atende ao [backend C §6](../keel-c-backend.md#6-mapeamento-de-linhas), e a separação de E/S segue [cgen §3](../cgen-tool-spec.md#3-separação-de-es).

---

## 1. Responsabilidade e Escopo do Lexer

O lexer recebe um buffer de fonte `.k` e produz tokens classificados pela grafia, com os intervalos de origem necessários à emissão e aos diagnósticos. Não abre arquivos nem consulta imports ou tabelas de símbolos.

Suas responsabilidades são:

1. **Emenda de linhas (*line splicing*)**: processar `\` seguido imediatamente de quebra de linha antes de reconhecer comentários, literais, identificadores e diretivas.
2. **Espaços e comentários**: reconhecê-los como separadores, sem entregá-los como tokens sintáticos. Seus bytes permanecem disponíveis no fonte original para emissão.
3. **Classificação lexical**: produzir identificadores, palavras C da lista fechada, números, literais de string e caractere, pontuações, operadores e diretivas.
4. **Origem e grafia**: distinguir a grafia lógica, após emendas, do intervalo correspondente no fonte original.
5. **Diagnósticos léxicos**: detectar `literal-com-newline` e inspecionar o nome alvo de `#define` e `#undef` para `define-sobre-keel`, sem interpretar o corpo da macro.

As demais responsabilidades ficam separadas:

| Componente | Responsabilidade |
| --- | --- |
| Analisador estrutural | Consumir tokens e diretivas; balancear `()`, `[]`, `{}` por alternativa condicional; emitir `delimitador-sem-par` e `chaves-em-ramos`. |
| Parser | Reconhecer palavras contextuais, declarações e construções keel; consumir argumentos conforme a gramática e a aridade registrada. |
| Tabela de símbolos e resolução de nomes | Registrar e consultar tipos, modificadores, aliases, variáveis e demais símbolos, respeitando imports e escopos. |
| Backend | Aplicar substituições e copiar os intervalos preservados do fonte, com o mapeamento de linhas exigido. |

O analisador estrutural pode ser consumido durante o parsing; sua separação não exige uma passagem adicional sobre todo o arquivo. O scanner permanece independente das pilhas de delimitadores e dos símbolos.

---

## 2. Categorias de Tokens

### 2.1 Identificadores e Palavras Reservadas

- **`TOK_IDENT`**: segue a forma lexical de identificador C e exclui as palavras da lista fechada abaixo. `[a-zA-Z_][a-zA-Z0-9_]*` descreve apenas seu subconjunto ASCII, não uma restrição adicional da linguagem.
- **`TOK_C_KW`**: reúne todas as palavras C reconhecidas pela lista fechada de `keel-spec.md` §2.2:
  - *Tipos/Qualificadores*: `void`, `char`, `short`, `int`, `long`, `float`, `double`, `signed`, `unsigned`, `bool`, `const`, `volatile`, `restrict`, `inline`, `auto`, `register`, `extern`, `static`, `typedef`, `struct`, `union`, `enum`, `constexpr`, `typeof`, `typeof_unqual`, `thread_local`.
  - *Fluxo C*: `if`, `else`, `switch`, `case`, `default`, `for`, `while`, `do`, `break`, `continue`, `goto`, `return`.
  - *Operadores/Literais*: `sizeof`, `alignof`, `alignas`, `static_assert`, `true`, `false`, `nullptr`.
  - *Formas com sublinhado*: `_Alignas`, `_Alignof`, `_Atomic`, `_BitInt`, `_Bool`, `_Complex`, `_Decimal32`, `_Decimal64`, `_Decimal128`, `_Generic`, `_Imaginary`, `_Noreturn`, `_Static_assert`, `_Thread_local`.
- **Vocabulário contextual keel emitido como `TOK_IDENT`**:
  - `module`, `import`, `import_c`, `extern_c`, `as`, `types`, `pub`, `priv`
  - `array`, `ref`, `type`, `dim`, `modifier`, `instance`, `byref`
  - `defer`, `now`, `later`, `foreach`, `walk`, `apply`, `parallel`, `ALL`, `ANY`, `win`, `fail`
  - `tags`, `match`

`constexpr` e `else` pertencem à lista C e são emitidas como `TOK_C_KW`, inclusive quando exercem função keel. O parser determina essa função pela posição.

A lista C é a mesma nos perfis C11 e C23. Sua finalidade é distinguir as palavras que não casam `IDENT`; ela não valida o dialeto do C preservado. Assim, `module int;` não casa a produção de módulo, e `return xs;` não entra no padrão `IDENT IDENT` de possível redeclaração.

A grafia permite ao parser distinguir `return`, `static`, `const`, `struct` e as demais palavras quando a produção as nomeia. Não há um `TokenKind` por palavra. Uma função como `token_is_c_kw(token, "return")` basta inicialmente; um código auxiliar de grafia pode ser acrescentado se necessário, sem alterar as categorias.

**Tipos e nomes conhecidos não são categorias lexicais.** Não há `TOK_C_TYPE` nem tokens próprios para símbolos registrados:

| Grafia | Token | Interpretação pelo parser e pela resolução |
| --- | --- | --- |
| `char`, `bool`, `int` | `TOK_C_KW` | Aceitação conforme a grafia explicitamente prevista pela produção. |
| `i32`, `arena` | `TOK_IDENT` | Consulta de tipo registrado pelo prelúdio, import ou declaração. |
| `buffer` | `TOK_IDENT` | Consulta de modificador e sua aridade. |
| `Person`, `FILE` | `TOK_IDENT` | Consulta conforme o contexto; headers C não fornecem conhecimento de tipos ao keel. |
| `module`, `defer`, `match` | `TOK_IDENT` | Reconhecimento contextual, preservando a possibilidade de uso como identificador fora dessas posições. |

Por exemplo, `buffer struct Person pessoas;` produz `TOK_IDENT TOK_C_KW TOK_IDENT TOK_IDENT TOK_SEMICOLON`. O parser reconhece `buffer` e consome `struct Person` como argumento. Uma representação de tipo, como `TypeRef`, pertence ao resultado do parser e pode representar nomes qualificados e aplicações de modificadores sobre outros tipos.

### 2.2 Números

**`TOK_NUM`** representa o terminal `NUM` da gramática. O scanner delimita o token numérico segundo a tokenização C, com a separação adicional de `..`, preservando sua grafia completa. Não distingue `TOK_LIT_INT` de `TOK_LIT_FLOAT` nem converte o token em valor.

Exemplos: `42`, `1000ULL`, `0x1A2B`, `0755`, `0b101010`, `3.14`, `.5`, `1e-6`, `2.0f` e `0x1.fp3` são todos `TOK_NUM`. O consumo lexical de uma grafia não certifica sua validade como constante C. A validação dos sufixos e valores do C preservado cabe ao compilador C; quando keel exige um valor, a construção consumidora aplica as restrições do seu contrato.

**Regra dos pontos:** ao encontrar um ponto seguido de outro durante a leitura numérica, encerrar o número antes do primeiro ponto e deixar o scanner de pontuação aplicar a prioridade `...`, `..`, `.`. Portanto:

```text
2..7       → NUM DOTDOT NUM
1.5..3.0   → NUM DOTDOT NUM
2...7      → NUM ELLIPSIS NUM
```

`..` é uma pontuação; o lexer não agrupa os operandos em um token de intervalo.

### 2.3 Strings e Literais de Caractere

- Prefixos aceitos: nenhum (padrão), `u8`, `u`, `U`, `L`.
- Strings: `"..."` com sequências de escape (`\n`, `\t`, `\\`, `\"`, `\xHH`, `\uXXXX`, etc.).
- Caracteres: `'...'`.
- Strings produzem `TOK_LIT_STRING`; caracteres produzem `TOK_LIT_CHAR`. O prefixo faz parte do token, e literais adjacentes permanecem tokens separados.
- O scanner reconhece escapes para localizar o fechamento, preservando seu conteúdo sem decodificá-lo. A validação semântica dos escapes fica com o compilador C.
- Restrição: qualquer newline restante dentro do literal, após a etapa de emenda, gera `literal-com-newline`.

### 2.4 Pontuação e Operadores

- Delimitadores de balanceamento: `(`, `)`, `[`, `]`, `{` e `}`.
- Pontuação com ponto (ordem de prioridade no casamento):
  1. `...` (`TOK_ELLIPSIS`)
  2. `..`  (`TOK_DOTDOT` - pontuação usada nos intervalos)
  3. `.`   (`TOK_DOT`)
- Demais operadores e pontuações C: `,`, `;`, `:`, `?`, `->`, `+`, `-`, `*`, `/`, `%`, `&`, `|`, `^`, `~`, `!`, `<`, `>`, `=`, `+=`, `-=`, `==`, `!=`, `<=`, `>=`, `&&`, `||`, `<<`, `>>`, etc.

### 2.5 Diretivas de Pré-processador

Reconhecidas quando `#` aparece em posição de diretiva C, após emendas e levando em conta espaços e comentários anteriores. Não basta testar a coluna física nem apenas espaços e tabs. A diretiva forma um único token até o newline não emendado, preservado como parte da unidade quando existente; em EOF, termina no fim do buffer.

- Diretivas estruturais: `#if`, `#ifdef`, `#ifndef`, `#elif`, `#elifdef`, `#elifndef`, `#else`, `#endif`.
- Diretivas de macro inspecionadas: `#define`, `#undef` (inspeciona o identificador seguinte para detectar violação de `keel_` ou palavra contextual).
- Outras diretivas opacas: `#include`, `#pragma`, `#embed`, `#line`, etc.

A inspeção de `#define` e `#undef` compara a grafia do alvo com todo o vocabulário contextual, incluindo `constexpr` e `else`, e com o prefixo `keel_`. Essa tabela de verificação não transforma as palavras keel em keywords lexicais. Comentários entre `#`, o nome da diretiva e seu alvo são tratados como separadores. O corpo da diretiva não fornece tokens de delimitadores ao analisador estrutural.

---

## 3. Algoritmo de Balanceamento de Delimitadores e Grupos `#if`

Este algoritmo pertence ao **analisador estrutural**, consumidor dos tokens do lexer. Conforme `keel-spec.md` §2.4, cada alternativa produz uma variação de delimitadores, e todas devem concordar. Não é necessário que cada alternativa feche tudo o que abriu.

### 3.1 Estruturas de Dados do Balanceamento

Esboço das estruturas; `SourceLocation` é definido na §4. As sequências têm armazenamento dinâmico.

```c
typedef enum {
    DELIM_PAREN,   // ()
    DELIM_BRACKET, // []
    DELIM_BRACE    // {}
} DelimKind;

typedef struct {
    DelimKind kind;
    SourceLocation loc; // origem do delimitador registrado
} DelimEntry;

typedef struct {
    DelimEntry *items;
    size_t count;
    size_t capacity;
} DelimSequence;

// Delta estrutural produzido por uma alternativa ou bloco
typedef struct {
    // Sequência de fechamentos que consumiram delimitadores anteriores ao ramo
    DelimSequence closed_before;

    // Delimitadores que foram abertos dentro do ramo e permaneceram abertos
    DelimSequence opened;
} BranchDelta;

// Estado de um grupo condicional aninhado (#if ... #endif)
typedef struct CondGroup {
    SourceLocation if_loc;
    DelimSequence entry_stack; // snapshot completo, não apenas a profundidade
    BranchDelta current_alt_delta;
    BranchDelta first_alt_delta;
    bool has_first_alt;
    bool has_else;
    struct CondGroup *parent;
} CondGroup;
```

A comparação dos deltas usa a quantidade, a ordem e os tipos dos delimitadores, nunca suas posições de origem. As posições são preservadas para diagnósticos. Um snapshot precisa permitir restaurar também aberturas anteriores que a alternativa tenha consumido; guardar apenas o tamanho da pilha não basta.

### 3.2 Regras do Balanceamento Condicional

1. **Início de grupo (`#if`, `#ifdef`, `#ifndef`)**:
   - Cria um novo `CondGroup` empilhado no contexto atual.
   - Salva a base da pilha de delimitadores.
2. **Nova alternativa (`#elif`, `#elifdef`, `#elifndef`, `#else`)**:
   - Finaliza a alternativa corrente calculando o delta `(fechou_anteriores, abriu_novos)`.
   - Se for a primeira alternativa examinada, salva como `first_alt_delta`.
   - Se for uma alternativa subsequente, compara com `first_alt_delta`. Se divergirem, emite o erro `chaves-em-ramos`.
   - Restaura o contexto de delimitadores para a base do início do grupo e inicia nova medição.
   - Se a diretiva for `#else`, marca `has_else = true`.
3. **Fim de grupo (`#endif`)**:
   - Finaliza a última alternativa e valida o delta com `first_alt_delta`.
   - Se não houver `#else` explícito, valida contra a alternativa vazia (delta zero). Um efeito estrutural não nulo produz `chaves-em-ramos`; pares inteiramente abertos e fechados dentro da alternativa são permitidos.
   - Desempilha o `CondGroup` e aplica o delta consensual exatamente uma vez à pilha de delimitadores pai.
4. **Delimitadores regulares fora de diretivas**:
   - Ao abrir `(`, `[`, `{`: empilha no rastreador do escopo ativo.
   - Ao fechar `)`, `]`, `}`: verifica se corresponde ao topo da pilha corrente, incluindo o contexto de entrada da alternativa. Se não corresponder ou a pilha estiver vazia, emite `delimitador-sem-par`, inclusive dentro de ramo condicional.
   - Fechamentos válidos de aberturas anteriores ao ramo são registrados no delta. No EOF, aberturas restantes produzem `delimitador-sem-par`.

O balanceamento permanece ativo em `extern_c`. Comentários, literais e conteúdo de diretivas não participam. A pilha condicional também precisa detectar diretivas de alternativa ou fechamento sem grupo, alternativas após `#else` e grupos não encerrados; a recuperação desses erros não pode aplicar um delta como se houvesse consenso.

---

## 4. Representação e Tipos de Dados em C

### 4.1 Fonte Original e Visão Lógica

O buffer original é imutável e pertence à unidade de fonte carregada pela ferramenta. Uma visão lógica contígua é produzida para reconhecimento, normalizando `\r\n` para `\n` e removendo as emendas, com um mapa de origem que permita recuperar os intervalos físicos. Essas operações não alteram o buffer original.

Comentários e espaços permanecem nessa visão e são saltados pelo scanner como separadores. Assim, `a/**/b` produz dois identificadores. Em contraste, `ret\` seguido de newline e `urn` forma a grafia lógica `return`, classificada como `TOK_C_KW`.

Cada token carrega a grafia lógica e um intervalo `[begin, end)` no fonte original. O comprimento lógico pode diferir de `end - begin`. A linha e a coluna de diagnóstico vêm da origem física; a posição de diretiva é determinada pelo fluxo lógico.

O backend copia regiões do buffer original, incluindo os espaços e comentários entre tokens. Não reconstrói C opaco concatenando lexemas. Os buffers e o mapa de origem devem permanecer vivos enquanto houver tokens ou artefatos que os referenciem, inclusive os tokens retidos para módulos genéricos.

### 4.2 Token

Esboço em C; os operadores não enumerados seguem a mesma representação por `TokenKind`.

```c
typedef struct {
    uint32_t line;
    uint32_t col;
    const char *file;
} SourceLocation;

typedef struct {
    size_t begin; // offset em bytes no buffer original
    size_t end;   // limite exclusivo no mesmo buffer
} SourceSpan;

typedef enum {
    // Especiais
    TOK_EOF,
    TOK_INVALID,

    // Classificação exclusivamente pela grafia
    TOK_IDENT,
    TOK_C_KW,

    // Números e literais
    TOK_NUM,
    TOK_LIT_CHAR,
    TOK_LIT_STRING,

    // Delimitadores
    TOK_LPAREN,   // (
    TOK_RPAREN,   // )
    TOK_LBRACKET, // [
    TOK_RBRACKET, // ]
    TOK_LBRACE,   // {
    TOK_RBRACE,   // }

    // Pontuações
    TOK_DOT,      // .
    TOK_DOTDOT,   // .. (pontuação)
    TOK_ELLIPSIS, // ...
    TOK_COMMA,    // ,
    TOK_SEMICOLON,// ;
    TOK_COLON,    // :

    // Operadores
    TOK_ASSIGN,   // =
    TOK_PLUS,     // +
    TOK_MINUS,    // -
    TOK_STAR,     // *
    TOK_SLASH,    // /
    TOK_PERCENT,  // %
    // ... demais operadores C

    // Diretivas (para passar verbatim ou guiar balanceamento)
    TOK_PP_IF,
    TOK_PP_IFDEF,
    TOK_PP_IFNDEF,
    TOK_PP_ELIF,
    TOK_PP_ELIFDEF,
    TOK_PP_ELIFNDEF,
    TOK_PP_ELSE,
    TOK_PP_ENDIF,
    TOK_PP_DEFINE,
    TOK_PP_UNDEF,
    TOK_PP_OPAQUE_DIRECTIVE
} TokenKind;

typedef struct {
    TokenKind kind;
    SourceLocation loc;
    SourceSpan source;
    const char *lexeme; // ponteiro para a grafia na visão lógica
    size_t length;     // comprimento lógico em bytes; não exige NUL após o token
} Token;

bool token_is_c_kw(const Token *token, const char *word);
bool token_is_ident(const Token *token, const char *word);
```

As funções conferem a categoria e comparam a grafia usando seu comprimento. Não consultam símbolos. `TOK_EOF` tem comprimento zero e intervalo vazio no fim do fonte. A política de recuperação de `TOK_INVALID` deve garantir avanço do scanner e preservar o intervalo diagnosticado.

---

## 5. Plano de Implementação Passo a Passo

### Fase 1: Fundação e Pré-processamento Léxico (Buffer & Line Splicing)

- Definir a entrada como buffer e comprimento fornecidos pela ferramenta; leitura de arquivo fica fora do lexer.
- Manter o buffer original imutável, construir a visão lógica e seu mapa de origem.
- Normalizar quebras e remover emendas antes de qualquer reconhecimento, preservando a localização física para diagnósticos.
- Definir o tempo de vida dos buffers e o acesso aos intervalos originais para a emissão.

### Fase 2: Scanner Básico (Whitespace, Comentários, Operadores)

- Reconhecer espaços em branco e comentários como separadores, sem removê-los do material disponível para emissão.
- Saltar comentários:
  - Linha: `//` até `\n`.
  - Bloco: `/*` até `*/` (com erro caso atinja EOF sem fechar).
- Reconhecimento dos operadores e pontuações de múltiplos caracteres, com atenção especial para:
  - `...`, `..` e `.`.
  - Operadores compostos (`==`, `!=`, `<=`, `>=`, `->`, `<<`, `>>`, etc.).

### Fase 3: Literais e Identificadores

- **Números**:
  - Delimitar a grafia numérica e emitir `TOK_NUM`, sem conversão de valor ou validação completa de constantes C.
  - Separar a pontuação `..` e respeitar a prioridade de `...`.
- **Strings e Chars**:
  - Detecção de prefixos (`u8`, `L`, etc.).
  - Localizar o fechamento respeitando escapes, sem decodificar o conteúdo.
  - Verificar `literal-com-newline` ao encontrar newline restante após emendas.
- **Identificadores e Keywords**:
  - Implementar a tabela fechada de palavras C e emitir `TOK_C_KW` ou `TOK_IDENT`.
  - Implementar comparação por categoria e grafia para uso pelo parser.
  - Manter palavras contextuais e nomes de tipos como identificadores, ressalvadas `constexpr` e `else`.

### Fase 4: Diretivas de Pré-processador e Verificações Léxicas Especiais

- Detectar `#` em posição de diretiva C, considerando emendas, espaços e comentários.
- Emitir cada diretiva como unidade com grafia e intervalo original preservados.
- Classificar diretivas de grupo (`#if`, `#ifdef`, etc.).
- Implementar a regra de `define-sobre-keel`:
  - Se `#define` ou `#undef`, ler a grafia do nome alvo, inclusive quando pertence à lista C.
  - Emitir erro caso o nome pertença ao vocabulário contextual de Keel ou inicie com `keel_`.

### Fase 5: Motor de Balanceamento de Delimitadores e Ramos Condicionais

- Implementar o analisador estrutural como consumidor independente do scanner.
- Implementar a pilha de delimitadores regulares `()`, `[]`, `{}` e snapshots restauráveis do contexto de entrada.
- Implementar a pilha de `CondGroup` com medição de deltas estruturais entre ramos.
- Testar diagnósticos:
  - `delimitador-sem-par`
  - `chaves-em-ramos`

### Fase 6: Verificação dos Contratos

- **Lexer**: conferir classificação da lista C, palavras contextuais como identificadores, exceções `constexpr`/`else` e ausência de dependência de imports ou perfil.
- **Números e pontos**: conferir `0..10`, `1.5..3.0`, `2...7`, `.5`, expoentes e preservação da grafia numérica sem conversão.
- **Origem e preservação**: conferir reconstrução das regiões copiadas com comentários, espaços, CRLF, emendas no meio de identificadores e comentários de linha continuados por emenda.
- **Literais e diretivas**: conferir prefixos, escapes, newline em literal, diretivas precedidas por comentários e ausência de efeitos estruturais dos delimitadores contidos nessas unidades.
- **Nomes de macros**: conferir `#define keel_buffer_int foo`, `#undef defer`, `#define constexpr 1` e macros permitidas cujo corpo contém palavras keel.
- **Analisador estrutural**: conferir o exemplo MIPS da especificação, grupos aninhados, alternativas que consomem aberturas anteriores, deltas divergentes e incompatibilidade de tipos ou ordem dos delimitadores.
- **Alternativa vazia**: aceitar pares inteiramente internos a um `#ifdef` sem `#else` e recusar efeitos estruturais não nulos.
- **Integração futura com o parser**: conferir rejeição de `module int;`, distinção de `return xs;` em relação ao padrão de redeclaração e resolução de `buffer struct Person` a partir dos símbolos.
- Usar os fontes de `golden/casos/` como corpus complementar. Comparar o C esperado desses casos exige o parser e o backend; não é um teste unitário do lexer.
