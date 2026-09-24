# keel — Pré-processador de C - Especificação

> © 2026 Faculdade de Engenharia Elétrica e de Computação, Universidade
> Federal do Amazonas (FEEC/UFAM). Licenciado sob
> [CC BY-SA 4.0](LICENSE-DOCS.md) ([tradução](LICENSE-DOCS.pt.md)) — este
> documento é prosa sobre a linguagem, não código; o `cgen` e a Base keel têm
> licença própria, ver [`LICENSE.md`](LICENSE.md). Escrita e revisão tiveram
> auxílio de Claude Opus e Claude Sonnet (Anthropic), sob direção humana.
>
> **Licença do código descrito aqui.** `cgen` copia trechos de fonte da Base
> keel para dentro do C gerado de todo projeto — não é vínculo de biblioteca,
> é fonte colado. Por isso a Base é distribuída sob GPLv3 com uma exceção que
> isenta esse texto gerado, para que o programa do usuário não seja arrastado
> ao copyleft por transitividade de transpilação. Ver
> [rationale](keel-rationale.md#por-que-a-base-é-copyleft-com-exceção-e-não-gpl-simples-nem-mit)
> para o argumento. Esta nota não é normativa.

Este documento especifica a sintaxe, o reconhecimento, as transformações e as restrições do PPC keel. As decisões e justificativas pertencem ao [rationale](keel-rationale.md); a representação e a emissão formal de C pertencem ao [keel-c-backend.md](keel-c-backend.md); a interface de linha de comando pertence à ferramenta [cgen-tool-spec.md](cgen-tool-spec.md).

**Convenções de apresentação:**

- Os pares keel/C mostram a semântica essencial da tradução. O C apresentado é uma representação lógica próxima da saída, sem compromisso com sua grafia exata; detalhes como `#line` e `[[nodiscard]]` podem ser omitidos.
- A especificação da geração, inclusive diferenças entre perfis, pertence ao backend. Os exemplos desta especificação não a substituem. [Justificativa: perfis e exemplos de tradução](keel-rationale.md#perfis-e-exemplos-de-tradução).
- Os contratos dos capítulos 4 e 5 seguem o formato abaixo, em frases curtas. A justificativa fica no rationale, ligada por link.

1. **Sintaxe:** formas aceitas, contexto e escopo. Nos módulos da base, a tabela de verbos, com o que cada um devolve. Depois, o **reconhecimento**: as condições que distinguem a construção de C opaco.
2. **Regras:** uma linha de uso e as regras numeradas, uma afirmação por regra, incluindo avaliação, efeitos e saídas. Termina com as referências ao rationale e ao backend.
3. **Exemplo:** par keel/C, quando necessário para fixar o significado.
4. **Erros:** os identificadores de diagnóstico; condição, severidade e responsável estão no catálogo (§6.2).
5. **Casos especiais:** casos de borda e limites da análise. As obrigações que keel não verifica começam por "O programa garante…".

## 1. Escopo e princípios

keel é um pré-processador de C (PPC) que oferece sintaxe própria para expressar
operações que, em C, seriam organizadas por macros, sem exigir truques de
expansão. Essa sintaxe é a linguagem de uso do PPC: descreve transformações em
**C11** ou **C23**, conforme o perfil, mantendo os tipos e sua validação no C.
keel processa o fonte antes do pré-processador C; o código emitido é compilado
pelo compilador C do projeto. [Justificativa: macros e sintaxe do PPC](keel-rationale.md#macros-e-sintaxe-do-ppc).

keel é um parser de ilhas em um mar de C. keel reconhece suas construções e seus símbolos no código-fonte, inclusive quando aparecem dentro de expressões. Não realiza análise semântica de expressões C puras nem resolve o sistema de tipos do C.

Um modificador atua sobre o tipo que o segue e define sua representação e suas
operações. Em `buffer struct Person people;`, `buffer` modifica `struct Person`
para representar uma sequência desse tipo com seus metadados de controle.
`outcome T` associa ao valor de `T` um código de resultado; `tagged E T`
associa a ele uma etiqueta de um conjunto declarado. O PPC expande essas declarações para
tipos e funções C. Neste documento, instanciação designa essa expansão para os
argumentos fornecidos. [Justificativa: modificador e tipo modificado](keel-rationale.md#modificador-e-tipo-modificado).

### 1.1 Princípios de projeto

1. **Saída em C e substituição localizada.** O código gerado não contém construções keel. Expressões C são copiadas verbatim, exceto pelas substituições de elementos keel reconhecidos dentro delas. A integração pode exigir restrições léxicas explícitas no fonte keel. [Justificativa: preservação e fronteira com C](keel-rationale.md#fronteira-com-c-e-conflitos-léxicos).
2. **Tradução previsível.** Cada construção deve ter uma tradução para C especificada e legível. Custos e dependências introduzidos pela tradução devem ser documentados. [Justificativa: previsibilidade do C gerado](keel-rationale.md#quem-escreve-c-pensa-em-máquina-quem-escreve-keel-pensa-em-c).
3. **Validação pelo compilador C.** A compatibilidade de tipos e a validade semântica das expressões C são verificadas pelo compilador C. keel verifica as regras próprias de suas construções. [Justificativa: o compilador C como verificador final](keel-rationale.md#c-usado-como-o-assembly-portável).
4. **Identidade nominal.** A identidade nominal representada pelos nomes C emitidos deriva das declarações de origem e dos argumentos canônicos. Semelhança de layout não estabelece identidade. [Justificativa: identidade e nomes canônicos](keel-rationale.md#por-que-o-nome-canônico-é-fixado-na-declaração).
5. **Reconhecimento determinístico.** A tradução depende de regras explícitas de reconhecimento e dos símbolos registrados. Uma construção keel que não satisfaça essas regras deve ser diagnosticada, sem inferir a intenção do programa a partir de tipos ou expressões C desconhecidos. [Justificativa: reconhecimento e conflitos léxicos](keel-rationale.md#fronteira-com-c-e-conflitos-léxicos).
6. **Decisões de dados e execução explícitas.** Layout, tempo de vida do armazenamento e travessia devem ser definidos pelo programa ou pelo contrato da construção ou biblioteca utilizada. Não são deduzidos de código C opaco. [Justificativa: separação entre capacidade e armazenamento](keel-rationale.md#o-que-o-vla-errou).
7. **Análise limitada.** O reconhecimento e as verificações de keel obedecem ao contrato da §1.3. Recurso que exija análise semântica de C puro fica fora do escopo do PPC. [Justificativa: limites da análise](keel-rationale.md#fronteira-com-c-e-conflitos-léxicos).
8. **Núcleo mínimo.** Funcionalidade expressável por um módulo keel deve ser implementada como biblioteca. Privilégios que exijam participação do PPC devem ser enumerados na especificação. [Justificativa: critério de participação do núcleo](keel-rationale.md#onde-keel-concorda-com-o-openmp).
9. **Construções com contrato próprio.** Uma construção deve expressar uma operação ou garantia que C não oferece diretamente ou expressa de modo inadequado ao modelo de keel. Redução de tokens, isoladamente, não é critério de admissão. [Justificativa: critério de admissão de construções](keel-rationale.md#a-motivação-em-uma-frase).

### 1.2 Modelo de tradução e perfis

```text
fonte.k → cgen → fonte.c + fonte.h → pré-processador C → compilador C → executável
```

- Os perfis de geração são **C11** e **C23**. Sua seleção pertence à ferramenta; as regras de emissão de cada perfil pertencem ao backend.
- O perfil seleciona a representação C das construções keel. Diferenças de diagnóstico, requisitos do alvo e exceções à conformidade estrita devem ser documentados explicitamente. [Justificativa: perfis de geração](keel-rationale.md#perfis-e-exemplos-de-tradução).
- Trechos C preservados permanecem sujeitos às regras do dialeto selecionado para o compilador C. O perfil não implica conversão geral de código C23 escrito pelo usuário para C11.
- **O dialeto do C escrito pelo programa é assunto do build, e keel não o observa.** Um módulo pode usar `nullptr`, `typeof` ou qualquer outra forma do C23 no meio do código que atravessa; keel não sabe que aquilo aconteceu, porque reconhece as próprias construções e copia o resto. Gerar sob o perfil C11 um módulo que usa palavra do C23 é erro do programa, relatado pelo compilador C com a mensagem dele. Vigiar palavra-chave por dialeto tornaria o PPC dependente da versão do C — exatamente o que o reconhecimento por ilhas evita.
- Diretivas de pré-processamento são preservadas. keel pode reconhecer sua estrutura para aplicar regras próprias, mas não expande macros nem avalia condições de compilação.
- Construções e símbolos que keel precise reconhecer devem estar presentes antes da expansão de macros; não podem depender dessa expansão para adquirir sua forma ou identidade.

### 1.3 Contrato de análise

**keel pode realizar:**

- Tokenização e varredura de sequências de tokens.
- Reconhecimento de delimitadores, declarações, escopos e pontos de saída necessários à tradução de suas construções.
- Registro e consulta de símbolos definidos em keel ou disponibilizados por módulos keel, com as informações explícitas de suas declarações.
- Reconhecimento de construções keel dentro de expressões, incluindo operações sobre símbolos keel conhecidos.
- Verificações léxicas sobre esses símbolos, conforme o conjunto de formas especificado para cada construção.
- Reescrita das construções reconhecidas e emissão do código correspondente.

**keel não realiza:**

- Análise semântica, avaliação ou inferência de tipos de expressões C puras.
- Resolução de tipos C a partir de headers, inclusive da biblioteca padrão.
- Dedução do tipo de uma chamada C desconhecida, de um cast ou de uma expressão C arbitrária para decidir uma tradução.

Conhecer o tipo declarado de um símbolo keel, ou o retorno declarado de uma função keel, permite consultar essa informação. Isso não autoriza deduzir o tipo da expressão C que envolva o símbolo ou a chamada.

Uma região sem análise semântica de C ainda pode conter construções keel. Essas construções são reconhecidas e traduzidas; os tokens C restantes são preservados. Pelo princípio 3 acima, cabe ao compilador C a análise e definição final da expressão. A ausência de análise semântica não implica ausência de varredura. [Justificativa: fronteira de análise](keel-rationale.md#fronteira-com-c-e-conflitos-léxicos).

### 1.4 Compatibilidade e conflitos léxicos

- keel utiliza as características léxicas de C para localizar suas próprias construções e gerar o C correspondente, nos limites da §1.3.
- As palavras-chaves de keel são contextuais, conforme as posições definidas pela gramática. Nomes e formas reservados devem ser especificados explicitamente.
- Código C preexistente pode conflitar com essas posições, nomes ou formas. Nesses casos, keel pode exigir adaptação do fonte ou emitir um diagnóstico.
- Grupos `#if`/`#ifdef`/`#ifndef` estão sujeitos às restrições estruturais necessárias ao reconhecimento de keel, sem avaliação de suas condições.
- Não há garantia geral de aceitação de todo programa C preexistente nem de estabilidade irrestrita ao acrescentar imports. As garantias de nomes e imports dependem das respectivas regras de reconhecimento e conflito. [Justificativa: conflitos léxicos](keel-rationale.md#fronteira-com-c-e-conflitos-léxicos).

## 2. Léxico, vocabulário e gramática

### 2.1 Finalidade

Definir a formação dos tokens e as condições sintáticas e simbólicas usadas para reconhecer construções keel dentro do fonte C.

### 2.2 Sintaxe

#### Convenções da gramática

A gramática descreve as construções keel e a estrutura C necessária para localizá-las. Não descreve a gramática completa de C. Os contratos das construções pertencem à §4.

| Notação | Significado |
| --- | --- |
| `::=` | Define uma produção. |
| `\|` | Separa alternativas. |
| `{ … }` | Repete zero ou mais vezes. |
| `[ … ]` | Indica parte opcional. |
| `( … )` | Agrupa alternativas. |
| `'…'` | Indica a grafia de um terminal. |
| `IDENT` | Identificador que não pertence ao conjunto de palavras C desta seção. |
| `NUM` | Token numérico; a construção consumidora restringe sua forma e seu valor. |
| `STRING` | Literal de string, incluindo seu prefixo quando houver. |
| `<opaque>` | Sequência de tokens delimitada pelo contexto, sem análise semântica de C. |
| `<opaque-no-parens>` | Região opaca sem `(` no nível externo da região. |

“Nível externo” ou “de topo” refere-se à profundidade de delimitadores da produção em análise, não necessariamente ao escopo de arquivo. Separadores dentro de parênteses, colchetes, chaves, literais ou diretivas não encerram uma região externa.

Uma região `<opaque>` pode conter construções keel. Essas construções são reconhecidas e traduzidas; os demais trechos são preservados. O corpo de `extern_c`, os literais e o conteúdo das diretivas têm as regras específicas da §2.3. [Justificativa: papel da região opaca](keel-rationale.md#por-que-opaque-é-o-terminal-central).

#### Elementos léxicos

| Elemento | Regra |
| --- | --- |
| Emenda de linha | Uma barra invertida seguida imediatamente de newline une as linhas antes da tokenização. |
| Espaços e comentários | Separam tokens e não participam das produções. São reconhecidos comentários de múltiplas linhas: `/* … */` e comentários de uma linha: `//` até o fim da linha lógica. Seguem as regras de comentários de C. Seu conteúdo é ignorado, mesmo contendo construções keel ou construções C. |
| Identificadores | Seguem a forma lexical de identificador C. A classificação como palavra C usa a lista fechada abaixo; o reconhecimento de tipos e palavras contextuais cabe ao parser. |
| Literais | Strings e caracteres são reconhecidos com escapes e com os prefixos `L`, `u8`, `u` e `U`. O conteúdo é indivisível para o reconhecimento de keel. Um newline não emendado dentro do literal produz o diagnóstico `literal-with-newline`. |
| Números e pontos | Seguem a tokenização numérica de C, com a separação adicional da pontuação `..`: `2..7` produz `NUM`, `'..'`, `NUM`. Entre as pontuações iniciadas por ponto, a prioridade é `'...'`, depois `'..'`, depois `'.'`. |
| Delimitadores | `()`, `[]` e `{}` são emparelhados sobre os tokens, respeitando as alternativas de pré-processamento (§2.4). Delimitadores dentro de comentários, literais e diretivas não entram no balanceamento. |
| Diretivas | A diretiva iniciada por `#` em posição de diretiva C estende-se até o newline não emendado. Sua linha lógica forma uma unidade preservada na saída. |

`..` é pontuação, não um token que englobe o intervalo. Seus operandos são reconhecidos pelo parser: `xs[(a + 1)..(b * 2)]` contém expressões e delimitadores próprios. `...` permanece uma pontuação distinta. [Justificativa: pontuação e intervalo](keel-rationale.md#por-que--não-é-um-token).

As classes de diretiva usadas no reconhecimento estrutural são:

| Classe | Diretivas |
| --- | --- |
| Abertura de grupo | `#if`, `#ifdef`, `#ifndef` |
| Nova alternativa | `#elif`, `#elifdef`, `#elifndef`, `#else` |
| Fechamento de grupo | `#endif` |
| Demais diretivas | Incluem `#define`, `#undef`, `#include`, `#pragma` e `#embed`. |

A classificação não depende da condição escrita nem de sua validade no perfil C selecionado. A verificação de nomes em `#define` e `#undef` é descrita na §2.5.

#### Vocabulário do núcleo

As palavras abaixo têm função keel nas posições indicadas pela gramática. Fora dessas posições, aplica-se a classificação C ou de identificador, com as restrições de nomes da §2.5.

| Grupo | Palavras |
| --- | --- |
| Unidade e visibilidade | `module`, `import`, `import_c`, `extern_c`, `as`, `types`, `pub`, `priv` |
| Tipos, marcadores e genéricos | `array`, `constexpr`, `ref`, `type`, `dim`, `modifier`, `instance`, `byref`, `extent` |
| Fluxo | `defer`, `now`, `later`, `foreach`, `walk`, `apply`, `parallel`, `ALL`, `ANY`, `win`, `fail` |
| Valores etiquetados | `tags`, `match` |
| Tratamento de resultado | `else` na cauda de declaração |

`constexpr` e `else` também pertencem à lista de palavras C. Sua função keel é determinada pela posição; não exige reclassificação lexical. As produções também usam palavras C como `auto`, `static` e `const`.

Nomes de tipos, modificadores e módulos da base, como `arena`, `buffer`, `slice`, `tagged`, `outcome`, `corot` e `routine`, não são palavras do núcleo. `parallel` é o único nome que ocupa as duas posições: palavra do núcleo em `parallel nome politica (…)`, e alias do módulo `keel.parallel` em `parallel.ok(nome)`, distinguidos pelo `.` da §2.3. Nomes de operações, como `length`, `get` e `alloc`, também não são palavras-chave. Sua resolução depende dos símbolos disponíveis e do contrato da operação. [Justificativa: prelúdio e base](keel-rationale.md#prelúdio-e-base-mínima).

As formas `x[i]`, `x[i,j]`, `x[a..b]`, `x[a..]`, `x[..b]`, `x[..]` e `a..b` são sintaxe, não vocabulário. A indexação e os `range-index` dependem do símbolo reconhecido e das regras da §4.5; sua grafia, isoladamente, não define as permissões de acesso ou mutação.

#### Palavras C reconhecidas

O conjunto abaixo é fechado e pertence ao reconhecimento de keel. É o mesmo nos perfis C11 e C23. Pertencer a esta lista não garante que uma grafia seja aceita pelo compilador C no perfil selecionado.

| Grupo | Palavras |
| --- | --- |
| Tipos, qualificadores e declarações | `void`, `char`, `short`, `int`, `long`, `float`, `double`, `signed`, `unsigned`, `bool`, `const`, `volatile`, `restrict`, `inline`, `auto`, `register`, `extern`, `static`, `typedef`, `struct`, `union`, `enum`, `constexpr`, `typeof`, `typeof_unqual`, `thread_local` |
| Fluxo | `if`, `else`, `switch`, `case`, `default`, `for`, `while`, `do`, `break`, `continue`, `goto`, `return` |
| Operadores e literais | `sizeof`, `alignof`, `alignas`, `static_assert`, `true`, `false`, `nullptr` |
| Formas com sublinhado | `_Alignas`, `_Alignof`, `_Atomic`, `_BitInt`, `_Bool`, `_Complex`, `_Decimal32`, `_Decimal64`, `_Decimal128`, `_Generic`, `_Imaginary`, `_Noreturn`, `_Static_assert`, `_Thread_local` |

Essas palavras não casam `IDENT`. Uma produção as aceita quando nomeia sua grafia; as demais ocorrências integram regiões C opacas. Assim, `unsigned long x` não é uma sequência de três identificadores, e `char` e `bool` precisam de alternativas explícitas quando aceitos como argumentos de tipo.

#### Unidade e declarações de topo

```ebnf
unit      ::= decl-module { top-item }

decl-module  ::= 'module' module-name [ dim-binder ] [ tags-binder ]
                 [ type-binder ] ';'
module-name  ::= IDENT { '.' IDENT }
dim-binder   ::= 'dim' IDENT { ',' IDENT }
tags-binder  ::= 'tags' IDENT { ',' IDENT }
type-binder  ::= 'type' IDENT { ',' IDENT }

top-item    ::= import | import-c | extern-c | top-decl | <opaque>

import       ::= 'import' module-name [ 'as' IDENT ] [ 'types' ] ';'
import-c     ::= 'import_c' ( system-header | STRING ) ';'
extern-c     ::= [ 'pub' | 'priv' ] 'extern_c' [ '[type_h]' ] '{' <opaque> '}'

top-decl    ::= [ 'pub' | 'priv' ]
                 ( decl-modifier | decl-instance | decl-tags
                 | decl-extent | decl-function | decl-keel | <opaque> )

decl-modifier ::= 'modifier' IDENT [ 'byref' ] '{' <opaque> '}'
decl-instance   ::= 'instance' known-type ';'
decl-tags        ::= 'tags' IDENT tags-list ';'
tags-list       ::= '[' tag-item { ',' tag-item } ']'
tag-item         ::= IDENT [ '=' tag-value ]
tag-value        ::= [ '-' ] NUM | qualified-name

decl-extent   ::= 'extent' 'struct' IDENT extent-dim { extent-dim }
                  '{' { extent-field } '}' ';'
extent-dim    ::= '[' IDENT ',' ( IDENT | NUM ) ']'
extent-field  ::= extent-column | <opaque> ';'
extent-column ::= 'array' argument
                  ( { '*' } IDENT dimensions | '*' { '*' } IDENT ) ';'

decl-function   ::= return-type fn-declarator ( block | ';' )
return-type       ::= { spec-c } ( known-type | <opaque-no-parens> )
fn-declarator ::= { '*' { qual-c } } IDENT '(' [ params ] ')'
params        ::= 'void' | param { ',' param } [ ',' '...' ]
param         ::= param-array | param-type
                | ( known-type | <opaque-no-parens> )
                  { '*' { qual-c } } [ IDENT ] { suffix }
spec-c        ::= 'inline' | 'static' | 'extern' | '_Noreturn'
                | '_Thread_local' | 'alignas' '(' <opaque> ')'
                | '[[' <opaque> ']]' | qual-c
param-array  ::= { spec-c } 'array' argument { '*' } IDENT dimensions
param-type   ::= 'type' IDENT
```

`system-header` designa a forma `<…>` de header usada por `import_c`. O corpo de `extern-c` é preservado segundo a §4.1. `param-array` usa as dimensões de `array`; as restrições de rank em parâmetros pertencem ao contrato do marcador. `param-type` declara um parâmetro que recebe um tipo escrito na chamada (§4.4). `decl-extent` e `extent-column` pertencem à §4.11.

#### Tipos e declarações

```ebnf
decl-keel    ::= { spec-c } specifier declarator [ '=' <opaque> ]
                 { ',' declarator [ '=' <opaque> ] } ';'
               | { spec-c } specifier declarator '=' <opaque> else-tail
               | decl-array | decl-constexpr

else-tail   ::= 'else' ( block | <opaque> ';' )

specifier ::= known-type
known-type     ::= modifier argument { argument } | named-type
named-type  ::= qualified-name
modifier   ::= qualified-name [ '(' dim-value { ',' dim-value } ')' ]
dim-value     ::= NUM | IDENT
argument     ::= { qual-arg } ( known-type | qualified-name
                  | tagged-type | base-type | 'void' ) { qual-arg }
tagged-type  ::= ( 'struct' | 'union' | 'enum' ) qualified-name
qual-arg      ::= 'const' | 'volatile' | '_Atomic'
base-type     ::= 'char' | 'bool'
qualified-name ::= IDENT { '.' IDENT }

declarator   ::= { '*' { qual-c } } direct-declarator
direct-declarator ::= IDENT { suffix }
                    | '(' declarator ')' { suffix }
suffix       ::= '[' [ <opaque> ] ']' | '(' <opaque> ')'
qual-c       ::= 'const' | 'volatile' | 'restrict' | '_Atomic' | 'ref'

decl-constexpr ::= 'constexpr' <opaque> ';'

decl-array   ::= { spec-c } 'array' argument decl-array-1 { ',' decl-array-1 } ';'
decl-array-1 ::= { '*' } IDENT dimensions [ '=' <opaque> ]
dimensions    ::= '[' [ <opaque> { ',' <opaque> } ] ']'
               | '[' [ <opaque> ] ']' { '[' <opaque> ']' }
```

`void` em posição de argumento de modificador segue o protocolo de omissão de campos parametrizados; não declara um objeto C de tipo `void`. `outcome void` representa um resultado sem valor associado (§5.5).

`tagged-type` consome a palavra C e o nome como um único argumento de tipo:
`buffer struct Person people;` aplica `buffer` a `struct Person`, e `people`
é o declarador. O PPC preserva a forma C do tipo na substituição, com a
qualificação dos nomes que reconhece; não precisa interpretar os campos do
agregado para substituir esse argumento.

`named-type` só casa um nome registrado como tipo, como `arena`. Um nome registrado como modificador, como `buffer`, exige o tipo ao qual será aplicado: `buffer T` é o tipo resultante. O número de argumentos de um modificador é determinado pelo módulo declarado, não pela repetição livre da EBNF. Qualificadores de argumento são aceitos dos dois lados do tipo; a identidade canônica segue o contrato de tipos e genéricos.

`dim-value` admite um literal ou uma constante nomeada conhecida nas condições da §4.2. A validade de um valor como dimensão não decorre apenas de sua classificação como `NUM` ou `IDENT`. [Justificativa: constantes nomeadas](keel-rationale.md#constantes-nomeadas).

#### Statements e blocos

```ebnf
stmt         ::= stmt-keel | stmt-c | label | block | <opaque> ';'
block        ::= '{' { stmt } '}'

label       ::= IDENT ':' stmt | 'case' <opaque> ':' stmt | 'default' ':' stmt

stmt-c       ::= 'if' '(' <opaque> ')' stmt [ 'else' stmt ]
               | ( 'while' | 'switch' ) '(' <opaque> ')' stmt
               | 'for' '(' <opaque> ')' stmt
               | 'do' stmt 'while' '(' <opaque> ')' ';'
               | 'return' [ <opaque> ] ';'
               | ( 'break' | 'continue' ) ';'
               | 'goto' IDENT ';'
               | ';'

stmt-keel    ::= decl-keel | decl-tags
               | defer | foreach | walk | apply | parallel
               | match | worker-exit

defer        ::= 'defer' [ '[' defer-options ']' ] defer-body
defer-options ::= 'later' | 'now' typed-capture
typed-capture ::= entry { ',' entry }
entry      ::= <opaque> IDENT
defer-body  ::= block | <opaque> ';'
capture      ::= '(' IDENT { ',' IDENT } ')'

foreach      ::= 'foreach' '(' binder ',' binder ':' container ')' stmt
               | 'foreach' '(' binder ':' countable ')' stmt
binder       ::= binder-type IDENT
binder-type  ::= ( known-type | qualified-name | base-type | 'auto' ) { '*' }
apply        ::= 'apply' '(' binder-type ',' container ',' IDENT
                 { ',' <opaque> } ')' ';'
walk         ::= 'walk' '(' binder [ ',' binder ] ':' container ')' stmt

parallel     ::= 'parallel' IDENT policy
                 '(' binder ':' countable ';'
                     binder ':' container
                     [ ';' capture ] ')' block
policy     ::= 'ALL' | 'ANY' | NUM | qualified-name
worker-exit ::= ( 'win' | 'fail' ) ';'

match        ::= 'match' '(' container ')' tags-block
tags-block   ::= '{' { tag-arm } '}'
tag-arm    ::= tag-label { tag-label } { stmt }
tag-label   ::= IDENT ':'
```

`stmt-c` descreve a estrutura necessária para localizar corpos, escopos e pontos de saída; suas expressões permanecem opacas. Dentro de `tags-block`, os rótulos externos identificam tags, e cada braço abre um escopo até o próximo rótulo ou até a chave final. Rótulos consecutivos, sem statements entre eles, compartilham o braço seguinte.

Um argumento de tipo pode nomear um conjunto de tags declarado por `decl-tags`, como em `tagged Cycle void`. A produção `argument` já admite essa forma por `qualified-name`; o papel do nome vem do parâmetro correspondente na assinatura do módulo. O contrato completo está na §4.9.

#### Contêineres, intervalos e `range-index`

```ebnf
container    ::= IDENT
               | container '[' <opaque> { ',' <opaque> } ']'
               | container '[' range-index ']'
               | container '.'  IDENT
               | container '->' IDENT
               | verb '(' container { ',' <opaque> } ')'
               | '*' container | '&' container | '(' container ')'

countable     ::= interval | container
interval    ::= <opaque> '..' <opaque>
range-index      ::= [ <opaque> ] '..' [ <opaque> ]
verb        ::= module-name '.' IDENT
```

A produção `container` descreve as formas em que keel pode consultar a identidade de um contêiner a partir de símbolos conhecidos. Não atribui tipos a expressões C arbitrárias. A assinatura da operação determina qual argumento ocupa essa posição. [Justificativa: limite da análise de contêiner](keel-rationale.md#a-posição-de-contêiner-não-é-um-sistema-de-tipos).

### 2.3 Reconhecimento

#### Regiões opacas e pontos de parada

- O reconhecimento percorre as expressões e os corpos C para localizar as construções keel, nos limites da §1.3. Não deduz o tipo das expressões C que as envolvem.
- Em uma lista, `<opaque>` termina no separador ou delimitador exigido pela produção, no nível externo daquela região. Grupos internos são atravessados como unidades balanceadas.
- Em posição de statement, a varredura distingue blocos e as formas de controle de `stmt-c`. A região opaca termina no `;` externo; não absorve o início de um bloco ou de uma construção keel reconhecida em posição inicial de statement.
- O `else` associado a um `if` pertence a `stmt-c`. A cauda `else` de keel pertence a uma declaração reconhecida, sujeita ao contrato de tratamento de resultado (§4.10).
- Dentro de `extern_c`, construções keel não são traduzidas. O balanceamento de delimitadores permanece ativo; o registro dos nomes declarados segue a §4.1.
- Literais, comentários e corpos de diretivas não são percorridos em busca de chamadas ou outras construções keel.

#### Informação de símbolos

O reconhecimento consulta nomes e informações declaradas, com as seguintes origens:

| Origem | Informação disponível |
| --- | --- |
| Prelúdio | Somente o import implícito `import keel types;`, conforme a §4.1. |
| Imports explícitos | Qualificadores de módulo, aliases e símbolos disponibilizados pelo módulo; `types` injeta os nomes de tipo segundo as regras de importação. |
| Declarações keel | Nomes de tipos, modificadores e suas aridades, parâmetros genéricos, funções e retornos declarados, variáveis, marcadores, constantes e símbolos das construções cooperativas e paralelas. |
| `extern_c` | Nomes declarados explicitamente e distinção entre variável e função, sem conhecimento semântico de seus tipos. |
| Grafias de tipos normalizadas | Correspondências explícitas definidas pelo contrato de tipos (§4.2), sem leitura de headers. |

Nomes que apareçam apenas em headers C ou que sejam produzidos pela expansão de macros não passam a ser símbolos conhecidos por keel. O reconhecimento de um nome declarado não autoriza inferir o tipo de uma expressão C arbitrária.

#### Declarações e palavras contextuais

| Forma | Condição de reconhecimento |
| --- | --- |
| Função | Varre-se até o primeiro `;` ou `{` externo. Um grupo de parâmetros externo, não precedido por `=` externo, termina imediatamente antes desse token; seu `(` é precedido pelo `IDENT` do nome declarado. |
| Tipo nomeado | O nome deve estar registrado como tipo reconhecido pelo PPC. A sequência de dois identificadores, por si só, não basta. |
| Modificador com argumentos | O nome e a aridade vêm dos símbolos registrados. Os argumentos são consumidos recursivamente segundo essa aridade, após os especificadores e qualificadores admitidos. |
| Parâmetro com tipo reconhecido pelo PPC | Depois do tipo nomeado ou da aplicação completa do modificador, são admitidos o declarador ou o fim do parâmetro; o nome pode ser omitido nas formas previstas por `param`. |
| `ref` | Só ocupa a posição de qualificador depois de `*` no declarador. Não qualifica o tipo antes do declarador. |
| `tags` | Em linha `module`, introduz parâmetros do módulo. Em declaração, o nome é seguido de `[` e da lista de tags. |
| `extent` | Em posição de declaração de arquivo, seguido de `struct`, do nome e de `[`, inicia a declaração de `extent`. |
| `type` | Em linha `module`, introduz parâmetros do módulo. Em lista de parâmetros de função, seguido de `IDENT` e de `,` ou `)`, declara parâmetro de tipo. |
| `match` | Seguido de `(`, identifica o despacho; o corpo entre `{` e `}` contém rótulos de tag. |
| `walk` | Seguido de `(`, identifica a travessia por cursor; o segundo binder declara o cursor. |
| `foreach` | Dois binders são separados por `,` antes de `:`; a forma com um binder usa `countable`. |
| `instance` | Em posição de declaração, inicia uma declaração de instância. É proibido declarar um modificador com esse nome. [Justificativa da reserva](keel-rationale.md#por-que-instance-é-a-única-ressalva-do-documento). |

Uma sequência de identificadores pode indicar uma forma candidata, mas a aceitação do tipo depende dos símbolos e da aridade. A varredura até um delimitador pode ter comprimento variável; o reconhecimento não é descrito como lookahead de tamanho fixo.

Um declarador como `int (*f)(void);` não satisfaz a regra de função acima: o token anterior ao grupo final de parâmetros é `)`. Uma declaração como `int x = f(1);` também não satisfaz a regra, pois há `=` externo antes da chamada. Isso não classifica o tipo C desses objetos. [Justificativa e comparação dos declaradores](keel-rationale.md#reconhecimento-de-funções-e-declaradores-c).

Se a varredura de uma forma candidata atinge o fim do arquivo antes do token
de terminação previsto, sem sobrar delimitador aberto para `unmatched-delimiter`
apontar, o diagnóstico é `unexpected-eof` (§2.5).

#### Qualificação, acesso e sombreamento

- Na resolução de `.` consultam-se, nesta ordem, um alias de módulo, um nome de tipo declarado em keel e uma expressão de contêiner reconhecida. As demais formas permanecem acesso a campo ou designador C.
- Uma declaração local reconhecida que sombreie um qualificador desativa sua interpretação como qualificador naquele escopo. O sombreamento tem os diagnósticos da §2.5.
- A coexistência de alias de módulo e nome de tipo com a mesma grafia é aceita quando ambos provêm do mesmo módulo. Nos demais casos aplica-se o diagnóstico `alias-type-collision`.
- A posição de contêiner usa a produção `container` e a informação dos símbolos keel. Uma expressão C desconhecida, como uma chamada C ou um cast arbitrário, não fornece a identidade de contêiner exigida pelo despacho.
- Índices, valores e demais argumentos continuam sendo regiões opacas, inclusive quando contêm outras construções keel reconhecíveis.
- A indexação sobre um símbolo C desconhecido permanece C. A reescrita de índices múltiplos de `array` requer um símbolo com esse marcador; os demais contêineres seguem seus contratos de acesso.

### 2.4 Semântica

#### Tokenização e preservação

- A emenda de linha precede o reconhecimento de comentários, literais e diretivas.
- Comentários não chegam ao C gerado: cada um é substituído por espaços, e
  suas quebras de linha se preservam (backend §6). O conteúdo de `extern_c` é
  C e atravessa intacto, comentários inclusive.
- O reconhecimento léxico não registra tipos nem decide a identidade de instâncias. Essas decisões usam o parser e a tabela de símbolos.
- Diretivas são preservadas para o pré-processador C. keel não expande macros, não avalia condições e não escolhe alternativas condicionais.
- A classificação lexical de palavras C não muda com o perfil de geração. A validade do C preservado é verificada pelo compilador C.
- Constantes nomeadas por `constexpr` podem ocupar posições antes restritas a literais, conforme a §4.2. Quando uma construção requer que keel conheça o valor numérico, aplica-se a leitura de literal prevista em seu contrato; a produção `NUM | IDENT` não autoriza avaliação de expressões.

#### Balanceamento de grupos condicionais

- Um grupo começa em `#if`, `#ifdef` ou `#ifndef` e termina no `#endif` correspondente.
- `#elif`, `#elifdef`, `#elifndef` e `#else` iniciam uma nova alternativa. A ausência de `#else` acrescenta uma alternativa vazia.
- Cada alternativa é examinada a partir do mesmo contexto de entrada e produz uma variação de delimitadores `()`, `[]` e `{}`. Todas as alternativas devem concordar nessa variação, mantendo o emparelhamento dos delimitadores.
- A variação comum é aplicada uma única vez ao contexto que contém o grupo.
- Grupos aninhados são resolvidos de dentro para fora.
- Divergência entre alternativas produz o diagnóstico `delimiter-mismatch-across-branches`. Delimitador sem par produz o diagnóstico `unmatched-delimiter`, inclusive dentro de `extern_c`.

Uma alternativa pode abrir um bloco que termina depois do grupo, desde que as demais alternativas produzam a mesma estrutura. Um bloco aberto apenas sob `#ifdef` sem `#else` não satisfaz essa regra, pois a alternativa vazia não abre o bloco. [Justificativa: balanceamento por alternativa](keel-rationale.md#por-que-a-contagem-é-por-alternativa-e-não-sobre-o-texto).

### 2.5 Restrições e diagnósticos

Os diagnósticos abaixo são emitidos por keel durante a tradução. Seus identificadores são os mesmos do catálogo geral. Regras específicas das construções acrescentam os diagnósticos de seus próprios contratos.

| Condição | Identificador | Severidade |
| --- | --- | --- |
| `extern_c` fora do nível de arquivo | `nested-extern-c` | `error` |
| Dois imports com `types` injetam o mesmo nome nu | `duplicate-injected-name` | `error` |
| Declaração local sombreia nome injetado por `types` | `shadowed-injected-name` | `warning` |
| Import injeta nomes por `types`; a mensagem lista os nomes | `injected-names` | `info` |
| Expressão fora da gramática de contêiner em posição que a exige | `not-a-container-expression` | `error` |
| Padrão local de redeclaração de símbolo conhecido, definido abaixo | `symbol-redeclaration` | `error` |
| Delimitador sem par; a mensagem localiza a abertura quando existente | `unmatched-delimiter` | `error` |
| Fim de arquivo durante o reconhecimento de uma forma candidata, sem token de terminação nem delimitador aberto | `unexpected-eof` | `error` |
| Alternativas condicionais discordam na estrutura de delimitadores | `delimiter-mismatch-across-branches` | `error` |
| `#define` ou `#undef` de palavra contextual keel ou de nome no espaço `keel_` | `define-over-keel-name` | `error` |
| Newline não emendado em literal de string ou caractere | `literal-with-newline` | `error` |
| Sombreamento de palavra contextual, verbo ou nome de módulo | `keel-name-shadowed` | `warning` |
| Alias de módulo e tipo de origens distintas têm a mesma grafia no arquivo | `alias-type-collision` | `error` |
| Modificador declarado com o nome `instance` | `modifier-named-instance` | `error` |

Para o diagnóstico `symbol-redeclaration`, keel reconhece os padrões `IDENT IDENT` e `IDENT '*' IDENT` no início de statement, quando o segundo identificador é um símbolo keel conhecido. A verificação recusa a possível redeclaração sem precisar resolver o primeiro identificador como tipo C. [Justificativa: recusa de possíveis redeclarações](keel-rationale.md#por-que-a-redeclaração-é-recusada-em-vez-de-classificada).

Para o diagnóstico `define-over-keel-name`, keel lê o nome alvo de `#define` ou `#undef`. Essa inspeção é adicional à classificação pela palavra da diretiva; não examina semanticamente o corpo da macro, não o expande e não altera a diretiva. A preservação do conteúdo não exclui essa verificação lexical.

### 2.6 Pré-condições e limites

- As construções e os nomes necessários ao reconhecimento devem existir antes da expansão de macros, conforme a §1.2.
- O programa deve respeitar as restrições de nomes e de estrutura condicional mesmo quando o compilador C descartaria um ramo.
- A validação das expressões C, dos tipos C desconhecidos e das diretivas preservadas cabe ao compilador C e ao seu pré-processador.
- A gramática de contêiner não admite que keel deduza a identidade de uma chamada C desconhecida ou de um cast arbitrário.
- As verificações de sombreamento e redeclaração alcançam as formas reconhecidas. Não constituem análise geral das declarações C ou dos nomes introduzidos por headers e macros.
- Um import pode alterar o reconhecimento de nomes e formas antes tratados como C. Aplicam-se as regras de conflito e sombreamento; não há garantia irrestrita de estabilidade ao acrescentar imports.

### 2.7 Exemplo mínimo

O marcador `array` identifica o símbolo sobre o qual a indexação multidimensional é reescrita. O comentário e o literal não participam desse reconhecimento.

```keel
//keel
void example(void) {
    array char grade[2,3];
    grade[1,2] = 'x';
    const char *text = "grade[1,2] .. defer";
    /* grid[1,2] stays text inside this comment. */
}
```

C correspondente à operação essencial:

```c
//C gerado
void example(void) {
    char grade[2][3];
    grade[1][2] = 'x';
    const char *text = "grade[1,2] .. defer";
    /* grid[1,2] stays text inside this comment. */
}
```

O par mostra um corpo de função; a declaração de módulo foi omitida. Detalhes de nomes gerados e verificações de acesso pertencem ao backend.

### 2.8 Referências

- [Rationale](keel-rationale.md): “Fronteira com C e conflitos léxicos”, “Prelúdio e base mínima”, “Constantes nomeadas”, e as justificativas de reconhecimento — “Por que `<opaque>` é o terminal central”, “Por que a contagem é por alternativa” e “Por que a redeclaração é recusada em vez de classificada”.
- [Backend](keel-c-backend.md): §§2, 5, 6 e 9, para nomes, emissão das construções, mapeamento de linhas e perfis de geração.

## 3. keel por exemplos

Os exemplos em pares keel/C estão no [README](README.md#keel-por-exemplos). Cada contrato dos capítulos 4 e 5 traz o seu no item 3.

## 4. Construções do núcleo

Os contratos deste capítulo descrevem as construções do núcleo: as formas que keel
reconhece, liga e emite. Eles usam as regras de reconhecimento do capítulo 2.
Os módulos distribuídos com keel, e os protocolos pelos quais estas construções
os alcançam, estão no capítulo 5; o capítulo 6 reúne os identificadores de
diagnóstico citados em cada contrato.

### 4.1 Módulos e interoperabilidade

#### 1. Sintaxe

```keel
module geom;
import util;
import shapes as f types;
import_c <stdio.h>;
extern_c [type_h] { struct point { double x, y; }; }
extern_c { int external_op(int); }
priv extern_c { static int cache; }
pub i32 counter;
priv i32 helper(i32 x) { return x; }
```

`module`, `import`, `import_c` e `extern_c` ocupam o nível de arquivo. `module` é a primeira construção significativa, antes de diretivas C; seu nome é um caminho de identificadores separados por ponto, e a forma genérica acrescenta os parâmetros da §4.3. `import M [as A] [types];` admite alias antes de `types`. `import_c` recebe um nome de header entre `<` e `>`, ou entre aspas. `pub` e `priv` antecedem uma declaração de módulo.

**Reconhecimento**

- A declaração de módulo estabelece o qualificador de seus símbolos. Os imports carregam interfaces keel e registram qualificadores e aliases.
- A coleta registra as declarações reconhecidas do arquivo antes da resolução dos usos. A análise de cada corpo respeita os escopos léxicos.
- `import_c` introduz uma inclusão C. keel não abre o header nem registra os tipos, funções, variáveis ou macros que ele possa declarar.
- `extern_c` delimita texto C preservado, sem tradução de construções keel. O registro de nomes explícitos pode distinguir variável e função segundo a §2.3, sem determinar seus tipos.

#### 2. Regras

Organizar declarações em módulos e integrar interfaces e implementações C.

1. O nome do módulo corresponde ao caminho do `.k` relativo à raiz de fontes. Cada componente forma um identificador admitido para esse caminho.
2. Sem `pub` nem `priv`, a declaração é `pub`. Em módulo genérico, o default também é `inline` (§4.3).
3. Declarações públicas compõem a interface; declarações privadas e corpos fora de linha compõem a implementação, conforme o backend.
4. `priv` não muda por si só o linkage C. `static` conserva seu significado C.
5. Um alias muda a escrita usada pelo importador e preserva a identidade de origem.
6. `types` disponibiliza sem qualificador os nomes de tipos e modificadores do módulo importado. Não injeta funções, variáveis nem constantes de enum. A forma qualificada continua disponível, e a injeção não se propaga por imports.
7. O módulo e o modificador que ele declara têm identidades distintas. Em `import keel.outcome as outcome types;`, `outcome` é o alias do módulo e `outcome.outcome` é o modificador; `types` permite escrever o modificador como `outcome` em posição de tipo, e em `outcome.ok(r)` o prefixo qualifica um verbo do módulo.
8. A resolução alcança símbolos públicos de imports transitivos. O uso sem import direto é `indirect-import`.
9. Colisões são verificadas entre os símbolos exportados do módulo e os do fecho transitivo de seus imports, pelos nomes canônicos efetivos, inclusive tags, typedefs e constantes de enum (§4.2).
10. `import_c` emite a inclusão na interface.
11. `extern_c` preserva o conteúdo, sem mangling dos símbolos ali declarados. keel não inspeciona esse conteúdo.
12. Sem `priv`, o conteúdo de `extern_c` compõe a interface: com `[type_h]`, a camada de tipos (`.type.h`); sem `[type_h]`, o `.h`. Com `priv`, compõe a implementação (`.c`). `priv extern_c [type_h]` é recusado.
13. Diretivas de pré-processamento no nível de arquivo vão para o `.h`; dentro de um construto, acompanham o destino dele.
14. `main` é uma função pública do módulo e recebe seu prefixo. A seleção do módulo de entrada pela ferramenta gera o wrapper C `main`, que chama essa função. Cada módulo pode declarar a sua `main`.
15. O único import implícito é `import keel types;`. Os módulos da base — `keel.arena` (§5.2), `keel.buffer`, `keel.slice` e `keel.range` (§5.3), `keel.tagged` (§5.4), `keel.outcome` e `keel.corot` (§5.5), `keel.routine` (§5.6) e `keel.parallel` (§5.7) — exigem import explícito. [R: prelúdio e base mínima](keel-rationale.md#prelúdio-e-base-mínima)

Referências: [Rationale: módulos e identidade](keel-rationale.md#módulos-e-identidade); [Rationale: fronteira com C](keel-rationale.md#fronteira-com-c-e-conflitos-léxicos); [Backend: artefatos](keel-c-backend.md#4-artefatos) e [ponto de entrada](keel-c-backend.md#58-ponto-de-entrada).

#### 3. Exemplo

O [exemplo 1 do README](README.md#1-módulo-completo-e-chamada-c) mostra `module`, `import_c` e o wrapper de entrada.

#### 4. Erros

De keel, na tradução; condições no [catálogo](#62-catálogo): `missing-module`, `module-path-mismatch`, `invalid-stem`, `case-ambiguous-stem`, `symbol-collision`, `circular-import`, `nested-extern-c`, `main-in-extern-c`, `private-main`, `invalid-main-signature`, `import-clause-order`, `duplicate-injected-name`, `duplicate-alias`, `shadowed-injected-name`, `injected-names`, `pub-static`, `inline-without-visibility`, `static-on-type`, `type-layer-on-priv-extern-c`, `indirect-import`.

A validação semântica das declarações C é do compilador C. Os delimitadores de `extern_c` seguem sujeitos a `unmatched-delimiter`.

#### 5. Casos especiais

- O programa garante ao compilador e ao linker C os headers e objetos externos necessários. Registrar um nome não verifica a implementação nem a assinatura.
- O programa garante que um `extern_c` público com corpo não-`inline` não gere definição múltipla.
- Colisões entre módulos sem relação no fecho analisado só aparecem no linker.
- Acrescentar um import pode alterar o reconhecimento de nomes (§2.3).

### 4.2 Tipos, declarações e marcadores

#### 1. Sintaxe

```keel
buffer i32 dados;
buffer struct Person people;
slice const char text;
array i32 matrix[2,3];
array char message[] = "keel";
i32 *ref element = origin;
constexpr size_t N = 3;
```

A aplicação de um modificador ao tipo forma o especificador que precede o declarador C: em `buffer struct Person people;`, o modificador é `buffer`, o tipo modificado é `struct Person` e o objeto declarado é `people`. Ponteiros, qualificadores de ponteiro e extensões de vetor pertencem ao declarador. `array` marca uma declaração de vetor em arquivo, bloco, campo ou parâmetro; num `extent`, marca as colunas, e `array T *col` sem dimensões é coluna por ponteiro (§4.11). `ref` ocupa a posição de qualificador depois de `*`. `constexpr` declara uma constante em arquivo ou bloco, com inicializador e declarador simples.

**Operações do núcleo sobre `array`**, qualificadas por `keel` (§4.4):

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `keel.length(v)` | `size_t` | total de elementos, constante de compilação |
| `keel.capacity(v)` | `size_t` | total de elementos |
| `keel.dim(v, k)` | `size_t` | dimensão de índice `k`, a partir de zero; `k` é decimal conhecido |
| `keel.get(v, i)`, `keel.set(v, i, x)` | `T`, — | lê ou escreve o elemento `i` |
| `keel.ptr(v)` | `T *` | início do vetor unidimensional |
| `keel.ptr(v, i)` | `T *` | endereço do elemento `i` |
| `keel.at(v, i)` | [`outcome T`](#55-keeloutcome-e-keelcorot) | o elemento `i`, ou `NONE` fora da extensão, em toda build |

Protocolos (§5.1): Indexável e Indexável por intervalo, por estas operações; `begin` e `partition` não existem sobre `array`.

**Reconhecimento**

- Um tipo ou modificador precisa estar registrado; seus argumentos são lidos conforme a aridade declarada. Declaradores simples registram nome, forma de valor ou ponteiro e os marcadores escritos.
- Declaradores C mais gerais podem receber substituição dos nomes keel sem fornecer um símbolo utilizável como contêiner. O reconhecimento de funções segue a §2.3.
- Campos keel em `struct` são registrados com o tipo e os marcadores escritos; campos C desconhecidos permanecem opacos. A interface de uma struct pública exporta o necessário para resolver caminhos como `w->dados`, sem interpretar os tipos C dos demais campos.
- `array` registra que o símbolo é vetor e sua quantidade de dimensões. A extensão unidimensional pode vir do compilador C, inclusive por inicializador; keel não conta os elementos.
- Em `constexpr`, o nome é o identificador imediatamente anterior ao `=`. Sem inicializador, a declaração segue para o compilador C e não registra constante utilizável pela tradução.

#### 2. Regras

Declarar tipos e símbolos e registrar as propriedades usadas pelas construções keel.

1. `i8`, `i16`, `i32` e `i64` têm a largura indicada e representação com sinal em complemento de dois; `u8`, `u16`, `u32` e `u64` são os correspondentes sem sinal.
2. `f16`, `f32` e `f64` são os formatos binários IEEE de 16, 32 e 64 bits; `bf16` tem um bit de sinal, oito de expoente e sete de fração. Disponibilidade e representação C pertencem ao backend.
3. `bool`, `char`, `size_t`, `ptrdiff_t` e `uintptr_t` conservam seus contratos C. As grafias `int8_t` a `int64_t` e `uint8_t` a `uint64_t` normalizam para os nomes keel correspondentes, sem consultar headers.
4. A identidade nominal inclui o módulo de origem e os argumentos canônicos. Alias e `types` não criam outra identidade; tipos de módulos distintos são distintos mesmo com campos iguais.
5. Vários declaradores na mesma declaração registram símbolos separados, salvo a restrição de `else` (§4.10).
6. Ponteiro como argumento de modificador exige tipo nomeado: `typedef i32 *pint; buffer pint b;` é buffer de ponteiros; `buffer i32 *b;` é ponteiro para buffer.
7. Modificadores se aninham sem teto próprio de profundidade, sujeitos ao limite de nome do alvo.
8. Qualificadores de argumento escritos antes ou depois do tipo normalizam para a mesma identidade. Qualificadores e especificadores C antes do modificador qualificam a declaração externa, são preservados e não mudam o argumento.
9. Constantes de enum nomeado recebem o escopo do tipo: `M.State.STOPPED`. Enum sem nome usa o escopo do módulo. Dentro do módulo, a constante pode ser escrita sem qualificação; de fora, o nível do tipo não se omite.
10. `array` não cria tipo: `array T v[2,3]` traduz para `T v[2][3]`. A escrita com colchetes sucessivos também é aceita. Em parâmetro multidimensional, a primeira extensão sai com `static` no C.
11. `keel.ptr`, `buffer.of` e `slice.of` exigem `array` unidimensional.
12. As dimensões de um argumento `array` são conferidas contra as do parâmetro: a aridade e as dimensões de índice 1 em diante coincidem, e a dimensão 0 do argumento não é menor que a declarada. A conferência alcança argumento que é símbolo `array` de dimensões conhecidas.
13. `constexpr` é constante nomeada e tipada, sem endereço e sem usos que exijam lvalue. O compilador C verifica o inicializador contra o tipo escrito, nos limites de cada perfil.
14. Posições que admitem literal inteiro admitem `constexpr` conhecido, respeitadas as restrições do uso. Quando keel precisa do número, como em `dim` e `keel.dim(v,k)`, lê somente um literal decimal ou um inicializador decimal conhecido; não calcula expressões.
15. `ref` marca um ponteiro para um elemento, admite `NULL`, exige inicialização e desaparece no C. A restrição vale para o símbolo declarado, sem seguir cópias do endereço.
16. `ptr(x)` entrega ponteiro comum; `ptr(x,i)` entrega o endereço de um elemento.
17. `restrict` conserva a semântica C em declaradores de ponteiro e não se aplica como prefixo de modificador.
18. Os nomes emitidos derivam da identidade nominal; sua grafia pertence ao backend.

Referências: [Rationale: constantes nomeadas](keel-rationale.md#constantes-nomeadas) e [marcadores e declarações](keel-rationale.md#marcadores-e-declarações); [Backend: tipos primitivos](keel-c-backend.md#3-tipos-primitivos), [declarações](keel-c-backend.md#51-declarações-substituição-local-de-nome) e [perfis](keel-c-backend.md#9-perfis-de-geração).

#### 3. Exemplo

```keel
//keel
array i32 m[2,3];
i32 *ref p = origin;
```

```c
//C gerado
i32 m[2][3];
i32 *p = origin;
```

#### 4. Erros

De keel, na tradução; condições no [catálogo](#62-catálogo): `c-type-as-argument`, `array-1d-as-parameter`, `array-parameter-without-dimension`, `array-argument-wrong-dimension`, `partial-array-index`, `flat-view-of-n-dim-array`, `nonconstant-dim-index`, `ref-without-initializer`, `ref-arithmetic`, `enum-constant-without-type`, `restrict-on-container`, `hidden-declarator`, `nonscalar-constexpr`, `constexpr-as-lvalue`, `canonical-name-collision`, `reserved-name`, `name-too-long`. `specific-format-unavailable` é do backend ou do compilador C.

Conflitos de nomes seguem a §2.5.

#### 5. Casos especiais

- O compilador C verifica tipos, inicializadores, dimensões C e acessos fora das verificações de keel.
- Sobre região C opaca, as dimensões de um argumento `array` não são conferidas.
- O programa garante que o índice de `keel.dim(v,k)` designa uma dimensão existente.
- `ref` não prova validade, não nulidade nem tempo de vida do endereço.
- Formatos estreitos dependem da guarda de disponibilidade do alvo. A representabilidade de `constexpr` sob C11 está na §6.3.
- Um objeto constante com endereço usa a forma C `static const T nome = valor;`.

### 4.3 Módulos genéricos

#### 1. Sintaxe

```keel
module coll type T;
pub modifier stack byref { size_t cap, len; T *ptr; }

// In another module, after importing coll:
coll.stack i32 stack;
instance coll.stack i32;
```

```keel
module blocks dim N type T;
pub modifier block { T dados[N]; }
pub inline void fill(block *b, T values[static N]) { /* ... */ }
```

```keel
module marked tags E type T;
pub modifier mark { i32 tag; T value; }
```

Os parâmetros pertencem à linha `module`, nesta ordem: `dim`, `tags`, `type`. Cada um admite lista separada por vírgulas. O uso espelha a assinatura: `blocos.bloco(3) i32`, `marked.mark Kind i32`. `modifier` e `instance` são declarações de arquivo; `byref` segue o nome do modificador. Um módulo pode declarar mais de um modificador, e o nome do módulo não precisa coincidir com o de nenhum deles.

**Reconhecimento**

- A assinatura do módulo fixa a quantidade e a ordem dos argumentos de todos os seus modificadores. A substituição usa os nomes ligados por essa assinatura.
- Uma declaração pertence à instância quando seus tokens mencionam um parâmetro ou um modificador do módulo. As demais pertencem ao módulo, sem análise de dependências indiretas.
- A pertença de verbos ao modificador e suas aridades vêm das assinaturas declaradas. O compilador C verifica os corpos depois da substituição.

#### 2. Regras

Definir modificadores de tipos e expandir suas declarações para os argumentos fornecidos.

1. A aplicação do modificador produz um tipo C com a representação e os metadados declarados, acompanhado das operações definidas pelo módulo.
2. A declaração original do tipo modificado não muda. O objeto declarado com o modificador tem a representação resultante, validada no C emitido.
3. `type` liga um parâmetro, como `T`, ao argumento de tipo escrito. Na expansão, o parâmetro é substituído pela forma C correspondente, como `i32` ou `struct Person`, com os nomes reconhecidos qualificados conforme sua origem.
4. `dim` substitui um inteiro positivo conhecido na tradução: literal decimal ou `constexpr` de inicializador decimal conhecido.
5. `tags` liga um parâmetro, como `E`, ao **nome** de um conjunto declarado por `tags Nome [ … ];`. Na expansão, o parâmetro é substituído pelo `enum` C correspondente, e as constantes do conjunto ficam disponíveis para as declarações da instância.
6. Num verbo com parâmetro declarado com o nome do parâmetro `tags` — `mark(m *t, E e)` —, uma constante de tag reconhecida no argumento pertence ao conjunto daquela instância, sob pena de `tag-from-other-set`. Outro argumento segue para o compilador C. A regra vale para todo módulo com parâmetro `tags`.
7. Dentro do módulo genérico, o parâmetro de tipo é opaco: um valor de tipo `T`, ou ponteiro para ele, não participa de protocolo (§4.4) — `foreach`, `walk`, `apply`, `else`, `match`, indexação, `range-index`, `at` e chamadas qualificadas não se resolvem sobre ele. Expressões C sobre `T` atravessam como texto e são validadas pelo compilador C depois da substituição.
8. Uma instância de modificador que menciona `T`, como `outcome buffer T`, tem tipo conhecido e participa normalmente dos protocolos.
9. O significado de `N` pertence ao modificador. Não há associação automática entre `dim`, rank e quantidade de índices de um verbo.
10. A substituição não gera listas de parâmetros, campos ou funções; a aridade escrita de cada verbo é fixa. `T valores[static N]` é um parâmetro, cuja extensão exigida muda com a instância.
11. Com argumento `void`, os campos escritos como `T campo` ou `T *campo` são omitidos, e com eles todo verbo que menciona o parâmetro em posição de valor.
12. Com argumento qualificado `const`, são omitidos os verbos que escreveriam através do parâmetro.
13. A omissão é transitiva: um verbo cuja emissão chamaria outro ausente na instância também é omitido. Ela decorre só do argumento escrito, e é a mesma em toda instância com o mesmo argumento.
14. Um verbo que devolve ou recebe `T` por valor é emitido sobre o tipo sem o qualificador de topo.
15. Um `typedef` que menciona um parâmetro pertence à instância e é emitido por instância. Ele não tem forma escrita com argumento: só modificador aceita argumento em posição de tipo.
16. O uso de um modificador instancia suas declarações e, recursivamente, os modificadores usados por elas. A identidade inclui todos os argumentos canônicos. Um uso finito aninhado, como `stack stack i32`, é permitido.
17. `byref` recusa parâmetros por valor e diagnostica a cópia entre instâncias. Retorno por valor é permitido, sujeito à procedência da memória.
18. Em módulo genérico, o default é `pub inline`. Declarações fora de linha exigem que um módulo do programa coloque seus corpos por `instance`.
19. `instance` não declara nome nem substitui o import: determina onde ficam os corpos da instância já solicitada pelos usos.
20. Uma declaração de módulo genérico que não menciona parâmetro nem modificador pertence ao módulo e é emitida uma vez. Ela é tipo, `constexpr` ou função `inline`.

Referências: [Rationale: modificador e tipo modificado](keel-rationale.md#modificador-e-tipo-modificado); [Rationale: substituição e aridade fixa](keel-rationale.md#substituição-e-aridade-fixa); [Backend: headers de instância](keel-c-backend.md#43-headers-de-instância), [camadas de emissão](keel-c-backend.md#431-camadas-de-emissão), [os três artefatos](keel-c-backend.md#432-os-três-artefatos) e [definição fora de linha](keel-c-backend.md#44-definição-fora-de-linha-de-instância).

#### 3. Exemplo

```keel
//keel
module people;
import keel.buffer as buffer types;
import keel.outcome as outcome types;
import keel.tagged as tagged types;

pub tags State [NEW, ACTIVE];

struct Person { i32 age; };
buffer struct Person group = {0};
outcome struct Person result = {0};
tagged State struct Person mark = {0};
```

C da representação essencial, com as operações omitidas:

```c
//C gerado
#include <stddef.h>
#include <stdint.h>

typedef int32_t i32;
struct people_Person { i32 age; };

typedef struct {
    size_t cap, len;
    struct people_Person *ptr;
} keel_buffer_people_Person;

typedef struct {
    i32 code;
    struct people_Person value;
} keel_outcome_people_Person;

typedef enum { people_State_NEW, people_State_ACTIVE } people_State;

typedef struct {
    i32 tag;
    struct people_Person value;
} keel_tagged_people_State_people_Person;

keel_buffer_people_Person people_group = {0};
keel_outcome_people_Person people_result = {0};
keel_tagged_people_State_people_Person people_mark = {0};
```

O tipo `struct Person` é o mesmo nos três usos. A disposição exata dos campos
e seus nomes pertencem ao backend.

Para a substituição numérica, em `blocos.bloco(3) i32`, `T dados[N]` torna-se
`i32 dados[3]`, e `T valores[static N]` torna-se `i32 valores[static 3]`,
mantendo um único parâmetro.

#### 4. Erros

De keel, na tradução; condições no [catálogo](#62-catálogo): `byref-assignment`, `modifier-outside-generic`, `parameter-name-reuse`, `circular-generic`, `layout-cycle`, `protocol-on-parameter`, `verb-not-in-instance`, `instance-outside-file-scope`, `instance-not-modifier`, `redundant-instance`, `nonparametric-out-of-line`, `byref-param`, `nonconstant-dim`, `undeclared-tags`, `dim-generates-declaration`, `modifier-named-instance`, `dim-below-one`, `tag-from-other-set`.

#### 5. Casos especiais

- A instanciação não prova a validade das operações C sobre o tipo fornecido. A omissão de campos para `void` não torna válidos usos C que dependam deles.
- O programa garante que o chamador de `T valores[static N]` fornece pelo menos `N` elementos.
- O programa garante que os corpos fora de linha estão colocados, sem definições conflitantes. A validação final é do build e do linker C.

### 4.4 Resolução de operações

#### 1. Sintaxe

```keel
buffer.length(b)
slice.of(b)
buffer.clone(a, s)
slice.from(i32, p, n)
arena.alloc(a, i32, n)
```

Uma chamada qualificada tem a forma `m.f(argumentos)`, com `m` módulo ou alias ativo, e pode aparecer dentro de expressões. A assinatura determina quais argumentos são tipos, contêineres ou expressões C. Um argumento de tipo ocupa a posição de um parâmetro declarado `type X`:

```keel
module keel.arena;
pub T *alloc(arena *a, type T, size_t n);      // erased

module keel.slice type T;
pub slice from(type T, T *p, size_t n);        // selects the instance
```

**Reconhecimento**

A resolução aplica, nesta ordem:

1. Verbo do tipo conhecido do contêiner, na aridade escrita.
2. Função do módulo qualificador, na aridade escrita.
3. Diagnóstico de qualificador incompatível, se o verbo pertence a outro módulo; caso contrário, emissão da chamada qualificada para validação C.

A posição de contêiner exige a produção `container` da §2.2 e informação de símbolos. Uma chamada C desconhecida não fornece essa informação. Um verbo que o genérico declara e a instância escrita não admite (§4.3) não segue para a validação C: é `verb-not-in-instance`, e a mensagem nomeia o argumento que o removeu.

#### 2. Regras

Resolver chamadas keel a partir de módulos, assinaturas e identidades declaradas.

1. Verbos produtores, como `of`, `from` e `clone`, são qualificados pelo módulo do produto; os demais, pelo módulo do contêiner receptor. O envelope `outcome` não muda o qualificador: `buffer.clone` produz `outcome buffer T`.
2. Operações do núcleo sobre `array` são qualificadas por `keel`.
3. A aridade é contada sintaticamente, sem sobrecarga por tipos C. Um módulo pode declarar o mesmo verbo em aridades diferentes. Um argumento de tipo conta como um argumento.
4. A origem da instância é determinada pela tabela abaixo. O tipo de expressões C não é deduzido para completar argumentos ausentes.

   | Origem da instância | Operações da base |
   | --- | --- |
   | Objeto no primeiro argumento | Caso geral, incluindo todos os verbos de `outcome` |
   | Tipo escrito em parâmetro `type` de seleção | `slice.from(T,p,n)` |
   | Contêiner em outro argumento | `buffer.clone(a,x)`; `slice.clone(a,x)` |
   | Tipo do alvo declarado, atribuído ou retornado | `buffer.from(p,cap)` |

5. A adaptação de argumentos usa a forma declarada do parâmetro e do símbolo:

   | Parâmetro | Argumento conhecido | Emissão |
   | --- | --- | --- |
   | Ponteiro | Valor `x` | `&x` |
   | Ponteiro | Ponteiro `x` | `x` |
   | Valor | Valor `x` | `x` |
   | Valor | Ponteiro `x` | `*x` |

6. A adaptação também vale para função de módulo com parâmetro declarado como ponteiro para instância. Os demais argumentos seguem como escritos.
7. O objeto é escrito sem operador de endereço; o `&` vem da adaptação. Escrever `&` sobre o objeto é `address-in-object-position`.
8. Cada argumento de operação é avaliado uma vez. A ordem relativa entre argumentos é a da chamada C, salvo regra explícita de outra construção.
9. Operações `of` usam informação de uma origem conhecida; `from` recebe propriedades afirmadas pelo programa. Cada contrato define o modo de falha.

##### Parâmetro `type`

10. A espécie de um parâmetro `type X` é decidida na declaração da função, e não na chamada.
11. Se `X` é parâmetro `type` da linha `module`, o parâmetro é de **seleção**: o tipo escrito na chamada fixa esse argumento da instância e não é passado ao C. Não é `parameter-name-reuse`.
12. Um verbo com parâmetro de seleção declara um parâmetro `type` para cada parâmetro da linha `module`. Um módulo com `dim` ou `tags` na linha `module` não declara verbo de seleção.
13. Se `X` não é parâmetro do módulo, o parâmetro é **apagado**. A função é uma só no C, sem instância, identidade ou nome derivado de `X`.
14. Um parâmetro apagado é emitido como dois parâmetros `size_t`, tamanho e alinhamento, na posição escrita. Na chamada, keel escreve `sizeof` e `alignof` do tipo escrito.
15. No corpo da função, `sizeof(X)` e `alignof(X)` designam esses dois parâmetros. Não são constantes de tradução.
16. `X *` é admitido no retorno e nos parâmetros da assinatura, e é emitido `void *`. Um retorno `X *` chega ao ponto de chamada convertido para ponteiro ao tipo escrito.
17. Um parâmetro apagado só aparece no corpo em `sizeof(X)` e `alignof(X)`, fora de dimensão de vetor.
18. O nome de um parâmetro apagado não nomeia tipo conhecido no escopo.
19. Uma função pode ter parâmetros das duas espécies; cada um segue a sua regra.

Referências: [Rationale: resolução por declaração](keel-rationale.md#resolução-por-declaração); [Rationale: parâmetro de tipo em função](keel-rationale.md#parâmetro-de-tipo-em-função); [Backend: contêineres e funções](keel-c-backend.md#52-containers-struct-e-funções-static-inline) e [parâmetro `type`](keel-c-backend.md#516-parâmetro-type).

#### 3. Exemplo

```keel
//keel
buffer.length(b);
buffer.length(p);
```

Para `b` declarado `buffer i32` e `p` declarado `buffer i32 *`:

```c
//C gerado
keel_buffer_i32_length(&b);
keel_buffer_i32_length(p);
```

#### 4. Erros

De keel, na tradução; condições no [catálogo](#62-catálogo): `not-a-container-expression`, `instance-field-access`, `address-in-object-position`, `wrong-qualifier`, `verb-not-in-instance`, `from-without-target`, `partial-instance-selection`, `type-param-outside-size`, `type-param-shadows-type`. Tipos ou argumentos C incompatíveis depois da resolução são diagnosticados pelo compilador C.

#### 5. Casos especiais

- A resolução não escolhe alocador, não calcula extensões externas e não infere propriedade nem tempo de vida.
- O programa garante os argumentos exigidos pelo verbo.
- Acesso direto a campos não tem estabilidade de layout.

### 4.5 Indexação e `range-index`

#### 1. Sintaxe

```keel
x[i]
x[i,j]
x[a..b]
x[a..]
x[..b]
x[..]
```

São formas de expressão sobre `container`. Índices e limites são regiões C opacas. O `range-index` usa uma dimensão; índices por intervalo multidimensionais são operações dos módulos que os implementam.

**Reconhecimento**

- O símbolo ou caminho fornece a identidade do contêiner. Em `array`, a declaração fornece a quantidade de dimensões; em modificador, a assinatura do verbo fornece as aridades aceitas.
- Colchetes sobre símbolos C desconhecidos permanecem C, inclusive seu operador vírgula. A presença de vírgulas não registra um `array`.
- `..` dentro dos colchetes distingue `range-index` de acesso a elemento.

#### 2. Regras

Acessar elementos e delimitar fatias a partir de contêineres conhecidos.

1. `x[i]` equivale a `*ptr(x,i)`: é lvalue e designa o elemento original. `x[i,j,...]` chama `ptr` com os índices escritos. `dim` não gera essa assinatura nem determina sua aridade.
2. Sobre `array`, a forma de vários índices traduz para colchetes C sucessivos, com todos os índices, no layout do vetor multidimensional C.
3. Sobre coluna de `extent`, a indexação segue a §4.11.
4. Cada índice é avaliado uma vez. A tradução não impõe ordem adicional entre expressões que o C não ordena.
5. `x[a..b]` chama o verbo de `range-index` declarado pelo lado **memória** do par memória/visão a que `x` pertence, e nunca pelo lado visão, e produz um descritor por valor. Sobre a base, é `buffer.as_slice(x,a,b)` quando `x` é `buffer`, e `slice.of(x,a,b)` quando `x` é `slice`.
6. `x[..b]` fornece início zero; `x[a..]` fornece `length(x)` como fim; `x[..]` usa a forma de um argumento.
7. O nome do verbo de `range-index` é do módulo do contêiner. A única exigência deste contrato é a direção: memória para visão.
8. O `range-index` é rvalue. Um elemento da vista segue o contrato do elemento.
9. Na forma `x[a..]`, o contêiner é usado duas vezes, e o caminho só admite identificadores, `.`, `->`, `*`, `&` e parênteses.
10. As verificações de índice e limite são de debug. Um `range-index` exige `a <= b <= length(x)`; o trecho vazio é permitido.
11. Sobre `array`, cada índice é verificado contra a dimensão declarada correspondente. Quando índice e dimensão são decimais conhecidos, a verificação é da tradução e recusa; nos demais casos é de execução em perfil debug.
12. Na dimensão 0 de um parâmetro `array`, o número declarado é o contrato, e não a extensão do vetor que o chamador entregou.
13. `inverted-range-index` e `array-index-above-dimension` admitem literais e `constexpr` de valor decimal conhecido. Não calculam expressões C.

Referências: [Rationale: acesso e travessia](keel-rationale.md#acesso-e-travessia); [Rationale: memória e visão](keel-rationale.md#memória-e-visão-a-direção-da-conversão); [Backend: açúcar de indexação](keel-c-backend.md#53-açúcar-de-indexação).

#### 3. Exemplo

```keel
//keel
x[i++] = 7;
```

Para `x` declarado `buffer i32`:

```c
//C gerado
*keel_buffer_i32_ptr(&x, i++) = 7;
```

O incremento ocorre uma vez e a escrita alcança o armazenamento original.

#### 4. Erros

Condições no [catálogo](#62-catálogo). De keel, na tradução: `partial-array-index`, `array-index-above-dimension`, `no-ptr-for-arity`, `no-range-index-verb`, `inverted-range-index`, `open-range-index-on-complex-path`. Do backend, em execução debug: `array-index-out-of-bounds`, `range-index-out-of-bounds`.

#### 5. Casos especiais

- O programa garante os limites também em release, mantém o armazenamento válido e respeita qualificadores.
- O açúcar de indexação não tem o resultado falível de `at`; quem precisa dele escreve o verbo.

### 4.6 Cleanup léxico

#### 1. Sintaxe

```keel
defer release(p);
defer { close(a); close(b); }
defer [later] release(p);
defer [now int fd, FILE *out] { report(out); close(fd); }
```

`defer` é statement de corpo de função, dentro de bloco explícito. Sem lista, a captura é `later`. Em `[now ...]`, cada entrada tem tipo escrito e nome imediatamente antes de `,` ou `]`; a lista é declarativa, sem inferência de tipos.

**Reconhecimento**

- O corpo adiado é um statement ou bloco balanceado. keel localiza seu escopo, os símbolos capturados e os pontos de saída reconhecidos.
- A reconstrução de capturas e de temporários de retorno usa os tokens dos tipos escritos, sem consultar o tipo de uma expressão C retornada.
- `return`, `break`, `continue` e destinos de `goto` são reconhecidos pela estrutura de statements e escopos.

#### 2. Regras

Registrar operações de limpeza para as saídas de um escopo léxico.

1. A saída de um escopo executa seus registros aplicáveis em ordem inversa. Escopos internos são limpos antes dos externos.
2. O cleanup é emitido nos pontos de saída, sem pilha de registros em execução.
3. Os pontos de saída são o fim natural do bloco, `return` e saltos que deixam o escopo. Uma saída anterior ao registro não executa aquele cleanup.
4. Um registro no corpo de laço é limpo na saída de cada iteração.
5. `later` consulta os valores na saída. `[now]` copia as entradas no registro, e o corpo usa essas cópias; copiar um ponteiro não copia os dados.
6. Em função com retorno não `void`, `return expr;` avalia `expr` uma vez num temporário do tipo de retorno escrito, executa o cleanup e retorna o temporário.
7. Em função `void`, a expressão de `return`, quando escrita, é avaliada antes do cleanup, seguida de `return;`, sujeita às regras C.
8. Saídas introduzidas por construções keel também executam o cleanup dos escopos que deixam. Uma consulta de estado que não sai de um escopo não dispara limpeza.
9. Salto externo não entra em escopo por cima de um registro. `case` ou `default` posterior a `defer` no mesmo corpo de `switch` é essa entrada; um bloco próprio por caso delimita o registro.

Referências: [Rationale: limpeza e saídas](keel-rationale.md#limpeza-e-saídas); [Backend: defer](keel-c-backend.md#55-defer).

#### 3. Exemplo

O [exemplo 2 do README](README.md#2-retorno-e-cleanup) mostra a avaliação do retorno antes da chamada de limpeza.

#### 4. Erros

De keel, na tradução; condições no [catálogo](#62-catálogo): `defer-without-braces`, `defer-at-file-scope`, `jump-over-defer`, `later-with-capture`, `defer-in-control-block`, `hidden-declarator`, `defer-later-shadowed`.

#### 5. Casos especiais

- O programa garante que os recursos referidos pelo corpo adiado continuam válidos até a execução. keel não deduz aquisição, liberação nem propriedade.
- Saídas não locais de C, como `longjmp`, não recebem cleanup.
- O programa garante que macros não escondem saídas que a tradução precisa reconhecer.
- A análise de registro e saída é lexical: não interpreta o fluxo C nem acompanha aquisições em execução.

### 4.7 Travessia sequencial

#### 1. Sintaxe

```keel
foreach (i32 valor, size_t i : xs) { use(valor, i); }
foreach (i32 *p, size_t i : xs) { *p += 1; }
foreach (auto i : 0..n) { use_index(i); }
apply(i32, xs, use);
apply(i32 *, xs, visit, context);
walk (i32 *p, buffer.cursor c : xs) { use(*p); }
```

São statements de corpo de função. Em `foreach`, dois binders antes de `:` pedem travessia de contêiner por índice; um binder pede intervalo ou objeto contável. Em `apply`, o primeiro argumento é o tipo do binder sem nome, com `*` quando a operação recebe ponteiro; os argumentos depois da função são de contexto. Em `walk`, o primeiro binder recebe o elemento e o segundo declara o cursor, com o tipo escrito e qualificado pelo módulo do contêiner.

**Reconhecimento**

- A quantidade de binders distingue as formas de `foreach`, sem consultar tipos C.
- Dois binders exigem `length` e `get`, para valor, ou `length` e `ptr`, para ponteiro. Um binder exige `first` e `limit`, ou um literal `a..b`.
- `walk` tem uma forma só, de dois binders. A forma de um binder é reconhecida e recusada por `walk-without-cursor`.
- `walk` exige `begin`, `has_next` e `next`. O tipo do cursor escrito é o produto declarado de `begin`; o tipo do elemento é o produto declarado de `next`. A compatibilidade final é verificada pelo compilador C.
- Indexável e percorrível por cursor são capacidades independentes (§5.1): `foreach` e `walk` pedem cada um a sua.

#### 2. Regras

Percorrer elementos ou valores contáveis em ordem sequencial, por índice ou por cursor.

##### `foreach` e `apply`

1. O contêiner e seu comprimento são avaliados uma vez, na entrada. O índice percorre de zero até esse comprimento, excluído, em ordem crescente.
2. O binder por valor recebe uma cópia do elemento a cada iteração; por ponteiro, recebe seu endereço. Ambos ficam no escopo do corpo.
3. Na forma contável, início e limite são obtidos uma vez, e percorre-se `[first,limit)`. O binder recebe o próprio contador a cada passo, sem verbo de acesso por posição. Um intervalo vazio executa zero iterações.
4. `auto` no binder de intervalo é traduzido como `size_t`. Os demais tipos escritos seguem para o compilador C.
5. `range` nomeado admite dois binders, com valor e posição. O literal de intervalo admite só a forma de um binder.
6. `apply(T,x,f)` equivale à travessia de dois binders que chama `f(elemento,i)` em cada iteração. Os argumentos de contexto seguem elemento e índice na ordem escrita e são avaliados a cada chamada, segundo as regras C, sem captura nem dedução de tipo.

##### `walk`

7. `begin(x)` é avaliado uma vez, na entrada, e inicializa o cursor. O contêiner também é avaliado uma vez.
8. Cada iteração testa `has_next(x, cursor)` e, sendo verdadeiro, liga o elemento a partir de `next(x, cursor)`. O avanço do cursor pertence a `next`.
9. `next` é chamado uma vez por iteração, depois do teste.
10. O tipo do elemento é o que `next` declara: `T` atende o binder por valor, e `T *` o binder por ponteiro. Não há conversão nem seleção por sobrecarga.
11. O cursor é um objeto do programa, no escopo do corpo. Escrevê-lo é permitido, e as consequências pertencem ao contrato do módulo.
12. `walk` não exige `length`, `get` nem `ptr`.

##### Comum

13. `break`, `continue` e `return` conservam o significado C no laço resultante, com o cleanup da §4.6.
14. A travessia é linear. Escrever nos elementos não muda o comprimento; operações estruturais sobre o contêiner percorrido são recusadas.

Referências: [Rationale: acesso e travessia](keel-rationale.md#acesso-e-travessia); [Rationale: cursor explícito](keel-rationale.md#cursor-explícito); [Backend: foreach e apply](keel-c-backend.md#57-foreach-e-apply).

#### 3. Exemplo

O [exemplo 3 do README](README.md#3-trecho-fixo-elementos-mutáveis-e-intervalo) mostra o percurso de um intervalo nomeado. A travessia por cursor produz o laço abaixo:

```c
//C gerado
keel_buffer_cursor c = keel_buffer_i32_begin(&xs);
while (keel_buffer_i32_has_next(&xs, &c)) {
    i32 *p = keel_buffer_i32_next(&xs, &c);
    example_use(*p);
}
```

#### 4. Erros

De keel, na tradução; condições no [catálogo](#62-catálogo): `not-iterable`, `not-cursor-iterable`, `walk-without-cursor`, `cursor-type-mismatch`, `binder-copies-container`, `mutation-during-traversal`, `index-not-size-t`, `not-countable`, `foreach-two-binders-on-literal`, `pointer-binder-on-range`, `open-range-outside-index`.

#### 5. Casos especiais

- O programa garante que a sequência capturada na entrada continua válida e não é alterada por aliases ou chamadas opacas. A verificação local não segue os efeitos de funções C.
- O programa garante que as operações de seus tipos cumprem os próprios contratos de comprimento e acesso.
- Em `walk`, a terminação depende de `next` avançar o cursor até `has_next` ser falso. keel não prova essa propriedade.

### 4.8 Execução particionada

#### 1. Sintaxe

```keel
//keel
parallel update ALL (size_t w : 0..4; slice i32 part : xs) {
    foreach (i32 *p, size_t i : part) { *p += 1; }
}
if (parallel.failed(update)) handle();
```

A forma é `parallel nome politica (workers; particao [; (capturas)]) { corpo }`, em corpo de função. `workers` usa um binder sobre `0..k`. `particao` é um binder único que recebe a parte atribuída ao worker. Capturas são uma lista explícita de identificadores. A política é `ALL`, `ANY` ou constante `N` conhecida. `win;` e `fail;` são saídas do corpo do worker.

**Reconhecimento**

- O nome declara, no escopo que contém o bloco, um símbolo de tipo `parallel.control` (§5.7). Consultá-lo exige o import de `keel.parallel`; a construção não depende desse import.
- O nome é único na função, e não apenas no escopo. Em funções diferentes, o mesmo nome é livre.
- O binder de workers e as constantes seguem as regras de `foreach` e de `constexpr`.
- O contêiner particionado declara `partition` (§5.1). O tipo escrito no binder de partição é o produto declarado de `partition`; a compatibilidade final é verificada pelo compilador C.
- `parallel` seguido de `.` é qualificação de módulo (§2.3). A construção é `parallel` seguido do nome e da política.
- A captura é escrita pelo programa, e não deduzida dos identificadores do corpo. O registro de cada símbolo fornece sua forma declarada.
- `win` e `fail` são verbos de fluxo nesse corpo, inclusive dentro de travessias aninhadas, e não se confundem com as chamadas qualificadas dos módulos de resultado.

#### 2. Regras

Distribuir um contêiner em partes disjuntas, executar um corpo de worker sobre cada parte e resolver uma política de conclusão.

##### Partição

1. Com `k` workers, keel avalia `partition(x, k, w)` uma vez por worker, antes de entrar em seu corpo, e liga o resultado ao binder de partição.
2. O contêiner é avaliado uma vez, antes da distribuição.
3. `parallel` não exige `length`, `get` nem `ptr`. A parte entregue não precisa ser do mesmo tipo do todo.
4. Que as `k` partes sejam disjuntas e cubram o contêiner é contrato do módulo que declara `partition`; a divisão da base está na §5.3.
5. O binder de partição recebe a instância por valor quando o módulo assim a declara, como `slice T`, sem `binder-copies-container`. Um produto `byref` é entregue por ponteiro.

##### Execução e política

6. keel não fixa teto de workers. O número pedido define a divisão, sem garantir execução simultânea. A execução serial das partes em ordem crescente é permitida.
7. A escolha do lowering — série, OpenMP, pool de threads ou outro — pertence ao backend e ao perfil de compilação, e não altera esta seção.
8. `ALL`, representado por zero, pede que nenhuma parte falhe; `ANY`, por um, pede uma vitória; `N` pede `N` vitórias.
9. Só `win` conta para a política, e só `fail` conta contra ela. O fim natural do corpo do worker não é nenhum dos dois.
10. Sob `ANY` ou `N`, atingir o alvo de vitórias sinaliza interrupção para os demais workers; `fail` não sinaliza.
11. O bloco aguarda todos os workers antes de prosseguir.
12. Ao sinalizar interrupção, o controle ativa o flag, e `parallel.interrupted(n)` passa a devolver verdadeiro. A consulta não depende de outro worker já ter parado e não é, por si só, uma saída.
13. O símbolo de controle é inicializado antes da distribuição e vale enquanto existir o escopo que o contém. Dentro do corpo do worker, só `parallel.interrupted` tem leitura definida. Depois do bloco, as quatro consultas descrevem a execução terminada e não mudam mais.
14. Capturas escalares são cópias por worker; instâncias `byref` são passadas por ponteiro. Objetos de arquivo permanecem acessíveis segundo C.
15. `win` e `fail` deixam todos os escopos do worker, inclusive os de travessias aninhadas, e executam seus `defer`. Laços C escritos no corpo preservam seus `break` e `continue`.
16. Uma travessia escrita no corpo do worker segue a §4.7 sobre a parte, e não sobre o todo.

Referências: [Rationale: políticas e sinalização](keel-rationale.md#políticas-e-sinalização-de-interrupção); [Rationale: particionável e percorrível](keel-rationale.md#particionável-e-percorrível); [Backend: parallel](keel-c-backend.md#59-parallel).

#### 3. Exemplo

O exemplo da sintaxe admite a execução sequencial abaixo, omitidos símbolos auxiliares e consultas de resultado:

```c
//C gerado
keel_parallel_control update = {0};
for (size_t w = 0; w < 4; ++w) {
    keel_slice_i32 part = keel_buffer_i32_partition(&xs, 4, w);
    for (size_t i = 0; i < part.len; ++i) {
        i32 *p = &part.ptr[i];
        *p += 1;
    }
}
```

#### 4. Erros

De keel, na tradução; condições no [catálogo](#62-catálogo): `not-partitionable`, `partition-type-mismatch`, `mutation-during-traversal`, `unnamed-parallel`, `flow-verb-outside-parallel`, `nested-parallel`, `duplicate-parallel-name`, `nonconstant-parallel`, `captured-write`, `return-in-parallel`. O compilador C verifica nomes ausentes da captura e tipos incompatíveis.

#### 5. Casos especiais

- O programa não depende de simultaneidade, de ordem entre workers nem da quantidade de trabalho feita depois de um pedido de interrupção.
- A disjunção das partes não prova disjunção dos objetos alcançados pelos elementos. O programa garante a disciplina de acesso e a sincronização de ponteiros capturados, arenas e objetos compartilhados; keel não insere sincronização.
- Um pedido de interrupção não confirma que os workers já o trataram. Efeitos realizados antes da observação permanecem.
- As consultas do símbolo de controle não sincronizam: `interrupted` pode passar de falso a verdadeiro entre duas leituras do mesmo worker.
- A política de `parallel` não usa os ciclos cooperativos de `routine.par` (§5.6).

### 4.9 Conjuntos de tags e despacho

#### 1. Sintaxe

```keel
pub tags Cycle [WAIT, END];
pub tags Status [SUCCESS = -1, ONGOING = 0, FAILED = 1];

tagged Cycle void state = {0};
tagged Kind struct Node no = {0};
Cycle step = Cycle.WAIT;

match (state) {
    WAIT:
        prepare(ctx);
    END:
        finish(ctx);
}

match (step) {
    WAIT: prepare(ctx);
    END:  finish(ctx);
}
```

`tags` é declaração de arquivo ou de bloco; em arquivo, sua visibilidade segue `pub`/`priv`. O nome declarado é tipo e argumento de tipo. Como tipo, declara variáveis, parâmetros, campos e colunas de `extent` (§4.11). Como argumento, `tagged Cycle void` aplica o modificador `tagged` (§5.4) ao conjunto `Cycle` e ao tipo associado `void`.

Os valores das tags são opcionais. Quando presentes, são literal decimal com sinal opcional ou constante nomeada conhecida nas condições da §4.2, e um conjunto não mistura tags com e sem valor escrito. Uma constante de tag é escrita com o nível do conjunto, `Cycle.END`, quando o nome não está injetado no arquivo. Dentro de um `match`, os rótulos são escritos sem qualificação.

**Reconhecimento**

- `tags` em linha `module` introduz parâmetros do módulo; em posição de declaração, o nome é seguido de `[` e da lista.
- `match` é seguido de `(`; o operando ocupa posição de contêiner. O corpo é um bloco de rótulos.
- Dentro do corpo, os rótulos externos identificam tags, e cada braço abre um escopo até o próximo rótulo ou até a chave final. Rótulos consecutivos sem statements entre eles compartilham o braço seguinte.
- O tipo declarado do operando é um conjunto `tags`, ou um tipo que declara a operação `tag` (§5.1). keel não deduz o conjunto de uma expressão C arbitrária.
- O conjunto exaustivo vem de um de três lugares, nesta ordem: o tipo declarado do operando, quando é um conjunto; o argumento `tags` escrito na instância, quando o tipo é instância de modificador cujo módulo tem parâmetro `tags` (em `tagged Cycle void`, `Cycle`); ou o conjunto declarado pelo módulo do operando, como em `corot`.
- No terceiro caso, o módulo declara exatamente um conjunto. Com dois, é `ambiguous-match-tags`; com nenhum, é `match-without-tags`.
- A verificação de exaustividade alcança os rótulos presentes antes do pré-processamento C. Rótulos ocultos por macro não satisfazem o contrato.

#### 2. Regras

Declarar conjuntos fechados de etiquetas e despachar o controle pela etiqueta corrente de um valor.

##### Conjuntos de tags

1. Um conjunto `tags` é traduzido como um `enum` C nomeado pela identidade do módulo. Suas constantes seguem a qualificação por tipo da §4.2.
2. Sem valores escritos, as tags recebem ordinais a partir de zero, na ordem escrita. Valores escritos são conservados sem normalização.
3. O conjunto é fechado: seus nomes são conhecidos na tradução.
4. Um objeto declarado com o tipo do conjunto tem a representação do `enum`.
5. Um `enum` C não é conjunto, ainda que escrito num módulo keel. Constantes vindas de `extern_c` ou de header também não formam conjunto.

##### `match`

6. O operando é avaliado uma vez, antes da seleção.
7. A operação seleciona o braço cuja tag corresponde à etiqueta do operando e executa seu corpo. A etiqueta é o próprio valor, quando o tipo do operando é o conjunto, ou o resultado de `tag(operando)`.
8. Cada braço abre seu próprio escopo. Chegar ao fim do corpo de um braço encerra o despacho, sem executar o braço seguinte.
9. `break;` no nível do braço encerra o despacho, com o cleanup dos escopos que deixa. Um `break` que pertence a laço ou `switch` escrito no braço conserva o significado C.
10. O corpo do braço não é envolvido por um `switch` gerado: o despacho escolhe um rótulo, e os corpos ficam fora dele.
11. A mudança de etiqueta é uma escrita comum e não causa novo despacho até que o controle execute outro `match`.
12. `match` não acrescenta laço em torno do bloco e não produz valor. Um ponto de saída escrito no corpo pertence à função que contém a estrutura.

##### Exaustividade

13. Todo nome da lista declarada tem rótulo no corpo, e todo rótulo pertence à lista.
14. A verificação é sobre nomes, e não sobre valores: duas tags com o mesmo valor escrito exigem rótulos distintos.
15. Não há braço padrão. O `default` gerado no C existe apenas para a verificação de debug.

Referências: [Rationale: conjuntos fechados e exaustividade](keel-rationale.md#conjuntos-fechados-e-exaustividade); [Rationale: estrutura de controle e máquina completa](keel-rationale.md#estrutura-de-controle-e-máquina-completa); [Backend](keel-c-backend.md): §§5.6 e 5.10, para despacho e rótulos; §5.5, para cleanup.

#### 3. Exemplo

```keel
//keel
module ast;
import keel.tagged as tagged types;
import keel.buffer as buffer types;

struct Node { u32 a, b; };
pub tags Kind [LIT, ADD, MUL];

pub void eval(tagged Kind struct Node *n) {
    match (n) {
        LIT:
            leaf(tagged.value(n));
        ADD:
        MUL:
            binary(tagged.value(n));
    }
}
```

```c
//C gerado
typedef enum { ast_Kind_LIT, ast_Kind_ADD, ast_Kind_MUL } ast_Kind;

struct ast_Node { u32 a, b; };

typedef struct {
    int32_t tag;
    struct ast_Node value;
} keel_tagged_ast_Kind_ast_Node;

void ast_eval(keel_tagged_ast_Kind_ast_Node *n) {
    switch (n->tag) {
    case ast_Kind_LIT: goto keel__m0_LIT;
    case ast_Kind_ADD: goto keel__m0_ADD;
    case ast_Kind_MUL: goto keel__m0_MUL;
    default:      goto keel__m0_end;
    }
    keel__m0_LIT: { ast_leaf(n->value); }
    goto keel__m0_end;
    keel__m0_ADD:
    keel__m0_MUL: { ast_binary(n->value); }
    keel__m0_end: ;
}
```

`ADD` e `MUL` compartilham um braço porque os rótulos são consecutivos; chegar ao fim do braço encerra o despacho.

#### 4. Erros

Condições no [catálogo](#62-catálogo). De keel, na tradução: `unnamed-tags`, `duplicate-tags-name`, `duplicate-tag`, `partial-tag-values`, `nonconstant-tag-value`, `empty-tags`, `match-without-tag`, `match-without-tags`, `ambiguous-match-tags`, `tag-not-in-set`, `tag-without-label`, `tag-from-other-set`, `enum-constant-without-type`. Do backend, em execução debug: `tag-out-of-range`.

#### 5. Casos especiais

- O programa garante que a etiqueta armazenada pertence ao conjunto. keel não o prova; a verificação é de debug.
- Onde `match` é recusado, o programa pode escrever `switch`.

### 4.10 Tratamento de resultado

#### 1. Sintaxe

```keel
outcome i16 r = produce() else 0;
outcome i16 s = produce() else return -1;
outcome i16 t = produce() else { handle(t); }
r = produce() else break;
```

A cláusula `else` é cauda de declaração com um único declarador e inicializador, ou de atribuição simples a um identificador já declarado. A primeira forma usa default; as outras executam o tratamento escrito quando o resultado falha.

**Reconhecimento**

- O protocolo exige `failed`. A forma de default exige também `win`, na forma que recebe o resultado e o valor de default. A presença de `ongoing` não exclui o tipo.
- O tipo do símbolo vem de sua declaração escrita; o tipo de uma expressão C não é deduzido.
- Na forma de atribuição, o alvo é um identificador simples de declaração keel conhecida. Campo, índice, deref, cast e símbolo C desconhecido são `else-on-complex-target`.
- O `else` do `if` pertence à gramática de controle C; o `else` de resultado pertence à declaração ou atribuição reconhecida. Depois dele, `{` ou uma palavra de salto C (`return`, `break`, `continue`, `goto`) identifica tratamento; os demais inícios identificam expressão de default.
- Expressões de default e corpos de tratamento são C opaco, com reconhecimento normal das construções keel e dos pontos de saída.

#### 2. Regras

Testar um resultado falível no ponto em que ele é produzido, e tratá-lo com uma saída escrita ou com um valor default.

1. O inicializador, ou o lado direito da atribuição, é avaliado uma vez e armazenado no símbolo.
2. A cláusula consulta `failed` desse resultado. Não aplica comparação numérica.
3. Se `failed` é falso, o tratamento ou o default não é executado.
4. Se `failed` é verdadeiro, a forma de tratamento executa o statement ou bloco escrito. Não converte erros, não extrai o valor e não exige que o bloco saia do escopo ou repare o resultado.
5. Na forma de default, a expressão é avaliada somente na falha, e a tradução chama `win(resultado, default)` sobre o próprio símbolo.
6. A declaração conserva seu tipo de resultado; a extração é uma chamada escrita, como `outcome.value(r)`. `i16 res = outro else 0;`, com `outro` de tipo `outcome i16`, é recusado: `i16` não participa do protocolo.
7. Saídas escritas no tratamento seguem os contratos da região em que ocorrem, inclusive `defer`.

Referências: [Rationale: resultados finais e estados cooperativos](keel-rationale.md#resultados-finais-e-estados-cooperativos); [Backend](keel-c-backend.md): §5.12 para `else`, §5.5 para cleanup.

#### 3. Exemplo

O [exemplo 4 do README](README.md#4-resultado-com-default-e-extração-explícita) mostra a forma de default sobre o próprio objeto e a extração explícita.

#### 4. Erros

De keel, na tradução; condições no [catálogo](#62-catálogo): `else-without-initializer`, `else-on-infallible-type`, `else-on-complex-target`, `else-multiple-declarators`, `else-default-without-win`. Argumento, retorno ou atribuição com tipos C incompatíveis são diagnosticados pelo compilador C.

#### 5. Casos especiais

- O programa garante que `failed` e `win` de seus tipos cumprem o protocolo: `failed` indica falha ou ausência de resultado final, e `win(resultado, default)` deixa `failed` falso. keel não prova essa propriedade.
- A conversão entre códigos e resultados de funções diferentes é escrita pelo programa.

### 4.11 `extent`

#### 1. Sintaxe

```keel
constexpr size_t MAX = 1024;
constexpr size_t H = 480;
constexpr size_t W = 640;

extent struct position [len, MAX] {
    array f32 x[MAX];
    array f32 y[MAX];
    size_t len;
};

extent struct particles [len, cap] {
    array f32 *x;
    array f32 *y;
    bool active;
    size_t len, cap;
};

extent struct image [h, H] [w, W] {
    array u8 r[H, W];
    array u8 g[H, W];
    size_t h, w;
};

struct particles ps;
ps.x[i] = 0;
img.r[i, j] = 255;
```

`extent struct NOME` é declaração de arquivo. Cada grupo `[contagem, capacidade]` descreve uma dimensão, da externa para a interna. Um campo marcado `array` é uma coluna; os demais campos são comuns. O tipo é usado como `struct NOME`, sem marca no ponto de uso.

**Reconhecimento**

- A contagem de cada grupo nomeia um campo do struct.
- A capacidade de cada grupo nomeia um campo do struct. Se nenhum campo tem esse nome, ela nomeia um `constexpr` conhecido ou é literal decimal.
- O rank é a quantidade de grupos.
- Uma coluna embutida, `array T col[D₀, …]`, tem uma dimensão por grupo. A dimensão `k` é, pelo nome, a capacidade do grupo `k`; valores não são comparados.
- Uma coluna por ponteiro, `array T *col`, não escreve dimensões e tem o rank da declaração. O último `*` é a coluna; o elemento é `T` com os `*` restantes.
- Colunas embutidas exigem capacidade constante em todos os grupos.
- Colunas embutidas e por ponteiro não se misturam na mesma declaração.
- `P.col[i₀, …]` e `P->col[i₀, …]` são acesso de coluna quando `col` é coluna de um `extent` e `P` segue a produção `container` (§2.2) sem chamada.
- `P.col` sem índice é o campo C.

#### 2. Regras

Declarar um struct cujas colunas compartilham um controle de extensão, e verificar o acesso às colunas contra esse controle.

1. A declaração emite o struct como escrito. A coluna embutida é vetor C multidimensional (§4.2); a coluna por ponteiro é ponteiro. keel não insere campo.
2. O acesso `P.col[i₀, …, iᵣ₋₁]` equivale a `*a(&P, i₀, …, iᵣ₋₁)`, com `a` a função de acesso que o núcleo emite para a coluna. É lvalue e designa o elemento.
3. O caminho `P` e cada índice são avaliados uma vez. A tradução não impõe ordem adicional entre eles.
4. Na coluna embutida, o elemento é `col[i₀]…[iᵣ₋₁]`.
5. Na coluna por ponteiro, o elemento é `col[(…(i₀·c₁ + i₁)·c₂ + …)·cᵣ₋₁ + iᵣ₋₁]`, com `cₖ` a capacidade do grupo `k`. A capacidade externa `c₀` não entra no endereço.
6. O número de índices escritos é o rank.
7. Quando um índice e a capacidade do seu grupo são decimais conhecidos, índice maior ou igual à capacidade é recusado na tradução. A contagem nunca é conhecida na tradução.
8. Em execução debug, cada índice escrito é verificado: `iₖ < contagemₖ && contagemₖ <= capacidadeₖ`.
9. Em release, não há verificação.
10. keel não gera verbo e não inicializa, aloca ou mantém contagem e capacidade.
11. A interface de um `extent` público exporta os grupos, as colunas e as funções de acesso. O acesso de um importador segue as mesmas regras.
12. Um `extent` pode ser campo de outro struct ou elemento de contêiner; o acesso por caminho (`w.pos.x[i]`) segue as mesmas regras.
13. Uma coluna cujo elemento é um conjunto `tags` é operando de `match` (§4.9).

Referências: [Rationale: `extent`, da recusa à admissão](keel-rationale.md#extent-da-recusa-à-admissão); [Backend: `extent`](keel-c-backend.md#515-extent).

#### 3. Exemplo

```keel
//keel
module game;

extent struct particles [len, cap] {
    array f32 *x;
    size_t len, cap;
};

bool push(struct particles *p, f32 v) {
    if (p->len == p->cap) return false;
    size_t i = p->len++;
    p->x[i] = v;
    return true;
}
```

```c
//C gerado
struct game_particles { f32 *x; size_t len, cap; };

static inline f32 *game_particles_x_ptr(struct game_particles *p, size_t i) {
    return &p->x[i];
}

bool game_push(struct game_particles *p, f32 v) {
    if (p->len == p->cap) return false;
    size_t i = p->len++;
    *game_particles_x_ptr(p, i) = v;
    return true;
}
```

Em execução debug, a função de acesso verifica `i < p->len && p->len <= p->cap` antes de devolver o endereço.

#### 4. Erros

Condições no [catálogo](#62-catálogo). De keel, na tradução: `extent-count-not-field`, `extent-unknown-capacity`, `extent-without-column`, `extent-mixed-columns`, `extent-embedded-field-capacity`, `extent-dimension-mismatch`, `extent-index-arity`, `extent-index-above-capacity`, `extent-path-with-call`. Do backend, em execução debug: `extent-index-out-of-bounds`.

#### 5. Casos especiais

- O programa garante cada contagem menor ou igual à sua capacidade, também em release.
- O programa garante que uma coluna por ponteiro aponta para pelo menos o produto das capacidades, em elementos.
- Um elemento só é acessível abaixo da contagem: para acrescentar uma linha, o programa incrementa a contagem antes de escrever nela.
- Mudar uma capacidade interna de coluna por ponteiro muda o endereço dos elementos; reorganizar o armazenamento é do programa.
- Aritmética sobre a coluna (`P.col + i`) e acesso fora da forma reconhecida não são verificados.

## 5. Protocolos e módulos da base

Os módulos deste capítulo acompanham a distribuição de keel e são módulos
comuns: declaram tipos, modificadores e verbos pelas regras dos §§4.1–4.4, e
são importados explicitamente. Os contratos das §§5.2 a 5.7 seguem o formato
de cinco itens do capítulo 4.

### 5.1 Protocolos

#### Verbos exigidos por construção

Um protocolo é o conjunto de verbos que uma construção do núcleo exige do tipo sobre o qual opera. Toda construção procura seus verbos por nome e aridade, pela resolução da §4.4; nenhuma consulta uma lista de tipos privilegiados, e a verificação não se restringe aos módulos da base.

| Protocolo | Verbos exigidos | Construção que o consome | Declarado na base por |
| --- | --- | --- | --- |
| Indexável | `length`, e `get` ou `ptr` conforme o binder | `foreach` de dois binders, `apply`, `x[i]` | `buffer`, `slice` |
| Contável | `first`, `limit` | `foreach` de um binder | `range` |
| Percorrível por cursor | `begin`, `has_next`, `next` | `walk` | `buffer`, `slice` |
| Particionável | `partition` | `parallel` | `buffer`, `slice`, `range` |
| Indexável por intervalo | o verbo que o próprio módulo declara, na aridade escrita | `x[a..b]` e suas formas abertas | `buffer`, por `as_slice`; `slice`, por `of` |
| Etiquetado | `tag` | `match` | `tagged`, `corot` |
| Falível | `failed`, mais `win` para a forma de default | `else` | `outcome` |

`array` participa de Indexável e de Indexável por intervalo pelas operações do núcleo qualificadas por `keel` (§4.2, §4.4). Um valor cujo tipo é um conjunto `tags` é operando de `match` sem verbo (§4.9).

#### O que um módulo do programa declara

1. Declarar os verbos de um protocolo basta para participar da construção correspondente. Não há registro, marcação nem permissão; keel não distingue módulo da base de módulo do programa ao resolver.
2. A forma dos argumentos vem do bit `byref` do modificador (§4.3), e não do protocolo.
3. As aridades escritas nas assinaturas são as que valem: `ptr` declarado em duas aridades serve a `x[i]` e a `x[i,j]`.
4. O nome do verbo de Indexável por intervalo é do módulo que o declara, e não do protocolo. [R: memória e visão](keel-rationale.md#memória-e-visão-a-direção-da-conversão)

#### O que o núcleo conhece pelo nome

Fora dos protocolos, três pontos ligam uma construção a um módulo determinado. A lista é fechada:

| Onde | O que o núcleo assume |
| --- | --- |
| `arena` | quatro verificações da tradução nomeiam este módulo: procedência do retorno, origem de `from_array`, constância de `from_stack` e uso de filha depois do reset do pai; e a definição sem inicializador recebe `= {0}` (§5.2) |
| `a..b` | o literal de intervalo produz um `range` (§5.3) |
| `parallel` | o nome do bloco declara um símbolo de tipo `parallel.control` (§5.7) |

O módulo `keel`, do prelúdio, não declara operação: declara os nomes de tipo primitivos, cuja grafia participa das regras da §4.2.

#### Convenção de grafia

A convenção abaixo é a da base; um módulo do programa pode segui-la ou não.

| O que a base declara | Grafia |
| --- | --- |
| tipos, modificadores e verbos | minúscula — `arena`, `slice`, `corot`, `slot`, `has_next` |
| conjuntos de tags | maiúscula inicial — `Status` |
| constantes de tag e de enum | caixa alta — `SUCCESS`, `ONGOING`, `FAILED` |

#### Inventário

| Módulo | O que declara | Contrato |
| --- | --- | --- |
| `keel.arena` | o tipo `arena`, seus construtores e verbos | §5.2 |
| `keel.buffer`, `keel.slice`, `keel.range` | as sequências, o trecho, o intervalo, e os protocolos de acesso, cursor e partição | §5.3 |
| `keel.tagged` | o modificador que associa etiqueta a valor | §5.4 |
| `keel.outcome`, `keel.corot` | o modificador de resultado final, de dois estados, e o tipo de estado cooperativo, de três | §5.5 |
| `keel.routine` | a tabela de participantes e as composições `seq` e `par` | §5.6 |
| `keel.parallel` | o tipo do símbolo de controle e suas consultas | §5.7 |

### 5.2 `keel.arena`

#### 1. Sintaxe

```keel
import keel.arena as arena types;

arena a;
arena.from_memory(a, p, n);
i32 *dados = arena.alloc(a, i32, n);
```

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `arena.from_array(a, v)` | `bool` | usa como região o `array u8` `v` |
| `arena.from_stack(a, N)` | `bool` | cria no escopo corrente um vetor automático de `N` bytes e o usa como região |
| `arena.from_parent(a, pai, n)` | `bool` | reserva `n` bytes em `pai` e os usa como região da filha |
| `arena.from_memory(a, p, n)` | `bool` | usa como região os `n` bytes externos em `p` |
| `arena.alloc(a, T, n)` | `T *` | reserva `n` objetos de `T`; `NULL` sem espaço |
| `arena.alloc(a, n, sz, al)` | `void *` | reserva `n` objetos de `sz` bytes, em endereço múltiplo de `al`; `NULL` sem espaço |
| `arena.mark(a)` | `size_t` | topo atual |
| `arena.restore(a, m)` | — | volta ao topo `m` |
| `arena.reset(a)` | — | volta ao topo zero |
| `arena.length(a)` | `size_t` | bytes ocupados |
| `arena.capacity(a)` | `size_t` | capacidade total, em bytes |

- `arena` é tipo, e não modificador: `typedef struct { … } arena;`, emitido como `keel_arena`.
- Os verbos recebem o descritor por referência (§4.4).
- `from_stack` só aparece em corpo de função, e `N` é literal ou `constexpr` conhecido.
- `from_array` exige símbolo conhecido como `array u8`.

#### 2. Regras

Armazenamento por região, que delimita a validade dos dados derivados.

1. Toda definição de objeto `arena` sem inicializador escrito recebe `= {0}`, em arquivo ou em bloco, inclusive `static` e vetor de `arena`.
2. A arena vazia tem capacidade zero, e toda alocação nela falha.
3. Os construtores devolvem verdadeiro quando a capacidade resultante é maior que zero. Origem nula, tamanho zero ou falta de espaço no pai deixam a arena vazia, e o construtor devolve falso.
4. `alloc(a, T, n)` é `alloc(a, n, sizeof(T), alignof(T))`, com `type T` apagado (§4.4).
5. O padding de alinhamento consome capacidade. Falta de espaço, ou `n * sz` que transborda, devolve `NULL`.
6. `reset` e `restore` não devolvem memória ao sistema nem limpam objetos.
7. Resetar ou restaurar o pai invalida as filhas e os dados derivados. Resetar a filha depois não os revalida.
8. Sair do escopo da filha não devolve espaço ao pai. A duração do descritor não determina a do armazenamento.
9. `from_memory` não adquire nem libera a região externa.
10. A procedência registra as construções conhecidas e as relações entre pai e filha. Uma reatribuição que a perde marca a origem como desconhecida.
11. keel não insere `defer` por reconhecer uma arena.

Referências: [Rationale: memória por região](keel-rationale.md#memória-por-região); [Rationale: arena é um tipo](keel-rationale.md#arena-é-um-tipo); [Backend: arena](keel-c-backend.md#54-arena), inclusive o respaldo de tipo-caractere.

#### 3. Exemplo

```keel
//keel
arena a;
arena.from_memory(a, memory, bytes);
i32 *dados = arena.alloc(a, i32, n);
```

```c
//C gerado
keel_arena a = {0};
keel_arena_from_memory(&a, memory, bytes);
i32 *dados = (i32 *)keel_arena_alloc2(&a, sizeof(i32), _Alignof(i32), n);
```

#### 4. Erros

De keel, na tradução; condições no [catálogo](#62-catálogo): `nonconstant-arena-stack`, `arena-from-array-not-u8`, `arena-escape`, `child-arena-after-reset`, `byref-param`. Do backend, em execução debug: `alloc-overflow`.

#### 5. Casos especiais

- Um inicializador escrito é preservado.
- `extern` e campo de struct não recebem `= {0}`. Uma arena em campo, ou vinda por ponteiro de C, é responsabilidade de quem a contém.
- `child-arena-after-reset` incide sobre o uso posterior da filha, inclusive `reset(filha)`, e não sobre a operação que reutiliza a memória do pai.
- O programa garante que `al` é potência de dois, na forma crua.
- O programa não entrega a `free` nem a `realloc` um endereço vindo de `alloc`.
- O programa não usa objetos descartados por `reset` ou `restore`, e cada marca pertence à arena e a um topo ainda restaurável.
- O programa mantém a região externa válida enquanto houver dado derivado dela; a liberação é da origem.
- O alinhamento não resolve o tipo efetivo do C. Respaldo em armazenamento de tipo declarado segue a rota do backend; `from_memory` conserva as condições da memória recebida.
- A análise de escape e de reset é lexical: chamadas C, parâmetros de saída e cópias escapam dela.
- A arena não sincroniza o topo. Antes de `parallel`, pode-se recortar uma arena por worker.

### 5.3 `keel.buffer`, `keel.slice` e `keel.range`

#### 1. Sintaxe

```keel
import keel.buffer as buffer types;
import keel.slice as slice types;

buffer i32 b = buffer.of(vec);
buffer i32 empty = buffer.from(p, capacity);
slice i32 s = slice.of(b, start, end);
slice i32 external = slice.from(i32, p, n);
range r = start..end;
```

`buffer T` é sequência com comprimento e capacidade; `slice T` é trecho com comprimento e ponteiro; `range` é intervalo de `size_t`.

**`buffer`**

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `buffer.of(v)` | `buffer T` | buffer sobre o `array` `v`, cheio: comprimento igual à capacidade |
| `buffer.from(p, cap)` | `buffer T` | buffer vazio sobre `cap` elementos em `p`; `T` vem do alvo (§4.4) |
| `buffer.length(b)` | `size_t` | elementos em uso |
| `buffer.capacity(b)` | `size_t` | elementos reservados |
| `buffer.get(b, i)` | `T` | cópia do elemento `i` |
| `buffer.set(b, i, v)` | — | escreve o elemento `i`, sem estender o comprimento |
| `buffer.ptr(b)` | `T *` | início da sequência |
| `buffer.ptr(b, i)` | `T *` | endereço do elemento `i` |
| `buffer.push(b)` | `T *` | acrescenta uma posição e devolve seu endereço; `NULL` se cheio |
| `buffer.push(b, v)` | `T *` | acrescenta `v` e devolve seu endereço; `NULL` se cheio |
| `buffer.pop(b)` | `T *` | retira o último e devolve seu endereço; `NULL` se vazio |
| `buffer.clear(b)` | — | comprimento zero, mesma capacidade e região |
| `buffer.at(b, i)` | [`outcome T`](#55-keeloutcome-e-keelcorot) | o elemento `i`, ou `NONE` fora do comprimento, em toda build |
| `buffer.as_slice(b)` | [`slice T`](#53-keelbuffer-keelslice-e-keelrange) | vista do comprimento atual |
| `buffer.as_slice(b, a, c)` | `slice T` | vista de `[a, c)` |
| `buffer.clone(a, b)` | [`outcome buffer T`](#55-keeloutcome-e-keelcorot) | cópia do comprimento atual na [`arena`](#52-keelarena) `a`; falha sem espaço |
| `begin(b)`, `has_next(b, c)`, `next(b, c)` | `buffer.cursor`, `bool`, `T *` | cursor de `walk` (§4.7) |
| `partition(b, k, w)` | `slice T` | parte `w` de `k`, para `parallel` (§4.8) |

**`slice`**

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `slice.from(T, p, n)` | `slice T` | vista dos `n` elementos afirmados em `p`; `type T` seleciona a instância (§4.4) |
| `slice.of(x)` | `slice T` | vista do comprimento atual de `x`: `buffer`, `slice` ou `array` unidimensional |
| `slice.of(x, a, c)` | `slice T` | vista de `[a, c)` de `x` |
| `slice.of(x, r)` | `slice T` | vista dos limites do `range` `r` |
| `slice.length(s)` | `size_t` | elementos da vista |
| `slice.get(s, i)`, `slice.set(s, i, v)` | `T`, — | lê ou escreve o elemento `i` |
| `slice.ptr(s)`, `slice.ptr(s, i)` | `T *` | início, ou endereço do elemento `i` |
| `slice.at(s, i)` | [`outcome T`](#55-keeloutcome-e-keelcorot) | o elemento `i`, ou `NONE` fora do comprimento, em toda build |
| `slice.clone(a, s)` | [`outcome slice T`](#55-keeloutcome-e-keelcorot) | cópia na [`arena`](#52-keelarena) `a`; falha sem espaço |
| `begin(s)`, `has_next(s, c)`, `next(s, c)` | `slice.cursor`, `bool`, `T *` | cursor de `walk` (§4.7) |
| `partition(s, k, w)` | `slice T` | parte `w` de `k`, para `parallel` (§4.8) |

**`range`**

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `a..b` | `range` | o intervalo `[a, b)` |
| `range.first(r)`, `range.limit(r)` | `size_t` | os limites |
| `range.length(r)` | `size_t` | `limit - first` |
| `range.get(r, i)` | `size_t` | `first + i` |
| `partition(r, k, w)` | `range` | parte `w` de `k`, para `parallel` (§4.8) |

Protocolos (§5.1): Indexável — `length`, `get`, `ptr`; Contável — `first`, `limit`; Percorrível por cursor — `begin`, `has_next`, `next`; Particionável — `partition`; Indexável por intervalo — `buffer.as_slice` e `slice.of`. O programa não chama `begin`, `has_next`, `next` nem `partition`: quem os chama é a construção.

**Reconhecimento**

- A instância vem das declarações ou da origem indicada na §4.4.
- `buffer.of(v)` exige `array` unidimensional conhecido. `slice.of` exige `buffer`, `slice` ou `array` unidimensional de elemento compatível.
- `a..b` constrói um `range`. Os limites são expressões C; `..` é reconhecido fora de literais, comentários e diretivas.

#### 2. Regras

Sequências de comprimento variável, trechos de extensão fixa e intervalos.

1. A capacidade de `buffer` é fixa desde a construção. Ele cresce até ela e encolhe, sem realocação implícita.
2. Ponteiro nulo na construção dá capacidade zero, e as inserções falham pelo canal normal.
3. `slice` não cresce. Seus elementos podem ser alterados por índice, `set` ou ponteiro, respeitando `const` e a região válida.
4. As formas de intervalo de `slice.of` exigem `a <= b <= length(x)` e admitem trecho vazio.
5. Escrever pelo ponteiro de `slice.from` não altera o comprimento de um buffer.
6. Nenhum descritor libera armazenamento. Cópia de `slice` compartilha os dados; cópia de `buffer` compartilha os dados e duplica o controle de comprimento, com o aviso de `byref`.
7. `range` não declara `ptr`, porque não tem armazenamento de elementos.
8. O cursor de `buffer` e de `slice` é uma posição em `size_t`, declarada uma vez por módulo: por isso se escreve `buffer.cursor`, sem argumento. `next` devolve o endereço do elemento, e `walk` sobre esses contêineres usa binder por ponteiro.
9. As `k` partes de `partition` são disjuntas e cobrem o contêiner. Para `buffer T` e `slice T`, com `n` elementos e passo igual ao teto de `n/k`, a parte `w` é a fatia `[w*passo, min((w+1)*passo, n))`. Para `range`, a divisão é a análoga de `[first, limit)`. Parte vazia é normal quando `k` excede o comprimento.
10. A posição devolvida por `pop` continua no armazenamento, fora do comprimento, e pode ser reutilizada pela próxima inserção.
11. `clone` copia o comprimento da origem, e não a capacidade.
12. Indexação e `range-index` seguem a §4.5.

Referências: [Rationale: memória por região](keel-rationale.md#memória-por-região); [Rationale: acesso e travessia](keel-rationale.md#acesso-e-travessia); [Backend: contêineres](keel-c-backend.md#52-containers-struct-e-funções-static-inline); [Backend: acessor verificado](keel-c-backend.md#513-at--o-acessor-verificado).

#### 3. Exemplo

O [exemplo 3 do README](README.md#3-trecho-fixo-elementos-mutáveis-e-intervalo) mostra `range-index`, mutabilidade dos elementos e travessia de `range`.

#### 4. Erros

De keel, na tradução; condições no [catálogo](#62-catálogo): `buffer-over-const`, `element-copy-in-get-set`, `buffer-of-unknown-size`, `slice-from-ref`, `open-range-outside-index`. Do backend, em execução debug: `set-out-of-length`. Qualificadores e compatibilidade das cópias são do compilador C.

#### 5. Casos especiais

- O programa garante que ponteiros e extensões externos descrevem memória válida e acessível pelo tipo escrito. Nenhum descritor prolonga a vida dessa memória.
- O programa garante índice dentro do comprimento em `get`, `set` e `ptr(x, i)`, também em release. `at` verifica em toda build.
- O programa garante `first <= limit` num `range` usado como sequência; a subtração em `size_t` não corrige limites invertidos.
- O programa inicializa a posição de `push(b)` sem valor antes de lê-la, e não pressupõe que a posição removida por `pop` sobreviva a outra inserção.
- Crescer o buffer não aumenta um slice existente.
- Clones não ampliam a validade da arena de destino.
- Um cursor descreve uma posição, e não uma referência ao contêiner: alteração estrutural durante a travessia o invalida.
- Uma parte obtida por `partition` é vista sobre a mesma região, e vale enquanto valer o armazenamento de origem.

### 5.4 `keel.tagged`

#### 1. Sintaxe

```keel
import keel.tagged as tagged types;

tagged Cycle void state = {0};
tagged Kind struct Node no = {0};
```

```keel
module keel.tagged tags E type T;
pub modifier tagged { i32 tag; T value; }
```

| Chamada | Devolve | O que faz | Com `T` `void` |
| --- | --- | --- | --- |
| `tagged.tag(t)` | `i32` | etiqueta corrente | disponível |
| `tagged.value(t)` | `T` | cópia do valor | omitido |
| `tagged.mark(t, E)` | — | escreve a etiqueta, preservando o valor | disponível |
| `tagged.set(t, E, v)` | — | escreve etiqueta e valor | omitido |

Protocolos (§5.1): Etiquetado — `tag` → [`match`](#49-conjuntos-de-tags-e-despacho).

**Reconhecimento**

- O modificador recebe dois argumentos, na ordem da assinatura: o conjunto de tags e o tipo associado (§4.3).
- O conjunto escrito nomeia uma declaração `tags` conhecida.

#### 2. Regras

Etiqueta de um conjunto declarado associada a um valor.

1. A instância contém a etiqueta, em `i32`, e o valor associado. Com `void`, o campo do valor e os verbos que o mencionam são omitidos (§4.3).
2. A leitura devolve `i32`; a escrita recebe `E`, e uma constante de tag escrita pertence ao conjunto da instância (§4.3).
3. O valor associado tem um só tipo, comum a todas as tags.

Referências: [Rationale: conjuntos fechados e exaustividade](keel-rationale.md#conjuntos-fechados-e-exaustividade); [Backend](keel-c-backend.md#52-containers-struct-e-funções-static-inline).

#### 3. Exemplo

O par completo, com o conjunto declarado e o despacho, está na §4.9.

#### 4. Erros

De keel, na tradução; condições no [catálogo](#62-catálogo): `undeclared-tags`, `byref-param`, `instance-field-access`, `tag-from-other-set`. Do backend, em execução debug: `tag-out-of-range`.

#### 5. Casos especiais

- A verificação da escrita alcança a constante escrita, e não o valor calculado: `mark(t, n)`, com `n` vindo de expressão C, passa, e só a verificação de debug o observa.
- Um campo de etiqueta escrito por C opaco fora dos verbos conserva a representação, mas perde a garantia de pertinência.

### 5.5 `keel.outcome` e `keel.corot`

#### 1. Sintaxe

```keel
import keel.outcome as outcome types;
import keel.corot   as corot   types;

outcome i16 result;
outcome void done;
corot step;
```

```keel
module keel.outcome type T;
pub modifier outcome { i32 code; T value; }

module keel.corot;
pub tags Status [SUCCESS = -1, ONGOING = 0, FAILED = 1];
pub typedef struct { i32 code; } corot;
```

**`outcome`** — resultado final, de dois estados, com valor associado.

| Chamada | Devolve | O que faz | Com `T` `void` |
| --- | --- | --- | --- |
| `outcome.win(r)` | `outcome T` | código zero, valor preservado | disponível |
| `outcome.win(r, v)` | `outcome T` | código zero e valor `v` | omitido |
| `outcome.fail(r, c)` | `outcome T` | código `c`, diferente de zero, valor preservado | disponível |
| `outcome.none(r)` | `outcome T` | código `outcome.NONE`, valor preservado | disponível |
| `outcome.ok(r)` | `bool` | `code == 0` | disponível |
| `outcome.failed(r)` | `bool` | `code != 0` | disponível |
| `outcome.code(r)` | `i32` | o código | disponível |
| `outcome.value(r)` | `T` | cópia do valor | omitido |
| `outcome.value(r, v)` | — | escreve só o valor, mantendo o código | omitido |

**`corot`** — estado cooperativo, de três estados, sem valor associado.

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `corot.win(r)` | `corot` | escreve `SUCCESS` |
| `corot.again(r)` | `corot` | escreve `ONGOING` |
| `corot.fault(r, c)` | `corot` | escreve o código de falha `c`, estritamente positivo |
| `corot.ok(r)`, `corot.ongoing(r)`, `corot.faulted(r)` | `bool` | consulta sucesso, andamento ou falha |
| `corot.code(r)` | `i32` | o código |
| `corot.tag(r)` | `i32` | a tag de [`Status`](#49-conjuntos-de-tags-e-despacho) correspondente à região do código |

Protocolos (§5.1): Falível — `outcome.failed` e `outcome.win` → [`else`](#410-tratamento-de-resultado); Etiquetado — `corot.tag` → [`match`](#49-conjuntos-de-tags-e-despacho).

**Reconhecimento**

- O tipo do objeto vem de sua declaração; não é procurado no destino da atribuição nem no retorno da função chamadora.
- Os verbos de escrita recebem o objeto modificável no primeiro argumento (§4.4).

#### 2. Regras

Resultados finais e retornos cooperativos.

1. O código é `i32`. `outcome.OK` é zero; `outcome.NONE` é o menor valor de `i32` e identifica ausência.
2. Os estados são regiões do código:

   | Código | `outcome` | `corot` |
   | --- | --- | --- |
   | `< 0` | falha ou ausência; `failed` | sucesso; `ok` |
   | `= 0` | resultado válido; `ok` | andamento; `ongoing` |
   | `> 0` | falha ou ausência; `failed` | falha cooperativa; `faulted` |

3. `outcome` tem dois estados. Ausência fica do mesmo lado que a falha.
4. `corot` tem três estados. `!corot.faulted(r)` inclui sucesso e andamento, e não equivale a `corot.ok(r)`.
5. `corot` é tipo, e não modificador: não há `corot T` nem `corot.value`. O que a passagem produziu viaja pelo contexto que o programa passa.
6. `corot` é struct de um campo, `struct { i32 code; }`; não há conversão implícita para inteiro.
7. `corot.tag` é leitura, e não armazenamento: dois códigos de falha diferentes devolvem a mesma tag.
8. `win`, `fail`, `none`, `again` e `fault` modificam o objeto recebido e devolvem uma cópia dele depois do ajuste. Não retornam da função: encerrar a função é `return corot.win(r);`, com o cleanup da §4.6.
9. Os códigos de `fail` e `fault` são escritos sem conversão, troca de sinal ou normalização. O código é avaliado uma vez.
10. As consultas não modificam o objeto.
11. `outcome.value(r, v)` pode ser usado com código de falha; escrever o valor não torna o resultado válido.
12. `corot` não declara `failed` e não participa de `else`. Uma composição de `routine` participa pelo `outcome u32` que produz (§5.6).

Referências: [Rationale: resultados finais e estados cooperativos](keel-rationale.md#resultados-finais-e-estados-cooperativos); [Rationale: resultado recebido pelo verbo](keel-rationale.md#resultado-recebido-pelo-verbo); [Rationale: produção e consulta da falha cooperativa](keel-rationale.md#produção-e-consulta-da-falha-cooperativa); [Backend](keel-c-backend.md): §5.14 para resultados, §5.12 para `else`.

#### 3. Exemplo

```keel
outcome i16 r = {0};
outcome.value(r, 12);       // value 12, code preserved
outcome.fail(r, -7);       // code -7, value 12 preserved
outcome.win(r);            // code 0, value 12 preserved
outcome.win(r, 42);        // code 0, value 42
outcome.none(r);           // code NONE, value 42 preserved
```

```keel
//keel
priv corot attempt(bool unavailable) {
    corot r = {0};
    if (unavailable) return corot.fault(r, 7);
    return corot.win(r);
}

pub bool had_failure(bool unavailable) {
    corot step = attempt(unavailable);
    return corot.faulted(step);
}
```

C da operação essencial, em um módulo `example`:

```c
//C gerado
#include <stdbool.h>
#include <stdint.h>

typedef struct { int32_t code; } keel_corot;

static keel_corot example_attempt(bool unavailable) {
    if (unavailable) return (keel_corot){7};
    return (keel_corot){-1};
}

bool example_had_failure(bool unavailable) {
    keel_corot step = example_attempt(unavailable);
    return step.code > 0;
}
```

O [exemplo 4 do README](README.md#4-resultado-com-default-e-extração-explícita) mostra retorno explícito e default sobre o próprio objeto.

#### 4. Erros

De keel, na tradução; condições no [catálogo](#62-catálogo): `invalid-fault-code`, `not-a-container-expression`. Argumento, retorno ou atribuição com tipos C incompatíveis são do compilador C.

#### 5. Casos especiais

- `invalid-fault-code` usa literais e constantes conhecidas (§4.2); não calcula expressões C para descobrir o sinal.
- O programa garante `c > 0` em `corot.fault` e `c != 0` em `outcome.fail` quando o código vem de expressão C.
- `win(r)`, `fail(r, c)` e `none(r)` não inicializam o valor associado; o programa inicializa o objeto antes, como com `= {0}`.
- O programa só lê `outcome.value(r)` com resultado válido e valor estabelecido.
- Ponteiros e descritores levados como valor associado valem enquanto valer o armazenamento de origem.

### 5.6 `keel.routine`

#### 1. Sintaxe

```keel
module keel.routine type C;
pub typedef corot (*routine)(C *ctx);
pub modifier slot { routine f; C *ctx; corot state; }
```

```keel
array routine.slot Ctx steps[3] = {
    { .f = prepare, .ctx = &ctx },
    { .f = measure, .ctx = &ctx },
    { .f = write,   .ctx = &ctx },
};

outcome u32 r = routine.par(slice.of(steps), 2);
if (outcome.ok(r)) finish(&ctx);
```

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `routine.seq(s)` | [`outcome u32`](#55-keeloutcome-e-keelcorot) | executa os slots em sequência |
| `routine.par(s, alvo)` | [`outcome u32`](#55-keeloutcome-e-keelcorot) | executa os slots em ciclos até a política de `alvo` se resolver |
| `routine.mask(s)` | `u64` | bit `i` ligado quando o slot `i` terminou em `SUCCESS` |
| `routine.state(slot)` | [`corot`](#55-keeloutcome-e-keelcorot) | estado final do slot |
| `routine.code(slot)` | `i32` | código do estado do slot |

`s` é uma [`slice slot`](#53-keelbuffer-keelslice-e-keelrange), obtida de um `array` ou de um `buffer` por `slice.of`. O parâmetro do módulo é o tipo do contexto, e só ele.

**Reconhecimento**

- As composições são funções do módulo; não há palavra do núcleo envolvida.
- A compatibilidade entre o campo `f` e a função escrita é verificada pelo compilador C, pelo tipo `routine` da instância. Tabelas de contextos diferentes são tipos diferentes.

#### 2. Regras

Composição de chamadas cooperativas descritas numa tabela, em sequência ou por política de conclusão.

##### Participantes

1. Uma participante é uma função de retorno `corot` e um parâmetro `C *`. Funções de outra forma exigem outra tabela ou um adaptador do programa.
2. O que uma participante produz viaja pelo contexto, e não pelo retorno.
3. Cada chamada executa até um retorno normal. Não há suspensão de frame, continuação implícita nem pilha preservada.
4. `ONGOING` é estado do resultado, e não pausa da ativação C. Os `defer` da participante executam nas saídas dela, inclusive quando ela devolve `ONGOING`.
5. O contexto que persiste entre chamadas é o `C *` do slot. Recursos que atravessam chamadas pertencem a ele ou ao chamador.
6. `routine` é `typedef` de instância (§4.3): o programa escreve nomes de função nos campos `.f`.
7. A composição não aloca, não é dona da tabela e não a redimensiona. Ela guarda só o estado de cada slot e os contadores de sucesso e falha.

##### `seq`

8. Os slots são etapas, na ordem da fatia. A etapa corrente é chamada até devolver estado diferente de `ONGOING`.
9. `SUCCESS` avança para a próxima etapa; `FAILED` encerra a sequência sem chamar as seguintes.
10. A sequência tem sucesso quando todas as etapas terminam em `SUCCESS`. Ela não entrega `ONGOING` ao chamador.

##### `par`

11. A composição executa ciclos. Em cada ciclo, chama uma vez cada slot ainda em `ONGOING`, na ordem da fatia.
12. Um slot em `SUCCESS` ou `FAILED` conserva o estado e não é mais chamado.
13. A política é avaliada ao fim de cada ciclo; mais slots que o mínimo podem ter sucesso no mesmo ciclo.
14. Com `m` slots e alvo `q` — `q` é `m` quando o alvo escrito é zero —, e com `S` sucessos e `F` falhas acumulados, a composição tem sucesso quando `S >= q` e falha quando `m - F < q`. Caso contrário, inicia outro ciclo.
15. Resolvida a política, slots ainda em `ONGOING` deixam de ser chamados. Não há cancelamento nem chamada de encerramento.
16. `par` é cooperativa: não pede threads nem simultaneidade.

##### Resultado

17. O código do `outcome u32` é zero no sucesso e positivo na falha da política. O valor associado é a quantidade de slots em `SUCCESS`.
18. Antes do primeiro ciclo, a composição escreve `ONGOING` no estado de cada slot; uma tabela zerada já está nesse estado.
19. O estado de cada slot continua legível depois da chamada, e é operando das consultas de `corot` (§5.5) e de `match` (§4.9).
20. Não há transporte de valor entre o `corot` de uma participante e o resultado da composição.

Referências: [Rationale: composição como biblioteca](keel-rationale.md#composição-como-biblioteca); [Rationale: rotina, tabela e contexto](keel-rationale.md#rotina-tabela-e-contexto); [Rationale: resultados finais e estados cooperativos](keel-rationale.md#resultados-finais-e-estados-cooperativos); [Backend](keel-c-backend.md): §5.10, para a emissão dos ciclos; §5.14, para a representação dos resultados.

#### 3. Exemplo

O par completo, com tabela estática, política e leitura do estado por slot, está no [exemplo 6 do README](README.md#6-routinepar-tabela-de-slots-e-política).

#### 4. Erros

De keel, na tradução; condições no [catálogo](#62-catálogo): `byref-param`. Do backend, em execução debug: `par-target-above-total`, `mask-above-64-slots`. Tipo de função incompatível com `routine` é do compilador C.

#### 5. Casos especiais

- O programa garante que a tabela tem armazenamento válido e modificável durante toda a composição: `seq` e `par` escrevem o estado de cada slot. Tabela `const` não serve.
- O programa não alimenta duas execuções concorrentes com a mesma fatia.
- O programa garante a disponibilidade dos recursos e a duração do contexto. Nenhuma suspensão estende a vida das variáveis locais de uma participante.
- Uma composição cujas participantes nunca resolvem a política não termina. Não há timeout implícito.
- A chamada por ponteiro de função impede o compilador C de inserir as participantes em linha.
- A composição é função: saltos, retornos e cleanup do chamador seguem as §§4.6 e 5.5.

### 5.7 `keel.parallel`

#### 1. Sintaxe

```keel
import keel.parallel as parallel types;

parallel update ALL (size_t w : 0..4; slice i32 part : xs) { /* … */ }
if (parallel.failed(update)) handle();
```

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `parallel.interrupted(n)` | `bool` | verdadeiro com o pedido de interrupção ativo |
| `parallel.ok(n)` | `bool` | verdadeiro se a política foi satisfeita |
| `parallel.failed(n)` | `bool` | verdadeiro se algum worker terminou por `fail` |
| `parallel.wins(n)` | `u32` | quantidade de workers que terminaram por `win` |

`n` é o símbolo de controle, de tipo `parallel.control`, que o nome do bloco declara (§4.8).

**Reconhecimento**

- O símbolo é declarado pela construção `parallel`, e não pelo programa. Consultá-lo exige o import do módulo.
- `parallel` seguido de `.` é qualificação de módulo (§2.3).

#### 2. Regras

O tipo do símbolo de controle de um bloco `parallel` e as consultas sobre a execução.

1. O símbolo é inicializado antes da distribuição das partes e vale enquanto existir o escopo que o contém.
2. Dentro do corpo do worker, só `interrupted` tem leitura definida.
3. Depois do bloco, as quatro consultas descrevem a execução terminada e não mudam mais.
4. As consultas não sincronizam e não são pontos de saída: ler `interrupted` não encerra o worker.
5. Percorrer a parte é escolha do programa; `parallel` não percorre.

Referências: [Rationale: políticas e sinalização](keel-rationale.md#políticas-e-sinalização-de-interrupção); [Backend: parallel](keel-c-backend.md#59-parallel).

#### 3. Exemplo

O par da §4.8 mostra a declaração do símbolo pelo bloco e sua consulta depois dele.

#### 4. Erros

O módulo não acrescenta diagnósticos; os da construção estão na §4.8.

#### 5. Casos especiais

- Uma leitura de `interrupted` não diz quanto trabalho os demais workers fizeram.
- O símbolo pertence ao escopo do bloco: guardá-lo além dele, por ponteiro, não tem significado definido.

### 5.8 Biblioteca padrão

A biblioteca padrão além da base (§§5.2–5.7) não faz parte desta versão. Os
candidatos estão registrados, sem valor normativo, em
[design/possibilidades.md](design/possibilidades.md).

## 6. Diagnósticos e conformidade

### 6.1 Emissão e responsabilidade

Cada diagnóstico tem um identificador estável em kebab-case, usado nas
referências, nas mensagens e nas opções da ferramenta. A condição normativa
é a do contrato referenciado; a tabela reúne os casos para consulta.

| Severidade | Obrigação |
| --- | --- |
| `error` | Recusar a tradução da unidade, sem publicar seus artefatos gerados |
| `warning` | Detectar e permitir o relato sem recusar por regra do PPC |
| `info` | Disponibilizar a informação; a apresentação é definida pela ferramenta |
| `debug` | Verificar em execução na build de debug; a instrumentação correspondente não integra a build release |

O backend pode materializar diagnósticos por guardas no C, inclusive a
indisponibilidade de formatos. Nesses casos, o compilador C emite a mensagem.
Erros semânticos do C preservado continuam pertencendo ao compilador C, sem
precisar de identificador keel. A promoção de avisos e o formato das mensagens
são políticas da ferramenta.

A mensagem deve identificar a construção e a posição no `.k`. Conflitos
informam as declarações envolvidas; ciclos informam a cadeia; falhas de
instanciação identificam a origem genérica e os argumentos. O backend conserva
esse vínculo no C emitido, conforme seu contrato de mapeamento de linhas.

### 6.2 Catálogo

| Identificador | Condição | Severidade | Responsável | Contrato |
| --- | --- | --- | --- | --- |
| `missing-module` | Arquivo sem `module` como primeiro token significativo | `error` | keel | §4.1 |
| `module-path-mismatch` | Nome declarado em `module` divergente do caminho relativo à raiz | `error` | keel | §4.1 |
| `invalid-stem` | Stem de arquivo que não é identificador C válido | `error` | keel | §4.1 |
| `case-ambiguous-stem` | Nomes de arquivo diferindo apenas por caixa | `error` | keel | §4.1 |
| `symbol-collision` | Colisão de símbolos exportados no módulo e no fecho transitivo de imports | `error` | keel | §4.1 |
| `circular-import` | Import circular, com a cadeia completa na mensagem | `error` | keel | §4.1 |
| `nested-extern-c` | `extern_c` em posição aninhada, isto é, fora do nível de arquivo | `error` | keel | §4.1 |
| `type-layer-on-priv-extern-c` | `priv extern_c [type_h]` — qualificadores contraditórios | `error` | keel | §4.1 |
| `main-in-extern-c` | `main` definida dentro de `extern_c` | `error` | keel | §4.1 |
| `private-main` | `priv` aplicado a `main` | `error` | keel | §4.1 |
| `invalid-main-signature` | `main` assinada fora das duas formas do C | `error` | keel | §4.1 |
| `import-clause-order` | `import M types as m;` — a mensagem dá a forma correta | `error` | keel | §4.1 |
| `duplicate-injected-name` | Dois imports com `types` injetando o mesmo nome nu | `error` | keel | §4.1 |
| `duplicate-alias` | Dois imports com o mesmo alias, ou alias que coincide com o qualificador de outro import (inclusive o `keel` implícito); a mensagem cita os dois imports | `error` | keel | §4.1 |
| `shadowed-injected-name` | Nome injetado por `types` sombreado por declaração local | `warning` | keel | §4.1 |
| `injected-names` | Lista dos nomes que `types` injetou, no ponto do import | `info` | keel | §4.1 |
| `pub-static` | `pub static` sem `inline` em escopo de arquivo | `error` | keel | §4.1 |
| `inline-without-visibility` | `static inline` sem `pub`/`priv` em nível de módulo | `error` | keel | §4.1 |
| `static-on-type` | `static` aplicada a tipo | `error` | keel | §4.1 |
| `name-too-long` | Nome gerado acima do teto de comprimento do alvo | `error` | Backend / compilador C | §4.2 |
| `not-a-container-expression` | Expressão fora da gramática de contêiner em posição de contêiner | `error` | keel | §4.4 |
| `c-type-as-argument` | Palavra-chave de tipo aritmético C como argumento de modificador, exceto `char` e `bool`, em vez da grafia keel ou de um tipo nomeado | `error` | keel | §4.2 |
| `reserved-name` | Identificador do usuário no espaço reservado `keel_` | `error` | Backend / compilador C | §4.2 |
| `symbol-redeclaration` | Padrão local de possível redeclaração de símbolo conhecido, conforme §2.5 | `error` | keel | §2.5 |
| `buffer-over-const` | `buffer` sobre vetor C `const` — a mensagem indica `slice const T` | `error` | keel | §5.3 |
| `element-copy-in-get-set` | `get` ou `set` sobre elemento que é instância de modificador | `error` | keel | §5.3 |
| `set-out-of-length` | `set` com índice fora de `length` | `debug` | Backend, em execução | §5.3 |
| `buffer-of-unknown-size` | `buffer.of` de um argumento sobre símbolo que não é `array` | `error` | keel | §5.3 |
| `array-1d-as-parameter` | `array` unidimensional em parâmetro de função | `error` | keel | §4.2 |
| `array-parameter-without-dimension` | `array T v[]` sem dimensão em parâmetro | `error` | keel | §4.2 |
| `array-argument-wrong-dimension` | Argumento `array` com aridade diferente, dimensão de índice 1 em diante diferente, ou dimensão 0 menor que a do parâmetro | `error` | keel | §4.2 |
| `partial-array-index` | Indexação parcial de `array` multidimensional | `error` | keel | §4.2 |
| `flat-view-of-n-dim-array` | `keel.ptr`, `buffer.of` ou `slice.of` sobre `array` multidimensional | `error` | keel | §4.2 |
| `nonconstant-dim-index` | Índice de `keel.dim(v,k)` sem valor decimal conhecido | `error` | keel | §4.2 |
| `ref-without-initializer` | `ref` sem inicializador | `error` | keel | §4.2 |
| `ref-arithmetic` | Operador aditivo binário, `+=`, `-=`, incremento, decremento ou índice aplicado a símbolo `ref`; inclui `q - p` com `p` `ref`, sem determinar o tipo de `q` | `error` | keel | §4.2 |
| `slice-from-ref` | `slice.from` sobre `ref` | `error` | keel | §5.3 |
| `nonconstant-arena-stack` | `arena.from_stack` com tamanho não constante; a mensagem indica `arena.from_parent` | `error` | keel | §5.2 |
| `arena-from-array-not-u8` | `arena.from_array` sobre símbolo que não é `array u8` | `error` | keel | §5.2 |
| `arena-escape` | Retorno de contêiner cuja procedência conhecida é armazenamento local | `error` | keel | §5.2 |
| `defer-without-braces` | `defer` como corpo de statement de controle sem chaves | `error` | keel | §4.6 |
| `defer-at-file-scope` | `defer` em escopo de arquivo | `error` | keel | §4.6 |
| `jump-over-defer` | Entrada externa em escopo por cima de registro de `defer`, inclusive `case`/`default` posterior no mesmo corpo de `switch` | `error` | keel | §4.6 |
| `later-with-capture` | `later` seguido de lista de captura, dentro do colchete do `defer` | `error` | keel | §4.6 |
| `unmatched-delimiter` | Chave, parêntese ou colchete sem par — inclusive dentro de `extern_c` | `error` | keel | §2.5 |
| `unexpected-eof` | Fim de arquivo durante o reconhecimento de uma forma candidata, sem token de terminação nem delimitador aberto | `error` | keel | §2.5 |
| `delimiter-mismatch-across-branches` | Alternativas de um grupo condicional que discordam na contagem de delimitadores | `error` | keel | §2.5 |
| `define-over-keel-name` | `#define` ou `#undef` de palavra contextual keel ou nome `keel_` | `error` | keel | §2.5 |
| `literal-with-newline` | Literal de string ou char com newline não-emendado | `error` | keel | §2.5 |
| `keel-name-shadowed` | Sombreamento de palavra contextual, de verbo ou de nome de módulo | `warning` | keel | §2.5 |
| `byref-assignment` | Atribuição entre instâncias de modificador `byref`, nomeando o aliasing | `warning` | keel | §4.3 |
| `defer-in-control-block` | `defer` registrado em corpo de `if`, `else` ou `switch` | `warning` | keel | §4.6 |
| `instance-field-access` | Acesso direto a campo de instância de modificador | `warning` | Backend / compilador C | §4.4 |
| `indirect-import` | Uso de símbolo de módulo não importado diretamente | `info` | keel | §4.1 |
| `modifier-outside-generic` | `modifier` fora de módulo genérico | `error` | keel | §4.3 |
| `parameter-name-reuse` | Declaração de símbolo com o nome de um parâmetro do módulo | `error` | keel | §4.3 |
| `circular-generic` | Módulo genérico que se instancia, com a cadeia na mensagem | `error` | keel | §4.3 |
| `layout-cycle` | Cadeia de tipos que se contêm por valor atravessando instância de modificador, com a cadeia na mensagem | `error` | keel | §4.3 |
| `protocol-on-parameter` | Construção keel aplicada a valor cujo tipo é parâmetro do módulo genérico, com a construção na mensagem | `error` | keel | §4.3 |
| `instance-outside-file-scope` | `instance` fora de escopo de arquivo | `error` | keel | §4.3 |
| `instance-not-modifier` | Argumento de `instance` que não é modificador de módulo genérico | `error` | keel | §4.3 |
| `redundant-instance` | `instance` sobre genérico inteiramente `pub inline` | `warning` | keel | §4.3 |
| `nonparametric-out-of-line` | Função fora de linha ou variável em declaração de genérico que não menciona parâmetro nem modificador | `error` | keel | §4.3 |
| `byref-param` | Instância `byref` por valor em parâmetro — `arena`, `buffer` e todo modificador marcado | `error` | keel | §4.3 |
| `child-arena-after-reset` | Uso de arena filha depois de `reset`/`restore` do pai, no mesmo escopo | `error` | keel | §5.2 |
| `unnamed-tags` | `tags` sem nome | `error` | keel | §4.9 |
| `duplicate-tags-name` | Dois conjuntos de tags com o mesmo nome no módulo | `error` | keel | §4.9 |
| `empty-tags` | Conjunto de tags sem nenhuma tag | `error` | keel | §4.9 |
| `partial-tag-values` | Conjunto que mistura tags com e sem valor escrito | `error` | keel | §4.9 |
| `nonconstant-tag-value` | Valor de tag sem literal decimal ou constante conhecida | `error` | keel | §4.9 |
| `undeclared-tags` | Argumento de parâmetro `tags` que não nomeia conjunto declarado | `error` | keel | §4.3 |
| `tag-from-other-set` | Tag escrita que não pertence ao conjunto exigido — rótulo de `match`, ou constante em verbo com parâmetro `tags` | `error` | keel | §4.9 |
| `duplicate-tag` | Tag repetida no mesmo conjunto, ou rótulo repetido no mesmo `match` | `error` | keel | §4.9 |
| `match-without-tag` | Operando de `match` cujo tipo não é conjunto `tags` nem declara `tag` | `error` | keel | §4.9 |
| `match-without-tags` | Operando cujo módulo não declara conjunto de tags | `error` | keel | §4.9 |
| `ambiguous-match-tags` | Operando cujo módulo declara mais de um conjunto, sem parâmetro `tags` que decida | `error` | keel | §4.9 |
| `invalid-fault-code` | `corot.fault(r,c)` com código conhecido zero ou negativo | `error` | keel | §5.5 |
| `tag-out-of-range` | Etiqueta fora da lista declarada | `debug` | Backend, em execução | §4.9 |
| `no-ptr-for-arity` | Índice de aridade N sobre modificador sem `ptr` dessa aridade — a mensagem lista as que existem | `error` | keel | §4.5 |
| `not-iterable` | `foreach` sobre tipo que não declara `length`, ou `get`/`ptr` conforme o binder | `error` | keel | §4.7 |
| `not-cursor-iterable` | `walk` sobre tipo que não declara `begin`, `has_next` e `next` | `error` | keel | §4.7 |
| `walk-without-cursor` | `walk` sem o binder de cursor | `error` | keel | §4.7 |
| `cursor-type-mismatch` | Tipo do binder de cursor diferente do produto declarado de `begin` | `error` | keel | §4.7 |
| `not-partitionable` | `parallel` sobre tipo que não declara `partition` | `error` | keel | §4.8 |
| `partition-type-mismatch` | Tipo do binder de partição diferente do produto declarado de `partition` | `error` | keel | §4.8 |
| `binder-copies-container` | Binder por valor de `foreach` ou `walk` sobre elemento que é instância de modificador — a mensagem indica `T *`. Não se aplica ao binder de partição de `parallel` | `error` | keel | §4.7 |
| `mutation-during-traversal` | `push`, `pop` ou `clear` sobre o contêiner percorrido ou particionado, no corpo do `foreach`, do `walk` ou do `parallel` | `error` | keel | §4.7 |
| `index-not-size-t` | Binder de índice cujo tipo não é `size_t` | `error` | keel | §4.7 |
| `not-countable` | `foreach` de um binder sobre tipo que não declara `first` e `limit` | `error` | keel | §4.7 |
| `no-range-index-verb` | Índice por intervalo sobre tipo que não declara o verbo de `range-index` da aridade que a forma exige | `error` | keel | §4.5 |
| `inverted-range-index` | Índice por intervalo com limites decimais conhecidos e início maior que fim | `error` | keel | §4.5 |
| `range-index-out-of-bounds` | Intervalo cujos limites violam `a <= b <= length(x)` | `debug` | Backend, em execução | §4.5 |
| `array-index-above-dimension` | Índice de `array` decimal conhecido acima da dimensão declarada | `error` | keel | §4.5 |
| `array-index-out-of-bounds` | Índice de `array` fora da dimensão declarada | `debug` | Backend, em execução | §4.5 |
| `foreach-two-binders-on-literal` | `foreach` de dois binders sobre literal de intervalo — a mensagem indica nomear o intervalo | `error` | keel | §4.7 |
| `pointer-binder-on-range` | Binder por ponteiro na forma de intervalo | `error` | keel | §4.7 |
| `open-range-outside-index` | Índice por intervalo com ponta aberta fora de índice | `error` | keel | §4.7 |
| `open-range-index-on-complex-path` | `x[a..]` sobre caminho que contém índice ou verbo | `error` | keel | §4.5 |
| `enum-constant-without-type` | Constante de enum, ou de tag, escrita sem o nível do tipo ou do conjunto, quando exigido | `error` | keel | §4.2 |
| `alias-type-collision` | Alias de módulo e tipo de origens distintas têm a mesma grafia no arquivo | `error` | keel | §2.5 |
| `unnamed-parallel` | `parallel` sem nome | `error` | keel | §4.8 |
| `flow-verb-outside-parallel` | `win` ou `fail` de fluxo fora do corpo de worker | `error` | keel | §4.8 |
| `nested-parallel` | `parallel` aninhado em corpo de `parallel` | `error` | keel | §4.8 |
| `duplicate-parallel-name` | Dois `parallel` com o mesmo nome na mesma função | `error` | keel | §4.8 |
| `nonconstant-parallel` | `k` ou política de `parallel` que não é constante de compilação | `error` | keel | §4.8 |
| `tag-not-in-set` | Rótulo que não está na lista declarada do conjunto | `error` | keel | §4.9 |
| `tag-without-label` | Tag na lista declarada sem rótulo correspondente no corpo | `error` | keel | §4.9 |
| `par-target-above-total` | Alvo de `routine.par` maior que o número de slots | `debug` | Backend, em execução | §5.6 |
| `mask-above-64-slots` | `routine.mask` sobre fatia com mais de 64 entradas | `debug` | Backend, em execução | §5.6 |
| `restrict-on-container` | `restrict` escrito antes de um modificador — a `note` dá a forma com ponteiro | `error` | keel | §4.2 |
| `specific-format-unavailable` | Módulo usa `f16` ou `bf16` e o alvo não oferece o formato | `error` | Backend / compilador C | §4.2 |
| `nonconstant-dim` | Argumento de `dim` sem literal decimal ou `constexpr` de inicializador decimal conhecido | `error` | keel | §4.3 |
| `dim-generates-declaration` | Uso de `dim` para gerar declarações em vez de substituir um número | `error` | keel | §4.3 |
| `alloc-overflow` | `arena.alloc` cujo `n * sz` não cabe em `size_t` | `debug` | Backend, em execução | §5.2 |
| `canonical-name-collision` | Duas declarações do mesmo módulo produzindo o mesmo nome canônico | `error` | keel | §4.2 |
| `modifier-named-instance` | Modificador declarado com o nome `instance` | `error` | keel | §4.3 |
| `address-in-object-position` | Operador de endereço sobre o objeto no primeiro argumento de um verbo | `error` | keel | §4.4 |
| `wrong-qualifier` | Qualificador incompatível com o receptor ou produto do verbo | `error` | keel | §4.4 |
| `verb-not-in-instance` | Verbo que o genérico declara e a instância escrita não admite — a mensagem nomeia o argumento que o removeu | `error` | keel | §4.4 |
| `from-without-target` | Verbo que depende do tipo do alvo fora de inicialização, atribuição a símbolo conhecido ou retorno | `error` | keel | §4.4 |
| `captured-write` | Atribuição a escalar capturado, no corpo de um `parallel` | `error` | keel | §4.8 |
| `else-without-initializer` | Cláusula `else` em declaração sem inicializador | `error` | keel | §4.10 |
| `else-on-infallible-type` | Cláusula `else` sobre tipo que não declara `failed`, inclusive ponteiro ou escalar | `error` | keel | §4.10 |
| `else-on-complex-target` | Cláusula `else` sobre alvo que não é identificador declarado em keel — campo, índice, deref, cast ou símbolo C | `error` | keel | §4.10 |
| `else-multiple-declarators` | Cláusula `else` em declaração com mais de um declarador | `error` | keel | §4.10 |
| `else-default-without-win` | Cláusula `else` na forma de default sobre tipo falível que não declara `win` | `error` | keel | §4.10 |
| `hidden-declarator` | Declarador cujo nome não é o último token, onde keel precisa reconstruir a declaração — `constexpr`, captura de `[now]`, tipo de retorno sob `defer`. A `note` manda usar `typedef` | `error` | keel | §4.2 |
| `nonscalar-constexpr` | `constexpr` com declarador de vetor ou inicializador entre chaves | `error` | keel | §4.2 |
| `return-in-parallel` | `return` no corpo de um `parallel`: o corpo do worker não sai da função que o contém | `error` | keel | §4.8 |
| `defer-later-shadowed` | `defer` sem `[now]` cujo corpo nomeia símbolo redeclarado em escopo mais interno com ponto de saída — a `note` dá as duas saídas, `[now]` ou `goto` | `error` | keel | §4.6 |
| `constexpr-as-lvalue` | `&` sobre símbolo `constexpr`, ou uso que exija lvalue — a `note` dá a saída, `static const T k = K;` | `error` | keel | §4.2 |
| `dim-below-one` | Argumento de `dim` que resolve para valor menor que 1 — a mensagem dá a cadeia de instanciação | `error` | keel | §4.3 |
| `extent-count-not-field` | Contagem de grupo de `extent` que não nomeia campo do struct | `error` | keel | §4.11 |
| `extent-unknown-capacity` | Capacidade de grupo de `extent` que não é campo, `constexpr` conhecido nem literal decimal | `error` | keel | §4.11 |
| `extent-without-column` | `extent` sem campo marcado `array` | `error` | keel | §4.11 |
| `extent-mixed-columns` | Colunas embutidas e por ponteiro no mesmo `extent` | `error` | keel | §4.11 |
| `extent-embedded-field-capacity` | Coluna embutida sob capacidade que é campo | `error` | keel | §4.11 |
| `extent-dimension-mismatch` | Coluna embutida cujas dimensões não são, em número e pelo nome, as capacidades dos grupos | `error` | keel | §4.11 |
| `extent-index-arity` | Acesso de coluna com número de índices diferente do rank | `error` | keel | §4.11 |
| `extent-index-above-capacity` | Índice decimal conhecido maior ou igual à capacidade decimal conhecida do grupo | `error` | keel | §4.11 |
| `extent-path-with-call` | Acesso de coluna por caminho que contém chamada | `error` | keel | §4.11 |
| `extent-index-out-of-bounds` | Índice de coluna fora da contagem, ou contagem acima da capacidade | `debug` | Backend, em execução | §4.11 |
| `partial-instance-selection` | Verbo de seleção sem parâmetro `type` para algum parâmetro da linha `module` | `error` | keel | §4.4 |
| `type-param-outside-size` | Parâmetro `type` apagado usado fora de `sizeof`, `alignof` e `X *` na assinatura, ou em dimensão de vetor | `error` | keel | §4.4 |
| `type-param-shadows-type` | Parâmetro `type` apagado com nome de tipo conhecido no escopo | `error` | keel | §4.4 |

### 6.3 Implementação conforme

- Uma implementação reconhece as construções e aplica as restrições descritas
  nesta especificação. Não pode recusar programas admitidos por acrescentar
  análise de tipos C ou restrições próprias às formas definidas.
- O C emitido deve conservar avaliação, efeitos, saídas, identidades e tempos
  de vida exigidos pelos contratos. Os exemplos fixam essas operações; nomes,
  arquivos e layout seguem o backend.
- O reconhecimento não abre headers C nem expande macros. O registro limitado
  de nomes explícitos de `extern_c` não permite resolver semanticamente seus
  tipos ou os tipos de expressões C.
- C11 e C23 são perfis de geração do mesmo PPC. Trechos C escritos pelo
  programa permanecem sujeitos ao dialeto escolhido; keel não converte
  automaticamente características C23 desses trechos para C11.
- Requisitos adicionais do alvo devem ter guarda e documentação no backend.
  Incluem formatos estreitos e respaldo de objetos de arena em armazenamento de
  tipo declarado. O mecanismo de execução de `parallel` não está entre eles: os
  lowerings especificados pelo backend produzem execuções que a §4.8 já permite,
  e a ausência de um deles seleciona outro em vez de exigir guarda. A presença de `from_memory` não
  dispensa examinar o contrato da região fornecida pelo programa.
- Sob C11, a representação de `constexpr` não exige a mesma recusa de valor
  não representável no tipo que o perfil C23 oferece. A conferência de
  constância e compatibilidade de tipo permanece exigida, conforme o backend.
- Verificações `debug` não são garantias de release. Operações cujo próprio
  contrato exige verificação, como `at`, conservam-na em ambos os modos.

A conformidade se aplica aos comportamentos e condições definidos nos
contratos. A ausência de regra para um caso não constitui autorização para
uma implementação acrescentar uma transformação ao PPC ou apresentar esse
caso como portável.

### 6.4 Programa conforme e limites

Um programa conforme satisfaz as regras keel aplicáveis, tem o C resultante
aceito no perfil escolhido e cumpre as pré-condições dos contratos utilizados.
Aceitação pela tradução não prova ausência de acesso inválido, corrida de
dados ou falha de tempo de vida.

São obrigações do programa, onde não houver checagem explicitamente prevista:

- Validade, extensão, alinhamento e condições de acesso de memória externa;
  uso de índices válidos nas operações sem verificação em release.
- Preservação do armazenamento enquanto suas vistas e ponteiros forem usados;
  ausência de uso de dados descartados por reset ou restore.
- Uso correto de marcas, ponteiros de arena e mecanismos de liberação da origem.
- Inicialização do valor associado antes de sua leitura, quando a composição
  ou o construtor não o estabelecer no caminho tomado.
- Sincronização de objetos compartilhados e término das entradas das quais
  uma composição ou travessia espera conclusão.
- Presença explícita dos tokens necessários à tradução, sem ocultá-los por
  expansão de macros ou por saídas C não locais.

Violações podem resultar nos comportamentos do C emitido, inclusive
comportamento indefinido. A lista de diagnósticos keel não é uma lista de todos
os erros possíveis de um programa C.

### 6.5 Variações e extensões

A representação dos tipos, a grafia e o limite dos nomes gerados, os artefatos,
o perfil, a disponibilidade de formatos estreitos, o mecanismo de execução de
`parallel` e as condições do
respaldo de arenas são documentados pelo backend. A ordem entre workers varia
somente dentro das execuções permitidas pela §4.10.

Módulos adicionais podem oferecer novos tipos e verbos pelos protocolos
existentes. Uma implementação pode acrescentar avisos, informações e recursos
de build, desde que preserve a aceitação e o significado das construções
especificadas. Novas palavras contextuais ou mudanças de reconhecimento
constituem mudança da sintaxe ou do reconhecimento do PPC, pois podem alterar programas existentes.

### 6.6 Referências

- [Rationale: fronteira com C](keel-rationale.md#fronteira-com-c-e-conflitos-léxicos) e [perfis](keel-rationale.md#perfis-e-exemplos-de-tradução).
- [Backend: obrigações](keel-c-backend.md#1-obrigações-do-backend), [mapeamento de linhas](keel-c-backend.md#6-mapeamento-de-linhas) e [perfis](keel-c-backend.md#9-perfis-de-geração).
- [Ferramenta](cgen-tool-spec.md): resolução de fontes, seleção de entrada, opções de build e apresentação dos diagnósticos.
