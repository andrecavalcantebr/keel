# keel — contexto compacto para geração de código

Este arquivo é autocontido: não depende de nenhum outro documento do
repositório para ser lido ou usado. Ele existe para caber no contexto de uma
IA que vai **gerar código `.k`** (a fonte `keel`) — não C: o C é trabalho do
`cgen`, o transpilador, e de quem escreve `.k`. Os pares .k/.c aqui mostrados 
o são não para mostrar a semântica correta de `keel`.

keel é um parser de ilhas sobre C: um arquivo `.k` é C comum mais um pequeno
conjunto de construções próprias, reconhecidas por forma léxica. O resto do
arquivo — toda expressão, toda chamada de função, todo `if`/`for`/`switch` —
é C, copiado para o C gerado sem reescrita.

O arquivo tem cinco partes:

1. **Gramática** — a forma de cada construção própria.
2. **Regras que a gramática não mostra** — o que decide se um programa é
   aceito, além da forma.
3. **A API da base** — os módulos que vêm com keel, no próprio código-fonte
   deles (comentado, e pensado para ser lido).
4. **Pares keel/C** — exemplos pequenos, com o C que cada um gera.
5. **Usando o `cgen`** — como invocar a ferramenta, o que ela escreve e como
   ler o resultado.

Os comentários do código da parte 3 citam seções de documentos externos
(`spec §5.3`, `backend §5.2`) por proveniência — quem escreveu aquele módulo
estava registrando de onde veio a decisão. Isso é informativo, não é preciso
resolver a citação para entender ou usar o código.

---

## 1. Gramática

Notação: `::=` define uma produção; `|` separa alternativas; `{ … }` repete
zero ou mais vezes; `[ … ]` é opcional; `( … )` agrupa; `'…'` é a grafia de um
terminal; `IDENT` é um identificador que não é palavra C; `NUM` é um token
numérico; `STRING` é um literal de string; `<opaque>` é uma sequência de
tokens delimitada pelo contexto, sem análise de C — pode conter construções
keel, que são reconhecidas e traduzidas, com o resto preservado; `<opaque-no-parens>`
é igual, mas sem `(` no nível externo da região. "Nível externo" é a
profundidade de delimitadores da produção em análise — `(`, `[`, `{`, literais
e diretivas não contam como abertura para esse efeito.

### Palavras C que keel reconhece

O conjunto é fechado e igual nos dois perfis (C11, C23): `void char short int
long float double signed unsigned bool const volatile restrict inline auto
register extern static typedef struct union enum constexpr typeof
typeof_unqual thread_local` (tipos e declarações); `if else switch case
default for while do break continue goto return` (fluxo); `sizeof alignof
alignas static_assert true false nullptr` (operadores e literais); `_Alignas
_Alignof _Atomic _BitInt _Bool _Complex _Decimal32 _Decimal64 _Decimal128
_Generic _Imaginary _Noreturn _Static_assert _Thread_local` (formas com
sublinhado). Essas palavras não casam `IDENT`: `unsigned long x` não é uma
sequência de três identificadores.

### Palavras keel e onde valem

| Palavra | Onde vale |
| --- | --- |
| `module` | primeira construção do arquivo |
| `import`, `import_c`, `extern_c` | nível de arquivo |
| `type_h` | dentro do `[ ]` depois de `extern_c` |
| `as`, `types` | na linha `import`, nesta ordem |
| `pub`, `priv` | antes de declaração de arquivo |
| `array` | antes do tipo, em declaração de vetor, campo, parâmetro e coluna de `extent` |
| `constexpr` | declaração de arquivo ou de bloco; também palavra C |
| `ref` | qualificador depois de `*` no declarador |
| `dim`, `tags`, `type` | na linha `module`, parâmetros do módulo; `tags` também inicia um conjunto; `type` também declara parâmetro de função |
| `modifier`, `byref` | declaração de modificador em módulo genérico |
| `instance` | declaração de arquivo; nenhum modificador pode ter esse nome |
| `extent` | declaração de arquivo, seguida de `struct`, do nome e de `[` |
| `defer`, `now`, `later` | statement de corpo de função; `now`/`later` dentro do `[ ]` do `defer` |
| `foreach`, `walk` | statement de corpo de função, seguido de `(` |
| `parallel`, `ALL`, `ANY`, `win`, `fail` | `parallel` seguido do nome e da política; `ALL`/`ANY` como política; `win;`/`fail;` no corpo do worker |
| `match` | statement de corpo de função, seguido de `(` |
| `else` | cauda de declaração ou de atribuição; também palavra C |

Nomes da base (`arena`, `buffer`, `slice`, `tagged`, `outcome`, `corot`,
`routine`) não são palavras-chave: são tipos/módulos comuns, resolvidos pelos
símbolos importados. `parallel` é o único nome que ocupa as duas posições —
palavra do núcleo em `parallel nome política (…)`, módulo em
`parallel.ok(nome)` — distinguidas pelo `.`. Nomes de operação (`length`,
`get`, `alloc`) também não são palavras-chave.

As formas `x[i]`, `x[i,j]`, `x[a..b]`, `x[a..]`, `x[..b]`, `x[..]`, `a..b` são
sintaxe, não vocabulário: o sentido depende do símbolo e das regras de
protocolo (seção 2).

### Como keel estende C

Só três coisas:

**Especificadores de tipo formados por modificador** — o modificador se
aplica a tipos, conjuntos de tags, valores de `dim`, e o declarador continua
C: `buffer i32 xs;` (modifier `buffer`, tipo `i32`, declarador `xs`).

**Declarações próprias**: `module`, `import`, `import_c`, `extern_c`, `tags`,
`modifier`, `instance`, `extent`, `constexpr`, os marcadores `array` e `ref`,
`protocol`.

**Estruturas de controle**: `defer`, `foreach`, `walk`, `parallel`, `match`,
a cauda `else`, `win;`/`fail;`.

Todo o resto — `arena.alloc`, `slice.of`, `outcome.win`, `routine.par` — são
funções comuns de módulos, chamadas como `m.f(…)`. Não são gramática: são
API, descrita na seção 3.

### Unidade e declarações de topo

```ebnf
unit      ::= decl-module { top-item }

decl-module  ::= 'module' module-name [ dim-binder ] [ tags-binder ]
                 [ type-binder ] ';'
module-name  ::= IDENT { '.' IDENT }
dim-binder   ::= 'dim' IDENT { ',' IDENT }
tags-binder  ::= 'tags' IDENT { ',' IDENT }
type-binder  ::= 'type' IDENT { ',' IDENT }

top-item    ::= import | import-c | extern-c | top-decl

import       ::= 'import' module-name [ 'as' IDENT ] [ 'types' ] ';'
import-c     ::= 'import_c' ( system-header | STRING ) ';'
extern-c     ::= [ 'pub' | 'priv' ] 'extern_c' [ '[' 'type_h' ']' ] '{' <opaque> '}'

top-decl    ::= [ 'pub' | 'priv' ]
                 ( decl-modifier | decl-instance | decl-tags | decl-extent
                 | decl-protocol
                 | decl-typedef | decl-struct | decl-function | decl-keel
                 | <opaque> )

decl-modifier ::= 'modifier' IDENT [ 'byref' ] '{' <opaque> '}'
decl-protocol ::= 'protocol' IDENT 'type' IDENT '{' { proto-item } '}'
                | 'protocol' IDENT '[' qualified-name { ',' qualified-name } ']' ';'
proto-item    ::= 'type' IDENT ';'
                | return-type fn-declarator ';'
decl-instance ::= 'instance' known-type ';'
decl-tags     ::= 'tags' IDENT tags-list ';'
tags-list     ::= '[' tag-item { ',' tag-item } ']'
tag-item      ::= IDENT [ '=' tag-value ]
tag-value     ::= [ '-' ] ( NUM | qualified-name )

decl-typedef  ::= 'typedef' ( specifier | struct-spec | <opaque-no-parens> )
                  ( declarator [ 'byref' ]
                  | declarator { ',' declarator } ) ';'
decl-struct   ::= struct-spec [ declarator { ',' declarator } ] ';'
struct-spec   ::= ( 'struct' | 'union' ) [ IDENT ] '{' { field } '}'
field         ::= decl-keel | decl-array | <opaque> ';'

decl-extent   ::= 'extent' 'struct' IDENT extent-dim { extent-dim }
                  '{' { extent-field } '}' ';'
extent-dim    ::= '[' IDENT ',' ( qualified-name | NUM ) ']'
extent-field  ::= extent-column | <opaque> ';'
extent-column ::= 'array' argument column-decl { ',' column-decl } ';'
column-decl   ::= { '*' } IDENT dimensions | '*' { '*' } IDENT

decl-function ::= return-type fn-declarator ( block | ';' )
return-type   ::= { spec-c } ( specifier | <opaque-no-parens> )
fn-declarator ::= { '*' { qual-c } } IDENT '(' [ params ] ')'
params        ::= 'void' | param { ',' param } [ ',' '...' ]
param         ::= param-array | param-type | param-proto
                | { role } { qual-arg } ( specifier | <opaque-no-parens> )
                  [ declarator | abstract-declarator ]
abstract-declarator ::= { '*' { qual-c } } [ '(' abstract-declarator ')' ]
                        { suffix }
spec-c        ::= 'inline' | 'static' | 'extern' | '_Noreturn'
                | '_Thread_local' | 'thread_local'
                | ( 'alignas' | '_Alignas' ) '(' <opaque> ')'
                | '[[' <opaque> ']]' | qual-c
param-array   ::= { spec-c } { role } 'array' argument { '*' } IDENT param-dims
param-dims    ::= '[' size-binder { ',' <opaque> } ']'
                | '[' size-binder ']' { '[' <opaque> ']' }
                | dimensions
size-binder   ::= 'size_t' IDENT
param-type    ::= 'type' IDENT
role          ::= 'parent' | 'child' | 'invalidates' | 'consumes'
param-proto   ::= qualified-name [ IDENT ] IDENT
```

