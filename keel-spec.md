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
- Cada tópico e cada contrato de construção seguem o formato abaixo:

1. **Finalidade:** uma frase descritiva, sem defesa da escolha.
2. **Sintaxe:** produção ou formas aceitas; indicar contexto e escopo.
3. **Reconhecimento:** condições que distinguem a construção de C opaco.
4. **Semântica:** regras em tópicos, incluindo avaliação, efeitos e saídas.
5. **Restrições e diagnósticos:** condição, responsável e identificador.
6. **Pré-condições e limites:** obrigações que não são verificadas na tradução.
7. **Exemplo mínimo:** keel/C apenas quando necessário para fixar o significado.
8. **Referências:** rationale e backend, sem reproduzir suas explicações. Links para justificativas também acompanham os princípios e as decisões a que se referem.

## 1. Escopo e princípios

keel é um pré-processador de C (PPC) que oferece sintaxe própria para expressar
operações que, em C, seriam organizadas por macros, sem exigir truques de
expansão. Essa sintaxe é a linguagem de uso do PPC: descreve transformações em
**C11** ou **C23**, conforme o perfil, mantendo os tipos e sua validação no C.
keel processa o fonte antes do pré-processador C; o código emitido é compilado
pelo compilador C do projeto. [Justificativa: macros e sintaxe do PPC](keel-rationale.md#macros-e-sintaxe-do-ppc).

keel é um parser de ilhas em um mar de C. keel reconhece suas construções e seus símbolos no código-fonte, inclusive quando aparecem dentro de expressões. Não realiza análise semântica de expressões C puras nem resolve o sistema de tipos do C.

Um modificador atua sobre o tipo que o segue e define sua representação e suas
operações. Em `buffer struct Person pessoas;`, `buffer` modifica `struct Person`
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
| `<opaco>` | Sequência de tokens delimitada pelo contexto, sem análise semântica de C. |
| `<opaco-sem-parenteses>` | Região opaca sem `(` no nível externo da região. |

“Nível externo” ou “de topo” refere-se à profundidade de delimitadores da produção em análise, não necessariamente ao escopo de arquivo. Separadores dentro de parênteses, colchetes, chaves, literais ou diretivas não encerram uma região externa.

Uma região `<opaco>` pode conter construções keel. Essas construções são reconhecidas e traduzidas; os demais trechos são preservados. O corpo de `extern_c`, os literais e o conteúdo das diretivas têm as regras específicas da §2.3. [Justificativa: papel da região opaca](keel-rationale.md#por-que-opaco-é-o-terminal-central).

#### Elementos léxicos

| Elemento | Regra |
| --- | --- |
| Emenda de linha | Uma barra invertida seguida imediatamente de newline une as linhas antes da tokenização. |
| Espaços e comentários | Separam tokens e não participam das produções. São reconhecidos comentários de múltiplas linhas: `/* … */` e comentários de uma linha: `//` até o fim da linha lógica. Seguem as regras de comentários de C. Seu conteúdo é ignorado, mesmo contendo construções keel ou construções C. |
| Identificadores | Seguem a forma lexical de identificador C. A classificação como palavra C usa a lista fechada abaixo; o reconhecimento de tipos e palavras contextuais cabe ao parser. |
| Literais | Strings e caracteres são reconhecidos com escapes e com os prefixos `L`, `u8`, `u` e `U`. O conteúdo é indivisível para o reconhecimento de keel. Um newline não emendado dentro do literal produz o diagnóstico `literal-com-newline`. |
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
| Tipos, marcadores e genéricos | `array`, `constexpr`, `ref`, `type`, `dim`, `modifier`, `instance`, `byref` |
| Fluxo | `defer`, `now`, `later`, `foreach`, `walk`, `apply`, `parallel`, `ALL`, `ANY`, `win`, `fail` |
| Valores etiquetados | `tags`, `match` |
| Tratamento de resultado | `else` na cauda de declaração |

`constexpr` e `else` também pertencem à lista de palavras C. Sua função keel é determinada pela posição; não exige reclassificação lexical. As produções também usam palavras C como `auto`, `static` e `const`.

Nomes de tipos, modificadores e módulos da base, como `arena`, `buffer`, `slice`, `tagged`, `outcome`, `corot` e `routine`, não são palavras do núcleo. `parallel` é o único nome que ocupa as duas posições: palavra do núcleo em `parallel nome politica (…)`, e alias do módulo `keel.parallel` em `parallel.ok(nome)`, distinguidos pelo `.` da §2.3. Nomes de operações, como `length`, `get` e `alloc`, também não são palavras-chave. Sua resolução depende dos símbolos disponíveis e do contrato da operação. [Justificativa: prelúdio e base](keel-rationale.md#prelúdio-e-base-mínima).

As formas `x[i]`, `x[i,j]`, `x[a..b]`, `x[a..]`, `x[..b]`, `x[..]` e `a..b` são sintaxe, não vocabulário. A indexação e os recortes dependem do símbolo reconhecido e das regras da §4.5; sua grafia, isoladamente, não define as permissões de acesso ou mutação.

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
unidade      ::= decl-modulo { item-topo }

decl-modulo  ::= 'module' nome-modulo [ binder-dim ] [ binder-tags ]
                 [ binder-tipo ] ';'
nome-modulo  ::= IDENT { '.' IDENT }
binder-dim   ::= 'dim' IDENT { ',' IDENT }
binder-tags  ::= 'tags' IDENT { ',' IDENT }
binder-tipo  ::= 'type' IDENT { ',' IDENT }

item-topo    ::= import | import-c | extern-c | decl-topo | <opaco>

import       ::= 'import' nome-modulo [ 'as' IDENT ] [ 'types' ] ';'
import-c     ::= 'import_c' ( cabecalho-sistema | STRING ) ';'
extern-c     ::= 'extern_c' '{' <opaco> '}'

decl-topo    ::= [ 'pub' | 'priv' ]
                 ( decl-modificador | decl-instancia | decl-tags
                 | decl-funcao | decl-keel | <opaco> )

decl-modificador ::= 'modifier' IDENT [ 'byref' ] '{' <opaco> '}'
decl-instancia   ::= 'instance' tipo-conhecido ';'
decl-tags        ::= 'tags' IDENT lista-tags ';'
lista-tags       ::= '[' item-tag { ',' item-tag } ']'
item-tag         ::= IDENT [ '=' valor-tag ]
valor-tag        ::= [ '-' ] NUM | nome-qualificado

decl-funcao   ::= retorno declarador-fn ( bloco | ';' )
retorno       ::= { spec-c } ( tipo-conhecido | <opaco-sem-parenteses> )
declarador-fn ::= { '*' { qual-c } } IDENT '(' [ params ] ')'
params        ::= 'void' | param { ',' param } [ ',' '...' ]
param         ::= param-array
                | ( tipo-conhecido | <opaco-sem-parenteses> )
                  { '*' { qual-c } } [ IDENT ] { sufixo }
spec-c        ::= 'inline' | 'static' | 'extern' | '_Noreturn'
                | '_Thread_local' | 'alignas' '(' <opaco> ')'
                | '[[' <opaco> ']]' | qual-c
param-array  ::= { spec-c } 'array' argumento { '*' } IDENT dimensoes
```

`cabecalho-sistema` designa a forma `<…>` de header usada por `import_c`. O corpo de `extern-c` é preservado segundo a §4.1. `param-array` usa as dimensões de `array`; as restrições de rank em parâmetros pertencem ao contrato do marcador.

#### Tipos e declarações

```ebnf
decl-keel    ::= { spec-c } especificador declarador [ '=' <opaco> ]
                 { ',' declarador [ '=' <opaco> ] } ';'
               | { spec-c } especificador declarador '=' <opaco> cauda-else
               | decl-array | decl-constexpr

cauda-else   ::= 'else' ( bloco | <opaco> ';' )

especificador ::= tipo-conhecido
tipo-conhecido     ::= modificador argumento { argumento } | tipo-nomeado
tipo-nomeado  ::= nome-qualificado
modificador   ::= nome-qualificado [ '(' valor-dim { ',' valor-dim } ')' ]
valor-dim     ::= NUM | IDENT
argumento     ::= { qual-arg } ( tipo-conhecido | nome-qualificado
                  | tipo-com-tag | tipo-base | 'void' ) { qual-arg }
tipo-com-tag  ::= ( 'struct' | 'union' | 'enum' ) nome-qualificado
qual-arg      ::= 'const' | 'volatile' | '_Atomic'
tipo-base     ::= 'char' | 'bool'
nome-qualificado ::= IDENT { '.' IDENT }

declarador   ::= { '*' { qual-c } } declarador-direto
declarador-direto ::= IDENT { sufixo }
                    | '(' declarador ')' { sufixo }
sufixo       ::= '[' [ <opaco> ] ']' | '(' <opaco> ')'
qual-c       ::= 'const' | 'volatile' | 'restrict' | '_Atomic' | 'ref'

decl-constexpr ::= 'constexpr' <opaco> ';'

decl-array   ::= { spec-c } 'array' argumento decl-array-1 { ',' decl-array-1 } ';'
decl-array-1 ::= { '*' } IDENT dimensoes [ '=' <opaco> ]
dimensoes    ::= '[' [ <opaco> { ',' <opaco> } ] ']'
               | '[' [ <opaco> ] ']' { '[' <opaco> ']' }
```

`void` em posição de argumento de modificador segue o protocolo de omissão de campos parametrizados; não declara um objeto C de tipo `void`. `outcome void` representa um resultado sem valor associado (§5.5).

`tipo-com-tag` consome a palavra C e o nome como um único argumento de tipo:
`buffer struct Person pessoas;` aplica `buffer` a `struct Person`, e `pessoas`
é o declarador. O PPC preserva a forma C do tipo na substituição, com a
qualificação dos nomes que reconhece; não precisa interpretar os campos do
agregado para substituir esse argumento.

`tipo-nomeado` só casa um nome registrado como tipo, como `arena`. Um nome registrado como modificador, como `buffer`, exige o tipo ao qual será aplicado: `buffer T` é o tipo resultante. O número de argumentos de um modificador é determinado pelo módulo declarado, não pela repetição livre da EBNF. Qualificadores de argumento são aceitos dos dois lados do tipo; a identidade canônica segue o contrato de tipos e genéricos.

`valor-dim` admite um literal ou uma constante nomeada conhecida nas condições da §4.2. A validade de um valor como dimensão não decorre apenas de sua classificação como `NUM` ou `IDENT`. [Justificativa: constantes nomeadas](keel-rationale.md#constantes-nomeadas).

#### Statements e blocos

```ebnf
stmt         ::= stmt-keel | stmt-c | rotulo | bloco | <opaco> ';'
bloco        ::= '{' { stmt } '}'

rotulo       ::= IDENT ':' stmt | 'case' <opaco> ':' stmt | 'default' ':' stmt

stmt-c       ::= 'if' '(' <opaco> ')' stmt [ 'else' stmt ]
               | ( 'while' | 'switch' ) '(' <opaco> ')' stmt
               | 'for' '(' <opaco> ')' stmt
               | 'do' stmt 'while' '(' <opaco> ')' ';'
               | 'return' [ <opaco> ] ';'
               | ( 'break' | 'continue' ) ';'
               | 'goto' IDENT ';'
               | ';'

stmt-keel    ::= decl-keel | decl-tags
               | defer | foreach | walk | apply | parallel
               | match | saida-worker

defer        ::= 'defer' [ '[' opcoes-defer ']' ] corpo-defer
opcoes-defer ::= 'later' | 'now' captura-tipada
captura-tipada ::= entrada { ',' entrada }
entrada      ::= <opaco> IDENT
corpo-defer  ::= bloco | <opaco> ';'
captura      ::= '(' IDENT { ',' IDENT } ')'

foreach      ::= 'foreach' '(' binder ',' binder ':' contentor ')' stmt
               | 'foreach' '(' binder ':' contavel ')' stmt
binder       ::= tipo-binder IDENT
tipo-binder  ::= ( tipo-conhecido | nome-qualificado | tipo-base | 'auto' ) { '*' }
apply        ::= 'apply' '(' tipo-binder ',' contentor ',' IDENT
                 { ',' <opaco> } ')' ';'
walk         ::= 'walk' '(' binder [ ',' binder ] ':' contentor ')' stmt

parallel     ::= 'parallel' IDENT politica
                 '(' binder ':' contavel ';'
                     binder ':' contentor
                     [ ';' captura ] ')' bloco
politica     ::= 'ALL' | 'ANY' | NUM | nome-qualificado
saida-worker ::= ( 'win' | 'fail' ) ';'

match        ::= 'match' '(' contentor ')' bloco-tags
bloco-tags   ::= '{' { braco-tag } '}'
braco-tag    ::= rotulo-tag { rotulo-tag } { stmt }
rotulo-tag   ::= IDENT ':'
```

`stmt-c` descreve a estrutura necessária para localizar corpos, escopos e pontos de saída; suas expressões permanecem opacas. Dentro de `bloco-tags`, os rótulos externos identificam tags, e cada braço abre um escopo até o próximo rótulo ou até a chave final. Rótulos consecutivos, sem statements entre eles, compartilham o braço seguinte.

Um argumento de tipo pode nomear um conjunto de tags declarado por `decl-tags`, como em `tagged Ciclo void`. A produção `argumento` já admite essa forma por `nome-qualificado`; o papel do nome vem do parâmetro correspondente na assinatura do módulo. O contrato completo está na §4.9.

#### Contêineres, intervalos e recortes

```ebnf
contentor    ::= IDENT
               | contentor '[' <opaco> { ',' <opaco> } ']'
               | contentor '[' recorte ']'
               | contentor '.'  IDENT
               | contentor '->' IDENT
               | verbo '(' contentor { ',' <opaco> } ')'
               | '*' contentor | '&' contentor | '(' contentor ')'

contavel     ::= intervalo | contentor
intervalo    ::= <opaco> '..' <opaco>
recorte      ::= [ <opaco> ] '..' [ <opaco> ]
verbo        ::= nome-modulo '.' IDENT
```

A produção `contentor` descreve as formas em que keel pode consultar a identidade de um contêiner a partir de símbolos conhecidos. Não atribui tipos a expressões C arbitrárias. A assinatura da operação determina qual argumento ocupa essa posição. [Justificativa: limite da análise de contêiner](keel-rationale.md#a-posição-de-contêiner-não-é-um-sistema-de-tipos).

### 2.3 Reconhecimento

#### Regiões opacas e pontos de parada

- O reconhecimento percorre as expressões e os corpos C para localizar as construções keel, nos limites da §1.3. Não deduz o tipo das expressões C que as envolvem.
- Em uma lista, `<opaco>` termina no separador ou delimitador exigido pela produção, no nível externo daquela região. Grupos internos são atravessados como unidades balanceadas.
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
| `match` | Seguido de `(`, identifica o despacho; o corpo entre `{` e `}` contém rótulos de tag. |
| `walk` | Seguido de `(`, identifica a travessia por cursor; o segundo binder declara o cursor. |
| `foreach` | Dois binders são separados por `,` antes de `:`; a forma com um binder usa `contavel`. |
| `instance` | Em posição de declaração, inicia uma declaração de instância. É proibido declarar um modificador com esse nome. [Justificativa da reserva](keel-rationale.md#por-que-instance-é-a-única-ressalva-do-documento). |

Uma sequência de identificadores pode indicar uma forma candidata, mas a aceitação do tipo depende dos símbolos e da aridade. A varredura até um delimitador pode ter comprimento variável; o reconhecimento não é descrito como lookahead de tamanho fixo.

Um declarador como `int (*f)(void);` não satisfaz a regra de função acima: o token anterior ao grupo final de parâmetros é `)`. Uma declaração como `int x = f(1);` também não satisfaz a regra, pois há `=` externo antes da chamada. Isso não classifica o tipo C desses objetos. [Justificativa e comparação dos declaradores](keel-rationale.md#reconhecimento-de-funções-e-declaradores-c).

#### Qualificação, acesso e sombreamento

- Na resolução de `.` consultam-se, nesta ordem, um alias de módulo, um nome de tipo declarado em keel e uma expressão de contêiner reconhecida. As demais formas permanecem acesso a campo ou designador C.
- Uma declaração local reconhecida que sombreie um qualificador desativa sua interpretação como qualificador naquele escopo. O sombreamento tem os diagnósticos da §2.5.
- A coexistência de alias de módulo e nome de tipo com a mesma grafia é aceita quando ambos provêm do mesmo módulo. Nos demais casos aplica-se o diagnóstico `alias-e-tipo-colidem`.
- Um alias de módulo ativo seguido de `(` ocupa uma forma reservada e produz o diagnóstico `alias-com-argumento`.
- A posição de contêiner usa a produção `contentor` e a informação dos símbolos keel. Uma expressão C desconhecida, como uma chamada C ou um cast arbitrário, não fornece a identidade de contêiner exigida pelo despacho.
- Índices, valores e demais argumentos continuam sendo regiões opacas, inclusive quando contêm outras construções keel reconhecíveis.
- A indexação sobre um símbolo C desconhecido permanece C. A reescrita de índices múltiplos de `array` requer um símbolo com esse marcador; os demais contêineres seguem seus contratos de acesso.

### 2.4 Semântica

#### Tokenização e preservação

- A emenda de linha precede o reconhecimento de comentários, literais e diretivas.
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
- Divergência entre alternativas produz o diagnóstico `chaves-em-ramos`. Delimitador sem par produz o diagnóstico `delimitador-sem-par`, inclusive dentro de `extern_c`.

Uma alternativa pode abrir um bloco que termina depois do grupo, desde que as demais alternativas produzam a mesma estrutura. Um bloco aberto apenas sob `#ifdef` sem `#else` não satisfaz essa regra, pois a alternativa vazia não abre o bloco. [Justificativa: balanceamento por alternativa](keel-rationale.md#por-que-a-contagem-é-por-alternativa-e-não-sobre-o-texto).

### 2.5 Restrições e diagnósticos

Os diagnósticos abaixo são emitidos por keel durante a tradução. Seus identificadores são os mesmos do catálogo geral. Regras específicas das construções acrescentam os diagnósticos de seus próprios contratos.

| Condição | Identificador | Severidade |
| --- | --- | --- |
| `extern_c` fora do nível de arquivo | `extern-c-aninhado` | `error` |
| Dois imports com `types` injetam o mesmo nome nu | `types-duplicado` | `error` |
| Declaração local sombreia nome injetado por `types` | `types-sombreado` | `warning` |
| Import injeta nomes por `types`; a mensagem lista os nomes | `types-injetados` | `info` |
| Expressão fora da gramática de contêiner em posição que a exige | `fora-da-gramatica-de-conteiner` | `error` |
| Padrão local de redeclaração de símbolo conhecido, definido abaixo | `redeclaracao-de-simbolo` | `error` |
| Delimitador sem par; a mensagem localiza a abertura quando existente | `delimitador-sem-par` | `error` |
| Alternativas condicionais discordam na estrutura de delimitadores | `chaves-em-ramos` | `error` |
| `#define` ou `#undef` de palavra contextual keel ou de nome no espaço `keel_` | `define-sobre-keel` | `error` |
| Newline não emendado em literal de string ou caractere | `literal-com-newline` | `error` |
| Sombreamento de palavra contextual, verbo ou nome de módulo | `sombreamento` | `warning` |
| Alias de módulo e tipo de origens distintas têm a mesma grafia no arquivo | `alias-e-tipo-colidem` | `error` |
| Modificador declarado com o nome `instance` | `modificador-chamado-instance` | `error` |
| Alias de módulo ativo seguido de `(` | `alias-com-argumento` | `error` |

Para o diagnóstico `redeclaracao-de-simbolo`, keel reconhece os padrões `IDENT IDENT` e `IDENT '*' IDENT` no início de statement, quando o segundo identificador é um símbolo keel conhecido. A verificação recusa a possível redeclaração sem precisar resolver o primeiro identificador como tipo C. [Justificativa: recusa de possíveis redeclarações](keel-rationale.md#por-que-a-redeclaração-é-recusada-em-vez-de-classificada).

Para o diagnóstico `define-sobre-keel`, keel lê o nome alvo de `#define` ou `#undef`. Essa inspeção é adicional à classificação pela palavra da diretiva; não examina semanticamente o corpo da macro, não o expande e não altera a diretiva. A preservação do conteúdo não exclui essa verificação lexical.

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
void exemplo(void) {
    array char grade[2,3];
    grade[1,2] = 'x';
    const char *texto = "grade[1,2] .. defer";
    /* grade[1,2] permanece texto neste comentário. */
}
```

C correspondente à operação essencial:

```c
//C gerado
void exemplo(void) {
    char grade[2][3];
    grade[1][2] = 'x';
    const char *texto = "grade[1,2] .. defer";
    /* grade[1,2] permanece texto neste comentário. */
}
```

O par mostra um corpo de função; a declaração de módulo foi omitida. Detalhes de nomes gerados e verificações de acesso pertencem ao backend.

### 2.8 Referências

- [Rationale](keel-rationale.md): “Fronteira com C e conflitos léxicos”, “Prelúdio e base mínima”, “Constantes nomeadas”, e as justificativas de reconhecimento — “Por que `<opaco>` é o terminal central”, “Por que a contagem é por alternativa” e “Por que a redeclaração é recusada em vez de classificada”.
- [Backend](keel-c-backend.md): §§2, 5, 6 e 9, para nomes, emissão das construções, mapeamento de linhas e perfis de geração.

## 3. keel por exemplos

Os pares mostram a operação essencial da tradução. O C usa representações locais dos tipos necessários para tornar os exemplos legíveis; a organização em headers, os nomes internos exatos, o mapeamento de linhas e as verificações de debug pertencem ao backend. Nos exemplos cooperativos, o código `1` representa uma falha sem estabelecer a codificação detalhada de diagnóstico por participante.

Os exemplos §§3.1–3.4 e 3.7 são módulos independentes. Os exemplos §§3.5 e 3.6 compõem o mesmo módulo `cooperacao`: as funções auxiliares são apresentadas uma vez na §3.5. Nenhum import da base, além de `import keel types;`, é implícito.

### 3.1 Módulo completo e chamada C

`import_c` disponibiliza um header ao compilador C. O marcador `array` registra o vetor; a chamada `puts` permanece C.

```keel
//keel
module ola;
import_c <stdio.h>;

int main(void) {
    array char mensagem[] = "Ola, keel!";
    puts(mensagem);
    return 0;
}
```

```c
//C gerado
#include <stdio.h>

int ola_main(void) {
    char mensagem[] = "Ola, keel!";
    puts(mensagem);
    return 0;
}

/* Wrapper de entrada, emitido pelo build para o módulo selecionado. */
int main(void) { return ola_main(); }
```

`ola_main` é a função do módulo; o wrapper `main` fornece a entrada do programa. O header é processado pelo compilador C; keel não precisa conhecer a assinatura de `puts` para preservar a chamada.

Contratos: §§1.2, 4.1 e 4.2. [Justificativa: fronteira com C](keel-rationale.md#fronteira-com-c-e-conflitos-léxicos).

### 3.2 Retorno e cleanup

A função devolve o primeiro caractere lido e fecha o arquivo antes de retornar. Se a abertura falhar, o registro de `defer` não é alcançado.

```keel
//keel
module leitura;
import_c <stdio.h>;

pub int primeiro(const char *caminho) {
    FILE *arquivo = fopen(caminho, "rb");
    if (!arquivo) return EOF;
    defer fclose(arquivo);
    return fgetc(arquivo);
}
```

```c
//C gerado
#include <stdio.h>

int leitura_primeiro(const char *caminho) {
    FILE *arquivo = fopen(caminho, "rb");
    if (!arquivo) return EOF;
    int resultado = fgetc(arquivo);
    fclose(arquivo);
    return resultado;
}
```

A expressão de retorno é avaliada antes da limpeza. `FILE`, `fopen`, `fgetc`, `fclose` e `EOF` são tratados pelo compilador C.

Contratos: §§4.1 e 4.6. [Justificativa: limite de análise](keel-rationale.md#fronteira-com-c-e-conflitos-léxicos).

### 3.3 Trecho fixo, elementos mutáveis e intervalo

Um `slice` de dois elementos aponta para parte de um vetor. Alterar seus elementos por índice ou por ponteiro modifica o mesmo armazenamento. O trecho não cresce.

```keel
//keel
module trecho;
import keel.slice as slice types;
import keel.range as range types;

pub i16 somar(void) {
    array i16 dados[3] = {1, 2, 3};
    slice i16 parte = slice.of(dados, 1, 3);
    parte[0] = 20;
    *(slice.ptr(parte) + 1) = 30;

    range indices = 0..2;
    i16 total = 0;
    foreach (size_t i : indices) {
        total += parte[i];
    }
    return total;
}
```

```c
//C gerado
#include <stddef.h>
#include <stdint.h>

typedef int16_t i16;
typedef struct { size_t len; i16 *ptr; } keel_slice_i16;
typedef struct { size_t first, limit; } keel_range;

i16 trecho_somar(void) {
    i16 dados[3] = {1, 2, 3};
    keel_slice_i16 parte = {2, dados + 1};
    parte.ptr[0] = 20;
    *(parte.ptr + 1) = 30;

    keel_range indices = {0, 2};
    i16 total = 0;
    for (size_t i = indices.first; i < indices.limit; ++i) {
        total += parte.ptr[i];
    }
    return total;
}
```

O resultado é `50`; `dados` termina com `{1, 20, 30}`. O intervalo é semiaberto: inclui zero e um, exclui dois. O armazenamento permanece válido durante toda a utilização do trecho.

Contratos: §§5.2 e 5.3, e §§4.5 e 4.7. [Justificativa: memória por região](keel-rationale.md#memória-por-região).

### 3.4 Resultado com default e extração explícita

A função aceita um valor não negativo e ajusta um resultado explicitamente declarado. Um código negativo representa falha em `outcome`; o default ajusta o próprio resultado para sucesso com valor zero.

```keel
//keel
module resultado;
import keel.outcome as outcome types;

pub outcome i16 aceitar(i16 entrada) {
    outcome i16 r = {0};
    if (entrada < 0) return outcome.fail(r, entrada);
    return outcome.win(r, entrada);
}

pub i16 obter(i16 entrada) {
    outcome i16 res = aceitar(entrada) else 0;
    return outcome.value(res);
}
```

```c
//C gerado
#include <stdint.h>

typedef int16_t i16;
typedef struct { int32_t code; i16 v; } keel_outcome_i16;

keel_outcome_i16 resultado_aceitar(i16 entrada) {
    keel_outcome_i16 r = {0};
    if (entrada < 0) { r.code = entrada; return r; }
    r.code = 0;
    r.v = entrada;
    return r;
}

i16 resultado_obter(i16 entrada) {
    keel_outcome_i16 res = resultado_aceitar(entrada);
    if (res.code != 0) {
        /* outcome.win(res, 0): ajusta o próprio res. */
        res.code = 0;
        res.v = 0;
    }
    return res.v;
}
```

`obter(7)` devolve `7`; `obter(-7)` devolve `0`. A declaração de `res` continua tendo tipo `outcome i16`; a extração para `i16` só ocorre na chamada explícita a `outcome.value`.

Contratos: §§4.10 e 5.5. [Justificativa: resultados finais](keel-rationale.md#resultados-finais-e-estados-cooperativos).

### 3.5 `match`: o laço pertence à função

A função `anotar`, declarada abaixo, é usada também no exemplo seguinte. `contar` mantém seu progresso no contador passado pelo programa. Cada chamada retorna normalmente: `corot.again` entrega `ONGOING`; `corot.win` entrega `SUCCESS`; `corot.fault(r, 1)` entrega `FAILED`. O retorno é `corot`, que carrega o estado e o código, e nenhum valor: o que a passagem produz fica no contador que o programa passou. `anotar` é uma função comum de retorno `void`.

```keel
//keel
module cooperacao;
import keel.corot as corot types;
import keel.outcome as outcome types;
import keel.tagged as tagged types;

priv corot contar(i16 *contador, i16 limite, bool falha) {
    corot r = {0};
    ++*contador;
    if (*contador < limite) return corot.again(r);
    if (falha)              return corot.fault(r, 1);
    return corot.win(r);
}

priv void anotar(i16 *contador) {
    ++*contador;
}

pub tags Ciclo [ESPERA, FIM];

pub i16 manual(void) {
    tagged Ciclo void state = {0};
    tagged.mark(state, ESPERA);
    i16 chamadas = 0;
    bool terminou = false;

    while (!terminou) {
        match (state) {
            ESPERA:
                corot r = contar(&chamadas, 2, false);
                if (corot.faulted(r) || corot.ok(r)) tagged.mark(state, FIM);
            FIM:
                terminou = true;
        }
    }
    return chamadas;
}
```

```c
//C gerado
#include <stdbool.h>
#include <stdint.h>

typedef int16_t i16;
typedef struct { int32_t code; } keel_corot;
typedef struct { int32_t code; i16 v; } keel_outcome_i16;

typedef enum { cooperacao_ESPERA, cooperacao_FIM } cooperacao_Ciclo;
typedef struct { int32_t tag; } keel_tagged_cooperacao_Ciclo_void;

static keel_corot cooperacao_contar(i16 *contador,
                                         i16 limite, bool falha) {
    keel_corot r = {0};
    ++*contador;
    if (*contador < limite) { r.code =  0; return r; }
    if (falha)              { r.code =  1; return r; }
    r.code = -1; return r;
}

static void cooperacao_anotar(i16 *contador) {
    ++*contador;
}

i16 cooperacao_manual(void) {
    keel_tagged_cooperacao_Ciclo_void state = {0};
    state.tag = cooperacao_ESPERA;
    i16 chamadas = 0;
    bool terminou = false;

    while (!terminou) {
        switch (state.tag) {
        case cooperacao_ESPERA: goto keel__m0_ESPERA;
        case cooperacao_FIM:    goto keel__m0_FIM;
        default:                goto keel__m0_end;
        }
        keel__m0_ESPERA: {
            keel_corot r = cooperacao_contar(&chamadas, 2, false);
            if (r.code > 0 || r.code < 0) state.tag = cooperacao_FIM;
        }
        goto keel__m0_end;
        keel__m0_FIM: {
            terminou = true;
        }
        keel__m0_end: ;
    }
    return chamadas;
}
```

`manual()` devolve `2`. A mudança para `FIM` só é observada no próximo despacho do laço escrito pelo usuário; não há passagem automática de um braço ao seguinte. O teste `corot.faulted(r) || corot.ok(r)` significa que a chamada terminou, com falha ou sucesso; não é o predicado de falha de `outcome`.

O C usa `switch` somente para escolher um rótulo; os corpos dos braços ficam fora dele. Isso preserva a associação de controles escritos pelo usuário aos seus próprios laços. `match` não produz o valor retornado por `manual`: esse retorno é um statement da função.

A variável de estado é um `tagged Ciclo void`: um valor etiquetado sem valor associado. É a mesma construção usada para despachar um `corot`, que é um valor etiquetado com três tags e valor associado.

Contratos: §§4.9 e 5.4. [Justificativa: controle e máquina completa](keel-rationale.md#estrutura-de-controle-e-máquina-completa).

### 3.6 `routine.par`: tabela de slots e política

Esta função continua o módulo `cooperacao` da §3.5, acrescentando os imports de
`keel.routine` e `keel.slice`. As participantes são descritas em um vetor de
`routine.slot`, montado pelo programa. `routine.par` percorre esse recorte em
ciclos e devolve um `outcome u32` cujo valor associado é a quantidade de
sucessos. O estado de cada slot fica no próprio slot e é lido depois.

```keel
//keel
import keel.routine as routine types;
import keel.slice   as slice   types;

priv corot etapa(i16 *contador) {
    corot r = {0};
    ++*contador;
    if (*contador < 2) return corot.again(r);
    return corot.win(r);
}

pub outcome u32 conjunto(i16 *a, i16 *b) {
    array routine.slot i16 passos[2] = {
        { .f = etapa, .ctx = a },
        { .f = etapa, .ctx = b },
    };

    outcome u32 r = routine.par(slice.of(passos), 1);

    foreach (routine.slot i16 *sl, size_t i : passos) {
        if (corot.ok(routine.state(sl))) anotar(a);
    }
    return r;
}
```

```c
//C gerado
typedef keel_corot (*keel_routine_i16)(i16 *);

typedef struct {
    keel_routine_i16 f;
    i16             *ctx;
    keel_corot  state;
} keel_routine_slot_i16;

typedef struct {
    size_t len;
    keel_routine_slot_i16 *ptr;
} keel_slice_keel_routine_slot_i16;

typedef struct { int32_t code; uint32_t v; } keel_outcome_u32;

static keel_corot cooperacao_etapa(i16 *contador) {
    keel_corot r = {0};
    ++*contador;
    if (*contador < 2) { r.code = 0; return r; }
    r.code = -1; return r;
}

static inline keel_outcome_u32
keel_routine_par_i16(keel_slice_keel_routine_slot_i16 s, uint32_t alvo) {
    uint32_t m = (uint32_t)s.len, S = 0, F = 0;
    uint32_t q = alvo ? alvo : m;
    for (size_t i = 0; i < s.len; ++i) s.ptr[i].state.code = 0;
    for (;;) {
        for (size_t i = 0; i < s.len; ++i) {
            if (s.ptr[i].state.code != 0) continue;
            keel_corot r = s.ptr[i].f(s.ptr[i].ctx);
            s.ptr[i].state = r;
            if (r.code < 0) ++S; else if (r.code > 0) ++F;
        }
        if (S >= q)     return (keel_outcome_u32){ 0, S };
        if (m - F < q)  return (keel_outcome_u32){ 1, S };
    }
}

keel_outcome_u32 cooperacao_conjunto(i16 *a, i16 *b) {
    keel_routine_slot_i16 passos[2] = {
        { cooperacao_etapa, a, {0} },
        { cooperacao_etapa, b, {0} },
    };
    keel_slice_keel_routine_slot_i16 s = { 2, passos };
    keel_outcome_u32 r = keel_routine_par_i16(s, 1);

    for (size_t i = 0; i < 2u; ++i) {
        keel_routine_slot_i16 *sl = &passos[i];
        if (sl->state.code < 0) cooperacao_anotar(a);
    }
    return r;
}
```

Com `*a` e `*b` em zero, o alvo `1` é alcançado no **fim** do segundo ciclo, e
não na primeira vitória: as duas entradas são chamadas nesse ciclo e as duas
terminam em `SUCCESS`. O resultado é válido, com valor `2`; a travessia
seguinte chama `anotar` duas vezes, e os contadores terminam `*a == 4` e
`*b == 2`.

O estado é lido pelos verbos de `corot`, e não pelo campo: `routine.state(s)`
devolve um `corot`, sobre o qual valem `corot.ok`, `corot.ongoing`,
`corot.faulted`, `corot.code` e também `match`, pelo contrato da §4.9. O
inicializador sem `state` deixa o campo zerado, que em `corot` é `ONGOING` — o
estado inicial correto, sem escrita.

A composição é uma chamada de função da base, não uma construção do núcleo.
Não há região de finalização com fronteira léxica: o que antes seria escrito
depois de um rótulo de saída é simplesmente o statement seguinte à declaração
de `r`, sujeito às regras comuns de `return`, `goto` e `defer`.

A tabela é escrita pelo programa e pode ser estática ou montada em runtime;
`slice.of` fornece o recorte em qualquer dos casos, e exige o marcador `array`
sobre o vetor. Como `routine.par` escreve o estado de cada entrada, a tabela não
pode ser `const` e um mesmo vetor não deve alimentar duas execuções concorrentes
da composição.

Contrato: §5.6.

### 3.7 Partição e travessia por cursor

Cada worker recebe uma parte do contêiner e a percorre por cursor, escrevendo
seu resultado numa posição própria. Nada é compartilhado entre workers além do
contêiner de saída, e cada um escreve num índice diferente.

```keel
//keel
module varredura;
import keel.buffer   as buffer   types;
import keel.slice    as slice    types;
import keel.parallel as parallel;

pub void somar(buffer i32 *xs, buffer u32 *totais) {
    parallel soma ALL (size_t w : 0..4; slice i32 parte : xs; (totais)) {
        u32 t = 0;
        walk (i32 *p, slice.cursor c : parte) {
            t += (u32)*p;
        }
        totais[w] = t;
    }
}
```

```c
//C gerado
void varredura_somar(keel_buffer_i32 *xs, keel_buffer_u32 *totais) {
    keel_parallel_control soma = { .workers = 4, .target = 0 };
    {   keel_buffer_i32 *keel__c0 = xs;

        #pragma omp parallel for num_threads(4) default(none) \
                shared(keel__c0, soma, totais)
        for (size_t w = 0; w < 4; w++) {
            keel_slice_i32 parte = keel_buffer_i32_partition(keel__c0, 4, w);
            u32 t = 0;
            keel_slice_cursor c = keel_slice_i32_begin(&parte);
            while (keel_slice_i32_has_next(&parte, &c)) {
                i32 *p = keel_slice_i32_next(&parte, &c);
                t += (u32)*p;
            }
            *keel_buffer_u32_ptr(totais, w) = t;
            atomic_fetch_add_explicit(&soma.wins, 1, memory_order_relaxed);
            keel__end0: ;
        }
    }
}
```

Três coisas que o par mostra, e que os contratos detalham:

- **`parallel` não percorre.** Ele chama `partition` uma vez por worker e liga a
  parte ao binder; percorrer é escolha do corpo, que aqui usa `walk` e poderia
  usar `foreach` ou um `for` escrito à mão (§4.8).
- **`walk` não pede comprimento.** Pede `begin`, `has_next` e `next`, e o cursor
  é escrito com seu tipo — `slice.cursor`, do módulo e não da instância, porque
  guarda uma posição e não depende de `T` (§§4.7 e 5.3).
- **A saída por worker vai num contêiner indexado por `w`.** Não vai numa
  captura: captura escalar é cópia por worker, e escrever nela é o error
  `captura-escrita`. `totais` entra por ponteiro porque é instância `byref`, e
  cada worker escreve num índice diferente — a disjunção é do programa, não da
  construção.

Sob `ALL` não há interrupção, e o fim natural de um worker não emite veredito:
depois do bloco, `parallel.ok(soma)` é verdadeiro porque nenhum worker escreveu
`fail`.
A busca que para cedo é a outra política, e usa `win` mais
`parallel.interrupted` (§§4.8 e 5.7).

Contratos: §§4.7, 4.8, 5.3 e 5.7. [Justificativa: particionável e percorrível](keel-rationale.md#particionável-e-percorrível).

## 4. Construções do núcleo

Os contratos deste capítulo descrevem as construções do PPC: as formas que ele
reconhece, liga e emite. Eles usam as regras de reconhecimento do capítulo 2.
Os módulos distribuídos com keel, e os protocolos pelos quais estas construções
os alcançam, estão no capítulo 5; o capítulo 6 reúne os identificadores de
diagnóstico citados em cada contrato.

### 4.1 Módulos e interoperabilidade

#### 1. Finalidade

Organizar declarações em módulos e integrar interfaces e implementações C.

#### 2. Sintaxe

```keel
module geom;
import util;
import formas as f types;
import_c <stdio.h>;
extern_c { int operacao_externa(int); }
pub i32 contador;
priv i32 auxiliar(i32 x) { return x; }
```

`module`, `import`, `import_c` e `extern_c` ocupam o nível de arquivo.

`module` é a primeira construção significativa, antes de diretivas C. Seu nome é um caminho de identificadores separados por ponto. A forma genérica acrescenta os parâmetros da §4.3.

`import M [as A] [types];` admite alias antes de `types`. `import_c` recebe um nome de header entre `<` e `>`, ou entre aspas. `pub` e `priv` antecedem uma declaração de módulo; sua ausência significa `pub`, salvo o default adicional de `inline` dos módulos genéricos (§4.3).

#### 3. Reconhecimento

- A declaração de módulo estabelece o qualificador de seus símbolos. Os imports carregam interfaces keel e registram qualificadores e aliases.
- A coleta registra as declarações reconhecidas do arquivo antes da resolução dos usos. A análise de cada corpo respeita os escopos léxicos.
- `import_c` introduz uma inclusão C. keel não abre o header nem registra os tipos, funções, variáveis ou macros que ele possa declarar.
- `extern_c` delimita texto C preservado, sem tradução de construções keel. O registro de nomes explícitos pode distinguir variável e função segundo a §2.3, sem determinar semanticamente seus tipos.

#### 4. Semântica

- O nome do módulo corresponde ao caminho do `.k` relativo à raiz de fontes. Cada componente deve formar um identificador admitido para esse caminho.
- Declarações públicas compõem a interface; declarações privadas e corpos fora de linha compõem a implementação, conforme o backend. `priv` não muda por si só o linkage C. `static` conserva seu significado C.
- Um alias muda a escrita usada pelo importador, preservando a identidade de origem. `types` disponibiliza nomes de tipos e modificadores sem o qualificador; não injeta funções, variáveis ou constantes de enum. A forma qualificada permanece disponível. A injeção de nomes por `types` não se propaga por imports.
- O módulo e o modificador que ele declara têm identidades distintas. Em `import keel.outcome as outcome types;`, `outcome` é o alias do módulo; `outcome.outcome` é o nome qualificado do modificador. `types` permite escrever esse modificador como `outcome` em posição de tipo. Em `outcome.ok(r)`, o prefixo qualifica um verbo do módulo. Essa distinção vale também para módulos cujos nomes não coincidem com seus modificadores.
- A resolução pode alcançar símbolos públicos de imports transitivos; o uso sem import direto tem o diagnóstico informativo `import-indireto`.
- Colisões são verificadas entre os símbolos exportados do módulo e do fecho transitivo de seus imports. A comparação usa os nomes canônicos efetivos, incluindo tags, typedefs e constantes de enum (§4.2).
- `import_c` emite a inclusão na interface. `extern_c` preserva o conteúdo na implementação, sem aplicar mangling aos símbolos ali declarados.
- `main` é uma função pública do módulo e recebe seu prefixo. A seleção do módulo de entrada pela ferramenta gera o wrapper C `main`, que chama essa função. Módulos diferentes podem declarar suas próprias funções `main`.
- O único import implícito é `import keel types;`. A base é composta de módulos comuns, e todos exigem import explícito, como `import keel.slice as slice types;`: `keel.arena` (§5.2), `keel.buffer`, `keel.slice` e `keel.range` (§5.3), `keel.tagged` (§5.4), `keel.outcome` e `keel.corot` (§5.5), `keel.routine` (§5.6) e `keel.parallel` (§5.7). Nenhum deles é palavra do núcleo, e as construções os alcançam pelos protocolos da §4.4. 
- Bibliotecas adicionais possuem contratos próprios. 

[Justificativa do prelúdio](keel-rationale.md#prelúdio-e-base-mínima).

#### 5. Restrições e diagnósticos

As verificações abaixo pertencem a keel, durante a tradução:

| Condição | Identificador |
| --- | --- |
| Ausência de `module` inicial ou divergência do caminho | `sem-module`; `module-fora-do-caminho` |
| Stem inválido ou arquivos diferenciados somente por caixa | `stem-invalido`; `stem-ambiguo-por-caixa` |
| Colisão de símbolos exportados no fecho de imports ou ciclo de imports | `simbolo-colidido`; `import-circular` |
| `extern_c` fora do nível de arquivo | `extern-c-aninhado` |
| `main` em `extern_c`, privada ou fora das duas assinaturas C de entrada | `main-em-extern-c`; `main-privada`; `main-assinatura` |
| `as` depois de `types`, ou dois imports injetando o mesmo nome | `import-ordem-trocada`; `types-duplicado` |
| Nome injetado sombreado; lista dos nomes injetados | `types-sombreado` (`warning`); `types-injetados` (`info`) |
| `pub static` sem `inline`, `static inline` sem visibilidade explícita, ou `static` sobre tipo | `pub-static`; `inline-sem-visibilidade`; `static-em-tipo` |
| Uso de símbolo importado somente de modo transitivo | `import-indireto` (`info`) |

A validação semântica das declarações C é do compilador C. Delimitadores de
`extern_c` continuam sujeitos ao diagnóstico `delimitador-sem-par`.

#### 6. Pré-condições e limites

- O programa deve fornecer ao compilador e ao linker C os headers e objetos
  externos necessários. Registrar um nome não verifica a disponibilidade da
  implementação nem sua assinatura.
- Colisões entre módulos sem relação no fecho analisado podem ser detectadas
  apenas pelo linker. A coleta local não constitui análise global do programa.
- Acrescentar um import pode alterar o reconhecimento de nomes; aplicam-se os
  conflitos e limites da §2.3.

#### 7. Exemplo mínimo

O par completo da §3.1 mostra `module`, `import_c` e o wrapper de entrada.

#### 8. Referências

- [Rationale: módulos e identidade](keel-rationale.md#módulos-e-identidade).
- [Rationale: fronteira com C](keel-rationale.md#fronteira-com-c-e-conflitos-léxicos).
- [Backend: artefatos](keel-c-backend.md#4-artefatos) e [ponto de entrada](keel-c-backend.md#58-ponto-de-entrada).

### 4.2 Tipos, declarações e marcadores

#### 1. Finalidade

Declarar tipos e símbolos e registrar as propriedades usadas pelas construções keel.

#### 2. Sintaxe

```keel
buffer i32 dados;
buffer struct Person pessoas;
slice const char texto;
array i32 matriz[2,3];
array char mensagem[] = "keel";
i32 *ref elemento = origem;
constexpr size_t N = 3;
```

A aplicação de um modificador ao tipo forma o especificador que precede o
declarador C. Em `buffer struct Person pessoas;`, o modificador é `buffer`,
o tipo modificado é `struct Person` e o objeto declarado é `pessoas`.
Ponteiros, qualificadores de ponteiro e extensões de vetor pertencem ao declarador. `array` marca uma declaração de
vetor em arquivo, bloco, campo ou parâmetro, com as restrições abaixo. `ref`
ocupa a posição de qualificador depois de `*`. `constexpr` declara uma
constante em arquivo ou bloco, com inicializador e declarador simples.

#### 3. Reconhecimento

- Um tipo ou modificador precisa estar registrado; seus argumentos são lidos
  conforme a aridade declarada. Declaradores simples registram nome, forma de
  valor ou ponteiro e os marcadores escritos.
- Declaradores C mais gerais podem receber substituição dos nomes keel sem
  fornecer um símbolo utilizável como contêiner. O reconhecimento de funções
  segue a §2.3.
- Campos keel em `struct` são registrados com o tipo e os marcadores escritos;
  campos C desconhecidos permanecem opacos. A interface de uma struct pública
  exporta a informação necessária para resolver caminhos como `w->dados`.
  Isso não exige conhecer todos os campos nem interpretar seus tipos C.
- `array` registra que o símbolo é vetor e sua quantidade de dimensões. A
  extensão unidimensional pode ser determinada pelo compilador C, inclusive
  por inicializador; keel não conta seus elementos.
- Em `constexpr`, o nome é o identificador imediatamente anterior ao `=`.
  Sem inicializador, a declaração segue para validação pelo compilador C e
  não registra uma constante utilizável pela tradução.

#### 4. Semântica

- `i8`, `i16`, `i32` e `i64` têm a largura indicada e representação com sinal
  em complemento de dois; `u8`, `u16`, `u32` e `u64` são os correspondentes
  sem sinal. `f16`, `f32` e `f64` representam os formatos binários IEEE de
  16, 32 e 64 bits; `bf16` tem um bit de sinal, oito de expoente e sete de
  fração. Disponibilidade e representação C pertencem ao backend.
- `bool`, `char`, `size_t`, `ptrdiff_t` e `uintptr_t` conservam seus contratos
  C. As grafias explícitas `int8_t` a `int64_t` e `uint8_t` a `uint64_t`
  normalizam para os nomes keel correspondentes, sem consultar headers.
- A identidade nominal inclui módulo de origem e argumentos canônicos.
  Alias e `types` não criam outra identidade; tipos de módulos distintos
  continuam distintos mesmo que seus campos coincidam.
- Vários declaradores na mesma declaração registram símbolos separados, salvo
  a restrição de `else` (§4.10). Ponteiros como argumentos exigem tipo nomeado:
  `typedef i32 *pint; buffer pint b;` é buffer de ponteiros; `buffer i32 *b;`
  é ponteiro para buffer. Modificadores podem se aninhar sem teto próprio de
  profundidade, sujeitos ao limite de nome do alvo.
- Qualificadores de argumento aceitos antes ou depois do tipo normalizam para
  a mesma identidade. Qualificadores e especificadores C antes do modificador
  qualificam a declaração externa e são preservados; não mudam o argumento.
- Constantes de enum nomeado recebem o escopo do tipo: `M.Estado.PARADO`.
  Enum sem nome usa o escopo do módulo. Dentro do módulo, a constante pode
  ser escrita sem qualificação; de fora, não se omite o nível do tipo.
- `array` não cria tipo: `array T v[2,3]` traduz para `T v[2][3]`.
  A escrita com colchetes sucessivos também é aceita. Em parâmetro
  multidimensional, a primeira extensão é expressa com `static` no C.
  `keel.length` e `keel.capacity` medem o total de elementos; `keel.dim(v,k)`
  mede a dimensão de índice `k`, a partir de zero — as operações do núcleo
  sobre `array` levam o qualificador `keel` (§4.4).
- `constexpr` é constante nomeada e tipada, sem endereço e sem usos que
  exijam lvalue. O compilador C verifica o inicializador contra o tipo escrito,
  nos limites de cada perfil. Um objeto constante com endereço usa a forma C
  `static const T nome = valor;`.
- Posições que admitem literal inteiro também admitem `constexpr` conhecido,
  respeitadas as restrições do uso. Quando keel precisa do número, como em
  `dim` e `keel.dim(v,k)`, lê somente um literal decimal ou um inicializador
  decimal conhecido; não calcula expressões como `2 + 1`.
- `ref` marca um ponteiro para um elemento, admite `NULL`, exige inicialização
  e desaparece no C. Sua restrição é aplicada ao símbolo declarado, sem
  seguir cópias do endereço. `ptr(x)` entrega ponteiro comum; `ptr(x,i)`
  entrega o endereço de um elemento.
- `restrict` conserva a semântica C em declaradores de ponteiro. Não se aplica
  como prefixo de modificador keel.
- Os nomes emitidos derivam da identidade nominal, e sua grafia pertence ao
  backend. Três condições sobre eles são verificadas na tradução: duas
  declarações do mesmo módulo que produzam o mesmo nome canônico
  (`nome-canonico-colidido`), um identificador do programa no espaço reservado
  `keel_` (`nome-reservado`) e um nome emitido acima do limite de comprimento do
  alvo (`nome-acima-do-teto`).

#### 5. Restrições e diagnósticos

| Condição | Responsável | Identificador |
| --- | --- | --- |
| Palavra-chave de tipo aritmético C como argumento de modificador, exceto `char` e `bool`, em vez da grafia keel ou de um tipo nomeado | keel | `tipo-c-como-argumento` |
| `array` 1D em parâmetro ou sem dimensão nesse contexto | keel | `array-1d-em-parametro`; `array-sem-dimensao-em-parametro` |
| Indexação parcial de `array` multidimensional | keel | `array-indexacao-parcial` |
| `keel.ptr`, `buffer.of` ou `slice.of` sobre `array` multidimensional | keel | `view-sobre-array-nd` |
| `keel.dim(v,k)` sem valor decimal conhecido para `k` | keel | `dim-k-nao-constante` |
| `ref` sem inicializador | keel | `ref-sem-inicializador` |
| Operador aditivo binário, `+=`, `-=`, incremento, decremento ou índice aplicado ao símbolo `ref` | keel | `ref-aritmetica` |
| Qualificação de enum omite o nível do tipo | keel | `enum-sem-o-tipo` |
| `restrict` antes de modificador | keel | `restrict-em-conteiner` |
| Formato `f16` ou `bf16` indisponível | backend/compilador C | `formato-estreito-indisponivel` |
| Nome enterrado em declarador que precisa ser reconstruído | keel | `declarador-enterrado` |
| `constexpr` com vetor ou inicializador entre chaves | keel | `constexpr-agregado` |
| Endereço ou uso como lvalue de `constexpr` | keel | `constexpr-endereco` |

Conflitos de nomes seguem §§2.5 e 5. O diagnóstico `ref-aritmetica` inclui `q - p` quando
`p` é `ref`, sem determinar o tipo de `q`.

#### 6. Pré-condições e limites

- O compilador C verifica tipos, inicializadores, dimensões C e acessos que
  estejam fora das verificações explicitamente atribuídas a keel.
- O índice de `keel.dim(v,k)` deve designar uma dimensão existente. O argumento
  numérico conhecido não implica avaliação de outras expressões C.
- `ref` não prova validade, não nulidade ou tempo de vida do endereço.
- A representação de formatos estreitos depende da guarda de disponibilidade
  do alvo. A diferença de representabilidade de `constexpr` sob C11 está na §6.3.

#### 7. Exemplo mínimo

```keel
//keel
array i32 m[2,3];
i32 *ref p = origem;
```

```c
//C gerado
i32 m[2][3];
i32 *p = origem;
```

As declarações não acrescentam alocação nem representação além das formas C.

#### 8. Referências

- [Rationale: constantes nomeadas](keel-rationale.md#constantes-nomeadas) e [marcadores e declarações](keel-rationale.md#marcadores-e-declarações).
- [Backend: tipos primitivos](keel-c-backend.md#3-tipos-primitivos), [declarações](keel-c-backend.md#51-declarações-substituição-local-de-nome) e [perfis](keel-c-backend.md#9-perfis-de-geração).

### 4.3 Módulos genéricos

#### 1. Finalidade

Definir modificadores de tipos e expandir suas declarações para os argumentos fornecidos.

#### 2. Sintaxe

```keel
module coll type T;
pub modifier stack byref { size_t cap, len; T *ptr; }

// Em outro módulo, depois de importar coll:
coll.stack i32 pilha;
instance coll.stack i32;
```

```keel
module blocos dim N type T;
pub modifier bloco { T dados[N]; }
pub inline void preencher(bloco *b, T valores[static N]) { /* ... */ }
```

```keel
module marcados tags E type T;
pub modifier marcado { i32 tag; T value; }
```

Os parâmetros pertencem à linha `module`, nesta ordem: `dim`, `tags`, `type`.
Cada um admite lista separada por vírgulas. O uso espelha a assinatura:
`blocos.bloco(3) i32`, `marcados.marcado Kind i32`. `modifier` e `instance` são declarações de arquivo;
`byref` segue o nome do modificador.

Um módulo parametrizado contém as declarações que serão expandidas. Cada
`modifier` declara uma transformação do tipo fornecido e a representação que
sustenta suas operações. O nome do módulo não precisa coincidir com o nome do
modificador; um módulo pode declarar mais de um modificador.

Por exemplo, a declaração esquemática no módulo de resultados é:

```keel
module keel.outcome type T;
pub modifier outcome { i32 code; T value; }
```

Depois de `import keel.outcome as outcome types;`, as duas escritas abaixo
aplicam o mesmo modificador ao mesmo tipo:

```keel
outcome.outcome i32 a;
outcome i32 b;
```

O primeiro `outcome` da forma qualificada identifica o módulo pelo alias;
o segundo identifica o modificador nele declarado. O nome abreviado vem de
`types`. A chamada `outcome.win(a, 7)` continua qualificada pelo módulo.

#### 3. Reconhecimento

- A assinatura do módulo fixa a quantidade e a ordem dos argumentos de todos
  os seus modificadores. A substituição usa nomes ligados por essa assinatura.
- Uma declaração pertence à instância quando seus tokens mencionam um
  parâmetro ou um modificador do módulo. As demais declarações são emitidas
  uma vez no módulo, sem análise de dependências indiretas.
- A pertença de verbos ao modificador e suas aridades vêm das assinaturas
  declaradas. O compilador C verifica os corpos depois da substituição.

#### 4. Semântica

- A aplicação do modificador produz um tipo C com a representação e os
  metadados declarados, acompanhados das operações definidas pelo módulo.
  `buffer T` representa uma sequência de elementos de `T`; `outcome T`
  representa um valor de `T` com código de resultado; `tagged E T`, um valor de
  `T` com uma etiqueta do conjunto `E`.
- A declaração original de `T` permanece a mesma. O objeto declarado com o
  modificador tem a representação resultante dessa aplicação, incluindo os
  metadados. Sua validação e compatibilidade são verificadas no C emitido.
- `tags` liga um parâmetro, como `E`, ao nome de um conjunto declarado por
  `tags Nome [ … ];`. Na expansão, o parâmetro é substituído pelo `enum` C
  correspondente, e as constantes do conjunto ficam disponíveis para as
  declarações da instância. O argumento é o nome do conjunto, não a lista:
  a identidade canônica cita esse nome, o que mantém o nome gerado curto
  independentemente da quantidade de tags.
- Um parâmetro de função declarado com o nome do parâmetro `tags` — `mark(m *t, E e)` —
  é verificado no ponto de chamada: se o argumento escrito é uma constante de tag
  que keel reconhece, ela tem de pertencer ao conjunto daquela instância, sob pena
  de `tag-de-outro-conjunto`. Argumento que não seja constante reconhecida segue
  para o compilador C, que o aceita como inteiro. A regra é do protocolo, e vale
  para qualquer módulo com parâmetro `tags`.
- `type` liga um parâmetro, como `T`, ao argumento de tipo escrito. Na expansão,
  esse parâmetro é substituído pela forma C correspondente, como `i32` ou
  `struct Person`, com os nomes reconhecidos qualificados conforme sua origem.
  `dim` substitui um número inteiro positivo conhecido na tradução. Pode receber
  literal decimal ou `constexpr` de inicializador decimal conhecido.
- O significado de `N` pertence ao modificador. Não há associação automática
  entre `dim`, rank e quantidade de índices de um verbo.
- A substituição não gera listas de parâmetros, campos ou funções. A aridade
  escrita de cada verbo permanece fixa. `T valores[static N]` é um parâmetro,
  cuja extensão exigida muda com a instância.
- `void` indica ausência de valor associado: campos escritos diretamente como
  `T campo` ou `T *campo` são omitidos. Não há análise de equivalência de tipos
  C nem eliminação geral de código que mencione `T`.
- Um `typedef` que mencione um parâmetro pertence à instância e é emitido por
  instância, como qualquer outra declaração. Ele não tem forma escrita com
  argumento, porque só modificador aceita argumento em posição de tipo: o
  programa o utiliza indiretamente, pelos verbos e campos que o mencionam, ou
  declara o próprio `typedef` sobre a mesma forma C, que em C é o mesmo tipo.
- O uso de um modificador instancia suas declarações e, recursivamente, os
  modificadores utilizados por elas. A identidade inclui todos os argumentos
  canônicos. Um uso finito aninhado, como `stack stack i32`, é permitido.
- `byref` recusa parâmetros por valor e diagnostica a cópia entre instâncias;
  retorno por valor permanece permitido, sujeito à procedência da memória.
  `buffer T` tem esse contrato; `slice T` pode passar por valor. O tipo
  `arena` também tem passagem por referência, conforme §5.2; isso não o
  torna um modificador.
- Em módulo genérico, o default é `pub inline`. Declarações explícitas fora
  de linha exigem colocação de seus corpos por `instance` em um módulo do
  programa. `instance` não declara nome nem substitui o import; determina
  onde ficam os corpos da instância já solicitada pelos usos.

#### 5. Restrições e diagnósticos

Todas as verificações desta tabela são de keel:

| Condição | Identificador |
| --- | --- |
| Cópia por atribuição entre instâncias `byref` | `byref-atribuido` (`warning`) |
| `modifier` fora de módulo genérico | `modifier-fora-de-generico` |
| Declaração que reutiliza nome de parâmetro genérico | `nome-de-parametro` |
| Dependência de instanciação circular entre módulos genéricos | `generico-circular` |
| Cadeia de tipos que se contêm por valor atravessando instância de modificador | `ciclo-de-layout` |
| `instance` fora de arquivo ou sobre tipo que não é modificador genérico | `instance-fora-de-arquivo`; `instance-nao-modificador` |
| `instance` sem corpos fora de linha a colocar | `instance-inutil` (`warning`) |
| Parâmetro por valor de instância `byref` | `byref-param` |
| Argumento de `dim` sem valor decimal conhecido | `dim-nao-constante` |
| Argumento de `tags` que não nomeia conjunto declarado | `tags-nao-declarado` |
| Uso de `dim` para gerar declarações, em vez de substituir o número | `dim-gera-declaracao` |
| Modificador com nome `instance` | `modificador-chamado-instance` |
| Argumento conhecido de `dim` menor que um | `dim-abaixo-de-um` |

#### 6. Pré-condições e limites

- A instanciação não prova a validade das operações C sobre o tipo fornecido.
  A omissão de campos para `void` não torna válidos usos C que dependam deles.
- O chamador de `T valores[static N]` fornece acesso a pelo menos `N` elementos.
  keel não deduz a extensão de ponteiros C arbitrários.
- O programa deve colocar os corpos fora de linha necessários sem múltiplas
  definições conflitantes. A validação final dessas definições cabe ao build
  e ao linker C.

#### 7. Exemplo mínimo

A aplicação dos modificadores ao mesmo tipo mantém representações e contratos
próprios para cada uso:

```keel
//keel
module pessoas;
import keel.buffer as buffer types;
import keel.outcome as outcome types;
import keel.tagged as tagged types;

pub tags Estado [NOVO, ATIVO];

struct Person { i32 idade; };
buffer struct Person grupo = {0};
outcome struct Person resultado = {0};
tagged Estado struct Person marcado = {0};
```

C da representação essencial, com as operações omitidas:

```c
//C gerado
#include <stddef.h>
#include <stdint.h>

typedef int32_t i32;
struct pessoas_Person { i32 idade; };

typedef struct {
    size_t cap, len;
    struct pessoas_Person *ptr;
} keel_buffer_pessoas_Person;

typedef struct {
    i32 code;
    struct pessoas_Person value;
} keel_outcome_pessoas_Person;

typedef enum { pessoas_Estado_NOVO, pessoas_Estado_ATIVO } pessoas_Estado;

typedef struct {
    i32 tag;
    struct pessoas_Person value;
} keel_tagged_pessoas_Estado_pessoas_Person;

keel_buffer_pessoas_Person pessoas_grupo = {0};
keel_outcome_pessoas_Person pessoas_resultado = {0};
keel_tagged_pessoas_Estado_pessoas_Person pessoas_marcado = {0};
```

O tipo `struct Person` é o mesmo nos três usos. O buffer acrescenta o controle
da sequência; os outros dois acrescentam um inteiro ao valor — um código de
resultado e uma etiqueta. Em `resultado`, código zero indica resultado válido;
em `marcado`, a etiqueta é a primeira do conjunto declarado. A disposição exata
dos campos e seus nomes pertencem ao backend.

Para a substituição numérica, em `blocos.bloco(3) i32`, `T dados[N]` torna-se
`i32 dados[3]`, e `T valores[static N]` torna-se `i32 valores[static 3]`,
mantendo um único parâmetro.

#### 8. Referências

- [Rationale: modificador e tipo modificado](keel-rationale.md#modificador-e-tipo-modificado).
- [Rationale: substituição e aridade fixa](keel-rationale.md#substituição-e-aridade-fixa).
- [Backend: headers de instância](keel-c-backend.md#43-headers-de-instância), [camadas de emissão](keel-c-backend.md#431-camadas-de-emissão), [os quatro artefatos](keel-c-backend.md#432-os-quatro-artefatos) e [definição fora de linha](keel-c-backend.md#44-definição-fora-de-linha-de-instância).

### 4.4 Resolução de operações

#### 1. Finalidade

Resolver chamadas keel a partir de módulos, assinaturas e identidades declaradas.

#### 2. Sintaxe

```keel
buffer.length(b)
slice.of(b)
buffer.clone(a, s)
slice.from(i32, p, n)
arena.alloc(a, i32, n)
```

Uma chamada qualificada tem a forma `m.f(argumentos)`, com `m` módulo ou alias
ativo. A assinatura determina quais argumentos são tipos, contêineres ou
expressões C. Chamadas podem aparecer dentro de expressões.

#### 3. Reconhecimento

A resolução aplica, nesta ordem:

1. Verbo do tipo conhecido do contêiner, na aridade escrita.
2. Função do módulo qualificador, na aridade escrita.
3. Diagnóstico de qualificador incompatível, se o verbo pertence a outro
   módulo; caso contrário, emissão da chamada qualificada para validação C.

A posição de contêiner exige a produção `contentor` da §2.2 e informação de
símbolos. Uma chamada C desconhecida não fornece essa informação.

#### 4. Semântica

- Verbos produtores, como `of`, `from` e `clone`, são qualificados pelo módulo
  do produto; os demais, pelo módulo do contêiner receptor. O envelope
  `outcome` não muda o qualificador: `buffer.clone` produz `outcome buffer T`.
  Operações do núcleo sobre `array` usam `keel`.
- A aridade é contada sintaticamente, sem sobrecarga por tipos C. Um módulo
  pode declarar o mesmo verbo em aridades diferentes.
- A origem da instância é determinada pela tabela abaixo; não se deduz o tipo
  de expressões C para completar argumentos ausentes.

| Origem da instância | Operações da base |
| --- | --- |
| Objeto no primeiro argumento | Caso geral, incluindo todos os verbos de `outcome` |
| Tipo escrito na chamada | `slice.from(T,p,n)`; `arena.alloc(a,T,n)` |
| Contêiner em outro argumento | `buffer.clone(a,x)`; `slice.clone(a,x)` |
| Tipo do alvo declarado, atribuído ou retornado | `buffer.from(p,cap)` |

- A adaptação de argumentos usa a forma declarada do parâmetro e do símbolo:

| Parâmetro | Argumento conhecido | Emissão |
| --- | --- | --- |
| Ponteiro | Valor `x` | `&x` |
| Ponteiro | Ponteiro `x` | `x` |
| Valor | Valor `x` | `x` |
| Valor | Ponteiro `x` | `*x` |

- A adaptação também vale para função de módulo com parâmetro declarado como
  ponteiro para instância. Demais argumentos seguem como escritos.
- Cada argumento de operação é avaliado uma vez. A ordem relativa entre
  argumentos continua sendo a da chamada C, salvo regra explícita de outra
  construção; a adaptação não impõe avaliação da esquerda para a direita.
- Operações `of` usam informação de uma origem conhecida; `from` recebe
  propriedades afirmadas pelo programa. Cada contrato define o modo de falha.

#### 5. Restrições e diagnósticos

| Condição | Responsável | Identificador |
| --- | --- | --- |
| Argumento de contêiner fora da gramática reconhecida | keel | `fora-da-gramatica-de-conteiner` |
| Acesso direto a campo de instância | keel | `campo-de-instancia` (`warning`) |
| Qualificador não corresponde ao receptor ou produto do verbo | keel | `qualificador-errado` |
| Construtor dependente do alvo fora de inicialização, atribuição a símbolo ou retorno com tipo conhecido | keel | `from-sem-alvo` |
| Tipos ou argumentos C incompatíveis depois da resolução | compilador C | Diagnóstico do compilador C |

#### 6. Pré-condições e limites

A resolução não escolhe um alocador, não calcula extensões externas e não
infere propriedade ou tempo de vida. O programa deve fornecer os argumentos
exigidos pelo verbo. Acesso direto a campos não oferece estabilidade de layout.

#### 7. Exemplo mínimo

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

#### 8. Referências

- [Rationale: resolução por declaração](keel-rationale.md#resolução-por-declaração).
- [Backend: contêineres e funções](keel-c-backend.md#52-containers-struct-e-funções-static-inline).

### 4.5 Indexação e recortes

#### 1. Finalidade

Acessar elementos e delimitar recortes a partir de contêineres conhecidos.

#### 2. Sintaxe

```keel
x[i]
x[i,j]
x[a..b]
x[a..]
x[..b]
x[..]
```

São formas de expressão sobre `contentor`. Índices e limites são regiões
C opacas. O recorte por intervalo usa uma dimensão; recortes multidimensionais
são operações dos módulos que os implementam.

#### 3. Reconhecimento

- O símbolo ou caminho deve fornecer a identidade do contêiner. Em `array`,
  a declaração fornece a quantidade de dimensões; em modificador, a assinatura
  do verbo fornece as aridades aceitas.
- Colchetes sobre símbolos C desconhecidos permanecem C, inclusive seu
  operador vírgula. A presença de vírgulas não registra um `array`.
- `..` dentro dos colchetes distingue recorte de acesso a elemento.

#### 4. Semântica

- `x[i]` equivale a `*ptr(x,i)`: é lvalue que designa o elemento original.
  `x[i,j,...]` chama `ptr` com os índices escritos. `dim` não gera essa
  assinatura nem determina sua aridade.
- Sobre `array`, a forma de vários índices traduz para colchetes C sucessivos,
  com todos os índices. O layout permanece o do vetor multidimensional C.
- Cada índice é avaliado uma vez. A tradução não impõe uma ordem relativa
  adicional entre expressões que o C não ordena.
- `x[a..b]` chama o verbo `of` da aridade correspondente e produz um descritor
  por valor — sobre a base, `slice.of(x,a,b)`. `x[..b]` fornece início zero;
  `x[a..]` fornece `length(x)` como fim; `x[..]` usa o `of` de um argumento.
  Quem declara o `of` é o módulo do contêiner, e não este contrato.
- O recorte é rvalue; atribuir ao recorte inteiro é sujeito à recusa do
  compilador C. Alterar um elemento da vista segue o contrato do elemento.
- Na forma `x[a..]`, a tradução usa o contêiner para o recorte e para obter
  comprimento. Por isso, aceita somente caminho sem índice nem chamada:
  identificadores, `.`, `->`, `*`, `&` e parênteses.
- A verificação de índices e limites é de debug. Recortes exigem
  `a <= b <= length(x)`; o trecho vazio é permitido.

#### 5. Restrições e diagnósticos

| Condição | Responsável | Identificador |
| --- | --- | --- |
| Indexação parcial de `array` | keel | `array-indexacao-parcial` |
| Tipo não possui `ptr` da aridade escrita | keel | `aridade-sem-ptr` |
| Tipo não possui o `of` necessário ao recorte | keel | `recorte-sem-of` |
| Limites numericamente conhecidos com início maior que fim | keel | `recorte-invertido` |
| Limites violam `a <= b <= length(x)` | backend, em execução debug | `recorte-fora-de-faixa` |
| Fim omitido sobre caminho que contém índice ou verbo | keel | `recorte-aberto-com-indice` |

O diagnóstico `recorte-invertido` admite literais e valores decimais conhecidos de `constexpr`.
Não exige calcular expressões C.

#### 6. Pré-condições e limites

O programa deve satisfazer os limites também em release, manter o armazenamento
válido e respeitar qualificadores. O açúcar de indexação não oferece o
resultado falível de `at`; quem precisa desse contrato escreve o verbo.

#### 7. Exemplo mínimo

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

#### 8. Referências

- [Rationale: acesso e travessia](keel-rationale.md#acesso-e-travessia).
- [Backend: açúcar de indexação](keel-c-backend.md#53-açúcar-de-indexação).

### 4.6 Cleanup léxico

#### 1. Finalidade

Registrar operações de limpeza para as saídas de um escopo léxico.

#### 2. Sintaxe

```keel
defer liberar(p);
defer { fechar(a); fechar(b); }
defer [later] liberar(p);
defer [now int fd, FILE *saida] { relatar(saida); fechar(fd); }
```

`defer` é statement de corpo de função, dentro de bloco explícito. Sem lista,
a captura é `later`. Em `[now ...]`, cada entrada tem tipo escrito e nome
imediatamente antes de `,` ou `]`; a lista é declarativa, sem inferência de tipos.

#### 3. Reconhecimento

- O corpo adiado é um statement ou bloco balanceado. keel localiza seu
  escopo, os símbolos capturados e os pontos de saída reconhecidos.
- A reconstrução de capturas e de temporários de retorno utiliza os tokens
  dos tipos escritos. Não consulta o tipo de uma expressão C retornada.
- `return`, `break`, `continue` e destinos de `goto` são reconhecidos pela
  estrutura de statements e escopos.

#### 4. Semântica

- A saída de um escopo executa seus registros aplicáveis em ordem inversa.
  Escopos internos são limpos antes dos externos. O cleanup é emitido nos
  pontos de saída, sem pilha de registros em runtime.
- Os pontos são o fim natural do bloco, `return` e saltos que deixam o
  escopo. Uma saída anterior ao registro não executa aquele cleanup.
  Um registro no corpo de laço é limpo na saída de cada iteração.
- `later` consulta os valores na saída. `[now]` copia as entradas no registro
  e faz o corpo usar essas cópias; copiar um ponteiro não copia os dados.
- Em função com retorno não `void`, `return expr;` avalia `expr` uma vez em
  temporário do tipo de retorno escrito, executa o cleanup e retorna o
  temporário. Em função `void`, a expressão, quando escrita, é avaliada antes
  do cleanup, seguida de `return;`, sujeita às regras C.
- Saídas introduzidas por construções keel também executam o cleanup dos
  escopos que deixam. Uma consulta de estado que não sai de um escopo não
  dispara limpeza.
- Saltos externos não podem entrar em escopo por cima de um registro.
  `case` ou `default` posterior a `defer` no mesmo corpo de `switch` constitui
  essa entrada; um bloco próprio por caso delimita o registro.

#### 5. Restrições e diagnósticos

Todas as verificações desta tabela são de keel:

| Condição | Identificador |
| --- | --- |
| `defer` como corpo de controle sem chaves | `defer-sem-bloco` |
| Registro em escopo de arquivo | `defer-em-escopo-de-arquivo` |
| Entrada externa por cima de registro, inclusive por `case`/`default` | `salto-sobre-defer` |
| Lista de captura depois de `later` | `later-com-captura` |
| Registro em corpo de `if`, `else` ou `switch` | `defer-em-bloco-de-controle` (`warning`) |
| Nome enterrado em declarador de captura ou retorno que precisa ser reconstruído | `declarador-enterrado` |
| Símbolo de captura tardia redeclarado em escopo interno com saída que executaria o cleanup | `defer-later-sombreado` |

O diagnóstico `declarador-enterrado` indica o uso de um `typedef`. Para `defer-later-sombreado`, a captura explícita
`[now]` ou um ponto de saída fora do escopo que sombreia preserva a ligação.

#### 6. Pré-condições e limites

- Os recursos referidos pelo corpo adiado devem continuar válidos até a
  execução. keel não deduz aquisição, liberação ou propriedade de recursos.
- Saídas não locais de C, como `longjmp`, não recebem cleanup automático.
  Macros não podem esconder saídas que a tradução precise reconhecer.
- A análise de registro e saída é lexical; não constitui interpretação do
  fluxo de controle C nem acompanhamento dinâmico de aquisições.

#### 7. Exemplo mínimo

O par da §3.2 mostra a avaliação do retorno antes da chamada de limpeza.

#### 8. Referências

- [Rationale: limpeza e saídas](keel-rationale.md#limpeza-e-saídas).
- [Backend: defer](keel-c-backend.md#55-defer).

### 4.7 Travessia sequencial

#### 1. Finalidade

Percorrer elementos ou valores contáveis em ordem sequencial, por índice ou por cursor.

#### 2. Sintaxe

```keel
foreach (i32 valor, size_t i : xs) { usar(valor, i); }
foreach (i32 *p, size_t i : xs) { *p += 1; }
foreach (auto i : 0..n) { usar_indice(i); }
apply(i32, xs, usar);
apply(i32 *, xs, visitar, contexto);
walk (i32 *p, buffer.cursor c : xs) { usar(*p); }
```

São statements de corpo de função. Em `foreach`, dois binders separados por
vírgula antes de `:` pedem travessia de contêiner por índice; um binder pede
intervalo ou objeto contável. Em `apply`, o primeiro argumento é o tipo do
binder sem seu nome, incluindo `*` quando a operação deve receber ponteiro.
Argumentos opcionais depois da função são argumentos de contexto.

Em `walk`, o primeiro binder recebe o elemento e o segundo declara o cursor.
O tipo do cursor é escrito, qualificado pelo módulo do contêiner: keel não
o deduz nem o oculta. O cursor fica no escopo do corpo e pode ser lido pelo
programa.

#### 3. Reconhecimento

- A quantidade de binders distingue as formas de `foreach` sem consultar tipos C.
- Dois binders exigem `length` e `get` para valor, ou `length` e `ptr` para
  ponteiro. Um binder exige `first` e `limit`, ou um literal `a..b`.
- `walk` tem uma forma só, de dois binders. A forma de um binder é reconhecida
  para ser recusada por `walk-sem-cursor`, com a mensagem que a falta pede, em
  vez de um erro de sintaxe sobre a vírgula.
- `walk` exige `begin`, `has_next` e `next`. O tipo do cursor escrito deve ser
  o produto declarado de `begin`; o tipo do elemento é o produto declarado de
  `next`. A compatibilidade final é verificada pelo compilador C. Os verbos da
  base estão na §5.3.
- As operações exigidas são procuradas pelo protocolo da §4.4. A verificação
  não se restringe aos nomes dos módulos da base.
- Indexável e percorrível por cursor são capacidades independentes. Um tipo
  pode declarar uma, outra ou ambas; `foreach` e `walk` pedem cada um a sua.

#### 4. Semântica

##### `foreach` e `apply`

- O contêiner e seu comprimento são avaliados uma vez na entrada. O índice
  percorre zero até esse comprimento, excluído, em ordem crescente.
- O binder por valor recebe uma cópia do elemento a cada iteração; por
  ponteiro, recebe seu endereço. Ambos ficam no escopo do corpo.
- Na forma contável, início e limite são obtidos uma vez; percorre-se
  `[first,limit)`. Um intervalo vazio executa zero iterações.
- `auto` no binder de intervalo é traduzido como `size_t`. Os demais tipos
  escritos seguem para validação pelo compilador C.
- `range` também admite dois binders quando nomeado, fornecendo valor e
  posição. O literal de intervalo usa somente a forma de um binder.
- `apply(T,x,f)` equivale à travessia de dois binders que chama `f(elemento,i)`
  em cada iteração. Argumentos de contexto seguem elemento e índice na ordem
  escrita e são avaliados na chamada de cada iteração, segundo as regras C.
  Não há captura implícita desses valores nem dedução de seus tipos.

##### `walk`

- `begin(x)` é avaliado uma única vez, na entrada, e inicializa o cursor.
  O contêiner também é avaliado uma única vez.
- Cada iteração testa `has_next(x, cursor)` e, sendo verdadeiro, liga o
  elemento a partir de `next(x, cursor)`. O avanço do cursor pertence a `next`.
- `next` é chamado uma vez por iteração, depois do teste. keel não chama
  `next` para descartar o resultado nem o chama antes do teste.
- O tipo do elemento é o que `next` declara. Um módulo que devolve `T`
  atende o binder por valor; um que devolve `T *`, o binder por ponteiro.
  Não há conversão nem seleção por sobrecarga.
- O cursor é um objeto do programa, no escopo do corpo. Escrevê-lo é
  permitido e suas consequências pertencem ao contrato do módulo.
- `walk` não exige `length`, `get` nem `ptr`. Um contêiner cujos elementos
  não são endereçáveis, ou cujo comprimento não é conhecido de antemão,
  é percorrível por `walk` sem ser indexável.

##### Comum

- `break`, `continue` e `return` conservam o significado de fluxo C no laço
  resultante, com o cleanup da §4.6.
- A travessia é linear. Escrever nos elementos não muda o comprimento;
  operações estruturais no contêiner percorrido são recusadas.

#### 5. Restrições e diagnósticos

Todas as verificações desta tabela são de keel:

| Condição | Identificador |
| --- | --- |
| Ausência dos verbos exigidos na forma de dois binders | `nao-percorrivel` |
| Ausência de `begin`, `has_next` ou `next` em `walk` | `nao-percorrivel-por-cursor` |
| `walk` sem binder de cursor | `walk-sem-cursor` |
| Tipo do binder de cursor diferente do produto de `begin` | `cursor-de-outro-tipo` |
| Binder por valor copia elemento que é instância de modificador | `binder-copia-conteiner` |
| `push`, `pop` ou `clear` do contêiner percorrido no corpo | `mutacao-na-travessia` |
| Binder de índice de tipo diferente de `size_t` | `indice-nao-size-t` |
| Ausência de `first` ou `limit` na forma de um binder | `nao-contavel` |
| Dois binders sobre literal de intervalo | `foreach-dois-binders-em-literal` |
| Binder por ponteiro na forma de intervalo | `binder-ponteiro-em-intervalo` |
| Literal aberto fora de índice | `recorte-aberto` |

#### 6. Pré-condições e limites

O programa deve manter válida a sequência capturada na entrada e impedir
alterações estruturais por aliases ou chamadas opacas. A verificação local
não segue todos os efeitos de funções C. Operações de tipos do usuário devem
cumprir seus próprios contratos de comprimento e acesso.

Em `walk`, a terminação depende de `next` avançar o cursor até que `has_next`
seja falso. keel não prova essa propriedade: um módulo cujo `next` não avance
produz laço infinito, como produziria o `while` equivalente escrito à mão.

#### 7. Exemplo mínimo

O par da §3.3 mostra o percurso de um intervalo nomeado; os binders e o
comprimento dos contêineres seguem as mesmas regras de avaliação única.

A travessia por cursor produz o laço abaixo:

```c
//C gerado
keel_buffer_cursor c = keel_buffer_i32_begin(&xs);
while (keel_buffer_i32_has_next(&xs, &c)) {
    i32 *p = keel_buffer_i32_next(&xs, &c);
    exemplo_usar(*p);
}
```

#### 8. Referências

- [Rationale: acesso e travessia](keel-rationale.md#acesso-e-travessia).
- [Rationale: cursor explícito](keel-rationale.md#cursor-explícito).
- [Backend: foreach e apply](keel-c-backend.md#57-foreach-e-apply).

### 4.8 Execução particionada

#### 1. Finalidade

Distribuir um contêiner em partes disjuntas, executar um corpo de worker sobre cada parte e resolver uma política de conclusão.

#### 2. Sintaxe

```keel
//keel
parallel atualizar ALL (size_t w : 0..4; slice i32 parte : xs) {
    foreach (i32 *p, size_t i : parte) { *p += 1; }
}
if (parallel.failed(atualizar)) tratar();
```

A forma é `parallel nome politica (workers; particao [; (capturas)]) { corpo }`,
em corpo de função. `workers` usa um binder sobre `0..k`. `particao` é um
binder único que recebe a parte atribuída ao worker; seu tipo é o produto
declarado de `partition`. Capturas são uma lista explícita de identificadores.
A política é `ALL`, `ANY` ou constante `N` conhecida. `win;` e `fail;` são
saídas do corpo do worker.

O nome declara, no escopo que contém o bloco, um símbolo com esse nome e tipo
`parallel.control`. É por ele que o programa consulta a execução, dentro e
depois do bloco; o contrato do módulo `keel.parallel` está na §5.7.

#### 3. Reconhecimento

- O nome identifica a execução e declara seu símbolo de controle no escopo que
  contém o bloco. O binder de workers e as constantes usam as regras de
  `foreach` e `constexpr`.
- **O nome é único na função**, e não apenas no escopo. Dois blocos com o mesmo
  nome em escopos aninhados declarariam dois símbolos, e o de dentro sombrearia
  o de fora: uma consulta escrita depois leria o bloco errado sem que nada
  acusasse. Em funções diferentes o mesmo nome é livre.
- O contêiner particionado deve declarar `partition`, procurada pelo protocolo
  da §4.4. A verificação não se restringe aos nomes dos módulos da base.
- O nome do bloco é também a declaração do símbolo de controle. Consultá-lo
  exige o import de `keel.parallel`; a construção não depende desse import.
- `parallel` seguido de `.` é qualificação de módulo, e não a palavra da
  construção, pela resolução de `.` da §2.3. A construção é reconhecida por
  `parallel` seguido do nome e da política.
- O tipo escrito no binder de partição deve ser o produto declarado de
  `partition`. A compatibilidade final é verificada pelo compilador C.
- A captura é escrita pelo programa; não é deduzida dos identificadores
  usados no corpo. O registro de cada símbolo fornece sua forma declarada.
- `win` e `fail` são reconhecidos como verbos de fluxo nesse corpo, sem se
  confundirem com as chamadas qualificadas dos módulos de resultado. O
  reconhecimento alcança os verbos escritos dentro de travessias aninhadas
  no corpo do worker.

#### 4. Semântica

##### Partição

- Com `k` workers, keel avalia `partition(x, k, w)` uma vez por worker, antes
  de entrar em seu corpo, e liga o resultado ao binder de partição.
- O contêiner é avaliado uma única vez, antes da distribuição.
- `parallel` não exige `length`, `get` nem `ptr`. Um contêiner particionável
  não precisa ser indexável, e a parte entregue não precisa ser do mesmo tipo
  do todo.
- Que as `k` partes sejam disjuntas e cubram o contêiner é contrato do módulo
  que declara `partition`. keel não o prova. Para `buffer T` e `slice T`, a
  base declara a divisão contígua: com `n` elementos, o passo é o teto de
  `n/k` e a parte `w` é o recorte `[w*passo, min((w+1)*passo, n))`, possivelmente
  vazio. `range` declara a divisão análoga sobre `[first,limit)`.
- O binder de partição recebe uma instância por valor quando o módulo assim a
  declara, como `slice T`. Essa é a forma prevista desta construção e não
  incorre em `binder-copia-conteiner`. Um módulo cujo produto de `partition`
  seja `byref` entrega a parte por ponteiro.

##### Execução e política

- Não há teto de workers fixado pelo PPC. O número solicitado define a
  divisão, sem garantir execução simultânea. A execução serial das partes
  em ordem crescente é uma execução permitida.
- A escolha do lowering — série, OpenMP, pool de threads ou outro mecanismo —
  pertence ao backend e ao perfil de compilação. Nenhuma dessas escolhas altera
  a semântica desta seção, e nenhuma exige diagnóstico de keel.
- `ALL`, representado por zero, pede que nenhuma parte falhe; `ANY`, por um,
  pede uma vitória; `N` pede `N` vitórias.
- **Só `win` conta para a política, e só `fail` conta contra ela.** O fim
  natural do corpo do worker não é nem uma coisa nem outra: o worker terminou,
  e não emitiu veredito. É o que faz `ALL` ser satisfeito por workers que
  simplesmente terminam, e o que impede `ANY` de ser satisfeito por quem não
  achou nada.
- Sob `ANY` ou `N`, atingir o alvo de vitórias sinaliza interrupção para os
  demais workers; `fail` não ativa esse sinal.
- O bloco aguarda os workers antes de prosseguir. A política não permite
  devolver ao chamador enquanto uma ativação de worker ainda executa.
- Ao sinalizar interrupção, o controle ativa o flag. `parallel.interrupted(n)`
  consulta esse flag e retorna verdadeiro assim que ele está ativo. Cada
  worker pode verificar o pedido e executar seu tratamento; a consulta não
  depende de outro worker já ter parado e não é, por si só, uma saída.
- O símbolo de controle é inicializado antes da distribuição e vale enquanto o
  escopo que o contém existir. Dentro do corpo do worker, apenas
  `parallel.interrupted` tem leitura definida, porque as demais dependem de
  workers que ainda executam. Depois do bloco, as quatro consultas descrevem a
  execução terminada e não mudam mais.
- Capturas escalares são cópias por worker; instâncias `byref` são passadas
  por ponteiro. Objetos de arquivo permanecem acessíveis segundo C.
- `win` e `fail` deixam todos os escopos do worker, inclusive os de travessias
  aninhadas em seu corpo, e executam seus `defer`. Laços C escritos no corpo
  preservam seus próprios `break` e `continue`.

#### 5. Restrições e diagnósticos

| Condição | Responsável | Identificador |
| --- | --- | --- |
| Contêiner que não declara `partition` | keel | `nao-particionavel` |
| Tipo do binder de partição diferente do produto de `partition` | keel | `particao-de-outro-tipo` |
| Mutação estrutural reconhecida no contêiner particionado | keel | `mutacao-na-travessia` |
| Ausência de nome | keel | `parallel-sem-nome` |
| `win` ou `fail` de fluxo fora do corpo de worker | keel | `verbo-fora-de-parallel` |
| `parallel` aninhado | keel | `parallel-aninhado` |
| Nome repetido na mesma função | keel | `parallel-nome-repetido` |
| Quantidade de workers ou política sem constante admitida | keel | `parallel-nao-constante` |
| Atribuição a escalar capturado | keel | `captura-escrita` |
| `return` escrito no corpo | keel | `return-em-parallel` |

Os requisitos dos binders de uma travessia escrita no corpo do worker seguem
a §4.7 e incidem sobre a parte, não sobre o todo. O compilador C verifica
nomes ausentes da captura e tipos incompatíveis.

#### 6. Pré-condições e limites

- O programa não pode depender de simultaneidade, de ordem entre workers ou
  de uma quantidade determinada de trabalho realizado após um pedido de
  interrupção. O término do bloco exige que seus workers terminem.
- A disjunção das partes não prova disjunção dos objetos alcançados pelos
  elementos. Ponteiros capturados, arenas e objetos compartilhados exigem
  disciplina de acesso e sincronização pelo programa.
- Um pedido de interrupção não confirma que todos os workers já o trataram.
  Os efeitos realizados antes da observação do pedido permanecem realizados.
- Quando o lowering escolhido executa workers de fato em paralelo, o programa
  responde pela segurança dos acessos que suas partes compartilhem. keel não
  insere sincronização.
- A política de `parallel` não usa os ciclos cooperativos de `routine.par` (§5.6).
- As consultas do símbolo de controle não sincronizam nada: `interrupted` pode
  passar de falso a verdadeiro entre duas leituras do mesmo worker, e um efeito
  já realizado não é desfeito por uma leitura posterior.

#### 7. Exemplo mínimo

O exemplo do item 2 permite a execução sequencial abaixo, omitindo símbolos
auxiliares e consultas de resultado:

```c
//C gerado
keel_parallel_control atualizar = {0};
for (size_t w = 0; w < 4; ++w) {
    keel_slice_i32 parte = keel_buffer_i32_partition(&xs, 4, w);
    for (size_t i = 0; i < parte.len; ++i) {
        i32 *p = &parte.ptr[i];
        *p += 1;
    }
}
```

Esse C ilustra uma ordem permitida. A emissão por OpenMP, por pool de threads
ou por outro mecanismo pertence ao backend e não altera o que esta seção exige.

#### 8. Referências

- [Rationale: políticas e sinalização](keel-rationale.md#políticas-e-sinalização-de-interrupção).
- [Rationale: particionável e percorrível](keel-rationale.md#particionável-e-percorrível).
- [Backend: parallel](keel-c-backend.md#59-parallel).

### 4.9 Conjuntos de tags e despacho

#### 1. Finalidade

Declarar conjuntos fechados de etiquetas e despachar o controle pela etiqueta corrente de um valor.

#### 2. Sintaxe

A declaração de um conjunto de tags e o despacho são formas distintas:

```keel
pub tags Ciclo [ESPERA, FIM];
pub tags Status [SUCCESS = -1, ONGOING = 0, FAILED = 1];

tagged Ciclo void state = {0};
tagged Kind struct Node no = {0};

match (state) {
    ESPERA:
        preparar(ctx);
    FIM:
        concluir(ctx);
}
```

`tags` é declaração de arquivo ou de bloco. Em arquivo, sua visibilidade segue `pub`/`priv`. O nome declarado é um argumento de tipo: `tagged Ciclo void` aplica o modificador `tagged` ao conjunto `Ciclo` e ao tipo associado `void`. O modificador é do módulo `keel.tagged` (§5.4); o conjunto e o despacho são do núcleo.

Os valores das tags são opcionais. Quando ausentes, keel atribui ordinais a partir de zero, na ordem escrita. Quando presentes, admitem literal decimal com sinal opcional ou constante nomeada conhecida nas condições da §4.2. Um conjunto não mistura tags com e sem valor escrito.

Uma constante de tag é escrita com o nível do conjunto quando o nome não estiver injetado no arquivo: `Ciclo.FIM`. Dentro de um `match`, os rótulos são escritos sem qualificação, porque o conjunto vem do operando.

#### 3. Reconhecimento

- `tags` em linha `module` introduz parâmetros do módulo; em posição de declaração, o nome é seguido de `[` e da lista. O parser distingue as formas pela posição.
- `match` é seguido de `(`; o operando ocupa posição de contêiner e é resolvido pela §4.4. O corpo é um bloco de rótulos.
- Dentro do corpo, os rótulos externos identificam tags, e cada braço abre um escopo até o próximo rótulo ou até a chave final. Rótulos consecutivos sem statements entre eles compartilham o braço seguinte.
- O conjunto de tags do operando vem de sua declaração conhecida. keel não deduz o conjunto de uma expressão C arbitrária.
- O operando não precisa ser uma instância de `tagged`: basta declarar a operação `tag`, pela resolução comum da §4.4. A verificação não se restringe aos nomes dos módulos da base.
- O conjunto exaustivo vem de um de dois lugares, nesta ordem. Se o tipo do operando é instância de um modificador cujo módulo tem parâmetro `tags`, o conjunto é o **argumento escrito naquela instância** — em `tagged Ciclo void`, é `Ciclo`. Caso contrário, é o conjunto declarado pelo **módulo** do operando, como em `corot`.
- No segundo caso o módulo tem de declarar **exatamente um** conjunto: com dois, não há critério para escolher, e é o error `match-conjunto-ambiguo`. Um operando cujo módulo não declara conjunto nenhum não admite `match`, e é o `match-sem-conjunto`. As duas recusas são de tradução, e nenhuma delas impede o programa de escrever `switch`.

#### 4. Semântica

##### Conjuntos de tags

- Um conjunto `tags` é traduzido como um `enum` C nomeado pela identidade do módulo. Suas constantes seguem a qualificação por tipo da §4.2.
- O conjunto é fechado: seus nomes são conhecidos na tradução, o que sustenta a verificação de exaustividade sem análise de fluxo.
- A etiqueta é armazenada em `i32`. Tags com valor escrito conservam o valor sem normalização.

##### `match`

- A operação seleciona o braço cuja tag corresponde ao resultado de `tag(operando)` e executa seu corpo.
- Cada braço abre seu próprio escopo. Chegar ao fim do corpo de um braço encerra o despacho, sem executar o braço seguinte.
- `break;` escrito no nível do braço encerra o despacho, com o cleanup dos escopos que deixa. Dentro de um laço ou `switch` escrito pelo programa no braço, `break` conserva o significado C e pertence a essa estrutura.
- O corpo do braço não é envolvido por um `switch` gerado. O despacho escolhe um rótulo; os corpos ficam fora dele, para que controles escritos pelo usuário pertençam aos seus próprios laços.
- A mudança de etiqueta é uma escrita comum, por `tagged.mark` ou pelo verbo do módulo do operando. Ela não causa novo despacho até que o controle execute outro `match`.
- O laço de controle fica fora de `match`. keel não acrescenta repetição em torno do bloco.
- `match` não produz valor. Um `return` ou outro ponto de saída escrito em seu corpo pertence à função que contém a estrutura.
- O operando é avaliado uma única vez, antes da seleção.

##### Exaustividade

- Todo nome da lista declarada deve ter rótulo no corpo, e todo rótulo deve pertencer à lista. As duas direções são verificadas por keel.
- A verificação é sobre nomes, não sobre valores. Duas tags com o mesmo valor escrito continuam sendo dois nomes distintos e exigem rótulos distintos.
- Não há braço padrão. Um conjunto fechado com todos os rótulos presentes torna o padrão desnecessário; o `default` gerado no C existe apenas para a verificação de debug.

#### 5. Restrições e diagnósticos

| Condição | Responsável | Identificador |
| --- | --- | --- |
| `tags` sem nome | keel | `tags-sem-nome` (`error`) |
| Nome de conjunto repetido no módulo | keel | `tags-nome-repetido` (`error`) |
| Tag repetida no mesmo conjunto | keel | `tag-repetida` (`error`) |
| Conjunto que mistura tags com e sem valor escrito | keel | `tags-valores-parciais` (`error`) |
| Valor de tag sem literal ou constante conhecida | keel | `tag-valor-nao-constante` (`error`) |
| Conjunto vazio | keel | `tags-lista-vazia` (`error`) |
| Operando de `match` sobre tipo que não declara `tag` | keel | `match-sem-tag` (`error`) |
| Operando cujo módulo não declara conjunto de tags | keel | `match-sem-conjunto` (`error`) |
| Operando cujo módulo declara mais de um conjunto, sem parâmetro `tags` que decida | keel | `match-conjunto-ambiguo` (`error`) |
| Rótulo repetido no mesmo `match` | keel | `rotulo-de-tag-repetido` (`error`) |
| Rótulo ausente da lista, ou tag listada sem rótulo | keel | `tag-fora-da-lista`; `tag-sem-rotulo` (`error`) |
| Rótulo que não pertence ao conjunto do operando | keel | `tag-de-outro-conjunto` (`error`) |
| Valor de etiqueta fora da lista | backend, em execução debug | `tag-fora-de-faixa` (`debug`) |
| Constante de tag escrita sem o nível do conjunto, quando exigida | keel | `enum-sem-o-tipo` (`error`) |

A verificação de exaustividade alcança os rótulos presentes antes do pré-processamento C. Rótulos ocultos por macro não satisfazem o contrato da §1.2.

O `break` do braço é reescrito por keel como salto para a saída do despacho. A reescrita ocorre apenas no nível do braço: keel identifica a estrutura C que contém o `break` pelo reconhecimento da §2.2 e não altera os `break` que pertencem a laços ou `switch` do programa.

#### 6. Pré-condições e limites

- keel não prova que a etiqueta armazenada pertence ao conjunto. A verificação `tag-fora-de-faixa` é de execução em perfil debug.
- Um conjunto declarado em keel é conhecido na tradução. Constantes vindas de `extern_c` ou de header não formam um conjunto: não há lista para verificar, e `match` sobre elas é recusado por `match-sem-conjunto`. O programa continua podendo usar `switch`.
- O operando conserva sua representação: `match` lê a etiqueta por `tag`, e não impõe como ela é armazenada.

#### 7. Exemplo mínimo

```keel
//keel
module ast;
import keel.tagged as tagged types;
import keel.buffer as buffer types;

struct Node { u32 a, b; };
pub tags Kind [LIT, ADD, MUL];

pub void avaliar(tagged Kind struct Node *n) {
    match (n) {
        LIT:
            folha(tagged.value(n));
        ADD:
        MUL:
            binario(tagged.value(n));
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

void ast_avaliar(keel_tagged_ast_Kind_ast_Node *n) {
    switch (n->tag) {
    case ast_Kind_LIT: goto keel__m0_LIT;
    case ast_Kind_ADD: goto keel__m0_ADD;
    case ast_Kind_MUL: goto keel__m0_MUL;
    default:      goto keel__m0_end;
    }
    keel__m0_LIT: { ast_folha(n->value); }
    goto keel__m0_end;
    keel__m0_ADD:
    keel__m0_MUL: { ast_binario(n->value); }
    keel__m0_end: ;
}
```

`ADD` e `MUL` compartilham um braço porque os rótulos são consecutivos, sem statements entre eles. Isso não é fall-through: o braço compartilhado é único, e chegar ao seu fim encerra o despacho.

#### 8. Referências

- [Rationale: conjuntos fechados e exaustividade](keel-rationale.md#conjuntos-fechados-e-exaustividade).
- [Rationale: estrutura de controle e máquina completa](keel-rationale.md#estrutura-de-controle-e-máquina-completa).
- [Backend](keel-c-backend.md): §§5.6 e 5.10, para despacho e rótulos; §5.5, para cleanup.

### 4.10 Tratamento de resultado

#### 1. Finalidade

Testar um resultado falível no ponto em que ele é declarado, e tratá-lo com uma saída escrita ou com um valor default.

#### 2. Sintaxe

A cláusula `else` é cauda de declaração com um único declarador e inicializador:

```keel
outcome i16 r = produzir() else 0;
outcome i16 s = produzir() else return -1;
outcome i16 t = produzir() else { tratar(t); }
```

A primeira forma usa default; as outras executam o tratamento escrito quando o resultado falha. A declaração conserva seu tipo `outcome i16`. A extração é uma operação explícita: `i16 valor = outcome.value(r);`.

#### 3. Reconhecimento

- O protocolo de tratamento de resultado exige `failed`. A forma de default exige também `win`, na forma que recebe o resultado e o valor de default e ajusta o objeto para sucesso. A presença de `ongoing` não é um critério de exclusão.
- O tipo do símbolo declarado vem de sua declaração escrita. O parser não deduz o tipo de uma expressão C arbitrária para escolher o protocolo.
- O `else` do `if` pertence à gramática de controle C. O `else` de resultado pertence à declaração reconhecida. Depois dele, `{` ou uma palavra de salto C (`return`, `break`, `continue`, `goto`) identifica tratamento; os demais inícios identificam uma expressão de default.
- Expressões de default e corpos de tratamento permanecem C opaco quanto à análise semântica, com reconhecimento normal das construções keel e dos pontos de saída.
- Os verbos `failed` e `win` são procurados pelo protocolo da §4.4. A verificação não se restringe aos módulos da base: `keel.outcome` (§5.5) é a implementação distribuída, não uma exigência.

#### 4. Semântica

- O inicializador é avaliado uma vez e armazenado no símbolo declarado. A cláusula consulta `failed` desse resultado; não aplica uma comparação numérica universal a qualquer tipo.
- Se o predicado for falso, o tratamento ou default não é executado.
- Se o predicado for verdadeiro, a forma de tratamento executa o statement ou bloco escrito. Não converte automaticamente erros, não extrai o valor e não garante que o bloco saia do escopo ou repare o resultado.
- Na forma de default, a expressão é avaliada somente na falha. A tradução
  chama `win(resultado, default)` sobre o próprio símbolo declarado. Para
  `outcome`, o verbo estabelece código zero e o novo valor nesse objeto; não
  é necessário reatribuir a cópia devolvida pelo verbo.
- `outcome i16 res = outro else 0;` conserva o tipo resultado e repara uma falha com valor zero. `i16 res = outro else 0;`, com `outro` de tipo `outcome i16`, não introduz extração e é recusado: o tipo declarado não participa do protocolo, além da incompatibilidade C entre escalar e agregado.
- Saídas escritas no tratamento estão sujeitas aos contratos da região em que ocorrem, inclusive `defer`.

#### 5. Restrições e diagnósticos

| Condição | Responsável | Identificador |
| --- | --- | --- |
| Cláusula `else` sem inicializador | keel | `else-sem-inicializador` (`error`) |
| Tipo declarado não fornece o protocolo `failed` | keel | `else-tipo-nao-falivel` (`error`) |
| `else` sobre atribuição em vez de declaração | keel | `else-em-atribuicao` (`error`) |
| Mais de um declarador na declaração com `else` | keel | `else-multiplos-declaradores` (`error`) |
| Default sobre tipo que não fornece `win` | keel | `else-default-sem-win` (`error`) |
| Argumento, retorno ou atribuição com tipos C incompatíveis | compilador C | Diagnóstico do compilador C |

#### 6. Pré-condições e limites

- As funções `failed` e `win` de um tipo do usuário devem cumprir o protocolo declarado: o predicado indica ausência/falha de resultado final, e `win(resultado, default)` ajusta o objeto recebido para que `failed` seja falso. keel resolve esses nomes, mas não prova essa propriedade sobre seus corpos C.
- A conversão entre códigos e resultados de funções diferentes, quando necessária, é escrita pelo programa. Não há propagação ou extração implícita.
- A cláusula não extrai o valor: depois dela, o símbolo conserva seu tipo de resultado, e a extração é uma chamada escrita.

#### 7. Exemplo mínimo

O par da §3.4 mostra a forma de default sobre o próprio objeto e a extração explícita.

#### 8. Referências

- [Rationale: resultados finais e estados cooperativos](keel-rationale.md#resultados-finais-e-estados-cooperativos).
- [Backend](keel-c-backend.md): §5.12 para `else`, §5.5 para cleanup.

## 5. Protocolos e módulos da base

Os módulos deste capítulo acompanham a distribuição de keel e são módulos
comuns: declaram tipos, modificadores e verbos pelas regras dos §§4.1–4.4, e
são importados explicitamente. Cada contrato de módulo, das §§5.2 a 5.7, segue
o mesmo formato de oito itens do capítulo 4.

### 5.1 Protocolos

#### Verbos exigidos por construção

Um protocolo é o conjunto de verbos que uma construção do núcleo exige do tipo
sobre o qual opera. A procura é por nome e aridade, pela resolução da §4.4, e
não por uma lista de tipos privilegiados.

| Protocolo | Verbos exigidos | Construção que o consome | Declarado na base por |
| --- | --- | --- | --- |
| Indexável | `length`, e `get` ou `ptr` conforme o binder | `foreach` de dois binders, `apply`, `x[i]` | `buffer`, `slice`, `array` |
| Contável | `first`, `limit` | `foreach` de um binder | `range` |
| Percorrível por cursor | `begin`, `has_next`, `next` | `walk` | `buffer`, `slice` |
| Particionável | `partition` | `parallel` | `buffer`, `slice`, `range` |
| Recortável | `of` da aridade escrita | `x[a..b]` e suas formas abertas | `buffer`, `slice`, `array` |
| Etiquetado | `tag` | `match` | `tagged`, `corot` |
| Falível | `failed`, mais `win` para a forma de default | `else` | `outcome` |

#### O que um módulo do programa declara

A tabela lida da direita para a esquerda é o requisito de um módulo escrito
pelo programa: declarar os verbos de um protocolo é o que basta para participar
da construção correspondente. Não há registro, marcação nem permissão a pedir,
e o PPC não distingue um módulo da base de um módulo do usuário ao resolver.

Duas observações completam o requisito. A forma dos argumentos vem do bit
`byref` do modificador (§4.3), e não do protocolo. E as aridades escritas nas
assinaturas são as que valem: um módulo que declare `ptr` de duas aridades
serve a `x[i]` e a `x[i,j]`, e um que declare só uma serve a uma forma só.

#### O que o núcleo conhece pelo nome

Fora dos protocolos, três pontos ligam uma construção a um módulo determinado.
A lista é fechada:

| Onde | O que o núcleo assume |
| --- | --- |
| `arena` | quatro verificações da tradução nomeiam este módulo: procedência do retorno, origem de `from_array`, constância de `from_stack` e uso de filha depois do reset do pai (§5.2) |
| `a..b` | o literal de intervalo produz um `range` (§5.3) |
| `parallel` | o nome do bloco declara um símbolo de tipo `parallel.control` (§5.7) |

O módulo `keel`, do prelúdio, é o quarto caso e é de outra espécie: ele não
declara operação, apenas os nomes de tipo primitivos, cuja grafia participa das
regras de declaração da §4.2.

#### Convenção de grafia

keel não legisla grafia de identificador. O que segue é a convenção que a base
segue, e que um módulo do programa pode seguir ou não:

| O que a base declara | Grafia |
| --- | --- |
| tipos, modificadores e verbos | minúscula — `arena`, `slice`, `corot`, `slot`, `has_next` |
| conjuntos de tags | maiúscula inicial — `Status` |
| constantes de tag e de enum | caixa alta — `SUCCESS`, `ONGOING`, `FAILED` |

A maiúscula do conjunto não é ornamento: nome de conjunto é nome de tipo, e
`types` o injeta nu no arquivo de quem importa. Uma palavra comum em minúscula
ali dentro colidiria com identificadores do programa a cada import.

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

#### 1. Finalidade

Fornecer armazenamento por região e delimitar a validade dos dados derivados.

#### 2. Sintaxe

```keel
arena a;
arena.from_array(a, bytes);
arena.from_stack(a, N);
arena.from_parent(a, pai, n);
arena.from_memory(a, p, n);
T *dados = arena.alloc(a, T, quantidade);
```

`arena` é um tipo que representa uma estrutura de controle de memória em
pilha, com região e topo de alocação. Sua declaração C é um
`typedef struct { ... } arena;`: `arena a;` permanece `arena a;` no C emitido.
Os verbos recebem o descritor por referência. Os construtores
inicializam o descritor do primeiro argumento; `from_stack` é usado em corpo
de função e cria armazenamento no escopo correspondente. `N` é constante
admitida por keel. As demais operações são chamadas com resolução da §4.4.

#### 3. Reconhecimento

- `from_array` exige símbolo conhecido como `array u8`; `from_stack` exige
  literal ou `constexpr` conhecido como constante.
- `alloc` lê o tipo `T` escrito no segundo argumento, sem inferir o tipo pelo
  destino. A implementação C obtém seu tamanho e alinhamento.
- A procedência registra construções e relações conhecidas entre arenas,
  incluindo a origem local e as relações de pai e filha. Uma reatribuição que
  perde essa procedência marca a origem como desconhecida; a análise não
  presume que a variável local do ponteiro implique armazenamento local.

#### 4. Semântica

- A declaração de `arena` segue as regras C de inicialização. O PPC não
  acrescenta inicializador: `arena a;` declara o objeto e `arena a = {0};`
  o inicializa explicitamente como descritor vazio. Nesse estado, sua
  capacidade é zero e as alocações falham.
- Os quatro construtores devolvem `bool`, verdadeiro quando a capacidade
  resultante é maior que zero. Origem nula, tamanho zero ou falta de espaço
  no pai deixam uma arena válida e vazia, com resultado falso.
- `from_array` usa o armazenamento do vetor; `from_stack` cria um vetor
  automático de tamanho constante; `from_parent` reserva `n` bytes no pai e
  os usa como região da filha.
- `from_memory` recebe uma região externa de `n` bytes. Ela pode ter duração
  automática ou vir de um alocador. A arena não adquire nem libera a região.
- `alloc(a,T,n)` reserva espaço para `n` objetos de `T`, com alinhamento
  determinado por `T`. O padding consome capacidade. Falta de espaço ou
  transbordamento de `n * sizeof(T)` devolve `NULL`.
- `mark(a)` obtém o topo atual; `restore(a,m)` restaura um topo previamente
  marcado; `reset(a)` volta ao topo zero. `length(a)` mede bytes ocupados e
  `capacity(a)` a capacidade total. Reset e restore não liberam memória ao
  sistema nem executam limpeza dos objetos descartados.
- Resetar ou restaurar o pai invalida suas filhas e os dados derivados. Um
  reset posterior da filha não revalida a região invalidada.
- A saída do escopo de uma filha não devolve ao pai o espaço reservado.
  A duração do descritor não determina a duração do armazenamento.
- `defer` é explícito e opcional. keel não o insere nem garante liberação
  de memória externa por reconhecer uma arena.

#### 5. Restrições e diagnósticos

| Condição | Responsável | Identificador |
| --- | --- | --- |
| Tamanho de `from_stack` não é constante conhecida | keel | `arena-stack-nao-constante` |
| Origem de `from_array` não é `array u8` | keel | `arena-from-array-nao-u8` |
| Retorno de contêiner com procedência conhecida em armazenamento local | keel | `arena-escape` |
| Uso conhecido de filha após reset/restore do pai, na varredura do mesmo escopo | keel | `arena-filha-apos-reset` |
| Multiplicação do tamanho de alocação transborda | backend, em execução debug | `alloc-overflow` |

A recusa de parâmetros por valor de `arena` segue o diagnóstico `byref-param`. O
diagnóstico `arena-filha-apos-reset` incide no uso posterior, inclusive `reset(filha)`, e não na
operação que reutiliza a memória do pai.

#### 6. Pré-condições e limites

- Antes de consultar ou alocar, o programa deve inicializar o descritor,
  por um dos construtores ou por inicialização C válida.
- O programa garante validade, extensão e condições de acesso da região
  externa. Seu armazenamento não pode expirar enquanto houver uso dos dados
  derivados. Para liberação explícita, usa-se o mecanismo da origem.
- Um endereço entregue por `arena.alloc` não deve ser entregue diretamente a
  `free` ou `realloc`. O programa não deve usar objetos descartados por reset
  ou restore; uma marca deve pertencer à arena e a um topo ainda restaurável.
- A promessa de alinhamento por alocação não resolve as regras C de tipo
  efetivo. Respaldo em armazenamento de tipo declarado exige a rota de alvo
  documentada pelo backend; `from_memory` também conserva as condições da
  memória que recebeu, sem presumir origem em `malloc`.
- A análise de escape é lexical e limitada à procedência conhecida. Relações
  ocultas por chamadas C, parâmetros de saída ou cópias não constituem uma
  prova geral de ausência de escape ou de uso após invalidação.
- A arena não sincroniza seu topo. Acesso concorrente requer organização pelo
  programa; uma arena por worker pode ser recortada antes da execução paralela.

#### 7. Exemplo mínimo

Com os tipos e as funções da base já declarados:

```keel
//keel
arena a;
arena.from_memory(a, memoria, bytes);
i32 *dados = arena.alloc(a, i32, n);
```

```c
//C gerado
arena a;
keel_arena_from_memory(&a, memoria, bytes);
i32 *dados = keel_arena_alloc_n(&a, n, sizeof(i32), _Alignof(i32));
```

A região externa permanece sob o contrato de quem forneceu `memoria`.

#### 8. Referências

- [Rationale: memória por região](keel-rationale.md#memória-por-região).
- [Rationale: tipo arena e aplicação de modificadores](keel-rationale.md#arena-é-um-tipo).
- [Backend: arena](keel-c-backend.md#54-arena), incluindo o respaldo de tipo-caractere.

### 5.3 `keel.buffer`, `keel.slice` e `keel.range`

#### 1. Finalidade

Representar sequências de comprimento variável, trechos de extensão fixa e intervalos.

#### 2. Sintaxe

```keel
buffer i32 b = buffer.of(vetor);
buffer i32 vazio = buffer.from(p, capacidade);
slice i32 s = slice.of(b, inicio, fim);
slice i32 externa = slice.from(i32, p, n);
range r = inicio..fim;
```

`buffer` e `slice` modificam o tipo `T` para representar, respectivamente,
uma sequência com controle de comprimento e capacidade e um trecho com
comprimento e ponteiro. `buffer T` e `slice T` são os tipos resultantes.
`range` é um tipo que representa um intervalo. As
operações são chamadas qualificadas. Seus imports são explícitos.

`buffer.cursor` e `slice.cursor` são os tipos de cursor consumidos por `walk`
(§4.7); `partition` é a divisão consumida por `parallel` (§4.8). O programa
não escreve esses verbos: quem os chama é a construção.

#### 3. Reconhecimento

- A instância vem das declarações ou da origem indicada na §4.4.
  `buffer.of(v)` exige `array` conhecido, unidimensional.
- `slice.of` recebe `buffer`, `slice` ou `array` unidimensional de elemento
  compatível. `slice.from(T,p,n)` lê `T` na chamada.
- O cursor e a partição são procurados pelo protocolo da §4.4, como qualquer
  outro verbo. `walk` e `parallel` não conhecem estes módulos pelo nome.
- `a..b` constrói um `range`. Limites são expressões C; `..` é reconhecido
  fora de literais, comentários e diretivas.

#### 4. Semântica

- `buffer` mantém comprimento e capacidade fixa desde a construção. Permite
  crescer até essa capacidade e remover elementos, sem realocação implícita.
  `buffer.of(v)` nasce com comprimento igual à capacidade do vetor;
  `buffer.from(p,cap)` nasce com comprimento zero. Ponteiro nulo resulta em
  capacidade zero, permitindo que inserções falhem pelo canal normal.
- `slice` tem somente comprimento e ponteiro para um trecho contíguo de
  extensão fixa. Não oferece crescimento. Os elementos podem ser alterados
  por índice, `set` ou ponteiro, respeitando `const` e a região válida.
- `slice.of(x)` abrange o comprimento atual da origem. As formas `(x,a,b)`
  e `(x,r)` descrevem `[a,b)`, diretamente ou pelos limites de `r`. Exigem
  `a <= b <= length(x)` e admitem trecho vazio.
- `slice.from(T,p,n)` descreve os `n` elementos afirmados pelo chamador. A
  escrita direta pelo ponteiro não altera o comprimento de um buffer nem
  cria uma operação `set_length`.
- Os descritores não assumem liberação do armazenamento. Cópia de `slice`
  compartilha dados; cópia de `buffer` compartilha dados e duplica seu controle
  de comprimento, com o aviso de `byref`.
- `range` representa `[first,limit)` em `size_t`. `first(r)` e `limit(r)`
  obtêm os limites; `length(r)` é `limit - first`; `get(r,i)` é `first + i`.
  `range` não declara `ptr`, pois não fornece armazenamento de elementos.
- `buffer` e `slice` declaram o cursor de travessia, consumido por `walk`
  (§4.7). O tipo é `buffer.cursor` e `slice.cursor`: uma posição em `size_t`,
  declarada uma vez por módulo e não por instância, porque não depende de `T`.
  `begin(x)` devolve o cursor inicial; `has_next(x,c)` informa se há elemento
  na posição corrente; `next(x,c)` devolve o endereço do elemento corrente e
  avança o cursor. O elemento é entregue por ponteiro, então `walk` sobre esses
  contêineres usa binder por ponteiro.
- `buffer`, `slice` e `range` declaram a divisão consumida por `parallel`
  (§4.8). `partition(x,k,w)` devolve a parte `w` de uma divisão em `k`, e as
  `k` partes são disjuntas e cobrem o contêiner. Para `buffer T` e `slice T` o
  produto é `slice T`, o recorte contíguo `[w*passo, min((w+1)*passo, n))` com
  passo igual ao teto de `n/k`; para `range`, o produto é `range`, com a divisão
  análoga de `[first,limit)`. Parte vazia é o caso normal quando `k` excede o
  comprimento.

| Operação sobre `buffer`, `slice` ou `array` | Efeito e resultado |
| --- | --- |
| `length(x)` | Comprimento; constante de compilação para `array` |
| `capacity(x)` | Capacidade de `buffer` ou total de `array`; ausente em `slice` |
| `get(x,i)` / `set(x,i,v)` | Obtém cópia / escreve elemento existente, sem estender comprimento |
| `ptr(x)` / `ptr(x,i)` | Ponteiro para a sequência / endereço de um elemento |
| `at(x,i)` | `outcome T`: valor se `i < length(x)`, `NONE` caso contrário; verifica em toda build |
| `buffer.push(x)` / `buffer.push(x,v)` | Reserva uma posição / reserva e escreve; incrementa comprimento e devolve seu ponteiro, ou `NULL` se cheio |
| `buffer.pop(x)` | Reduz comprimento e devolve o ponteiro da posição removida, ou `NULL` se vazio |
| `buffer.clear(x)` | Zera comprimento, preservando capacidade e região |
| `buffer.clone(a,x)` / `slice.clone(a,x)` | Aloca em `a`, copia o comprimento da origem e produz `outcome` do contêiner indicado, com falha se a alocação falhar |
| `begin(x)` | Cursor inicial, do tipo declarado pelo módulo; ausente em `array` |
| `has_next(x,c)` | `bool`: há elemento na posição corrente do cursor |
| `next(x,c)` | Endereço do elemento corrente, avançando o cursor |
| `partition(x,k,w)` | Parte `w` de uma divisão em `k` partes disjuntas; ausente em `array` |

A posição devolvida por `pop` continua no armazenamento, embora fora do
comprimento atual, e pode ser reutilizada pela próxima inserção. `push` sem
valor exige que o programa a inicialize antes de ler seu conteúdo. Clones não ampliam o
prazo de validade da arena de destino.

#### 5. Restrições e diagnósticos

| Condição | Responsável | Identificador |
| --- | --- | --- |
| Construção de buffer sobre vetor `const` | keel | `buffer-sobre-const` |
| `get` ou `set` copia elemento que é instância de modificador | keel | `get-copia-conteiner` |
| `set` fora do comprimento | backend, em execução debug | `set-fora-de-length` |
| `buffer.of` sem origem conhecida como `array` | keel | `buffer-of-tamanho` |
| `slice.from` sobre símbolo `ref` | keel | `slice-from-sobre-ref` |
| Intervalo aberto fora de índice | keel | `recorte-aberto` |

Indexação, recortes e suas verificações seguem a §4.5. Qualificadores dos
objetos e compatibilidade das cópias continuam sujeitos ao compilador C.

#### 6. Pré-condições e limites

- Ponteiros e extensões externos devem descrever memória válida e acessível
  pelo tipo escrito. Nenhum descritor prolonga a vida dessa memória.
- `get`, `set` e `ptr(x,i)` exigem índice dentro do comprimento; a verificação
  de debug não permanece em release. `at` conserva sua verificação em release.
- Um intervalo usado como sequência deve ter `first <= limit`; subtração em
  `size_t` não corrige limites invertidos. O índice de `get(r,i)` deve estar
  no comprimento.
- Acesso à posição removida de um buffer não pode pressupor que outra inserção
  preserve seu conteúdo. Crescimento do buffer não aumenta um slice existente.
- Um cursor descreve uma posição, não uma referência ao contêiner: alteração
  estrutural durante a travessia o invalida, como invalidaria um índice guardado.
- Uma parte obtida por `partition` é uma vista sobre a mesma região. Sua
  validade termina com a do armazenamento de origem.

#### 7. Exemplo mínimo

O par da §3.3 mostra recorte, mutabilidade dos elementos e travessia de `range`.

#### 8. Referências

- [Rationale: memória por região](keel-rationale.md#memória-por-região) e [acesso e travessia](keel-rationale.md#acesso-e-travessia).
- [Backend: contêineres](keel-c-backend.md#52-containers-struct-e-funções-static-inline) e [acessor verificado](keel-c-backend.md#513-at--o-acessor-verificado).

### 5.4 `keel.tagged`

#### 1. Finalidade

Associar uma etiqueta de um conjunto declarado a um valor, e consultá-la ou escrevê-la por verbos.

#### 2. Sintaxe

```keel
import keel.tagged as tagged types;

tagged Ciclo void state = {0};
tagged Kind struct Node no = {0};
```

O módulo `keel.tagged` declara o modificador e seus verbos:

```keel
module keel.tagged tags E type T;

pub modifier tagged { i32 tag; T value; }

pub inline i32  tag(tagged *t);
pub inline T    value(tagged *t);
pub inline void mark(tagged *t, E e);
pub inline void set(tagged *t, E e, T v);
```

| Operação | Forma para `T` com valor | Forma sem valor associado |
| --- | --- | --- |
| Consultar a etiqueta | `tagged.tag(t)` | Mesma forma |
| Consultar o valor | `tagged.value(t)` | Não disponível |
| Escrever a etiqueta, preservando o valor | `tagged.mark(t, E)` | Mesma forma |
| Escrever etiqueta e valor | `tagged.set(t, E, v)` | Não disponível |

`void` indica ausência de valor associado, conforme §4.3: a instanciação omite o campo declarado com o parâmetro de tipo. `tagged Ciclo void` contém somente a etiqueta.

**A leitura devolve `i32` e a escrita recebe `E`, e a assimetria é deliberada:**
a etiqueta armazenada pode não pertencer ao conjunto — é o que a verificação de
debug observa —, então `tag` entrega o que está lá; escrever, ao contrário, é o
ponto em que a pertinência pode ser exigida, e o parâmetro tipado pelo conjunto
é o que a exige (§4.3).

#### 3. Reconhecimento

- O modificador é aplicado a dois argumentos, na ordem da assinatura do módulo: o conjunto de tags e o tipo associado (§4.3).
- O conjunto escrito deve nomear uma declaração `tags` conhecida; caso contrário aplica-se `tags-nao-declarado` (§4.3).
- Os verbos são resolvidos pela §4.4, com o objeto no primeiro argumento.

#### 4. Semântica

- A instância contém a etiqueta, em `i32`, e o valor associado. Com `void`, o campo do valor é omitido e restam a etiqueta e seus verbos.
- `tag(t)` devolve a etiqueta corrente; `value(t)` devolve uma cópia do valor.
- `mark(t, E)` escreve a etiqueta preservando o valor; `set(t, E, v)` escreve as duas coisas.
- Um `tagged` declara `tag`, portanto é operando de `match` (§4.9). Declarar `tag` é o que basta: o despacho não exige este módulo.

#### 5. Restrições e diagnósticos

| Condição | Responsável | Identificador |
| --- | --- | --- |
| Parâmetro por valor, se o modificador for declarado `byref` | keel | `byref-param` |
| Acesso direto ao campo da etiqueta ou do valor | keel | `campo-de-instancia` (`warning`) |
| Constante escrita em `mark` ou `set` que não pertence ao conjunto da instância | keel | `tag-de-outro-conjunto` |
| Valor de etiqueta fora da lista | backend, em execução debug | `tag-fora-de-faixa` (`debug`) |

#### 6. Pré-condições e limites

- Um `tagged` cujo campo de etiqueta seja escrito por C opaco fora dos verbos do módulo conserva a representação, mas não a garantia de pertinência.
- A verificação da escrita alcança a constante escrita, e não o valor calculado: `mark(t, X)` com `X` de outro conjunto é recusado, mas `mark(t, n)` com `n` vindo de expressão C passa, e só a verificação de debug o observa.
- O valor associado de um `tagged` tem um único tipo, comum a todas as tags. Um valor por tag não pertence a este contrato.

#### 7. Exemplo mínimo

O par completo, com o conjunto declarado e o despacho, está na §4.9.

#### 8. Referências

- [Rationale: conjuntos fechados e exaustividade](keel-rationale.md#conjuntos-fechados-e-exaustividade).
- [Backend](keel-c-backend.md#52-containers-struct-e-funções-static-inline).

### 5.5 `keel.outcome` e `keel.corot`

#### 1. Finalidade

Representar resultados finais e retornos cooperativos, e consultar ou ajustar seus estados.

#### 2. Sintaxe

`keel.outcome` declara o modificador `outcome`; `keel.corot` declara o tipo
`corot`, o conjunto de tags `Status` e os verbos que o leem e o escrevem:

```keel
import keel.outcome as outcome types;
import keel.corot   as corot   types;

outcome i16 resultado;
outcome void concluido;
corot passo;
```

`outcome T` é o tipo `T` modificado para ter um valor associado a um código em
uma struct marcada por esse código. Seus verbos interpretam e ajustam esses
metadados.

**`corot` não é modificador, e não tem valor associado.** É um tipo de um
campo — o código, em `i32` — cujos três estados são regiões desse campo:

```keel
module keel.corot;

pub tags Status [SUCCESS = -1, ONGOING = 0, FAILED = 1];
pub typedef struct { i32 code; } corot;
```

Ele representa **como uma passagem terminou**, e não o que ela produziu: o que
uma função cooperativa produz viaja pelo contexto que o programa lhe passa. Por
isso não há `corot T`, não há `corot.value` e não há instância por tipo —
`corot` é um tipo, como `arena` e `range`.

`corot.tag(r)` devolve a tag corrente a partir do código, o que torna `corot`
um operando de `match` pelo contrato da §4.9 sem alterar sua representação: o
código continua sendo um único `i32`, e o código de falha continua inteiro.
**O conjunto nomeia as três regiões do código, e não uma segunda
representação:** `tag` é leitura, não armazenamento, e dois códigos de falha
diferentes devolvem a mesma tag.

**Ele é struct de um campo, e não `typedef i32`, de propósito.** Um `typedef` de
inteiro não cria tipo em C, e deixaria passar em silêncio `if (r)`, `r == 0`,
`total += r` e a mistura com o código de um `outcome` — justamente onde o sinal
carrega a tag e a magnitude carrega o código. Com a struct, os quatro são erro
do compilador C, no ponto certo, e o programa escreve o verbo. O custo em
execução é nenhum: struct de um `i32` volta em registrador em toda ABI
corrente.

Com os aliases acima, os nomes qualificados são `outcome.outcome`, do
modificador, e `corot.corot`, do tipo; `outcome` e `corot`, em posição de tipo,
são suas formas abreviadas. Na qualificação de verbos, como `outcome.ok(r)`, o
prefixo nomeia o módulo.

| Operação sobre `corot` | Forma |
| --- | --- |
| Marcar sucesso cooperativo | `corot.win(r)` |
| Marcar andamento | `corot.again(r)` |
| Marcar falha cooperativa | `corot.fault(r, codigo)` |
| Consultar a etiqueta | `corot.tag(r)` |
| Consultar sucesso, andamento ou falha | `corot.ok(r)`, `corot.ongoing(r)`, `corot.faulted(r)` |
| Consultar o código | `corot.code(r)` |

`void` indica ausência de valor associado em `outcome void`: os predicados e o
código continuam disponíveis; não há campo de valor nem operação `value` nessa
instância.

| Operação sobre `outcome` | Forma para `T` com valor | Forma sem valor associado |
| --- | --- | --- |
| Marcar sucesso, preservando o valor | `outcome.win(r)` | Mesma forma |
| Marcar sucesso e escrever o valor | `outcome.win(r, valor)` | Não disponível |
| Marcar falha, preservando o valor | `outcome.fail(r, codigo)` | Mesma forma |
| Marcar ausência, preservando o valor | `outcome.none(r)` | Mesma forma |
| Consultar resultado final | `outcome.ok(r)`, `outcome.failed(r)`, `outcome.code(r)` | Mesmas formas |
| Ler ou escrever valor final | `outcome.value(r)`, `outcome.value(r, valor)` | Não disponíveis |

Os verbos dos dois módulos recebem o objeto no primeiro argumento. O PPC
obtém a instância — ou, no caso de `corot`, o tipo — de sua declaração
reconhecida, pela resolução comum da §4.4; não a procura no destino da
atribuição nem no retorno da função chamadora. Os verbos de escrita exigem
objeto modificável. Recebem seu endereço na emissão C; um símbolo já declarado
como ponteiro fornece esse endereço diretamente.

`win`, `fail` e `none` modificam o objeto recebido e devolvem uma cópia dele
após o ajuste. Esse retorno é do verbo; encerrar a função chamadora exige
`return outcome.win(r, valor);`, por exemplo. As consultas não modificam o
objeto. O setter `value(r, valor)` modifica apenas o valor e tem retorno `void`.

#### 3. Reconhecimento

- Os nomes dos tipos e os retornos das funções são consultados nas declarações conhecidas. O parser não deduz o tipo de uma expressão C arbitrária para escolher o protocolo.
- `corot.win`, `corot.again` e `corot.fault` são verbos do módulo: modificam o objeto recebido e devolvem uma cópia ajustada. Não retornam da função. Encerrar a função exige `return corot.win(r, valor);`. O cleanup da §4.6 se aplica a esse `return` como a qualquer outro.
- `corot.faulted` é uma operação de consulta do módulo. A grafia anterior `corot.failed` não é mantida como alias, pois voltaria a incluir `corot` no protocolo de `else`.
- Os verbos são resolvidos pela §4.4, com o objeto no primeiro argumento.

#### 4. Semântica

##### Estados e códigos

| Código | `outcome` | `corot` |
| --- | --- | --- |
| `< 0` | Falha ou ausência; `failed` é verdadeiro | Sucesso; `ok` é verdadeiro |
| `= 0` | Resultado válido; `ok` é verdadeiro | Andamento; `ongoing` é verdadeiro |
| `> 0` | Falha ou ausência; `failed` é verdadeiro | Falha cooperativa; `faulted` é verdadeiro |

- O código é `i32`. `outcome.OK` é zero; `outcome.NONE` é o menor valor de `i32` e identifica ausência.
- `outcome` tem dois estados. Códigos de erro positivos ou negativos são admitidos; zero é reservado ao resultado válido. Ausência, como em um optional sem valor, pertence ao mesmo lado de `failed`.
- `corot` tem três estados. `!corot.faulted(r)` inclui sucesso e andamento; não equivale a `corot.ok(r)`.
- `corot.fault(r, codigo)` exige código estritamente positivo. Não converte, troca o sinal ou normaliza o código: zero significaria andamento e negativo significaria sucesso.
- O código é avaliado uma única vez. Os verbos cooperativos não suspendem uma ativação nem alteram o fluxo: a saída da função é escrita pelo programa.
- `corot.win(r)` escreve `SUCCESS`; `corot.again(r)` escreve `ONGOING`; `corot.fault(r, codigo)` escreve o código da falha. Nenhuma delas carrega valor: o tipo não tem campo de valor, e o que a passagem produziu está no contexto do programa.
- `outcome.win(r)` escreve código zero, preserva o valor associado e devolve
  o objeto ajustado. `outcome.win(r, valor)` escreve código zero e o valor
  fornecido e devolve o objeto ajustado. Em `outcome void`, existe somente
  a forma `win(r)`.
- `outcome.fail(r, codigo)` exige código diferente de zero, escreve esse
  código sem normalização, preserva o valor associado e devolve o objeto.
- `outcome.none(r)` escreve `outcome.NONE`, preserva o valor associado e
  devolve o objeto. Ausência não introduz um terceiro estado no protocolo.
- `outcome.ok(r)` testa `code == 0`; `outcome.failed(r)` testa `code != 0`;
  ambos devolvem `bool`. `outcome.code(r)` devolve o código `i32`.
  `outcome.value(r)` devolve uma cópia do valor `T`, sem alterar o objeto.
- Cada argumento é avaliado uma vez segundo as regras de chamada C. Não há
  avaliação de um suposto destino implícito nem garantia adicional de ordem
  relativa entre os argumentos.

##### Composições e valor associado

- `match` é controle de fluxo e não produz resultado (§4.9). As composições `routine.seq` e `routine.par` são chamadas de função e produzem um `outcome u32`, conforme a §5.6.
- O resultado da composição não é o valor de uma participante: é o código da política e a contagem de sucessos. Qualquer outro valor é estabelecido pelo programa depois da chamada.
- `outcome.value(r, valor)` escreve somente o valor associado, mantendo o código. O setter pode ser usado durante a finalização mesmo que o código indique falha. Escrever um valor não torna esse resultado válido.
- `corot` não declara `failed`, portanto não participa diretamente de `else`. Uma composição concluída participa por seu resultado `outcome`.
- `corot` declara `tag` e seu módulo declara o conjunto `Status`, portanto participa de `match`. Isso é independente do protocolo de `else`.

#### 5. Restrições e diagnósticos

| Condição | Responsável | Identificador |
| --- | --- | --- |
| Código conhecido de `corot.fault` é zero ou negativo | keel | `cofault-codigo-invalido` (`error`) |
| Primeiro argumento não fornece objeto reconhecido exigido pelo verbo | keel | `fora-da-gramatica-de-conteiner` (`error`) |
| Argumento, retorno ou atribuição com tipos C incompatíveis | compilador C | Diagnóstico do compilador C |

O diagnóstico `cofault-codigo-invalido` cobre todo código conhecido não positivo. A verificação usa literais e valores de constantes conhecidos nos limites da §4.2; não calcula expressões C para descobrir o sinal.

#### 6. Pré-condições e limites

- Para um código obtido de expressão C não avaliada por keel, o programa deve garantir `codigo > 0` em `corot.fault` e `codigo != 0` em `outcome.fail`.
- O objeto deve ter armazenamento válido e ser modificável nos verbos de escrita. `win(r)`, `fail(r,c)` e `none(r)` não inicializam o valor associado; os exemplos usam `= {0}` para fornecer um objeto inicializado antes dessas operações.
- A leitura de `outcome.value(r)` exige resultado válido e valor estabelecido.
- O tempo de vida de ponteiros e descritores transportados como valor associado continua sendo o do armazenamento de origem.

#### 7. Exemplo mínimo

O receptor determina a operação, inclusive quando não há atribuição:

```keel
outcome i16 r = {0};
outcome.value(r, 12);       // valor 12, código preservado
outcome.fail(r, -7);       // código -7, valor 12 preservado
outcome.win(r);            // código 0, valor 12 preservado
outcome.win(r, 42);        // código 0, valor 42
outcome.none(r);           // código NONE, valor 42 preservado
```

A emissão dos verbos está no backend §5.14. O par completo da §3.4 mostra
retorno explícito da função e default sobre o próprio objeto.

O par de produção e consulta cooperativa é distinto do protocolo de resultado final:

```keel
//keel
priv corot tentar(bool indisponivel) {
    corot r = {0};
    if (indisponivel) return corot.fault(r, 7);
    return corot.win(r);
}

pub bool houve_falha(bool indisponivel) {
    corot passo = tentar(indisponivel);
    return corot.faulted(passo);
}
```

C da operação essencial, em um módulo `exemplo`:

```c
//C gerado
#include <stdbool.h>
#include <stdint.h>

typedef struct { int32_t code; } keel_corot;

static keel_corot exemplo_tentar(bool indisponivel) {
    if (indisponivel) return (keel_corot){7};
    return (keel_corot){-1};
}

bool exemplo_houve_falha(bool indisponivel) {
    keel_corot passo = exemplo_tentar(indisponivel);
    return passo.code > 0;
}
```

Imports explícitos e o uso de `else` com `outcome` aparecem nos exemplos §§3.4–3.6.

#### 8. Referências

- [Rationale: resultados finais e estados cooperativos](keel-rationale.md#resultados-finais-e-estados-cooperativos).
- [Rationale: resultado recebido pelo verbo](keel-rationale.md#resultado-recebido-pelo-verbo).
- [Rationale: produção e consulta da falha cooperativa](keel-rationale.md#produção-e-consulta-da-falha-cooperativa).
- [Backend](keel-c-backend.md): §5.14 para resultados, §5.12 para `else`.

### 5.6 `keel.routine`

#### 1. Finalidade

Compor chamadas cooperativas descritas em uma tabela de entradas, em sequência ou por política de conclusão, sem construção do núcleo.

#### 2. Sintaxe

A composição é chamada de função da base. O módulo `keel.routine` declara o tipo da função participante, o slot da tabela e as composições; o estado de cada entrada é um `corot void` da §5.5:

```keel
module keel.routine type C;
import keel.corot   as corot   types;
import keel.outcome as outcome types;
import keel.slice   as slice   types;

pub typedef corot (*routine)(C *ctx);

pub modifier slot { routine f; C *ctx; corot state; }

pub inline corot       state(slot *s);
pub inline i32         code(slot *s);

pub inline outcome u32 seq(slice slot s);
pub inline outcome u32 par(slice slot s, u32 target);
pub inline u64         mask(slice slot s);
```

No uso, o programa escreve a tabela e chama a composição:

```keel
array routine.slot Ctx passos[3] = {
    { .f = preparar, .ctx = &ctx },
    { .f = medir,    .ctx = &ctx },
    { .f = gravar,   .ctx = &ctx },
};

outcome u32 r = routine.par(slice.of(passos), 2);
if (outcome.ok(r)) concluir(&ctx);
```

O parâmetro do módulo é o tipo do contexto, e só ele. O estado de saída não é parametrizado: `corot` é tipo, e não modificador (§5.5).

Uma participante é uma função de retorno `corot` e um parâmetro `C *`. Essa é a assinatura da tabela; funções de outra forma exigem outra tabela ou um adaptador escrito pelo programa. O que uma participante produz viaja pelo contexto, e não pelo retorno.

**A linha da tabela chama-se `slot`, e não `entry`**, porque ela não é só entrada: guarda a função, o contexto e o estado com que aquela participante terminou. Tirar o estado de lá exigiria um segundo recorte, paralelo ao primeiro, com dois comprimentos a manter em acordo — o que a composição escreve fica onde a composição já está.

`routine` é um `typedef` de instância (§4.3): é emitido por instância e o programa não precisa escrevê-lo, porque escreve nomes de função nos campos `.f`. Quem precisar nomear o tipo declara o próprio `typedef` sobre a mesma forma C.

`slice slot` recebe tanto um vetor marcado com `array` quanto um `buffer` montado em execução, por `slice.of`. A composição não aloca, não é dona da tabela e não a redimensiona.

O alvo de `par` é um valor de execução. Zero significa todos os slots; `1` corresponde à antiga política `ANY`; `N` expressa o alvo numérico de sucessos.

#### 3. Reconhecimento

- `seq`, `par`, `mask`, `state` e `code` são operações do módulo, resolvidas pela §4.4. Não há palavra do núcleo envolvida na composição.
- A compatibilidade entre o campo `f` de uma entrada e a função escrita é verificada pelo compilador C, a partir do tipo `routine` da instância. Tabelas de contextos diferentes são tipos diferentes, e a incompatibilidade aparece no inicializador.
- `corot` é o retorno exigido das participantes. Isso é contrato do tipo `routine`, não uma verificação keel sobre uma lista de chamadas.
- O estado de um slot é um `corot`, então `routine.state(s)` é operando das consultas de `corot` (§5.5) e de `match` (§4.9), sem regra nova.

#### 4. Semântica

##### Ativações e contexto

- Cada chamada participante executa até um retorno normal. Não há suspensão de frame, continuação implícita nem pilha preservada pela composição.
- `ONGOING` é um estado do resultado lógico, não uma pausa da ativação C. Os `defer` da participante são executados nas saídas previstas por seu contrato, inclusive quando ela retorna `ONGOING`.
- O contexto que persiste entre chamadas é explícito: é o `C *` do slot. Recursos que atravessam chamadas pertencem a esse contexto ou ao chamador, que determina sua duração e liberação.
- A composição mantém apenas o controle necessário: o estado corrente de cada entrada e os contadores de sucesso e falha.

##### `seq`

- Os slots são etapas na ordem do recorte.
- A etapa corrente é chamada até retornar um estado diferente de `ONGOING`.
- `SUCCESS` avança para a próxima etapa; `FAILED` encerra a sequência imediatamente, sem chamar as etapas seguintes.
- Quando todas as etapas terminam com `SUCCESS`, a composição tem sucesso.
- A composição não entrega `ONGOING` ao chamador.

##### `par`

- A composição executa ciclos. Em cada ciclo, chama uma vez cada slot ainda em `ONGOING`, na ordem do recorte.
- Um slot que retorna `SUCCESS` ou `FAILED` conserva esse estado e não é chamado nos ciclos seguintes.
- A política é avaliada ao final de cada ciclo, depois das chamadas previstas para aquele ciclo. Mais slots podem obter sucesso no ciclo do que o mínimo exigido.
- Com `m` slots e alvo `q`, onde `q` é `m` quando o alvo escrito é zero, e com `S` sucessos e `F` falhas acumulados: a composição tem sucesso quando `S >= q` e falha quando `m - F < q`. Enquanto nenhuma condição for satisfeita, inicia outro ciclo.
- Quando a política é resolvida, slots ainda em `ONGOING` deixam de ser chamados. Não há ativação suspensa a cancelar, chamada de encerramento nem protocolo de interrupção.
- `par` é composição cooperativa; não solicita threads nem simultaneidade como `parallel`.

##### Resultado

- O retorno é `outcome u32`. O código é zero no sucesso e positivo na falha da política. O valor associado é a quantidade de slots que terminaram em `SUCCESS`.
- Antes do primeiro ciclo, a composição escreve `ONGOING` no estado de cada slot; uma tabela inicializada com zeros já está nesse estado.
- O estado de cada slot permanece legível depois da chamada, por `routine.state(s)`, ou como inteiro por `routine.code(s)`. A tabela é percorrível pelos contratos da §4.7.
- Não há transporte automático de valor entre o `corot` de uma participante e o resultado da composição. O que antes seria escrito em uma região de finalização é agora o statement seguinte à declaração do resultado, sujeito às regras comuns de `return`, `goto`, `defer` e `else`.
- `mask` devolve um `u64` cujo bit `i` indica que o slot `i` terminou em `SUCCESS`. É uma conveniência derivada da tabela, não a representação do resultado.

#### 5. Restrições e diagnósticos

| Condição | Responsável | Identificador |
| --- | --- | --- |
| Entrada por valor de instância `byref` em parâmetro | keel | `byref-param` (`error`) |
| Tipo da função escrita incompatível com `routine` da instância | compilador C | Diagnóstico do compilador C |
| Alvo de `par` maior que o número de slots | backend, em execução debug | `par-alvo-acima-do-total` (`debug`) |
| `mask` sobre recorte com mais de 64 entradas | backend, em execução debug | `mask-acima-de-64` (`debug`) |

A composição não acrescenta verificações de fluxo ao corpo das participantes. Ela é uma função: saltos, retornos e cleanup do chamador seguem os contratos das §§4.6 e 5.5.

#### 6. Pré-condições e limites

- A tabela deve ter armazenamento válido durante toda a composição e ser modificável: `par` e `seq` escrevem o estado de cada slot. Uma tabela `const` não satisfaz esse contrato.
- Um mesmo recorte não deve alimentar duas execuções concorrentes da composição.
- A disponibilidade de recursos e a duração do contexto são responsabilidades do programa. Não existe suspensão que estenda a vida de variáveis locais de uma participante.
- Uma composição pode continuar indefinidamente se suas participantes não produzirem estados suficientes para resolver a política. Não há timeout implícito.
- A chamada indireta por ponteiro de função impede a inserção em linha das participantes pelo compilador C. Esse custo é a contrapartida de a composição ser biblioteca e não construção do núcleo.
- `keel.routine` usa o tipo `corot` de `keel.corot`, por import comum. Nenhum módulo se instancia a si mesmo, e o grafo de genéricos continua acíclico.

#### 7. Exemplo mínimo

O par completo, com tabela estática, política e leitura do estado por slot, está na §3.6.

#### 8. Referências

- [Rationale: composição como biblioteca](keel-rationale.md#composição-como-biblioteca).
- [Rationale: resultados finais e estados cooperativos](keel-rationale.md#resultados-finais-e-estados-cooperativos).
- [Backend](keel-c-backend.md): §5.11, para a emissão dos ciclos; §5.14, para representação dos resultados.

### 5.7 `keel.parallel`

#### 1. Finalidade

Fornecer o tipo do símbolo de controle de um bloco `parallel` e as consultas sobre a execução.

#### 2. Sintaxe

O nome declara, no escopo que contém o bloco, um símbolo com esse nome e tipo
`parallel.control`, declarado pelo módulo `keel.parallel`. É por ele que o
programa consulta a execução, dentro e depois do bloco:

```keel
module keel.parallel;

pub typedef struct { /* … */ } control;

pub inline bool interrupted(control *c);
pub inline bool ok(control *c);
pub inline bool failed(control *c);
pub inline u32  wins(control *c);
```

| Consulta | Verdadeira quando |
| --- | --- |
| `parallel.interrupted(n)` | o pedido de interrupção está ativo |
| `parallel.ok(n)` | a política foi satisfeita |
| `parallel.failed(n)` | algum worker terminou por `fail` |
| `parallel.wins(n)` | — devolve a quantidade de workers que terminaram com sucesso |

O corpo do worker é código comum. Percorrer a parte é escolha do programa,
por `foreach`, por `walk` ou por qualquer outra forma; `parallel` não percorre.
#### 3. Reconhecimento

- O símbolo é declarado pela construção `parallel` (§4.8), e não pelo programa. Consultá-lo exige o import do módulo.
- `parallel` seguido de `.` é qualificação de módulo, e não a palavra da construção, pela resolução de `.` da §2.3.
- Os verbos são resolvidos pela §4.4, com o símbolo de controle no primeiro argumento.

#### 4. Semântica

- O objeto é inicializado antes da distribuição das partes e vale enquanto o escopo que o contém existir.
- `interrupted(c)` é a única consulta com leitura definida dentro do corpo do worker; as demais dependem de workers que ainda executam.
- Depois do bloco, as quatro consultas descrevem a execução terminada e não mudam mais.
- As consultas não sincronizam nada e não são pontos de saída: ler `interrupted` não encerra o worker.

#### 5. Restrições e diagnósticos

Este módulo não acrescenta diagnósticos. Os da construção estão na §4.8.

#### 6. Pré-condições e limites

- O programa não pode inferir, de uma leitura de `interrupted`, quanto trabalho os demais workers realizaram.
- O objeto pertence ao escopo do bloco: guardá-lo além dele, por ponteiro, não conserva significado definido.

#### 7. Exemplo mínimo

O par da §4.8 mostra a declaração implícita do símbolo e sua consulta depois do bloco.

#### 8. Referências

- [Rationale: políticas e sinalização](keel-rationale.md#políticas-e-sinalização-de-interrupção).
- [Backend: parallel](keel-c-backend.md#59-parallel).

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
| `sem-module` | Arquivo sem `module` como primeiro token significativo | `error` | keel | §4.1 |
| `module-fora-do-caminho` | Nome declarado em `module` divergente do caminho relativo à raiz | `error` | keel | §4.1 |
| `stem-invalido` | Stem de arquivo que não é identificador C válido | `error` | keel | §4.1 |
| `stem-ambiguo-por-caixa` | Nomes de arquivo diferindo apenas por caixa | `error` | keel | §4.1 |
| `simbolo-colidido` | Colisão de símbolos exportados no módulo e no fecho transitivo de imports | `error` | keel | §4.1 |
| `import-circular` | Import circular, com a cadeia completa na mensagem | `error` | keel | §4.1 |
| `extern-c-aninhado` | `extern_c` em posição aninhada, isto é, fora do nível de arquivo | `error` | keel | §4.1 |
| `main-em-extern-c` | `main` definida dentro de `extern_c` | `error` | keel | §4.1 |
| `main-privada` | `priv` aplicado a `main` | `error` | keel | §4.1 |
| `main-assinatura` | `main` assinada fora das duas formas do C | `error` | keel | §4.1 |
| `import-ordem-trocada` | `import M types as m;` — a mensagem dá a forma correta | `error` | keel | §4.1 |
| `types-duplicado` | Dois imports com `types` injetando o mesmo nome nu | `error` | keel | §4.1 |
| `types-sombreado` | Nome injetado por `types` sombreado por declaração local | `warning` | keel | §4.1 |
| `types-injetados` | Lista dos nomes que `types` injetou, no ponto do import | `info` | keel | §4.1 |
| `pub-static` | `pub static` sem `inline` em escopo de arquivo | `error` | keel | §4.1 |
| `inline-sem-visibilidade` | `static inline` sem `pub`/`priv` em nível de módulo | `error` | keel | §4.1 |
| `static-em-tipo` | `static` aplicada a tipo | `error` | keel | §4.1 |
| `nome-acima-do-teto` | Nome gerado acima do teto de comprimento do alvo | `error` | Backend / compilador C | §4.2 |
| `fora-da-gramatica-de-conteiner` | Expressão fora da gramática de contêiner em posição de contêiner | `error` | keel | §4.4 |
| `tipo-c-como-argumento` | Palavra-chave de tipo aritmético C como argumento de modificador, exceto `char` e `bool` | `error` | keel | §4.2 |
| `nome-reservado` | Identificador do usuário no espaço reservado `keel_` | `error` | Backend / compilador C | §4.2 |
| `redeclaracao-de-simbolo` | Padrão local de possível redeclaração de símbolo conhecido, conforme §2.5 | `error` | keel | §2.5 |
| `buffer-sobre-const` | `buffer` sobre vetor C `const` — a mensagem indica `slice const T` | `error` | keel | §5.3 |
| `get-copia-conteiner` | `get` ou `set` sobre elemento que é instância de modificador | `error` | keel | §5.3 |
| `set-fora-de-length` | `set` com índice fora de `length` | `debug` | Backend, em execução | §5.3 |
| `buffer-of-tamanho` | `buffer.of` de um argumento sobre símbolo que não é `array` | `error` | keel | §5.3 |
| `array-1d-em-parametro` | `array` unidimensional em parâmetro de função | `error` | keel | §4.2 |
| `array-sem-dimensao-em-parametro` | `array T v[]` sem dimensão em parâmetro | `error` | keel | §4.2 |
| `array-indexacao-parcial` | Indexação parcial de `array` multidimensional | `error` | keel | §4.2 |
| `view-sobre-array-nd` | `keel.ptr`, `buffer.of` ou `slice.of` sobre `array` multidimensional | `error` | keel | §4.2 |
| `dim-k-nao-constante` | Índice de `keel.dim(v,k)` sem valor decimal conhecido | `error` | keel | §4.2 |
| `ref-sem-inicializador` | `ref` sem inicializador | `error` | keel | §4.2 |
| `ref-aritmetica` | Aritmética ou indexação sobre `ref` | `error` | keel | §4.2 |
| `slice-from-sobre-ref` | `slice.from` sobre `ref` | `error` | keel | §5.3 |
| `arena-stack-nao-constante` | `arena.from_stack` com tamanho não constante; a mensagem indica `arena.from_parent` | `error` | keel | §5.2 |
| `arena-from-array-nao-u8` | `arena.from_array` sobre símbolo que não é `array u8` | `error` | keel | §5.2 |
| `arena-escape` | Retorno de contêiner cuja procedência conhecida é armazenamento local | `error` | keel | §5.2 |
| `defer-sem-bloco` | `defer` como corpo de statement de controle sem chaves | `error` | keel | §4.6 |
| `defer-em-escopo-de-arquivo` | `defer` em escopo de arquivo | `error` | keel | §4.6 |
| `salto-sobre-defer` | Entrada externa em escopo por cima de registro de `defer`, inclusive `case`/`default` posterior no mesmo corpo de `switch` | `error` | keel | §4.6 |
| `later-com-captura` | `later` seguido de lista de captura, dentro do colchete do `defer` | `error` | keel | §4.6 |
| `delimitador-sem-par` | Chave, parêntese ou colchete sem par — inclusive dentro de `extern_c` | `error` | keel | §2.5 |
| `chaves-em-ramos` | Alternativas de um grupo condicional que discordam na contagem de delimitadores | `error` | keel | §2.5 |
| `define-sobre-keel` | `#define` ou `#undef` de palavra contextual keel ou nome `keel_` | `error` | keel | §2.5 |
| `literal-com-newline` | Literal de string ou char com newline não-emendado | `error` | keel | §2.5 |
| `sombreamento` | Sombreamento de palavra contextual, de verbo ou de nome de módulo | `warning` | keel | §2.5 |
| `byref-atribuido` | Atribuição entre instâncias de modificador `byref`, nomeando o aliasing | `warning` | keel | §4.3 |
| `defer-em-bloco-de-controle` | `defer` registrado em corpo de `if`, `else` ou `switch` | `warning` | keel | §4.6 |
| `campo-de-instancia` | Acesso direto a campo de instância de modificador | `warning` | Backend / compilador C | §4.4 |
| `import-indireto` | Uso de símbolo de módulo não importado diretamente | `info` | keel | §4.1 |
| `modifier-fora-de-generico` | `modifier` fora de módulo genérico | `error` | keel | §4.3 |
| `nome-de-parametro` | Declaração de símbolo com o nome de um parâmetro do módulo | `error` | keel | §4.3 |
| `generico-circular` | Módulo genérico que se instancia, com a cadeia na mensagem | `error` | keel | §4.3 |
| `ciclo-de-layout` | Cadeia de tipos que se contêm por valor atravessando instância de modificador, com a cadeia na mensagem | `error` | keel | §4.3 |
| `instance-fora-de-arquivo` | `instance` fora de escopo de arquivo | `error` | keel | §4.3 |
| `instance-nao-modificador` | Argumento de `instance` que não é modificador de módulo genérico | `error` | keel | §4.3 |
| `instance-inutil` | `instance` sobre genérico inteiramente `pub inline` | `warning` | keel | §4.3 |
| `byref-param` | Instância `byref` por valor em parâmetro — `arena`, `buffer` e todo modificador marcado | `error` | keel | §4.3 |
| `arena-filha-apos-reset` | Uso de arena filha depois de `reset`/`restore` do pai, no mesmo escopo | `error` | keel | §5.2 |
| `tags-sem-nome` | `tags` sem nome | `error` | keel | §4.9 |
| `tags-nome-repetido` | Dois conjuntos de tags com o mesmo nome no módulo | `error` | keel | §4.9 |
| `tags-lista-vazia` | Conjunto de tags sem nenhuma tag | `error` | keel | §4.9 |
| `tags-valores-parciais` | Conjunto que mistura tags com e sem valor escrito | `error` | keel | §4.9 |
| `tag-valor-nao-constante` | Valor de tag sem literal decimal ou constante conhecida | `error` | keel | §4.9 |
| `tags-nao-declarado` | Argumento de parâmetro `tags` que não nomeia conjunto declarado | `error` | keel | §4.3 |
| `tag-de-outro-conjunto` | Tag escrita que não pertence ao conjunto exigido — rótulo de `match`, ou constante em verbo com parâmetro `tags` | `error` | keel | §4.9 |
| `tag-repetida` | Tag repetida no mesmo conjunto | `error` | keel | §4.9 |
| `rotulo-de-tag-repetido` | Rótulo repetido no mesmo `match` | `error` | keel | §4.9 |
| `match-sem-tag` | Operando de `match` sobre tipo que não declara `tag` | `error` | keel | §4.9 |
| `match-sem-conjunto` | Operando cujo módulo não declara conjunto de tags | `error` | keel | §4.9 |
| `match-conjunto-ambiguo` | Operando cujo módulo declara mais de um conjunto, sem parâmetro `tags` que decida | `error` | keel | §4.9 |
| `cofault-codigo-invalido` | `corot.fault(r,c)` com código conhecido zero ou negativo | `error` | keel | §5.5 |
| `tag-fora-de-faixa` | Etiqueta fora da lista declarada | `debug` | Backend, em execução | §4.9 |
| `aridade-sem-ptr` | Índice de aridade N sobre modificador sem `ptr` dessa aridade — a mensagem lista as que existem | `error` | keel | §4.5 |
| `nao-percorrivel` | `foreach` sobre tipo que não declara `length`, ou `get`/`ptr` conforme o binder | `error` | keel | §4.7 |
| `nao-percorrivel-por-cursor` | `walk` sobre tipo que não declara `begin`, `has_next` e `next` | `error` | keel | §4.7 |
| `walk-sem-cursor` | `walk` sem o binder de cursor | `error` | keel | §4.7 |
| `cursor-de-outro-tipo` | Tipo do binder de cursor diferente do produto declarado de `begin` | `error` | keel | §4.7 |
| `nao-particionavel` | `parallel` sobre tipo que não declara `partition` | `error` | keel | §4.8 |
| `particao-de-outro-tipo` | Tipo do binder de partição diferente do produto declarado de `partition` | `error` | keel | §4.8 |
| `binder-copia-conteiner` | Binder por valor de `foreach` ou `walk` sobre elemento que é instância de modificador — a mensagem indica `T *`. Não se aplica ao binder de partição de `parallel` | `error` | keel | §4.7 |
| `mutacao-na-travessia` | `push`, `pop` ou `clear` sobre o contêiner percorrido ou particionado, no corpo do `foreach`, do `walk` ou do `parallel` | `error` | keel | §4.7 |
| `indice-nao-size-t` | Binder de índice cujo tipo não é `size_t` | `error` | keel | §4.7 |
| `nao-contavel` | `foreach` de um binder sobre tipo que não declara `first` e `limit` | `error` | keel | §4.7 |
| `recorte-sem-of` | Índice por intervalo sobre tipo que não declara o `of` da aridade que a forma exige | `error` | keel | §4.5 |
| `recorte-invertido` | Recorte com limites decimais conhecidos e início maior que fim | `error` | keel | §4.5 |
| `recorte-fora-de-faixa` | Intervalo cujos limites violam `a <= b <= length(x)` | `debug` | Backend, em execução | §4.5 |
| `foreach-dois-binders-em-literal` | `foreach` de dois binders sobre literal de intervalo — a mensagem indica nomear o intervalo | `error` | keel | §4.7 |
| `binder-ponteiro-em-intervalo` | Binder por ponteiro na forma de intervalo | `error` | keel | §4.7 |
| `recorte-aberto` | Recorte com ponta aberta fora de índice | `error` | keel | §4.7 |
| `recorte-aberto-com-indice` | `x[a..]` sobre caminho que contém índice ou verbo | `error` | keel | §4.5 |
| `enum-sem-o-tipo` | Constante de enum escrita sem o nível do tipo | `error` | keel | §4.2 |
| `alias-e-tipo-colidem` | Alias de módulo e tipo de origens distintas têm a mesma grafia no arquivo | `error` | keel | §2.5 |
| `parallel-sem-nome` | `parallel` sem nome | `error` | keel | §4.8 |
| `verbo-fora-de-parallel` | `win` ou `fail` de fluxo fora do corpo de worker | `error` | keel | §4.8 |
| `parallel-aninhado` | `parallel` aninhado em corpo de `parallel` | `error` | keel | §4.8 |
| `parallel-nome-repetido` | Dois `parallel` com o mesmo nome na mesma função | `error` | keel | §4.8 |
| `parallel-nao-constante` | `k` ou política de `parallel` que não é constante de compilação | `error` | keel | §4.8 |
| `tag-fora-da-lista` | Rótulo que não está na lista declarada do conjunto | `error` | keel | §4.9 |
| `tag-sem-rotulo` | Tag na lista declarada sem rótulo correspondente no corpo | `error` | keel | §4.9 |
| `par-alvo-acima-do-total` | Alvo de `routine.par` maior que o número de slots | `debug` | Backend, em execução | §5.6 |
| `mask-acima-de-64` | `routine.mask` sobre recorte com mais de 64 entradas | `debug` | Backend, em execução | §5.6 |
| `restrict-em-conteiner` | `restrict` escrito antes de um modificador — a `note` dá a forma com ponteiro | `error` | keel | §4.2 |
| `formato-estreito-indisponivel` | Módulo usa `f16` ou `bf16` e o alvo não oferece o formato | `error` | Backend / compilador C | §4.2 |
| `dim-nao-constante` | Argumento de `dim` sem literal decimal ou `constexpr` de inicializador decimal conhecido | `error` | keel | §4.3 |
| `dim-gera-declaracao` | Uso de `dim` para gerar declarações em vez de substituir um número | `error` | keel | §4.3 |
| `alloc-overflow` | `arena.alloc` cujo `n * sizeof(T)` não cabe em `size_t` | `debug` | Backend, em execução | §5.2 |
| `nome-canonico-colidido` | Duas declarações do mesmo módulo produzindo o mesmo nome canônico | `error` | keel | §4.2 |
| `modificador-chamado-instance` | Modificador declarado com o nome `instance` | `error` | keel | §4.3 |
| `qualificador-errado` | Qualificador incompatível com o receptor ou produto do verbo | `error` | keel | §4.4 |
| `from-sem-alvo` | Verbo que depende do tipo do alvo fora de inicialização, atribuição a símbolo conhecido ou retorno | `error` | keel | §4.4 |
| `captura-escrita` | Atribuição a escalar capturado, no corpo de um `parallel` | `error` | keel | §4.8 |
| `else-sem-inicializador` | Cláusula `else` em declaração sem inicializador | `error` | keel | §4.10 |
| `else-tipo-nao-falivel` | Cláusula `else` sobre tipo que não declara `failed`, inclusive ponteiro ou escalar | `error` | keel | §4.10 |
| `else-em-atribuicao` | Cláusula `else` sobre atribuição a símbolo existente | `error` | keel | §4.10 |
| `else-multiplos-declaradores` | Cláusula `else` em declaração com mais de um declarador | `error` | keel | §4.10 |
| `else-default-sem-win` | Cláusula `else` na forma de default sobre tipo falível que não declara `win` | `error` | keel | §4.10 |
| `declarador-enterrado` | Declarador cujo nome não é o último token, onde keel precisa reconstruir a declaração — `constexpr`, captura de `[now]`, tipo de retorno sob `defer`. A `note` manda usar `typedef` | `error` | keel | §4.2 |
| `constexpr-agregado` | `constexpr` com declarador de vetor ou inicializador entre chaves | `error` | keel | §4.2 |
| `return-em-parallel` | `return` no corpo de um `parallel`: o corpo do worker não sai da função que o contém | `error` | keel | §4.8 |
| `defer-later-sombreado` | `defer` sem `[now]` cujo corpo nomeia símbolo redeclarado em escopo mais interno com ponto de saída — a `note` dá as duas saídas, `[now]` ou `goto` | `error` | keel | §4.6 |
| `constexpr-endereco` | `&` sobre símbolo `constexpr`, ou uso que exija lvalue — a `note` dá a saída, `static const T k = K;` | `error` | keel | §4.2 |
| `dim-abaixo-de-um` | Argumento de `dim` que resolve para valor menor que 1 — a mensagem dá a cadeia de instanciação | `error` | keel | §4.3 |
| `alias-com-argumento` | Alias de módulo seguido de `(` — vaga reservada; a `note` manda renomear o alias | `error` | keel | §2.5 |

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
