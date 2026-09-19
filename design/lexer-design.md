# keel — desenho do lexer

> Documento de implementação. O contrato é
> [keel-spec.md, §2](../keel-spec.md#2-léxico-vocabulário-e-gramática).

## 1. Modelo

O `cgen` lê o arquivo para um `buffer char`. O parser recebe
`buffer.as_slice(fonte)` — a visão sai da memória, nunca o contrário
(linguagem §4.5) — e inicializa o lexer com essa slice. A ferramenta é dona do
armazenamento; parser e lexer recebem somente uma vista e não podem realocar,
gravar ou liberar o fonte.

O lexer trabalha sob demanda. Sua saída é um token por chamada e token é
somente uma vista do fonte:

```c
typedef keel_slice_char KToken;

typedef enum {
    TK_PP_OTHER,
    TK_PP_IF,     /* #if, #ifdef, #ifndef */
    TK_PP_ELSE,   /* #elif, #elifdef, #elifndef, #else */
    TK_PP_ENDIF   /* #endif */
} TKPpKind;

typedef struct {
    keel_slice_char source;
    keel_slice_cursor cursor;
    bool line_clean;
    KDiagnosticSink *diagnostics;
} KLexer;

KToken k_lexer_next(KLexer *lexer, TKPpKind *pp_kind);
/* slice vazia é EOF; pp_kind é TK_PP_OTHER fora de diretiva estrutural */
```

Não há enum de token, span separado, tabela de linhas ou `buffer KToken` no
lexer. A única classificação devolvida é `TKPpKind`, necessária ao balanço
condicional. Ele delimita tokens, salta trivia e emite diagnósticos léxicos.
Parser e consumidor estrutural qualificam a vista por sua grafia.
`slice.cursor` já é o cursor protocolar de `slice`, com `begin`, `has_next` e
`next`; o lexer usa `cursor.i` para lookahead e não cria outro cursor.

## 2. Vistas e preservação

O lexer começa com `lexer.cursor = keel_slice_char_begin(&fonte)`. Antes de
reconhecer uma token, guarda `first = cursor.i`; depois guarda
`last = cursor.i` e devolve:

```c
return keel_slice_char_from(fonte.ptr + first, last - first);
```

Em `ret\\\nurn` a token inclui a barra e a quebra físicas. Os comparadores
pulam a emenda e a reconhecem como `return`; o backend conserva os bytes ao
copiar C opaco. O `cgen` obtém o offset por `token.ptr - fonte.ptr` e é dono da
tabela de linhas, dos diagnósticos apresentados e de `#line`.

O contrato interno é somente de leitura. `keel_slice_char` usa `char *` no C
gerado, mas lexer, parser e backend não escrevem pelo ponteiro. Uma futura
instância `slice const char` só reforça esse contrato.

## 3. Reconhecedores

| Reconhecedor | Trabalho |
| --- | --- |
| `peek` / `take` | Consulta ou consome o próximo byte lógico. |
| `splice` | Reconhece `\\` seguido de `\n`, `\r\n` ou `\r`. |
| `skip_trivia` | Espaços, newlines e comentários. |
| `scan_directive` | Linha lógica iniciada por `#`. |
| `scan_identifier` | Identificador C. |
| `scan_number` | PP-número C, respeitando `..`. |
| `scan_quoted` | String ou char, com escapes. |
| `scan_punct` | Maior pontuador aplicável. |

`peek` usa uma cópia de `cursor.i` e salta emendas; `take` faz o mesmo antes de
avançar. A emenda precede comentário, literal e diretiva. Newline não emendada
torna `line_clean` verdadeira; espaço horizontal e comentário preservam o
estado, e uma token comum o torna falso.

## 4. Trivia e diretivas

`skip_trivia` consome espaço horizontal, `//` até newline lógica e `/* ... */`
até o primeiro `*/`. Comentários não produzem token. Comentário de bloco sem
fechamento vai até EOF e não contém reconhecimento keel; o C preservado dá o
diagnóstico.

Um `#` chama `scan_directive` somente com `line_clean == true`. A função
devolve uma única `KToken` até a próxima newline lógica não emendada, incluindo
a quebra física quando houver. Nada no corpo da diretiva entra na sequência
ordinária.

`scan_directive` reconhece as três classes estruturais abaixo e escreve a
classe no argumento `pp_kind` de `k_lexer_next`:

| Grafia | Classe |
| --- | --- |
| `#if`, `#ifdef`, `#ifndef` | `TK_PP_IF` |
| `#elif`, `#elifdef`, `#elifndef`, `#else` | `TK_PP_ELSE` |
| `#endif` | `TK_PP_ENDIF` |

Qualquer outra diretiva devolve `TK_PP_OTHER`. Seu corpo é sempre opaco. A
token preserva a linha inteira, então uma mensagem ainda pode distinguir a
grafia concreta, como `#ifdef`, sem acrescentar classes ao fluxo.

Em `define` e `undef`, o lexer lê o identificador alvo. Se começa por `keel_`
ou pertence à lista contextual, emite `define-sobre-keel`. Não examina nem
expande o corpo:

```c
static const char *const k_contextual_words[] = {
    "ALL", "ANY", "apply", "array", "as", "byref", "constexpr", "defer",
    "dim", "else", "extern_c", "fail", "foreach", "import", "import_c",
    "instance", "later", "match", "modifier", "module", "now", "parallel",
    "priv", "pub", "ref", "tags", "type", "types", "walk", "win"
};
```

Essa é a lista de palavras-chave contextuais do núcleo keel. Ela não cria outra
classe de token: salvo `constexpr` e `else`, que são palavras C, as grafias
continuam identificadores até a posição da gramática que lhes dá significado.

## 5. Predicados de classificação

O lexer delimita; parser e consumidor estrutural qualificam. Os predicados
comparam grafia lógica, ignorando emendas:

```c
bool k_token_is_ident(KToken t);
bool k_token_is_c_word(KToken t);
bool k_token_is_ident_named(KToken t, const char *name);
bool k_token_is_c_word_named(KToken t, const char *name);
bool k_token_is_number(KToken t);
bool k_token_is_string(KToken t);
bool k_token_is_char(KToken t);
bool k_token_is_punct(KToken t, const char *spelling);
```

`k_token_is_c_word` consulta a lista fechada da spec: é como o lexer filtra
palavras C antes que o parser aceite `IDENT`. A lista é a mesma em C11 e C23:

```c
static const char *const k_c_words[] = {
    "_Alignas", "_Alignof", "_Atomic", "_BitInt", "_Bool", "_Complex",
    "_Decimal128", "_Decimal32", "_Decimal64", "_Generic", "_Imaginary",
    "_Noreturn", "_Static_assert", "_Thread_local",
    "alignas", "alignof", "auto", "bool", "break", "case", "char", "const",
    "constexpr", "continue", "default", "do", "double", "else", "enum",
    "extern", "false", "float", "for", "goto", "if", "inline", "int", "long",
    "nullptr", "register", "restrict", "return", "short", "signed", "sizeof",
    "static", "static_assert", "struct", "switch", "thread_local", "true",
    "typedef", "typeof", "typeof_unqual", "union", "unsigned", "void",
    "volatile", "while"
};
```

As tabelas são ordenadas por `strcmp`. `k_token_is_ident` exige a forma C do
identificador — ASCII mais `\\u` e `\\U` válidos — e nega
`k_token_is_c_word`.

## 6. Números, literais e pontuação

`scan_number` reconhece pp-número C, iniciado por dígito ou `.` seguido de
dígito. Consome letras, dígitos, `_` e pontos; `+` e `-` só logo após `e`,
`E`, `p` ou `P`. Isso preserva C opaco: `1.5`, `0x1.fp3` e `1e-6` continuam
uma vista única, mesmo quando keel aceita apenas inteiro decimal na construção.

Encontrando `..`, o número termina antes do primeiro ponto. Fora de número,
`scan_punct` casa `...`, depois `..`, depois `.`. Portanto `2..7` produz
número, `..`, número; `1.5..3` faz o mesmo; `2...7` produz número, `...`,
número.

O parser converte para inteiro apenas em posições que exigem valor conhecido.
Em `buffer(3) T`, a token satisfaz `k_token_is_number` e o parser aceita a
grafia decimal `3`; em `buffer(N) Y`, `N` é identificador resolvido pela tabela
de símbolos.

`scan_quoted` reconhece prefixos `u8`, `u`, `U`, `L`, aspas e escapes, mas não
decodifica. Newline não emendada antes do fechamento emite
`literal-com-newline`; a recuperação termina a token antes da newline.
Literais adjacentes continuam tokens distintos.

`scan_punct` usa maior grafia primeiro. Não há enum: o parser pergunta por
`"->"`, `".."` ou `"("`. Grafia não reconhecida avança ao menos um byte e fica
preservada para o compilador C.

## 7. Parser e condicionais

O parser mantém apenas a lookahead necessária:

```c
typedef struct {
    KLexer lexer;
    KToken lookahead;
    TKPpKind lookahead_pp_kind;
    bool has_lookahead;
} KParser;
```

`take` devolve a lookahead e sua classe de diretiva, ou chama `k_lexer_next`.
O parser só usa `buffer KToken` para trecho que precise reter, como corpo de módulo genérico
ou recuperação com revisita. Esse buffer é estado do parser, não saída
obrigatória do lexer.

O backend preserva uma região entre duas tokens copiando de `first.ptr` até
`last.ptr + last.len`. Trivia intermediária permanece intacta. Região vazia
guarda seu ponteiro de fronteira.

O consumidor estrutural recebe tokens e `TKPpKind` de `take`. O lexer não
balanceia `()`, `[]` ou `{}`. Para abertura, o consumidor guarda a própria
`KToken` e o tipo deduzido do pontuador; para fechamento, compara o topo e
emite `delimitador-sem-par` quando necessário.

Em `TK_PP_IF`, salva a pilha de aberturas. Em `TK_PP_ELSE`, finaliza e
compara a alternativa corrente, restaura a pilha de entrada e inicia a
seguinte. Em `TK_PP_ENDIF`, finaliza a última alternativa, compara-a e aplica
uma única pilha comum. Sem `#else`, compara também com a pilha de entrada, a
alternativa vazia. Divergência emite `chaves-em-ramos`. Pilhas e snapshots
usam `buffer Open` e arena do parser, não o lexer.

## 8. Casos de aceitação

| Fonte | Resultado |
| --- | --- |
| `ret\\\nurn x;` | Uma token cuja grafia lógica é `return`. |
| `a/**/b` | Duas tokens; comentário é trivia. |
| `// x \\\nmodule y;` | `module` ainda está no comentário lógico. |
| `2..7 1.5..3 2...7` | Número, intervalo e elipse separados. |
| `u8"x\\\n y" 'z'` | Literal após emenda; char separado. |
| `"x\ny"` | `literal-com-newline` e recuperação na newline. |
| `/*x*/ # if 1` | Diretiva `if`, pois comentário é espaço. |
| `#define keel_x 1` | Diretiva preservada e `define-sobre-keel`. |
| `#undef constexpr` | O mesmo diagnóstico, embora seja palavra C. |
| `#define F(x) match(x)` | Sem diagnóstico; corpo da macro opaco. |
| `#if X ( #else [ #endif` | `chaves-em-ramos`. |

Testes de limite cobrem EOF após `\\`, `//`, `/*`, prefixo `u8` sem aspas,
literal sem fechamento, CRLF, CR, diretiva no EOF e NUL. Em todos, o cursor
avança ou termina; não pode ler fora de `source.len` nem alterar o fonte.