`system-header` é a forma `<…>` de header, usada por `import_c`. O corpo de
`extern-c` é preservado, sem mangling de nomes, sem inspeção por keel.
`decl-typedef` registra como tipo o nome de cada declarador: em 
`typedef int a[4], *b;`, `a` é vetor de `int` e `b` é ponteiro para `int`. `struct-spec`
registra os campos que casam `decl-keel`/`decl-array`; os demais são opacos.
`size-binder` liga um nome à dimensão 0 do vetor recebido — só a dimensão 0
admite isso. `param-type` declara um parâmetro que recebe um **tipo** escrito
na chamada (`arena.alloc(a, u16, 100)`, onde `u16` ocupa esse parâmetro).
`role` (`parent`/`child`/`invalidates`/`consumes`) marca uma função (ou a
chamada a ela) para o rastreio de procedência da seção 2: o compilador C não
vê papel nenhum, mas o próprio keel segue, e erra em cima deles
(`region-escape`, `child-region-after-invalidation`) — não é documentação
inerte.

### Tipos e declarações

```ebnf
decl-keel    ::= { spec-c } specifier init-decl { ',' init-decl } ';'
               | { spec-c } specifier declarator '=' <opaque> else-tail
               | decl-array | decl-constexpr
init-decl    ::= declarator [ '=' <opaque> ]

else-tail   ::= 'else' ( block | <opaque> ';' )
assign-else ::= IDENT '=' <opaque> else-tail

specifier   ::= known-type | tagged-type
known-type  ::= modifier argument { argument } | named-type
named-type  ::= qualified-name
modifier    ::= qualified-name [ '(' dim-value { ',' dim-value } ')' ]
dim-value   ::= NUM | qualified-name
argument    ::= { qual-arg } ( known-type | tagged-type | base-type | 'void' )
                { qual-arg }
tagged-type ::= ( 'struct' | 'union' | 'enum' ) qualified-name
qual-arg    ::= 'const' | 'volatile' | '_Atomic'
base-type   ::= 'char' | 'bool'
qualified-name ::= IDENT { '.' IDENT }

declarator   ::= { '*' { qual-c } } direct-declarator
direct-declarator ::= IDENT { suffix }
                    | '(' declarator ')' { suffix }
suffix       ::= '[' [ <opaque> ] ']' | '(' [ <opaque> ] ')'
qual-c       ::= 'const' | 'volatile' | 'restrict' | '_Atomic' | 'ref'

decl-constexpr ::= 'constexpr' <opaque> [ IDENT '=' <opaque> ] ';'

decl-array   ::= { spec-c } 'array' argument decl-array-1 { ',' decl-array-1 } ';'
decl-array-1 ::= { '*' } IDENT dimensions [ '=' <opaque> ]
dimensions    ::= '[' [ <opaque> { ',' <opaque> } ] ']'
               | '[' [ <opaque> ] ']' { '[' <opaque> ']' }
```

`void` em posição de argumento de modificador omite o campo parametrizado
(não existe objeto C de tipo `void`): `outcome void` é um resultado sem valor
associado. `named-type` casa um nome registrado como tipo (um `typedef` keel,
um conjunto de tags); um nome registrado como **modificador**, como `buffer`,
exige o tipo ao qual se aplica: `buffer T` é o tipo resultante, e o número de
argumentos é fixado pelo módulo que declara o modificador, não é livre. Em
`decl-constexpr`, o grupo `[ IDENT '=' <opaque> ]` só entra quando há `'='` no
nível externo antes do `;`: nesse caso `IDENT` é o token imediatamente
anterior ao `'='`.

### Statements e blocos

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

stmt-keel    ::= decl-keel | decl-tags | assign-else
               | defer | foreach | walk | parallel
               | match | worker-exit | extern-c

defer        ::= 'defer' [ '[' defer-options ']' ] defer-body
defer-options ::= 'later' | 'now' typed-capture
typed-capture ::= entry { ',' entry }
entry        ::= <opaque> IDENT
defer-body   ::= block | <opaque> ';'

foreach      ::= 'foreach' '(' binder ',' binder ':' container ')' stmt
               | 'foreach' '(' binder ':' countable ')' stmt
binder       ::= binder-type IDENT
binder-type  ::= { qual-arg } ( specifier | base-type | 'auto' | <opaque-no-parens> )
                 { '*' { qual-c } }
walk         ::= 'walk' '(' binder [ ',' binder ] ':' container ')' stmt

parallel     ::= 'parallel' IDENT policy
                 '(' binder ':' countable ';'
                     binder ':' container
                     [ ';' capture ] ')' block
policy       ::= 'ALL' | 'ANY' | NUM | qualified-name
capture      ::= '(' IDENT { ',' IDENT } ')'
worker-exit  ::= ( 'win' | 'fail' ) ';'

match        ::= 'match' '(' container ')' tags-block
tags-block   ::= '{' { tag-arm } '}'
tag-arm      ::= tag-label { tag-label } { stmt }
tag-label    ::= IDENT ':'
```

`extern-c` dentro de `stmt-keel` é reconhecido só para ser recusado (não é
permitido abrir `extern_c` dentro de uma função). Dentro de `tags-block`, os
rótulos externos identificam tags; cada braço abre um escopo até o próximo
rótulo ou a chave final; rótulos consecutivos sem statements entre eles
compartilham o braço seguinte; um rótulo C dentro de um braço precisa estar
dentro de um bloco (senão seria lido como tag).

### Contêineres, intervalos e `range-index`

```ebnf
container    ::= IDENT
               | container '[' <opaque> { ',' <opaque> } ']'
               | container '[' range-index ']'
               | container '.'  IDENT
               | container '->' IDENT
               | verb '(' container { ',' <opaque> } ')'
               | '*' container | '&' container | '(' container ')'

countable    ::= interval | container
interval     ::= <opaque> '..' <opaque>
range-index  ::= [ <opaque> ] '..' [ <opaque> ]
verb         ::= module-name '.' IDENT

call         ::= ( verb | IDENT ) '(' [ call-arg { ',' call-arg } ] ')'
call-arg     ::= argument | <opaque>

keel-expr    ::= call | container | interval
```

`container` descreve só as formas em que keel consulta a identidade de um
contêiner a partir de símbolos conhecidos — não atribui tipo a expressão C
arbitrária; a assinatura da operação chamada decide qual argumento ocupa essa
posição. Em `call`, um `IDENT` sem qualificador é função do módulo corrente
ou nome injetado por `types`; uma chamada desconhecida permanece C comum.

---

## 2. Regras que a gramática não mostra

A forma por si só não decide se um programa é aceito. O essencial:

- **Visibilidade.** Sem `pub` nem `priv`, a declaração é `pub`. `priv` tira
  da interface (do `.h`), não muda por si só o *linkage* C.
- **Protocolo é estrutural, não declarado.** `foreach`, `walk`, `match`,
  `else`, indexação, `range-index` e `at` resolvem olhando se o **tipo tem os
  verbos certos** no módulo que o declarou — não existe `implements`. Os
  protocolos da base (seção 3, `keel.k`) são: `Countable` (`first`,`limit`),
  `IndexPtr`/`IndexGet` (`length`+`ptr`/`get`), `Sliceable` (`length`+`as_slice`),
  `Traversable` (`begin`,`has_next`,`next` — usado por `walk`),
  `Partitionable` (`partition` — usado por `parallel`), `Taggable` (`tag` —
  usado por `match`), `Failable`/`Winnable` (usados pela cauda `else`).
- **Um módulo declara protocolo próprio com `protocol Nome type C { … }`**:
  dentro das chaves, `type nome;` declara um tipo associado e cada
  `retorno verbo(parâmetros);` é um verbo exigido, sem corpo — a declaração
  não emite C. `protocol Nome [A, B];` compõe protocolos existentes (a união
  dos verbos). Conformidade continua estrutural e não se declara: um tipo
  atende quando o módulo dele tem os verbos pedidos, com o mesmo nome,
  aridade e posição do receptor. Um parâmetro cujo tipo é um protocolo torna
  a função genérica sobre ele (`size_t count(IndexPtr items)`); cada tipo
  concreto usado numa chamada gera uma instância, o corpo só pode chamar
  sobre esse parâmetro os verbos do protocolo (não outros que o tipo
  concreto tenha), e `C.nome` (com `IDENT C` entre o protocolo e o
  parâmetro) designa um tipo associado ligado pelo retorno do implementador.
  Papel escrito no protótipo do verbo do protocolo tem de ser repetido igual
  pelo implementador, ou a ausência de papel no implementador herda o do
  protótipo; discordar é `protocol-role-mismatch`.
- **Papel (`parent`/`child`/`invalidates`/`consumes`) alimenta um rastreio de
  procedência que o próprio keel confere — não é documentação solta.** Numa
  chamada reconhecida, o produto `child` (o parâmetro `child`, ou quem recebe
  o retorno `child` por declaração/atribuição simples) passa a depender de
  todo argumento `parent` daquela chamada. `invalidates` invalida os
  produtos anteriores do argumento, mas não o argumento; `consumes` invalida
  o próprio argumento junto com os produtos. Usar depois um símbolo inválido
  é `child-region-after-invalidation`; devolver da função algo cuja
  procedência inclui armazenamento local dela (um `array` automático) é
  `region-escape`. O rastreio é lexical — segue só chamada reconhecida,
  nunca `goto`, alias, cópia crua ou chamada opaca — e **não confere o corpo
  da função contra os papéis que ela declara**: é o programa que garante que
  a implementação cumpre o que o papel promete.
- **`byref` proíbe passagem por valor e cópia entre instâncias.** `buffer`
  é `byref`: sempre por ponteiro. `slice`, `outcome`, `tagged`, `routine.slot`
  não são `byref`: passam por valor/cópia, o que permite compor sem
  temporário (`slice.length(slice.of(b))`).
- **Módulo genérico (`module M type T`, `dim N`, `tags E`) é opaco em `T`.**
  Dentro dele, um valor de tipo `T` não participa de protocolo: `foreach`,
  `walk`, `match`, indexação não resolvem sobre `T` puro. Uma declaração que
  não cita nenhum parâmetro nem modificador é emitida **uma vez** para o
  módulo (ex.: `outcome.OK`, `buffer.cursor`), não uma vez por instância.
- **Parâmetro `type X` de função: duas espécies, nunca as confunda (§4.4,
  regras 10-20).** Decidida só pela declaração, nunca pela chamada.
  **Seleção**: `X` é o `type` da própria linha `module` (`keel.slice type T;`
  e `from(type T, T *p, size_t n)` usam o mesmo `T`) — o tipo escrito na
  chamada escolhe a instância, e some do C (`slice.from(f32, p, n)` →
  `keel_slice_f32_from(p, n)`, nenhum `sizeof` nem `alignof`). **Apagado**: `X`
  não é parâmetro do módulo (ou o módulo não tem `type`, só `dim`/`tags`) —
  vira dois `size_t` (tamanho, alinhamento) na assinatura, e a chamada escreve
  `sizeof`/`alignof` do tipo escrito; a função é **uma só** no C, sem
  instância nem nome derivado de `X` (`arena.alloc(a, i32, n)` →
  `keel_arena_alloc2(a, sizeof(i32), alignof(i32), n)`, sempre a mesma
  função). **Um módulo `dim`/`tags` sem `type` nunca tem seleção (regra 12)**:
  `type M` ali é sempre apagado, e não escolhe dimensão nenhuma — a instância
  (por `dim`) só vem do alvo da declaração/atribuição/`return` (regra 4 da
  tabela acima desta lista), nunca de um argumento de tipo. Tentar as duas
  coisas no mesmo argumento (seleção de dimensão **e** apagamento) é o erro
  mais comum nessa área. O tipo escrito num argumento apagado também não
  pode ser de outro módulo (regra 20, `type-arg-foreign`) — hoje a ferramenta
  ainda recusa isso. Passar `dim`/`type` como argumento para **gerar código
  especializado por substituição de fichas** (sem erasure, sem struct opaca)
  é o **molde** (`keel_code`), proposto e não implementado — ver
  `design/possibilidades.md` e `design/mold-impacto.md`.
- **`instance M.mod ARGS;` só decide onde o corpo mora**, não gera a
  instância: geração segue o **uso**. Uma função `pub` sem `inline` de
  genérico precisa de uma declaração `instance` em algum módulo do programa
  para o corpo fora de linha existir.
- **`outcome` é resultado final, com código e valor.** Extração é sempre
  explícita: `outcome.value(r)`. Nunca converte sozinho para o tipo do valor.
  `outcome.OK` é sucesso; qualquer outro código é falha, e `outcome.NONE`
  marca "nada a reportar" (ex.: `at` fora da faixa).
- **`corot` é estado cooperativo de uma função que se chama em ciclos.**
  Três estados: `again` (ainda não terminou), `win` (terminou com sucesso),
  `fault(r, código)` (terminou com falha). `match` sobre um `corot`/`tagged`
  despacha por tag; o laço que chama de novo é escrito pelo programa —
  `match` não laça por si.
- **`defer corpo;` registra limpeza que corre nas saídas da função**, em
  ordem reversa de registro. Com `return expr;`, `expr` é avaliado **antes**
  da limpeza (o valor já está guardado quando a limpeza roda).
- **`parallel nome política (...)` não percorre.** Chama `partition` uma vez
  por worker e liga a parte ao binder; percorrer a parte é escolha do corpo
  (`walk`, `foreach`, ou laço escrito à mão). Uma variável capturada por
  valor é cópia por worker: escrever nela é erro (`captured-write`); a saída
  tem que ir a um contêiner indexado por posição disjunta (como `totals[w]`).
  Política `ALL`: sucesso quando ninguém falhou. `ANY N`: sucesso com `N`
  vitórias; permite interromper os demais com `win;`/`fail;` dentro do
  worker.
- **`extent struct nome [dim, cap] { campos }`** declara uma struct com
  colunas (`array T *campo`) e os dois inteiros de controle, com acesso
  verificado pelos nomes das colunas — é o jeito de keel de ter uma "coluna
  de array" com limite conhecido, útil para tabelas/contêineres do programa
  que `buffer`/`slice` não cobrem porque têm mais de um campo por posição.
- **`match` sobre um valor marcado por `tags`.** Cada rótulo do corpo tem que
  ser uma tag do conjunto; um valor fora do conjunto é um erro de execução
  verificável (ligado por padrão, desligável na invocação do `cgen`).
- **`array T nome[N]`** é vetor C comum, de dimensão conhecida em tempo de
  geração; indexação (`nome[i]`) é verificada contra `N` quando as
  verificações estão ligadas. `buffer` é sequência de tamanho variável, que
  possui seu armazenamento (`byref`, cresce até a capacidade). `slice` é uma
  vista sobre armazenamento de outra coisa (um `array`, um `buffer`, outro
  `slice`): não possui, não cresce, passa por valor.
- **Mangling, `#line`, divisão entre `.h`/`.type.h`/`.c`** são decisão do
  `cgen`, não da fonte `.k`: escrever `.k` nunca exige prever o nome C final
  de nada.

---

## 3. A API da base

Onze módulos: o prelúdio `keel` (tipos primitivos e os protocolos citados na
seção 2) e dez módulos `keel.*`. Nenhum é importado implicitamente, exceto o
prelúdio. O código abaixo é a fonte real, completa.

### `keel` — prelúdio

```keel
module keel;

import_c <stdint.h>;
import_c <stddef.h>;
import_c <float.h>;

// fixed widths, signed and unsigned. bool, char, size_t, ptrdiff_t and
// uintptr_t keep their plain C meaning — they are not remapped.
pub typedef int8_t  i8;   pub typedef uint8_t  u8;
pub typedef int16_t i16;  pub typedef uint16_t u16;
pub typedef int32_t i32;  pub typedef uint32_t u32;
pub typedef int64_t i64;  pub typedef uint64_t u64;
pub typedef float   f32;  pub typedef double   f64;

// The core operations over `array` — keel.length, capacity, dim, get, set,
// ptr and at — are qualified by this module but generic over the element;
// their arity and dimension come from the declaration.

// The protocols of the base: the named contracts the core constructions
// consume. A protocol emits no C. Conformance is structural: a type meets a
// protocol when its module declares the matching verbs.

pub protocol Countable type C {
    type index;
    index first(C *x);
    index limit(C *x);
}
pub protocol IndexPtr type C {
    type elem;
    size_t length(C *x);
    child elem *ptr(parent C *x, size_t i);
}
pub protocol IndexGet type C {
    type elem;
    size_t length(C *x);
    elem get(C *x, size_t i);
}
pub protocol Sliceable type C {
    type view;
    size_t length(C *x);
    child view as_slice(parent C *x, size_t a, size_t b);
}
pub protocol Traversable type C {
    type cursor;
    type elem;
    child cursor begin(parent C *x);
    bool has_next(C *x, cursor *c);
    child elem *next(parent C *x, cursor *c);
}
pub protocol Partitionable type C {
    type part;
    child part partition(parent C *x, size_t k, size_t w);
}
pub protocol Taggable type C {
    type tagset;
    tagset tag(C *x);
}
pub protocol Failable type C {
    bool failed(C *x);
}
pub protocol Winnable type C {
    type fallback;
    C win(C *x, fallback d);
}
```

### `keel.arena` — memória por região

Um bloco linear: `alloc` avança um topo alinhado; `mark`/`restore` voltam a
um ponto; `reset` esvazia tudo. Não há `free` individual.

```keel
module keel.arena;

pub typedef struct { size_t top, cap; u8 *ptr; } arena;

pub inline bool from_array(child arena *a, parent array u8 storage[size_t n]) {
    *a = (arena){0};
    if (storage && n > 0) {
        a->cap = n;
        a->ptr = storage;
    }
    return a->cap > 0;
}

pub inline bool from_memory(arena *a, u8 *ptr, size_t len) {
    *a = (arena){0};
    if (ptr && len > 0) {
        a->cap = len;
        a->ptr = ptr;
    }
    return a->cap > 0;
}

pub inline bool from_parent(child arena *a, parent arena *owner, size_t n) {
    *a = (arena){0};
    u8 *base = alloc(owner, u8, n);
    if (base) {
        a->cap = n;
        a->ptr = base;
    }
    return a->cap > 0;
}

// raw form: count and element size enter separately; overflow returns NULL.
pub inline child void *alloc(parent arena *a, size_t n, size_t sz, size_t align) {
    KEEL_CHECK(sz == 0 || n <= SIZE_MAX / sz, "alloc-overflow");
    if (sz == 0 || n > SIZE_MAX / sz) return NULL;
    size_t need = n * sz;
    uintptr_t base = (uintptr_t)(a->ptr + a->top);
    size_t pad = (size_t)(-(uintptr_t)base & (align - 1));
    size_t avail = a->cap - a->top;
    if (pad > avail || need > avail - pad) return NULL;
    a->top += pad + need;
    return a->ptr + a->top - need;
}

// `type T` form: size and alignment are derived, the call writes `sizeof`/`alignof`.
pub inline child T *alloc(parent arena *a, type T, size_t n) {
    return alloc(a, n, sizeof(T), alignof(T));
}

pub inline size_t mark(const arena *a) { return a->top; }
pub inline void   restore(invalidates arena *a, size_t m) { if (m <= a->top) a->top = m; }
pub inline void   reset(invalidates arena *a) { a->top = 0; }
pub inline size_t length(const arena *a) { return a->top; }
pub inline size_t capacity(const arena *a) { return a->cap; }
```

### `keel.array` — acesso ao vetor C simples

Sem `type`/modificador próprio: a instância é escolhida pelo tipo do
elemento do vetor dado. `N` é a dimensão que chegou pela declaração.

```keel
module keel.array type T;

import keel.outcome as outcome types;
import keel.slice   as slice   types;
import keel.range   as range   types;

pub inline T    get(array T v[size_t N], size_t i)      { return v[i]; }
pub inline void set(array T v[size_t N], size_t i, T x) { v[i] = x; }
pub inline T   *ptr(array T v[size_t N])                { (void)N; return v; }
pub inline T   *ptr(array T v[size_t N], size_t i)      { return &v[i]; }

// the only accessor checked in release.
pub inline outcome T at(array T v[size_t N], size_t i) {
    outcome T r = {0};
    return i < N ? outcome.win(r, v[i]) : outcome.none(r);
}

pub inline slice as_slice(array T v[size_t N]) { return (slice){ N, v }; }

pub inline slice as_slice(array T v[size_t N], size_t a, size_t b) {
    if (b > N) b = N;
    if (a > b) a = b;
    return (slice){ b - a, v + a };
}

pub inline slice as_slice(array T v[size_t N], range r) { return as_slice(v, r.first, r.limit); }

pub inline size_t length(array T v[size_t N]) { (void)v; return N; }
```

### `keel.buffer` — sequência de tamanho variável, com dono

`byref`: sempre por ponteiro, nunca cópia silenciosa entre instâncias.

```keel
module keel.buffer type T;

import keel.slice   as slice;
import keel.outcome as outcome types;
import keel.range   as range   types;
import keel.arena   as arena   types;

pub modifier buffer byref { size_t cap, len; T *ptr; }

// mentions neither `T` nor `buffer`: emitted once for the module — `keel_buffer_cursor`.
pub typedef struct { size_t i; } cursor;

pub inline buffer from(T *p, size_t n) {           // raw pointer, starts empty
    return (buffer){ p ? n : 0, 0, p };
}

pub inline buffer of(T *p, size_t n) {             // over an array, starts full
    return (buffer){ p ? n : 0, p ? n : 0, p };
}

pub inline size_t length(buffer *b) { return b->len; }
pub inline size_t capacity(buffer *b) { return b->cap; }
pub inline T      get(buffer *b, size_t i) { KEEL_CHECK(i < b->len, "index-out-of-length"); return b->ptr[i]; }
pub inline void   set(buffer *b, size_t i, T v) { KEEL_CHECK(i < b->len, "set-out-of-length"); b->ptr[i] = v; }
pub inline T     *ptr(buffer *b) { return b->ptr; }
pub inline T     *ptr(buffer *b, size_t i) { KEEL_CHECK(i < b->len, "index-out-of-length"); return &b->ptr[i]; }

pub inline child T *push(parent buffer *b) {               // NULL when full
    if (b->len == b->cap) return NULL;
    return &b->ptr[b->len++];
}

pub inline child T *push(parent buffer *b, T v) {
    if (b->len == b->cap) return NULL;
    b->ptr[b->len] = v;
    return &b->ptr[b->len++];
}

pub inline child T *pop(parent buffer *b) {                // NULL when empty
    if (b->len == 0) return NULL;
    return &b->ptr[--b->len];
}

pub inline void clear(buffer *b) { b->len = 0; }

pub inline outcome T at(buffer *b, size_t i) {              // checked, even in release
    outcome T r = {0};
    return i < b->len ? outcome.win(r, b->ptr[i]) : outcome.none(r);
}

pub inline slice.slice as_slice(buffer *b) {
    return (slice.slice){ b->len, b->ptr };
}

pub inline slice.slice as_slice(buffer *b, size_t a, size_t c) {
    if (c > b->len) c = b->len; if (a > c) a = c;
    return (slice.slice){ c - a, b->ptr + a };
}

pub inline slice.slice as_slice(buffer *b, range r) { return as_slice(b, r.first, r.limit); }

pub inline child outcome buffer clone(parent arena *a, buffer *b) {  // allocates in `a`
    outcome buffer r = {0};
    T *data = arena.alloc(a, T, b->cap);
    if (!data) return outcome.none(r);
    if (b->len > 0) memcpy((void *)data, b->ptr, b->len * sizeof(T));
    return outcome.win(r, (buffer){ b->cap, b->len, data });
}

pub inline cursor begin(buffer *b) { (void)b; return (cursor){0}; }
pub inline bool   has_next(buffer *b, cursor *c) { return c->i < b->len; }
pub inline T     *next(buffer *b, cursor *c) { return &b->ptr[c->i++]; }

// `k` disjoint parts covering the buffer; the high-index one may come out empty.
pub inline slice.slice partition(buffer *b, size_t k, size_t w) {
    if (k == 0) return (slice.slice){0, b->ptr};
    size_t step = b->len / k + (b->len % k ? 1 : 0);
    size_t lo = w * step;
    if (lo >= b->len) return (slice.slice){0, b->ptr};
    size_t hi = lo + step;
    if (hi > b->len) hi = b->len;
    return (slice.slice){ hi - lo, b->ptr + lo };
}
```

### `keel.slice` — vista de extensão fixa

Não é `byref`: passa por cópia, o que permite compor sem temporário.

```keel
module keel.slice type T;

import keel.arena   as arena   types;
import keel.outcome as outcome types;
import keel.range   as range   types;

pub modifier slice { size_t len; T *ptr; }

pub typedef struct { size_t i; } cursor;      // emitted once for the module

pub inline slice from(type T, T *p, size_t n) {
    return (slice){ p ? n : 0, p };
}

pub inline size_t length(slice s) { return s.len; }
pub inline T      get(slice s, size_t i) { KEEL_CHECK(i < s.len, "index-out-of-length"); return s.ptr[i]; }
pub inline void   set(slice s, size_t i, T v) { KEEL_CHECK(i < s.len, "set-out-of-length"); s.ptr[i] = v; }
pub inline T     *ptr(slice s) { return s.ptr; }
pub inline T     *ptr(slice s, size_t i) { KEEL_CHECK(i < s.len, "index-out-of-length"); return &s.ptr[i]; }

pub inline outcome T at(slice s, size_t i) {
    outcome T r = {0};
    return i < s.len ? outcome.win(r, s.ptr[i]) : outcome.none(r);
}

pub inline slice as_slice(slice s, size_t a, size_t b) {
    if (b > s.len) b = s.len;
    if (a > b) a = b;
    return (slice){ b - a, s.ptr + a };
}

// `slice.of` is a function over `Sliceable`: one instance per concrete type
// (array, buffer, or anything else that declares `length`+`as_slice`).
pub inline C.view of(Sliceable C x) { return as_slice(x, 0, length(x)); }
pub inline C.view of(Sliceable C x, range r) { return as_slice(x, r.first, r.limit); }
pub inline C.view of(Sliceable C x, size_t a, size_t b) { return as_slice(x, a, b); }

pub inline child outcome slice clone(parent arena *a, slice s) {
    outcome slice r = {0};
    T *data = arena.alloc(a, T, s.len);
    if (!data) return outcome.none(r);
    if (s.len > 0) memcpy((void *)data, s.ptr, s.len * sizeof(T));
    return outcome.win(r, (slice){ s.len, data });
}

pub inline cursor begin(slice s) { (void)s; return (cursor){0}; }
pub inline bool   has_next(slice s, cursor *c) { return c->i < s.len; }
pub inline T     *next(slice s, cursor *c) { return &s.ptr[c->i++]; }

pub inline slice partition(slice s, size_t k, size_t w) {
    if (k == 0) return (slice){0, s.ptr};
    size_t step = s.len / k + (s.len % k ? 1 : 0);
    size_t lo = w * step;
    if (lo >= s.len) return (slice){0, s.ptr};
    size_t hi = lo + step;
    if (hi > s.len) hi = s.len;
    return (slice){ hi - lo, s.ptr + lo };
}
```

### `keel.range` — intervalo meio-aberto

```keel
module keel.range;

pub typedef struct { size_t first, limit; } range;

pub inline range of(size_t a, size_t b) { return (range){ a, b < a ? a : b }; }

pub inline size_t first (range r) { return r.first; }
pub inline size_t limit (range r) { return r.limit; }
pub inline size_t length(range r) { return r.limit - r.first; }
pub inline size_t get   (range r, size_t i) { return r.first + i; }

// no `ptr`, no cursor: a range supplies no element storage and no address.

pub inline range partition(range r, size_t k, size_t w) {
    if (k == 0) return (range){ r.first, r.first };
    size_t n = r.limit - r.first;
    size_t step = n / k + (n % k ? 1 : 0);
    size_t lo = w * step;
    if (lo >= n) return (range){ r.limit, r.limit };
    size_t hi = lo + step;
    if (hi > n) hi = n;
    return (range){ r.first + lo, r.first + hi };
}
```

### `keel.outcome` — resultado final, com código

```keel
module keel.outcome type T;

pub modifier outcome { i32 code; T value; }

pub constexpr i32 OK   = 0;
pub constexpr i32 NONE = INT32_MIN;

// `void` drops `value` and every verb that mentions it directly — win(r,v),
// value(r), value(r,v) vanish on their own in `outcome void`.

pub inline bool failed(outcome e) { return e.code != OK; }
pub inline bool ok    (outcome e) { return e.code == OK; }
pub inline i32  code  (outcome e) { return e.code; }
pub inline T    value (outcome e) { return e.value; }
pub inline void value (outcome *r, T v) { r->value = v; }

// writers adjust the object and return the adjusted copy: `return outcome.win(r, value);`
pub inline outcome win (outcome *r)         { r->code = OK; return *r; }
pub inline outcome win (outcome *r, T v)    { r->code = OK; r->value = v; return *r; }
pub inline outcome fail(outcome *r, i32 c)  { r->code = c; return *r; }
pub inline outcome none(outcome *r)         { r->code = NONE; return *r; }
```

### `keel.corot` — estado cooperativo de retorno

```keel
module keel.corot;

pub tags Status [SUCCESS = -1, ONGOING = 0, FAILED = 1];

pub typedef struct { i32 code; } corot;

pub inline bool ok     (corot r) { return r.code <  0; }
pub inline bool ongoing(corot r) { return r.code == 0; }
pub inline bool faulted(corot r) { return r.code >  0; }
pub inline i32  code   (corot r) { return r.code; }

pub inline Status tag (corot r) {
    return r.code < 0 ? Status.SUCCESS
         : r.code > 0 ? Status.FAILED
                      : Status.ONGOING;
}

// producers adjust the object and return the adjusted copy: `return corot.win(r);`
pub inline corot win  (corot *r)         { r->code = -1; return *r; }
pub inline corot again(corot *r)         { r->code =  0; return *r; }
pub inline corot fault(corot *r, i32 c)  { r->code =  c; return *r; }
```

### `keel.tagged` — valor rotulado por um conjunto declarado

```keel
module keel.tagged tags E type T;

// `void` drops the value field: `tagged Cycle void` is the label alone.
pub modifier tagged { i32 tag; T value; }

pub inline E    tag  (const tagged *t) { return (E)t->tag; }
pub inline T    value(const tagged *t) { return t->value; }

pub inline void mark (tagged *t, E e) { t->tag = e; }
pub inline void set  (tagged *t, E e, T v) { t->tag = e; t->value = v; }
```

### `keel.routine` — composição cooperativa sobre uma tabela

```keel
module keel.routine type C;

import keel.corot   as corot   types;
import keel.outcome as outcome types;
import keel.slice   as slice   types;

// the participant's signature; what it produces travels through `ctx`, not the return.
pub typedef corot (*routine)(C *ctx);

pub modifier slot { routine f; C *ctx; corot state; }

pub inline corot state(const slot *s) { return s->state; }
pub inline i32   code (const slot *s) { return s->state.code; }

// slots are steps, in order; a step is called again until it is not ONGOING.
// A failure ends the sequence without calling the following steps.
pub inline outcome u32 seq(slice slot s) {
    outcome u32 r = {0};
    size_t n = slice.length(s);
    u32 S = 0;
    for (size_t i = 0; i < n; ++i) slice.ptr(s, i)->state = (corot){0};
    for (size_t i = 0; i < n; ++i) {
        slot *p = slice.ptr(s, i);
        corot c;
        do { c = p->f(p->ctx); } while (corot.ongoing(c));
        p->state = c;
        if (corot.faulted(c)) { outcome.value(r, S); return outcome.fail(r, 1); }
        ++S;
    }
    return outcome.win(r, S);
}

// cycles: each one calls every ONGOING slot once, in order; the policy is
// judged at the end of the cycle (more slots may win than the minimum asked).
pub inline outcome u32 par(slice slot s, u32 target) {
    outcome u32 r = {0};
    size_t n = slice.length(s);
    KEEL_CHECK(target <= n, "par-target-above-total");
    u32 m = (u32)n, S = 0, F = 0, q = target ? target : m;
    for (size_t i = 0; i < n; ++i) slice.ptr(s, i)->state = (corot){0};
    for (;;) {
        for (size_t i = 0; i < n; ++i) {
            slot *p = slice.ptr(s, i);
            if (!corot.ongoing(p->state)) continue;
            p->state = p->f(p->ctx);
            if (corot.ok(p->state))           ++S;
            else if (corot.faulted(p->state)) ++F;
        }
        if (S >= q)    return outcome.win(r, S);
        if (m - F < q) { outcome.value(r, S); return outcome.fail(r, 1); }
    }
}

// convenience bitmask, 1 bit per slot that ended ok (up to 64 slots).
pub inline u64 mask(slice slot s) {
    u64 m = 0;
    size_t n = slice.length(s);
    KEEL_CHECK(n <= 64, "mask-above-64-slots");
    for (size_t i = 0; i < n && i < 64; ++i)
        if (corot.ok(slice.ptr(s, i)->state)) m |= (u64)1 << i;
    return m;
}
```

### `keel.parallel` — o símbolo de controle de um bloco `parallel`

```keel
module keel.parallel;

pub typedef struct {
    _Atomic u32  wins;
    _Atomic u32  fails;
    _Atomic bool flag;
    u32          workers;
    u32          target;      // 0 = ALL: the policy asks that nobody fail
} control;

pub inline bool interrupted(control *c) {
    return atomic_load_explicit(&c->flag, memory_order_relaxed);
}

pub inline u32 wins(control *c) {
    return atomic_load_explicit(&c->wins, memory_order_relaxed);
}

pub inline bool failed(control *c) {
    return atomic_load_explicit(&c->fails, memory_order_relaxed) > 0;
}

// under ALL the target is zero, and the policy is "nobody failed"; otherwise
// it is the number of wins asked for.
pub inline bool ok(control *c) {
    return c->target ? wins(c) >= c->target
                     : atomic_load_explicit(&c->fails, memory_order_relaxed) == 0;
}
```

---

## 4. Pares keel/C

Cada par mostra o keel escrito e o C que o `cgen` gera (perfil C23; o C11
difere só em detalhe de baixo nível, irrelevante para quem escreve `.k`). Um
bloco precedido de `<!-- golden: caminho:linhas -->` é um trecho literal de
um arquivo do oráculo de teste do próprio keel: o que está ali é exatamente o
que o cgen produz, sem o cabeçalho e os `#include` do arquivo gerado. Um
`#line` que aparece dentro de um trecho é o do arquivo gerado — liga aquele C
à linha do `.k` de origem; não precisa ser reproduzido por quem só lê.

### 4.1 Módulo completo e chamada C

`import_c` disponibiliza um header ao compilador C. O marcador `array`
registra o vetor; a chamada `puts` permanece C.

<!-- golden: cases/028-readme-main/ola.k:1-8 -->
```keel
//keel
module ola;
import_c <stdio.h>;

pub int main(int argc, char **argv) {
    array char message[] = "Ola, keel!";
    puts(message);
    return 0;
}
```

<!-- golden: cases/028-readme-main/expected/c23/ola.c:7-11 -->
```c
//C gerado
int ola_main(int argc, char **argv) {
    char message[] = "Ola, keel!";
    puts(message);
    return 0;
}
```

<!-- golden: cases/028-readme-main/expected/c23/main_ola.c:3-3 -->
```c
//C gerado
int main(int argc, char **argv) { return ola_main(argc, argv); }
```

`ola_main` é a função do módulo; o `main` do C vem numa unidade separada,
que repassa `argc`/`argv`. keel não precisa conhecer a assinatura de `puts`
para preservar a chamada.

### 4.2 Retorno e cleanup

A função devolve o primeiro caractere lido e fecha o arquivo antes de
retornar. Se a abertura falhar, o `defer` nunca é alcançado.

<!-- golden: cases/027-readme/reading.k:1-9 -->
```keel
//keel
module reading;
import_c <stdio.h>;

pub int first(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return EOF;
    defer fclose(file);
    return fgetc(file);
}
```

<!-- golden: cases/027-readme/expected/c23/reading.c:7-15 -->
```c
//C gerado
int reading_first(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return EOF;

    { int keel__rv0 = fgetc(file);
#line 7 "reading.k"
    fclose(file);
    return keel__rv0; }
}
```

A expressão de retorno é avaliada antes da limpeza. `FILE`, `fopen`,
`fgetc`, `fclose`, `EOF` são C comum.

### 4.3 Trecho fixo, elementos mutáveis e intervalo

Um `slice` de dois elementos aponta para parte de um vetor. Alterar por
índice ou por ponteiro modifica o mesmo armazenamento; o trecho não cresce.

<!-- golden: cases/027-readme/span.k:1-18 -->
```keel
//keel
module span;
import keel.slice as slice types;
import keel.range as range types;
import keel.array as array;

pub i16 sum(void) {
    array i16 dados[3] = {1, 2, 3};
    slice i16 part = slice.of(dados, 1, 3);
    part[0] = 20;
    *(slice.ptr(part) + 1) = 30;

    range indices = 0..2;
    i16 total = 0;
    foreach (size_t i : indices) {
        total += part[i];
    }
    return total;
}
```

<!-- golden: cases/027-readme/expected/c23/span.c:12-24 -->
```c
//C gerado
i16 span_sum(void) {
    i16 dados[3] = {1, 2, 3};
    keel_slice_i16 part = keel_slice_of2_keel_array_i16(dados, 3, 1, 3);
    *keel_slice_i16_ptr1(part, 0) = 20;
    *(keel_slice_i16_ptr(part) + 1) = 30;

    keel_range indices = keel_range_of(0, 2);
    i16 total = 0;
    { size_t keel__f0 = keel_range_first(indices); size_t keel__l0 = keel_range_limit(indices); for (size_t i = keel__f0; i < keel__l0; i++) {
        total += *keel_slice_i16_ptr1(part, i);
    } }
    return total;
}
```

`slice.of(dados, 1, 3)` vira a função da instância, que recebe o vetor e o
comprimento (`3`, a dimensão declarada); `part[0]` é um `ptr` indexado; o
literal `0..2` fora de uma travessia é `range.of(0, 2)`; o `foreach` sobre
`range` conta de `first` a `limit`. O resultado é `50`; `dados` termina
`{1, 20, 30}`. O intervalo é semiaberto: inclui `0` e `1`, exclui `2`.

### 4.4 Resultado com default e extração explícita

A função aceita um valor e ajusta um resultado explicitamente declarado. Um
código negativo representa falha; o `else` ajusta o próprio resultado para
sucesso com valor zero quando a chamada falha.

<!-- golden: cases/027-readme/result.k:1-13 -->
```keel
//keel
module result;
import keel.outcome as outcome types;

pub outcome i16 accept(i16 entry) {
    outcome i16 r = {0};
    if (entry < 0) return outcome.fail(r, entry);
    return outcome.win(r, entry);
}

pub i16 get_value(i16 entry) {
    outcome i16 res = accept(entry) else 0;
    return outcome.value(res);
}
```

<!-- golden: cases/027-readme/expected/c23/result.c:8-17 -->
```c
//C gerado
keel_outcome_i16 result_accept(i16 entry) {
    keel_outcome_i16 r = {0};
    if (entry < 0) return keel_outcome_i16_fail(&r, entry);
    return keel_outcome_i16_win1(&r, entry);
}

i16 result_get_value(i16 entry) {
    keel_outcome_i16 res = result_accept(entry); if (keel_outcome_i16_failed(res)) keel_outcome_i16_win1(&res, 0);
    return keel_outcome_i16_value(res);
}
```

`get_value(7)` devolve `7`; `get_value(-7)` devolve `0`. A declaração de
`res` continua `outcome i16`; a extração para `i16` só ocorre na chamada
explícita a `outcome.value`.

### 4.5 `match`: o laço pertence à função

`count` avança um contador cooperativamente: `corot.again` entrega
`ONGOING`; `corot.win` entrega `SUCCESS`; `corot.fault(r, 1)` entrega
`FAILED`. `note` é uma função comum, usada mais adiante.

<!-- golden: cases/031-readme-coop/coop.k:1-36 -->
```keel
//keel
module coop;
import keel.corot as corot types;
import keel.outcome as outcome types;
import keel.tagged as tagged types;

priv corot count(i16 *counter, i16 limit, bool failure) {
    corot r = {0};
    ++*counter;
    if (*counter < limit) return corot.again(r);
    if (failure)              return corot.fault(r, 1);
    return corot.win(r);
}

priv void note(i16 *counter) {
    ++*counter;
}

pub tags Cycle [WAIT, END];

pub i16 manual(void) {
    tagged Cycle void state = {0};
    tagged.mark(state, WAIT);
    i16 calls = 0;
    bool finished = false;

    while (!finished) {
        match (state) {
            WAIT:
                corot r = count(&calls, 2, false);
                if (corot.faulted(r) || corot.ok(r)) tagged.mark(state, END);
            END:
                finished = true;
        }
    }
    return calls;
}
```

<!-- golden: cases/031-readme-coop/expected/c23/coop.c:14-54 -->
```c
//C gerado
static keel_corot coop_count(i16 *counter, i16 limit, bool failure) {
    keel_corot r = {0};
    ++*counter;
    if (*counter < limit) return keel_corot_again(&r);
    if (failure)              return keel_corot_fault(&r, 1);
    return keel_corot_win(&r);
}

static void coop_note(i16 *counter) {
    ++*counter;
}



i16 coop_manual(void) {
    keel_tagged_coop_Cycle_void state = {0};
    keel_tagged_coop_Cycle_void_mark(&state, coop_Cycle_WAIT);
    i16 calls = 0;
    bool finished = false;

    while (!finished) {
        switch (state.tag) {
        case coop_Cycle_WAIT: goto keel__m0_WAIT;
        case coop_Cycle_END:  goto keel__m0_END;
        default:              KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
        }
#line 28 "coop.k"
        keel__m0_WAIT: {
            keel_corot r = coop_count(&calls, 2, false);
            if (keel_corot_faulted(r) || keel_corot_ok(r)) keel_tagged_coop_Cycle_void_mark(&state, coop_Cycle_END);
        }
        goto keel__m0_end;
#line 31 "coop.k"
        keel__m0_END: {
            finished = true;
        }
        keel__m0_end: ;
#line 34 "coop.k"
    }
    return calls;
}
```

A mudança para `END` só é observada no **próximo** despacho do laço escrito
pelo programa — não há passagem automática de um braço ao seguinte. O
`switch` só escolhe um rótulo; os corpos dos braços ficam fora dele, o que
preserva o controle (`goto`, `break` de fora) que o usuário escreveu.

### 4.6 `routine.par`: tabela de slots e política

Continua o módulo `coop` do exemplo anterior. As participantes são
descritas num vetor de `routine.slot`; `routine.par` percorre essa fatia em
ciclos.

<!-- golden: cases/031-readme-coop/coop.k:38-61 -->
```keel
//keel
import keel.routine as routine types;
import keel.slice   as slice   types;
import keel.array   as array;

priv corot stage(i16 *counter) {
    corot r = {0};
    ++*counter;
    if (*counter < 2) return corot.again(r);
    return corot.win(r);
}

pub outcome u32 set_of(i16 *a, i16 *b) {
    array routine.slot i16 steps[2] = {
        { .f = stage, .ctx = a },
        { .f = stage, .ctx = b },
    };

    outcome u32 r = routine.par(slice.of(steps), 1);

    foreach (routine.slot i16 *sl, size_t i : steps) {
        if (corot.ok(routine.state(sl))) note(a);
    }
    return r;
}
```

<!-- golden: cases/031-readme-coop/expected/c23/coop.c:60-79 -->
```c
//C gerado
static keel_corot coop_stage(i16 *counter) {
    keel_corot r = {0};
    ++*counter;
    if (*counter < 2) return keel_corot_again(&r);
    return keel_corot_win(&r);
}

keel_outcome_u32 coop_set_of(i16 *a, i16 *b) {
    keel_routine_slot_i16 steps[2] = {
        { .f = coop_stage, .ctx = a, .state = {0} },
        { .f = coop_stage, .ctx = b, .state = {0} },
    };

    keel_outcome_u32 r = keel_routine_i16_par(keel_slice_of_keel_array_keel_routine_slot_i16(steps, 2), 1);

    { for (size_t i = 0; i < 2; i++) { keel_routine_slot_i16 *sl = &steps[i];
        if (keel_corot_ok(keel_routine_slot_i16_state(sl))) coop_note(a);
    } }
    return r;
}
```

A instância `routine i16` é a unidade `keel_routine_i16`: o `typedef` da
participante e o `slot` saem no `.type.h` dela, e os verbos, no `.h`.

<!-- golden: cases/031-readme-coop/expected/c23/keel/keel_routine_i16.type.h:8-11 -->
```c
//C gerado
#line 18 "keel/routine.k"
typedef keel_corot (*keel_routine_i16)(i16 *ctx);
#line 25 "keel/routine.k"
typedef struct keel_routine_slot_i16 { keel_routine_i16 f; i16 *ctx; keel_corot state; } keel_routine_slot_i16;
```

<!-- golden: cases/031-readme-coop/expected/c23/keel/keel_routine_i16.h:44-61 -->
```c
//C gerado
static inline keel_outcome_u32 keel_routine_i16_par(keel_slice_keel_routine_slot_i16 s, u32 target) {
    keel_outcome_u32 r = {0};
    size_t n = keel_slice_keel_routine_slot_i16_length(s);
    KEEL_CHECK(target <= n, "par-target-above-total");
    u32 m = (u32)n, S = 0, F = 0, q = target ? target : m;
    for (size_t i = 0; i < n; ++i) keel_slice_keel_routine_slot_i16_ptr1(s, i)->state = (keel_corot){0};
    for (;;) {
        for (size_t i = 0; i < n; ++i) {
            keel_routine_slot_i16 *p = keel_slice_keel_routine_slot_i16_ptr1(s, i);
            if (!keel_corot_ongoing(p->state)) continue;
            p->state = p->f(p->ctx);
            if (keel_corot_ok(p->state))           ++S;
            else if (keel_corot_faulted(p->state)) ++F;
        }
        if (S >= q)    return keel_outcome_u32_win1(&r, S);
        if (m - F < q) { keel_outcome_u32_value1(&r, S); return keel_outcome_u32_fail(&r, 1); }
    }
}
```

O alvo `1` é alcançado no **fim** de um ciclo, não na primeira vitória: o
ciclo chama todas as entradas ainda `ONGOING`, e a política é julgada
depois. O estado é lido pelos verbos de `corot` (`routine.state(sl)`), nunca
pelo campo direto. O inicializador sem `.state` deixa o campo zerado, que em
`corot` já é `ONGOING` — o estado inicial correto, sem escrita.

### 4.7 Partição e travessia por cursor

Cada worker recebe uma parte do contêiner e a percorre por cursor, cada um
escrevendo num índice de saída diferente.

<!-- golden: cases/030-readme-scan/scan.k:1-14 -->
```keel
//keel
module scan;
import keel.buffer   as buffer   types;
import keel.slice    as slice;
import keel.parallel as parallel;

pub void sum(buffer i32 *xs, buffer u32 *totals) {
    parallel sum ALL (size_t w : 0..4; slice.slice i32 part : xs; (totals)) {
        u32 t = 0;
        walk (i32 *p, slice.cursor c : part) {
            t += (u32)*p;
        }
        totals[w] = t;
    }
}
```

<!-- golden: cases/030-readme-scan/expected/c23/scan.c:14-32 -->
```c
//C gerado
void scan_sum(keel_buffer_i32 *xs, keel_buffer_u32 *totals) {
    keel_parallel_control sum = { .workers = 4, .target = 0 };
    {   keel_buffer_i32 *keel__c0 = xs;

        #pragma omp parallel for num_threads(4) default(none) \
                shared(keel__c0, sum, totals)
        for (size_t w = 0; w < 4; w++) {
            keel_slice_i32 part = keel_buffer_i32_partition(keel__c0, 4, w);
#line 8 "scan.k"
            u32 t = 0;
            {   keel_slice_i32 keel__c1 = part; keel_slice_cursor c = keel_slice_i32_begin(keel__c1); while (keel_slice_i32_has_next(keel__c1, &c)) { i32 *p = keel_slice_i32_next(keel__c1, &c);
                t += (u32)*p;
            } }
            *keel_buffer_u32_ptr1(totals, w) = t;
            keel__end0: ;
        }
    }
#line 14 "scan.k"
}
```

`parallel` não percorre: chama `partition` uma vez por worker, e o corpo
escolhe como percorrer a parte (`walk`, aqui). A saída vai num contêiner
indexado por `w`, nunca numa captura escalar (escrever numa captura é
`captured-write`). Sob `ALL`, sem interrupção, o fim natural de todo worker
é sucesso: depois do bloco, `parallel.ok(sum)` é verdadeiro.

### 4.8 Módulo genérico: `modifier`, `instance`, uso

Três arquivos de um mesmo programa: o módulo genérico que declara o tipo
(`coll.k`), o módulo que decide onde o corpo fora de linha mora
(`instances.k`), e quem usa (`app/p.k`).

<!-- golden: cases/007-instance/coll.k:1-17 -->
```keel
//keel
module coll type T;

/* Mentions neither `T` nor `stack` → emitted ONCE, for the module, and the
   name does not carry the argument (§4.9). Same case as `outcome.OK`. */
pub constexpr i32 EMPTY = -1;

pub modifier stack byref { size_t cap, len; T *ptr; }

/* Mentions `stack`, though it never writes `T` → belongs to the instance. */
pub inline size_t length(stack *s) { return s->len; }

/* `pub` without `inline` → comes out `extern` in the instance, and the body needs an owner. */
pub bool push(stack *s, T v) {
    if (s->len == s->cap) return false;
    s->ptr[s->len++] = v;
    return true;
}
```

<!-- golden: cases/007-instance/instances.k:1-6 -->
```keel
//keel
module instances;
import coll;

/* It places, it does not generate: generation follows use; `instance` decides
   where the bodies of non-inline `pub` functions live (§4.9). */
instance coll.stack i32;
```

<!-- golden: cases/007-instance/app/p.k:1-16 -->
```keel
//keel
module app.p;
import coll types;
import_c <stdio.h>;

pub int main(int argc, char **argv) {
    array i32 v[4];
    stack i32 s;
    s.cap = 4; s.len = 0; s.ptr = v;

    if (length(s) != 0) return 1;
    if (!push(s, 7)) return 2;              /* body lives in instances.c */
    if (length(s) != 1) return 3;
    if (coll.EMPTY != -1) return 4;           /* bare name, without the argument */
    puts("ok");
    return 0;
}
```

O C gerado — a instância `coll_stack_i32`. O tipo, em `coll_stack_i32.type.h`:

<!-- golden: cases/007-instance/expected/c23/coll_stack_i32.type.h:7-8 -->
```c
//C gerado
#line 7 "coll.k"
typedef struct coll_stack_i32 { size_t cap, len; i32 *ptr; } coll_stack_i32;
```

Os protótipos e o verbo `inline`, em `coll_stack_i32.h`:

<!-- golden: cases/007-instance/expected/c23/coll_stack_i32.h:7-13 -->
```c
//C gerado
#line 10 "coll.k"
static inline size_t coll_stack_i32_length(coll_stack_i32 *s);
#line 13 "coll.k"
bool coll_stack_i32_push(coll_stack_i32 *s, i32 v);

#line 10 "coll.k"
static inline size_t coll_stack_i32_length(coll_stack_i32 *s) { return s->len; }
```

O corpo fora de linha, em `instances.c` — mora aqui porque foi aqui que
`instance` o colocou:

<!-- golden: cases/007-instance/expected/c23/instances.c:4-9 -->
```c
//C gerado
#line 13 "coll.k"
bool coll_stack_i32_push(coll_stack_i32 *s, i32 v) {
    if (s->len == s->cap) return false;
    s->ptr[s->len++] = v;
    return true;
}
```

`EMPTY` não leva o argumento no nome (`coll_EMPTY`, não `coll_stack_i32_EMPTY`):
não cita `T` nem o modificador. `length` é `inline`: sai por completo no
header da instância. `push` é `pub` sem `inline`: o protótipo sai no header,
mas o corpo só existe porque `instances.k` escreveu `instance coll.stack
i32;` — sem essa linha em algum módulo do programa, o link falharia por
símbolo ausente.

### 4.9 `extent`: coluna de array com dimensão verificada

Uma tabela privada ao módulo, com uma coluna de tags e os dois inteiros de
controle; `match` despacha sobre o elemento lido da coluna.

<!-- golden: cases/035-match-column/app/nd.k:1-22 -->
```keel
//keel
module app.nd;

pub tags Kind [LIT, ADD];

/* the column holds a set: its element is the operand of `match` (spec §4.11); the
   extent is private to keel, and its struct and accessors are in the headers all the same
   (backend §5.15, rule 3; D29) */
priv extent struct nodes [n, cap] {
    array Kind *kind;
    size_t n, cap;
};

pub i32 weight(struct nodes *t, size_t i) {
    i32 r = 0;
    match (t->kind[i]) {
        LIT:
            r = 1;
        ADD:
            r = 2;
    }
    return r;
}
```

O tipo: o conjunto de tags e a struct, com a coluna.

<!-- golden: cases/035-match-column/expected/c23/app/app_nd.type.h:7-13 -->
```c
//C gerado
#line 3 "app/nd.k"
typedef enum app_nd_Kind {app_nd_Kind_LIT, app_nd_Kind_ADD} app_nd_Kind;
#line 8 "app/nd.k"
struct app_nd_nodes {
    app_nd_Kind *kind;
    size_t n, cap;
};
```

O `match`:

<!-- golden: cases/035-match-column/expected/c23/app/app_nd.c:16-35 -->
```c
//C gerado
i32 app_nd_weight(struct app_nd_nodes *t, size_t i) {
    i32 r = 0;
    switch (*app_nd_nodes_kind_ptr(t, i)) {
    case app_nd_Kind_LIT: goto keel__m0_LIT;
    case app_nd_Kind_ADD: goto keel__m0_ADD;
    default:              KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
    }
#line 16 "app/nd.k"
    keel__m0_LIT: {
        r = 1;
    }
    goto keel__m0_end;
#line 18 "app/nd.k"
    keel__m0_ADD: {
        r = 2;
    }
    keel__m0_end: ;
#line 21 "app/nd.k"
    return r;
}
```

`t->kind[i]` indexa a coluna pelo acessor gerado (`..._kind_ptr`), não pelo
ponteiro cru: a struct e os acessores saem nos headers mesmo com `priv
extent` — `priv` aqui é sobre a declaração do tipo, não sobre a visibilidade
do acesso verificado.

---

## 5. Usando o `cgen`

`cgen` é um driver no molde do `gcc`: transpila o `.k` e, por padrão, chama o
compilador C sobre o que gerou. Trocar `gcc` por `cgen` numa linha de
compilação é a forma pretendida de uso.

```sh
cgen -I src -c src/main.k -o main.o -O2 -Wall    # compila um módulo
cgen main.o geom.o -o prog                       # sem .k na linha: só liga
cgen ola.k -o ola                                # módulo com entrada pública: compila e liga
```

### Regras da invocação

- **Um fonte por invocação:** um `.k`, ou um `--instance`. Fontes `.c`, `.o` e
  `.a` na mesma linha passam direto ao compilador C.
- **`-c` compila uma unidade:** a invocação escreve o `.c` do módulo pedido e
  os headers (`.type.h` e `.h`) dele e de tudo o que ele importa — nunca o
  `.c` de um módulo importado. Cada módulo tem a sua própria invocação.
  *Importar não é compilar.*
- **Raízes de módulo:** `-I <dir>` (repetível, a ordem importa) diz onde
  achar os `import`. Sem `-I`, a raiz é `.`. O `.k` dado tem de estar sob
  exatamente uma raiz, e o caminho dele relativo à raiz tem de concordar com
  o `module` declarado (`src/net/http.k` declara `module net.http;`).
- **O nome do gerado é o símbolo do módulo, não o caminho:** `src/net/http.k`
  produz `gen/net/net_http.c`, `gen/net/net_http.h` e
  `gen/net/net_http.type.h`.
- **Entrada do programa:** se a invocação liga (nenhum de `-c`, `-S`, `-E`,
  `-shared`, `--stop-after`) e o `.k` tem função de entrada pública, o cgen
  gera sozinho a unidade com o `main` do C (como no par 4.1). `--main <módulo>`
  escolhe a entrada explicitamente; o argumento é nome de módulo, não caminho.
- **Genérico não compila sozinho:** `cgen coll.k` sobre módulo com parâmetro é
  erro. O genérico entra no build por uso (um `.k` com `instance`) ou por
  `--instance`, tudo qualificado por inteiro:

  ```sh
  cgen -c --instance "coll.stack i32" -o coll_stack_i32.o
  cgen -c --instance "keel.buffer.buffer app.geom.Point" -o keel_buffer_app_geom_Point.o
  ```

### Opções do cgen

Estas opções são do cgen e nunca chegam ao compilador C. Todo o resto da
linha vai ao compilador C, na ordem, sem alteração.

| Opção | Efeito | Padrão |
| --- | --- | --- |
| `-I <dir>` | raiz de busca de módulos (também repassada ao `cc`) | `.` |
| `--dest-dir <dir>` | raiz da saída gerada | `./gen` |
| `--stop-after=<lex\|parse\|gen>` | para após a fase; `lex`/`parse` escrevem no stdout (ou em `-o`); `gen` escreve os `.c`/`.h` e não chama o `cc` (`-o` com `gen` é recusado) | não para |
| `--main <módulo>` | gera o `main` do C chamando a entrada do módulo | automático ao ligar |
| `--instance "<M.mod> <args>"` | compila uma instância de genérico, no lugar de um `.k` | — |
| `--checks=<on\|off>` | `off` passa `-DKEEL_CHECKS=0`; o C gerado é o mesmo | `on` |
| `--line=<on\|off>` | emite `#line` apontando para o `.k` | `on` |
| `--cc=<programa>` | compilador C a invocar | `cc` |
| `--profile=<auto\|c11\|c23>` | padrão C do gerado; `auto` lê o `-std=` da linha: `c23`/`gnu23`/`c2x`/`gnu2x` dão C23, qualquer outro valor ou a ausência dão C11 | `auto` |
| `--parallel-lowering=<auto\|serie\|openmp>` | mecanismo de `parallel`; `auto` escolhe `openmp` se há `-fopenmp` na linha, senão `serie`; `openmp` sem `-fopenmp` é erro | `auto` |
| `--base-dir <dir>` | raiz da base | `<exe>/../lib/base` |
| `--pedantic-names` | teto de nome gerado de 31/63 caracteres, em vez de 255 | desligado |
| `-f` | reescreve o gerado mesmo idêntico (muda o timestamp) | desligado |
| `--cgen-version`, `--cgen-help` | versão e ajuda do cgen (`--version` e `--help=…` são do compilador C) | — |

Quatro opções do compilador C o cgen também lê, sem retirá-las: `-I`, `-std=`
(perfil), `-fopenmp` (lowering) e `-MMD`/`-MD`/`-MF`/`-MT`/`-MQ` (depfile).
Não há arquivo de configuração nem variável de ambiente: a saída depende só
da linha de comando.

### Em um build

- Regra de make: `%.o : %.k` com `$(CC) $(KFLAGS) $(CFLAGS) -MMD -c $< -o $@`
  (`CC = cgen`) e `-include $(OBJS:.o=.d)`. O depfile lista, numa regra só, o
  que o compilador C reporta mais todo `.k` alcançável por `import`, então
  editar um módulo importado refaz quem depende dele. Sem `-MMD`/`-MD`, nenhum
  depfile é escrito.
- O build não precisa declarar os headers dos importados como saídas: o `.c`
  tem um escritor só, e os headers são determinísticos e escritos de forma
  atômica, sem tocar arquivo idêntico.
- O cgen acrescenta `-I <dest-dir>` à chamada do `cc`; as inclusões do gerado
  dependem dele.
- Para só ver o C: `cgen --stop-after=gen -I src --dest-dir gen src/mod.k`.

### Makefile completo: keel e C no mesmo projeto

Layout: `src/main.k` (módulo `main`, com `pub int main`), outros `.k` em
`src/` (subpastas seguem o nome do módulo) e `.c` puros em `src/`. Testado
com o `cgen` e o `gcc`.

```make
PRJ    := $(notdir $(CURDIR))
SRCDIR := src
OBJDIR := obj
GENDIR := gen
MAIN   := main

CC     := cgen          # o driver; o compilador C de verdade é o CCC
CCC    := gcc
CFLAGS := -std=c2x -I$(SRCDIR) -Wall -Wextra -g -O0
LDLIBS := -lm

ALL_K  := $(shell find $(SRCDIR) -name '*.k')
# genérico = cabeçalho 'module nome dim|tags|type ...;' (não compila sozinho)
GEN_K  := $(shell grep -lE '^module[[:space:]]+[A-Za-z0-9_.]+[[:space:]]+(dim|tags|type)[[:space:]]' $(ALL_K) /dev/null)
K_SRCS := $(filter-out $(SRCDIR)/$(MAIN).k $(GEN_K),$(ALL_K))
C_SRCS := $(shell find $(SRCDIR) -name '*.c')
OBJS   := $(patsubst $(SRCDIR)/%.k,$(OBJDIR)/%.o,$(K_SRCS)) \
          $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(C_SRCS))

KGEN := $(CC) --cc=$(CCC) --dest-dir $(GENDIR) -I $(SRCDIR) $(CFLAGS)

all: $(PRJ)

# um .k por invocação; -c só compila (não gera o main do C)
$(OBJDIR)/%.o: $(SRCDIR)/%.k
	@mkdir -p $(dir $@)
	$(KGEN) -MMD -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CCC) $(CFLAGS) -MMD -c $< -o $@

# o módulo de entrada compila e liga de uma vez (é ele que gera o main do C)
$(PRJ): $(SRCDIR)/$(MAIN).k $(OBJS)
	@mkdir -p $(OBJDIR)
	$(KGEN) -MMD -MF $(OBJDIR)/$(MAIN).d -MT $@ $< $(OBJS) $(LDLIBS) -o $@

-include $(shell find $(OBJDIR) -name '*.d' 2>/dev/null)

run: $(PRJ)
	./$(PRJ)

clean:
	rm -rf $(OBJDIR) $(GENDIR) $(PRJ)

.PHONY: all run clean
```

Pontos que o Makefile depende de:

- **`CC := cgen` e `--cc=$(CCC)`:** o `cgen` substitui o `gcc` na linha; o
  compilador C real vai em `--cc`. Os `.c` puros usam o `$(CCC)` direto.
- **Só o módulo de entrada liga:** com `-c` o cgen não gera o `main` do C.
  Por isso `main.k` fica fora de `K_SRCS` e entra na linha de ligação,
  junto dos `.o` (um `.k` mais `.o`/`.c` na mesma linha é permitido).
- **Genéricos ficam fora de `K_SRCS`:** um módulo genérico declara parâmetros
  no cabeçalho (`module box type T;`, `module m dim N;`, `tags`), e `cgen box.k`
  sobre ele é `generic-source-without-instance`. O `grep` acima os separa. Ele
  entra no build por uso (o `instance` num `.k` comum, que já está em
  `K_SRCS`) ou por `--instance`, com uma regra à mão, por exemplo
  `obj/coll_stack_i32.o: ; $(KGEN) -c --instance "coll.stack i32" -o $@`
  (o `grep` só vê o cabeçalho em uma linha).
- **`-MMD`** em todas as receitas; no módulo de entrada, `-MF`/`-MT`
  explícitos, para o depfile não colidir com o do alvo final. Editar um `.k`
  importado refaz quem o importa.
- **Chamar módulo importado:** `import net.util as util;` e `util.twice(…)`
  (com `net.util.twice(…)` o teste falhou: `'net' undeclared`).
- **Header C do projeto:** `import_c "hello.h";`, resolvido por
  `-I$(SRCDIR)` (repassado ao compilador C).

### Diagnósticos e código de saída

O formato é o do gcc, com a posição no `.k` (nunca no gerado) e o nome
estável do diagnóstico entre colchetes:

```plain
src/app/nd.k:16:5: error: <mensagem> [<nome>]
```

`-Wno-<nome>` silencia um `warning` ou `info` da linguagem (`error` não se
desliga); `-Werror` torna `warning` em parada. `-W…` com nome que não é do
keel passa ao compilador C.

| Código | Situação |
| --- | --- |
| 0 | sucesso, com ou sem `warning` |
| 1 | pelo menos um `error` da linguagem; nada foi escrito |
| 2 | erro da ferramenta (invocação) |
| *n* | o compilador C saiu com *n* ≠ 0 (por sinal *s*, 128+*s*) |

Erros de invocação mais comuns, todos com código 2 e sem posição:
`multiple-sources` (mais de um fonte), `generic-source-without-instance`,
`invalid-instance`, `source-not-found`, `source-outside-roots`,
`source-in-multiple-roots`, `module-not-found` (um `import` que nenhuma raiz
resolve), `missing-entry-point` (`--main` sobre módulo sem entrada pública),
`lowering-unavailable`, `invalid-option`, `output-with-gen`, `write-failure`
e `implementation-limit`.
