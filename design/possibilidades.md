# keel — Possibilidades

Documento **não normativo**. Registra ideias para versões futuras, e as
recusadas com o motivo, para que nenhuma se perca e nenhuma seja rediscutida
do zero. Nada aqui é promessa: uma entrada só vira contrato pelo caminho de
sempre — discussão, texto proposto, aceite — e então passa para a spec, o
rationale ou o backend, e sai daqui.

## Horizontes

| Versão | Escopo |
| --- | --- |
| **v0** | o núcleo e a base: a spec como está (§§1–4, §§5.1–5.7, §6), o backend e a ferramenta. Nada deste documento. |
| **v1** | a biblioteca padrão além da base — §1 abaixo. |
| **v2+** | extensões da linguagem e estruturas a estudar — §2 abaixo. |

---

## 1. Biblioteca padrão (v1)

Candidatos a módulo, sem contrato fechado nem `.k`. Nenhum pede trabalho de
núcleo: cabem em `module`/`modifier` com `dim`/`type`/`tags`.

### `keel.pair`: `pair` e `pairs`

Candidato próximo. Um módulo com dois modificadores de mesma assinatura `type`
de dois parâmetros (spec §4.3 admite mais de um modificador por módulo):

```keel
module keel.pair type K, V;
pub modifier pair { K key; V value; }
pub modifier pairs byref { K *keys; V *values; size_t len, cap; }
```

```keel
pair size_t size_t minmax(slice const i32 s);

pairs u32 f32 weights = pairs.from(keys, values, capacity);
pairs.push(weights, 7, 0.5f);
f32 *w = pairs.find(weights, 7);   // linear scan over keys
```

`pairs K V` é uma tabela de chave e valor com o layout de um `extent` de
duas colunas: dois vetores paralelos sob um único `len`/`cap`, e não um
`buffer pair K V`. A busca percorre só a coluna de chaves, contígua. Com uma
coluna que cabe na cache, a varredura sequencial com o prefetcher, e com SIMD
quando `K` é um inteiro, pode vencer o hash: não há função de hash, nem
ponteiro até o bucket, nem colisão. Achada a posição `i`, o valor está em
`values[i]`. Separar as colunas também elimina o padding entre `K` e `V`:
`pair u8 f64` ocupa 16 bytes, e `pairs u8 f64` ocupa 9 bytes por entrada. Até a
versão 1.23, o bucket do `map` de Go guardava as chaves juntas e os valores
juntos pelo mesmo motivo.

`pair K V` é o valor avulso, e é o que `pairs` entrega numa travessia ou num
`get`, montado por cópia das duas colunas. Como `pairs` não guarda `pair`,
não há `ptr` para um `pair` inteiro, só para a chave ou para o valor.

Os três níveis:

| Forma | Layout | Busca |
| --- | --- | --- |
| `pair K V` | um valor, os dois campos juntos | nenhuma |
| `pairs K V` | duas colunas, na ordem de inserção | linear sobre as chaves |
| `map K V` | `buffer` com hash | hash |

`map` é outro módulo e fica para depois. Pode entregar `pair` como elemento
percorrido, mas nem `pair` nem `pairs` dependem dele.

Abertos:

- **O ponto de cruzamento com o hash é empírico**, e depende do tamanho de `K`,
  do nível de cache e do custo da comparação. A entrada só afirma o regime:
  poucas entradas, chave pequena, muitas buscas. Um benchmark contra um `map`
  deve fixar a faixa antes que a documentação recomende um ou outro.
- **`extent` genérico.** O corpo de `modifier` é C opaco, e `K *keys` é um
  ponteiro comum, não uma coluna de `extent`: `weights.keys[i]` não tem a
  verificação da §4.11. Um modificador que declarasse colunas seria extensão
  da linguagem, do escopo da §2 abaixo.
- **Chave com `==`.** A comparação de `find` serve para inteiros, ponteiros e
  enums; `struct` e string pedem uma função de igualdade, como parâmetro do
  verbo ou numa variante.
- **Chave duplicada.** `push` recusa, sobrescreve ou aceita e deixa `find`
  achar a primeira ocorrência.
- **Ordem.** Com as chaves mantidas em ordem, `find` vira busca binária, e
  `push` passa a deslocar as duas colunas. Pode ser um terceiro modificador
  com a mesma memória.
- O par memória/visão: uma visão de `pairs` seria um par de `slice` com o
  mesmo comprimento.
- Com `V` igual a `void`, a regra 11 da §4.3 omite `value` e `values`:
  `pairs K void` vira um conjunto com busca linear, o que talvez seja útil, e
  `pair K void` fica reduzido a um campo, o que talvez valha recusar.
- `pair` não carrega significado de erro. Resultado com falha é `outcome`
  (spec §5.5); `find` devolve ponteiro nulo ou `outcome`.

### `keel.buffer(N)` / `keel.slice(N)`

*soa* homogêneo: `N` colunas paralelas do mesmo tipo `T`, sincronizadas por um
único `len`/`cap` (`slice(N)`, só `len`). Mesmos verbos de `buffer`/`slice`,
com um índice a mais para a coluna, na convenção de `array` multidimensional
(`keel.dim(v,k)`, spec §4.2). Cabe no mecanismo de `dim` (spec §4.3).

```keel
module keel.buffer_n dim N type T;
pub modifier buffer byref { T *col[N]; size_t len, cap; }

module keel.slice_n dim N type T;
pub modifier slice { T *view[N]; size_t len; }
```

```keel
buffer(3) f32 pos;
push(pos, 0, 1.0f, 2.0f, 3.0f);
```

Abertos:

- `T *col[N]` (colunas separadas) × `T dados[N]` (bloco contíguo). O cursor,
  `ptr(x,i)` e `partition` supõem memória contígua; com colunas separadas, os
  "mesmos verbos" precisam ser relidos coluna a coluna.
- Serve de **lote de tamanho fixo** para gather/scatter sobre um `extent` —
  reunir `N` linhas em AoS, processar, espalhar de volta —, com `N` conhecido
  em compilação para desenrolar o laço.
- O par segue a regra memória/visão (rationale, "Memória e visão"):
  `buffer(N)` produz `slice(N)`, nunca o contrário.
- **Alias seguido de `(`.** Com modificadores de `(N)` — `buffer(N)`,
  `slice(N)`, um futuro `tensor(N)` —, o `(N)` passa a ser obrigatório, e um
  alias de módulo com a mesma grafia pode escondê-lo. Como palavras e nomes do
  keel são posicionais, uma função do usuário com o nome do alias também tem de
  ser distinguida. A v0 retirou a forma reservada e o diagnóstico; revisar
  aqui a restrição, com o nome já escolhido: `alias-with-argument`.

### `keel.coll`: `stack`, `queue`, `ring`

Contêineres de acesso restrito, da família de `buffer`: a memória vem de fora
(arena ou `array`), e a capacidade é fixa. Um módulo com três modificadores de
mesma assinatura `type T` (spec §4.3 admite mais de um modificador por módulo).

- **`stack`** — LIFO: `push`/`pop`/`top`.
- **`queue`** — FIFO: `push`/`pop`/`front`.
- **`ring`** — circular: quando cheio, sobrescreve o mais antigo.

Abertos:

- Sem realocação, `queue` e `ring` convergem no armazenamento — os dois são
  circulares. A diferença que sobra é a política ao encher: `queue` recusa,
  `ring` sobrescreve. Talvez um modificador com a política como parâmetro.
- Participação nos protocolos: `walk` sim (cursor na ordem lógica); indexação
  por posição lógica em `queue`/`ring` é possível; `partition` e a visão
  contígua não, porque o conteúdo pode dar a volta — o recorte seria um par de
  `slice`.
- `stack` é `buffer` com os verbos restritos: vale o tipo à parte pela
  intenção, ou basta documentar o uso de `buffer.push`/`pop`? O caso golden
  007 já tem um `coll.stack` do usuário.

### `keel.string` / `keel.strview`

Typedefs de conveniência, não modificadores novos:

```keel
typedef buffer char string;
typedef slice const char strview;
```

O trabalho real é o conjunto de funções de string — comparação, concatenação,
formatação — que os verbos de `buffer`/`slice` não cobrem. Par memória/visão:
`string` produz `strview`.

`slice const char` só não está na base porque este par é que seria o nome
dele. O caso golden `021-else-assign` já o usa cru; se aparecerem mais usos,
vale trazer o par para a base antes do resto da v1.

### `keel.bitbuffer(W)` / `keel.bitslice(W)`

Array compacto de inteiros de `W` bits; `T` é o tipo de interface de
`get`/`set` (qualquer inteiro, na prática sem sinal). `bitbuffer(1) bool` é o
bitset clássico.

```keel
module keel.bitbuffer dim W type T;
pub modifier bitbuffer byref { u8 *words; size_t len, cap; }
// get/set convert between the W-bit packing and the call's T
```

```keel
bitbuffer(3) u8 groups;
bitbuffer(1) u8 marked;   // bitmask
```

Uso já à vista: a tabela de estados de `keel.routine` (spec §5.6) poderia ser
`bitbuffer(1)`, e a presença de componente numa leitura ECS (§3) também.

### `keel.grid`: `box(N)` e `index(N)`

A extensão de `range` para N dimensões. `range` está para `size_t` assim como
`box(N)`, o produto de `N` intervalos, está para `index(N)`, a coordenada de
um elemento. Precedentes: `CartesianIndex`/`CartesianIndices` de Julia, o
`operator[]` do `mdspan` do C++23 com índices em `std::array`, e os domínios
de Chapel.

```keel
module keel.grid dim N;
pub modifier index { size_t i[N]; }
pub modifier box { size_t first[N], limit[N]; }
```

```keel
index(2) argmax(array f32 m[R, C]);

index(2) p = (index(2)){ r, c };
m[p.i[0], p.i[1]] = 0.0f;
```

O uso mais fraco é receber uma coordenada como parâmetro, porque
`f(v, I, x)` pouco ganha sobre `f(v, i, j, x)`. O ganho está em:

- **devolver e guardar posições**: `argmax`, matriz esparsa em COO (uma
  `buffer` de `index(2)` com os valores), busca em grade, coordenadas de
  pixel, vizinhos de estêncil (`I` mais um deslocamento);
- **código independente da quantidade de dimensões**: `box(N)` é Contável
  (`foreach` produz `index(N)`) e Particionável (`parallel` em blocos), como
  `range` (spec §5.1);
- **vista multidimensional**: `v[box]` como `range-index` de N dimensões, que
  a spec §4.5 já deixa aos módulos que o implementam.

Abertos:

- **Açúcar `v[I]`.** Com `I` de tipo `index(k)` e `v` um `array` de `k`
  dimensões, `v[I]` expandiria para `v[I.i[0]]...[I.i[k-1]]`, com a
  verificação de cada dimensão (spec §4.5, regra 12). Esse açúcar é trabalho de
  núcleo, não de módulo: sem ele, `keel.grid` fica só com o acesso campo a
  campo.
- **Sem literal `[a,b]`.** `a..b` funciona porque `..` não é token C; um `[`
  em posição de expressão colide com designador (`{ [0] = x }`) e com atributo
  C23 (`[[nodiscard]]`). A construção fica no literal composto
  `(index(2)){ a, b }` ou num verbo `index.of(a, b)`.
- `box(N)` com `first`/`limit` separados ou como `range r[N]`; a segunda forma
  reaproveita os verbos de `range` dimensão a dimensão.
- O nome `array` não serve ao módulo: já é o marcador do núcleo e
  `keel.array`.

### `keel.slice.from(T, p, range)`

Extensão de `keel.slice` (spec §5.3): aceitar um `range` no lugar da contagem,
para descrever `[a,b)` sobre um ponteiro cru. Útil com coluna de `extent`, quando o
programa já tem o intervalo em mãos.

### Cooperativo: `keel.atomic`, `keel.chan`, `keel.barrier`

Completam o conjunto cooperativo. Com `corot`, `outcome`, `tags`/`match` e
`seq`/`par` já se montam rotinas que pausam e retomam no mesmo ponto, ao estilo
do Duff's device; estes três dão comunicação e sincronização.

- **`keel.atomic type T`** — `modifier atomic byref { _Atomic T v; }`, com
  `init`/`load`/`store`/`swap`. `byref` evita sincronizar com uma cópia.
  Diagnóstico `atomics-unavailable` sob `__STDC_NO_ATOMICS__`, no molde de
  `specific-format-unavailable`.
- **`keel.chan type T`** — `send`/`recv` como `corot`. Duas variantes:
  cooperativa (índice comum, mesmo fio, sem atomics) e SPSC entre threads
  (`acquire`/`release` nos dois índices). MPMC fica fora. Nome em aberto:
  `chan` (combina com `corot`) ou `channel`.
- **`keel.barrier`** — `corot r = arrive(b, total)`, `ONGOING` até completar;
  rendezvous sem busy-wait e sem `thrd_yield` — quem cede o controle é o
  `return` da participante.

Aberto: um módulo que ajude a gerir o **contexto que sobrevive à suspensão** —
o estado local de uma corrotina stackless. Se existir, é módulo comum, sem
acréscimo ao núcleo.

---

## 2. Linguagem e estruturas a estudar (v2+)

### O núcleo reduzido (síntese, 2026-09-30)

O que as entradas seguintes (papéis de procedência, moldes, protocolos) têm em
comum é encolher o núcleo até o que a biblioteca não resolve. **Fica no núcleo:**

- o `array` (só a parte que não se resolve por biblioteca: a sintaxe C de vetor);
- as construções `defer`, `walk`, `parallel`, `foreach`, `match` e `else` (o
  `apply` fica fora, por enquanto);
- a sintaxe de índice `x[i, j]` e de região `x[a..b]`.

Cada construção (salvo o `defer`) estabelece um protocolo da base. Todo o resto é
**biblioteca**: módulos, modificadores, protocolos (em paridade com os
modificadores) e moldes (`keel_code`). O núcleo dá **garantias**, que são as que
vêm dos papéis de procedência e dos protocolos.

Dois princípios delimitam a linguagem: **nada é escondido** (tudo se resolve por
análise sintática e substituição simbólica, sem macro do pré-processador) e o
**custo em execução é especificado**, de modo que o programador escolhe qual
incorrer.

### Estruturas flat: árvore e lista encadeada por índice

Nós num `buffer`, e os elos são índices (`u32`, por exemplo) em vez de
ponteiros. É o caminho DOD para estruturas encadeadas:

- memória contígua, amigável a cache, e compatível com a arena — nó removido
  é invalidação lógica (ou entra numa lista livre de índices ou faz swap com 
  o último), sem `free`;
- relocável e serializável: índice não muda quando o bloco muda de lugar;
- elos de 32 bits no lugar de ponteiros de 64.

A lista encadeada de ponteiros do caso golden 019 já mostra o protocolo de
`walk` sobre uma estrutura que não indexa; a versão flat mantém o mesmo
protocolo, com o cursor guardando um índice.

A estudar:

- a forma genérica: o nó carrega `T` e os índices de elo, e o módulo é
  `type T`, com `T` opaco (spec §4.3);
- as travessias da árvore — pré, in e pós-ordem, largura — como cursores
  distintos, ou como verbos `begin_*` do mesmo módulo;
- o valor sentinela de "sem elo" e a largura do índice (`u16`/`u32`) como
  parâmetro;
- a relação com `extent`: a árvore flat pode guardar os elos em colunas.

### Protocolo nominal

**Status: proposta para discussão, não normativa (reelaborada em 2026-09-30).**
O estudo do que mudaria na documentação e no código está em
[`protocolo-impacto.md`](protocolo-impacto.md).
Os protocolos da spec §5.1 são estruturais: quem declara `begin`/`has_next`/
`next` participa de `walk`, e o contrato está só na documentação. Dar-lhes nome
e usá-los como **tipo de parâmetro** de função é o que esta entrada propõe. Não
depende do molde (`keel_code`): é uma função genérica **instanciada pelo tipo
concreto do argumento**, e não uma expansão de código.

**Declaração.** Um protocolo é um conjunto de assinaturas, declarado num módulo
como um irmão do `modifier`: um módulo pode declarar vários protocolos, e a
lista de binders (`type T`, `dim`, `tags`) é a do módulo.

```keel
module keel.protocols type T;

protocol Indexable {
    pub size_t length(Indexable *c);
    pub T      *ptr(Indexable *c, size_t i);
}

protocol Traversable {
    /* ... */
}
```

O protocolo não emite nada em C: os símbolos são os do implementador.

**Composição.** Um protocolo pode compor outros, com a **lista entre colchetes que
a linguagem já usa** (a de `tags`, `tags-list`, spec §2.2); só muda o significado
da lista, que depende da declaração em que ela aparece: em `tags Tipo [lista]` é
a lista de enums do tipo; em `extent nome [len, cap]` são os campos de controle;
em `extern_c [type_h]` é onde colocar os símbolos; em `protocol Nome [lista]` são
os protocolos que compõem o novo. Na linha `module`, a lista é outra coisa, e não
usa colchetes: `dim`, `tags` e `type` dali definem os tokens de substituição, e
`protocol Indexable, Traversable` diz **o que o módulo implementa**. A cláusula
`protocol` vai depois de `type` e **não é um binder**: não entra na aridade dos
modificadores (§4.3). As chaves são opcionais e servem só para acrescentar
protótipos próprios, além dos importados:

```keel
protocol IndexTraverse [Indexable, Traversable];          // composition only, no braces

protocol Resizable [Indexable] {                          // includes Indexable, adds its own
    pub bool resize(Resizable *c, size_t n);
}
```

- A gramática é `decl-protocol ::= 'protocol' IDENT [ '[' IDENT { ',' IDENT } ']' ]
  ( ';' | '{' { prototype } '}' )`. Sem lista e sem chaves, o protocolo é vazio, e
  isso é erro.
- Os protótipos do protocolo composto são a **união** dos incluídos e dos
  próprios, por (nome, aridade). O mesmo verbo vindo de dois incluídos (o losango)
  vale se as assinaturas e os papéis são iguais; se diferem, é
  `protocol-verb-conflict`.
- Quem declara `IndexTraverse` na linha `module` **implementa também** `Indexable`
  e `Traversable`: `fn(Indexable b)` aceita esse tipo. Keel calcula o fecho dos
  protocolos declarados ao ler a linha `module`.
- A lista de binders (`type T`, `dim`, `tags`) dos protocolos incluídos é a do
  protocolo que os inclui, por espécie e ordem (regra 1 abaixo).
- Ciclo (`A [B]`, `B [A]`) é `circular-protocol`, no molde do `circular-generic`.
- O parâmetro de função continua com **um** protocolo: para precisar de dois,
  declara-se o composto.

**Implementação.** O módulo que implementa declara os verbos, como hoje, e pode
declarar a conformidade:

```keel
module keel.buffer type T protocol Indexable, Traversable;
```

A cláusula `protocol` **não é um import**: o import traz nomes visíveis, e a
cláusula só diz **o que aquele módulo implementa**. O nome do protocolo precisa
estar visível, então o implementador importa `keel.protocols` de qualquer forma.

**Não há `bound`.** Nenhuma palavra ou marca diz "agora vou usar protocolos": o
protocolo é importado e usado como um tipo, e não há cláusula na linha `module`
do genérico, nem na definição do protocolo, nem no uso.

**Verificação no uso.** É nominal. Ao importar `buffer`, keel lê na linha `module`
quais protocolos ele implementa. No uso, como `fn(x, b)`, já sabe que `buffer`
implementa `Indexable`, e recusa o tipo que não o declara
(`protocol-not-satisfied`).

**Verificação no implementador (exaustividade).** Keel varre as funções
declaradas no módulo e as compara com os protótipos de cada protocolo da
cláusula. O que falta é diagnosticado na cláusula, com **todos** os verbos
ausentes listados (`protocol-verb-missing`), e não no uso, com o símbolo manglado
de uma chamada C. A comparação é por **nome, aridade (contada sintaticamente,
backend §2.1.1), posição do receptor e papéis de procedência**. Não compara
tipos C nem tipos de retorno, porque keel não resolve tipos C (spec §1.3): uma
divergência aí cai no compilador C.

**Sem transitividade.** Só contam os verbos declarados no **próprio** módulo. O
verbo de um módulo importado não conta, porque a resolução pelo tipo do contêiner
(§4.4, passo 1) procura o verbo no módulo do contêiner. Para reaproveitar um
verbo de outro módulo, o implementador escreve uma função de repasse, que é
código comum. A cláusula está na linha `module`, então vale para todos os
modificadores do módulo, e cada um precisa ter os verbos; para tipos diferentes,
módulos diferentes.

**Uso.** O protocolo se usa como um tipo, depois de `import keel.protocols
types;`. Não é preciso outra marca: o `types` já significa "traga os tipos".

```keel
void fn(my i32 x, Indexable b) {
    size_t n = length(b);       // resolved by b's concrete type (spec §4.4, step 1)
    /* ... */
}

buffer i32 b;
slice i32 s;
my i32 x;
fn(x, b);        // instance with Indexable := buffer i32
fn(x, s);        // instance with Indexable := slice i32
```

**Instância da função.** O parser já descobre instâncias por uso reconhecido e
fecha o conjunto sobre os verbos (parser-design §5). A instância de `fn` é o
mesmo mecanismo, com outra identidade: (função, tipos concretos dos parâmetros
de protocolo). Cada uma chama os verbos do tipo concreto:

```c
void my_fn_keel_buffer_i32(my_i32 x, keel_buffer_i32 *b) { ... keel_buffer_i32_length(b); ... }
void my_fn_keel_slice_i32 (my_i32 x, keel_slice_i32  b) { ... keel_slice_i32_length(b); ... }
```

- **Nome.** `<módulo>_<função>_<tipos concretos manglados>`, com o sufixo de
  aridade por último (backend §2.1.1). O argumento carrega a própria
  qualificação (§2.1, regra 1), então `buffer i32` entra como `keel_buffer_i32`.
  A normalização do §2.2 faz `buffer int32_t` e `buffer i32` darem a mesma
  instância. O comprimento é limitado pelo `name-too-long` (§2.4), sem regra nova.
  A colisão por concatenação com um nome do usuário cai no `symbol-collision`
  existente.
- **Emissão.** Vale a regra das instâncias de módulo genérico: o corpo é
  `static inline` por padrão, e o corpo fora de linha só existe com um `instance`
  explícito, que escolhe o `.c` (backend §4.4). Não há exigência nova de
  `instance`, e nenhum diagnóstico especial para `static` local: a propriedade
  vale para toda função `static inline` e é do autor do módulo. O corpo em linha
  fica no **header da própria instância** (`my_fn_keel_buffer_i32.h`), como
  qualquer instância de modificador: quem instancia é quem usa, e dois módulos
  que usam o mesmo par geram o mesmo header, byte a byte (backend §4, regras 4
  e 5).
- **Sem símbolo C até instanciar.** Uma função com parâmetro de protocolo não
  tem símbolo C próprio, como o módulo genérico: não é chamável do C, e o `.c` do
  módulo não a contém.

**Regras.**

1. A lista de binders do protocolo é a mesma do implementador, por espécie e
   ordem. `buffer` e `slice` (só `type T`) conformam com um protocolo `type T`;
   um implementador com `dim` a mais precisa de outro protocolo.
2. O uso não fornece binders (`Indexable b`, e não `Indexable i32 b`): o `type
   T` do protocolo é o do implementador. O corpo de `fn` não nomeia o tipo do
   elemento; fixa-o pelo que escreve, e uma divergência com o argumento real cai
   no compilador C.
3. O tipo do argumento vem do **símbolo**, e não de inferência sobre expressão
   (§1.3): `fn(x, f())` é erro, e o argumento precisa ser um símbolo de tipo
   conhecido.
4. Cada ocorrência de protocolo é independente: `fn(Indexable a, Indexable b)`
   liga dois tipos que podem diferir.
5. **Passagem.** É definição do modificador, e keel a decide no uso, pelo bit
   `byref` da instância concreta (§5.1, item 2). O parâmetro se escreve sem `*`:
   para `fn(x, b)` com `b` um `buffer i32` (`byref`) a instância recebe
   `keel_buffer_i32 *b` e a chamada emite `&b`; para `fn(x, s)` com `s` um `slice`
   a instância recebe o valor e a chamada não leva `&`. Vale para a emissão da
   instância e para o ponto de uso.
6. **Só os verbos do protocolo.** Vale a regra da orientação a objetos: onde se
   espera a interface `List`, o uso interno é só dos métodos de `List`, mesmo que
   se passe um `ArrayList`. Na declaração de `fn`, `b` tem o tipo `Indexable`, e
   só os verbos do protocolo valem sobre ele. `push(b, 1)` é erro na declaração
   (`verb-not-in-protocol`), e não na instância. Consequências: `b` pode ser
   passado a outra função que espera `Indexable` (ou um protocolo que ele
   satisfaz), mas não a uma que espera `buffer i32 *` (não há conversão no
   sentido contrário); o qualificador de outro módulo (`buffer.push(b, 1)`) cai
   no `wrong-qualifier` existente; e `walk` sobre `b` exige um `b` do protocolo
   `Traversable`.
7. **Terminação.** Uma função que se chama com um tipo novo a cada volta
   instanciaria sem fim: precisa de um limite de profundidade.

**Diagnósticos propostos:** `protocol-not-satisfied` (o tipo do argumento não
declara o protocolo), `protocol-verb-missing` (o implementador declara o
protocolo e não tem os verbos), `verb-not-in-protocol` (verbo fora do protocolo,
no corpo), `protocol-verb-conflict` e `circular-protocol` (composição) e
`instance-depth`. O `protocol-on-parameter` da v0 continua valendo para
o parâmetro de tipo de módulo genérico, que segue opaco.

**Protocolos definidos por papéis.** Os protótipos podem levar os papéis de
procedência (`child`, `parent`, `invalidates`), e a conformidade exige as mesmas
assinaturas, papéis incluídos. Assim os protocolos de alocação e de hierarquia
saem definidos por assinatura, sem lista de verbos casada pelo nome.

**O que se ganha.**

1. **Algoritmo genérico escrito pelo usuário sobre um protocolo**, com uma
   instância por tipo, e não uma cópia do corpo por chamada (o que o molde faria).
   Hoje o parâmetro de tipo é opaco (spec §4.3, `protocol-on-parameter`), então
   `media` teria de ser escrita uma vez por contêiner.
2. **Conformidade verificada no implementador e na chamada**, com o verbo que
   falta nomeado, em vez de um erro dentro do corpo.

**Relação com o molde.** São independentes. O molde expande código no chamador,
serve para corpos pequenos e para injetar declarações no escopo dele. A função
com protocolo instancia uma vez por tipo, serve para qualquer tamanho, e o corpo
mora numa função. Um parâmetro de molde também pode ter tipo de protocolo, pelo
mesmo mecanismo, sem trabalho à parte.

**Quando.** Depois do M5: a instância de função depende da análise de corpo por
instância, que ainda não existe (`islands.h`: o módulo genérico "não tem ilhas
concretas"; o fecho transitivo é "uma passagem posterior"). O desenho fica no
papel agora, e o molde (que não depende de instâncias) vem antes, como decidido.

Abertos:

- **Grafia.** Os nomes de protocolo levam maiúscula inicial (`Indexable`), como
  os conjuntos de tags (`Status`) e os tipos do usuário (`Vec2`, `Particle`). A
  convenção da spec (§5.1) é da base e não é regra léxica: o conflito é só de
  leitura, e o uso (parâmetro de função, símbolo de outra espécie) desfaz a
  ambiguidade. A tabela da convenção ganharia uma linha.
- **Granularidade da cláusula.** Decidido: fica na linha `module` e vale para todos
  os modificadores do módulo. Um módulo que precise de conjuntos diferentes por
  modificador se divide; a base tem um por módulo.
- **Onde ficam os binders.** Na versão acima, no módulo (`module keel.protocols
  type T;`), e então todos os protocolos do módulo dividem a mesma lista (a spec
  fixa a aridade de tudo o que o módulo declara). A alternativa é cada protocolo
  declarar os seus, como um verbo declara o seu `type T`: `protocol Indexable type
  T { ... }`, `protocol Keyed type K, type V { ... }`. Protocolos não têm
  instâncias nem nomes manglados, então a regra que justifica a lista única do
  módulo não se aplica. Inclino-me pela segunda. A decidir.
- **Tipo do elemento.** Sem tipo associado, o corpo não o nomeia. Se a falta
  incomodar, o `type T` do protocolo poderia aparecer no uso (`Indexable i32 b`),
  ao custo de um argumento a mais em cada uso.

**As construções do núcleo consomem os protocolos (2026-09-30).** A spec §5.1 já
define sete protocolos, cada um como o conjunto de verbos que uma construção do
núcleo exige. Declarados como protocolos nominais, eles passam a ser o contrato
das construções, e o módulo que os declara tem de estar na base (`base/keel/
protocols.k`, módulo `keel.protocols`):

| Protocolo (spec) | Nome proposto | Verbos | Construção | Declarado por |
| --- | --- | --- | --- | --- |
| Indexável | `Indexable` | `length`, `ptr(x, i)` (`get` no `foreach` de binder por valor) | `x[i]`, `foreach` de dois binders | `buffer`, `slice` |
| Fatiável | `Sliceable` | `length`, `as_slice(x, a, b)` | `x[a..b]` | `array`, `buffer`, `slice` |
| Percorrível | `Traversable` | `begin`, `has_next`, `next` | `walk` | `buffer`, `slice` |
| Particionável | `Partitionable` | `partition` | `parallel` | `buffer`, `slice`, `range` |
| Contável | `Countable` | `first`, `limit` | `foreach` de um binder | `range` |
| Etiquetado | `Taggable` | `tag` | `match` | `tagged`, `corot` |
| Falível | `Failable` | `failed`, `win` | `else` | `outcome` |

`length` está em `Indexable` e em `Sliceable`: o losango da composição vale, com a
mesma assinatura. Os nomes em inglês são propostas; `Taggable` evita a confusão
com o módulo `keel.tagged`.

**Por que isso importa.** A regra 6 (só os verbos do protocolo sobre `b`) só é
coerente se os verbos que uma construção usa estão **dentro** do protocolo dela:
`foreach (T x : b)` sobre `b: Indexable` vale porque `length` e `ptr` são de
`Indexable`. Definir a construção pelo protocolo é o que faz as duas coisas
coincidirem, e substitui os diagnósticos escritos à mão de cada construção por
`protocol-not-satisfied`.

**Decidido (2026-09-30): nominal em tudo.** Como o protocolo é uma estrutura da
linguagem, as construções o usam: exigem a cláusula `protocol`, e o diagnóstico
de conformidade delas passa a ser o `protocol-not-satisfied`. A regra 1 da spec
§5.1 ("Declarar os verbos de um protocolo basta... Não há registro, marcação nem
permissão") é reescrita: a cláusula é o registro. O custo é alterar agora os
módulos do programa que implementam por estrutura; no golden, são
`019-walk/lst.k` e os `tasks.k` dos casos 022, 023 e 024, que declaram `begin`,
`has_next` e `next` sem cláusula. Alterar agora custa menos do que descobrir mais
adiante que tudo precisa mudar. A mudança nesses `.k` vai junto com o código que
passa a ler a cláusula (antes disso, o parse da linha `module` falharia); o
`expected/` não muda.

**Tipos que o usuário escreve.** `walk` exige que o usuário escreva o tipo do
cursor, que "é o produto declarado de `begin`" (spec §4.7: `walk (i32 *p,
buffer.cursor c : xs)`), e o binder de `parallel` escreve o tipo da partição.
Num corpo genérico, `fn(Traversable b)`, o tipo do cursor depende do argumento e
não pode ser escrito. É um **tipo associado** (o `Item` do Rust). `foreach` sobre
`Indexable` não tem o problema, porque o tipo do elemento é escrito pelo corpo
(`f64 *x`). Saídas: recusar `walk` e `parallel` sobre parâmetro de protocolo na
primeira versão; ou declarar o tipo no protocolo e nomeá-lo no corpo; ou permitir
omitir o tipo do cursor quando o contêiner é de protocolo.

**Protocolos da base e o núcleo.** Cada construção do núcleo estabelece o protocolo
que consome (a tabela acima), e o protocolo é declarado na base, em
`keel.protocols`. O protocolo é o **contrato** da construção, como a gramática é o
da sintaxe: a spec de cada construção o cita, e a biblioteca o fornece. O que o
núcleo referencia, então, não é um módulo privilegiado com comportamento próprio
(como a arena hoje), mas o contrato da construção. Continuam ligados a um módulo
**por nome**, e não por protocolo: o `as_slice` de `keel.array` para `x[a..b]`
sobre `array`, o `range` que o literal `a..b` produz, e o símbolo `parallel.control`
do bloco `parallel`. Fica a decidir se esses três também se expressam por
protocolo. `array` participa de `Indexable` pelo núcleo, sem módulo de verbos, e
não tem descritor (a extensão é um `dim` em tempo de tradução): não deve ser
argumento de parâmetro de protocolo na primeira versão.

### Protocolos de alocação e de hierarquia: Alocável e Hierarquizável

Hoje o núcleo trata a arena pelo nome: registra a procedência (spec §5.2, regra
10) e emite `child-arena-after-reset` só para os verbos de `keel.arena`. Dois
protocolos tirariam a arena da lista de pontos que o núcleo conhece pelo nome, e
abririam a porta para outros alocadores (um pool, um `slab`, uma pilha sobre
arena). São independentes: um módulo pode querer ser hierarquizável sem alocar.

| Protocolo | Verbos | Papel |
| --- | --- | --- |
| Alocável em grupo | `alloc(a, n, sz, al)` e `reset(a)` | entrega memória e a recolhe toda de uma vez |
| Alocável individualmente | `alloc` e `free` | entrega e devolve um elemento por vez |
| Hierarquizável | `from_parent(filho, pai, …)` | uma região feita de outra: invalidar o pai invalida as filhas |

(Nomes em inglês provisórios: `GroupAllocatable` e `SingleAllocatable`.)

Um verbo pode estar em mais de um protocolo: `reset` é o recolhimento em Alocável em
grupo, e é o que se propaga às filhas em Hierarquizável. O papel de cada verbo vem da
pertença ao conjunto, e não de uma marca nele: um módulo que só tem `reset`, sem
`alloc`, não é alocador, e o `reset` dele não conta. A arena é Alocável em grupo e
Hierarquizável; um pool de slots é Alocável individualmente e, por ter `reset`,
também em grupo; uma sessão com sessões filhas, sem `alloc`, é só Hierarquizável.

**Protocolo e papel são coisas diferentes (2026-09-30).** Ter papéis
(`child`, `parent`, `invalidates`) é um comportamento **analisado pelo parser
semântico**, e vale para o verbo de qualquer módulo. Fazer parte de um protocolo é
ser **usado obrigatoriamente em outro lugar**: por uma construção do núcleo ou por
um código genérico que recebe o protocolo como tipo. Daí: `mark` e `restore` da arena
são verbos necessários nela, com papéis (`invalidates`), mas **não fazem parte de
nenhum protocolo**, porque nada os exige em outro lugar. Não são "opcionais" de um
protocolo. O `realloc` fica de fora, por enquanto, de todos.

**A filosofia é a de muitos.** Alocar um e liberar um (`malloc` e `free`) é a
filosofia oposta, e o núcleo não a distingue: keel escolheu o lote por causa do
DOD. Um módulo que embrulhe `malloc`/`free` faz essa distinção por dentro; o
protocolo só exige `alloc` e `reset`.

**O que o núcleo consegue verificar é pouco.** Esta entrada inicialmente
considerava só o uso de região filha depois de `reset` ou `restore` do pai,
no mesmo escopo. A proposta específica abaixo preserva também o diagnóstico
de escape de contêiner com procedência local conhecida. Não há como mapear dado derivado usado depois do `reset`:
o ponteiro devolvido por `alloc` circula por struct, retorno, argumento e alias,
e a tradução não segue nada disso. Tampouco pega o caso interprocedural: se o
pai é invalidado numa função e a filha é usada no chamador, passa. Ficam com o
programa, como hoje (§5.2, "Casos especiais"). Os protocolos dão estrutura e
nome ao que já existe; não prometem mais análise.

Abertos:

- **Nome.** "Componentizável" colide com "componente" da leitura ECS (§3), que é
  coluna de `extent`. Alocável e Hierarquizável seguem o padrão de Indexável,
  Fatiável e Percorrível. O diagnóstico `child-arena-after-reset` precisaria de um
  nome que não diga `arena`, se o protocolo o generalizar.
- **`clone` genérico.** `clone(arena *a, …)` de `buffer` e `slice` fixa o tipo.
  Aceitar qualquer Alocável exige parâmetro genérico com exigência sobre ele,
  que é o protocolo nominal como tipo de parâmetro (§2 acima).
- **`restore` com marca.** `restore(pai, m)` invalida só o que foi derivado
  depois de `m`, e `reset` invalida tudo. A verificação precisa da ordem de
  derivação, ou fica conservadora e recusa todas as filhas.
- **Fora dos alocadores.** Visões sobre um `buffer`, como o `slice` depois de
  `clear` ou `push`, seguem uma lógica parecida de invalidação. Registrada aqui
  como vizinha, sem estar incluída.

### Proposta de texto: regiões, procedência e arena na Base (2026-09-29)

**Status: proposta para discussão, não normativa.** Reelaboração da entrada
anterior. O objetivo é manter no núcleo garantias mínimas e mecanismos gerais
para abstrações que se resolvem em C, deixando a arena como implementação
padrão da Base. Não altera a v0 nem autoriza mudanças nos normativos.

**Direção acordada nesta discussão:** `arena.from_stack` fica adiado. Ele
expande armazenamento no escopo do chamador e não pode virar uma função comum
sem mudar o tempo de vida da memória. Um futuro mecanismo geral de construção
pode permitir implementá-lo na biblioteca; até lá, escreve-se o armazenamento
local explicitamente.

A generalização deve preservar duas verificações distintas: escape de
contêiner cuja procedência conhecida é armazenamento local, e uso de região
filha após invalidação do pai. Validar o tipo do argumento de `from_array` não
substitui nenhuma delas.

#### Texto candidato para substituir a spec §5.2

##### Alocação, procedência e hierarquia de regiões

**1. Contratos**

Alocável e Hierarquizável são protocolos independentes. Seus verbos pertencem
a módulos comuns e são resolvidos pelas regras de resolução de operações.
O núcleo reconhece os contratos pelas declarações, sem depender do nome
`keel.arena`.

| Protocolo | Verbos | Papel |
| --- | --- | --- |
| Alocável em grupo | `alloc(a, n, sz, al)` e `reset(a)` | reservar armazenamento e invalidar alocações em lote |
| Alocável individualmente | `alloc` e `free` | reservar e devolver um elemento por vez |
| Hierarquizável | `from_parent(child, parent, …)` e `reset(region)` | estabelecer dependência entre regiões e invalidar derivadas |

`mark` e `restore` são verbos da arena com o papel `invalidates`, e não fazem
parte de protocolo: nada os exige em outro lugar (2026-09-30).

A presença isolada de um nome não estabelece um protocolo. Os verbos exigidos
devem operar sobre o mesmo tipo receptor. Uma região pode ser Hierarquizável
sem oferecer alocação; um Alocável pode não oferecer regiões filhas.

`alloc` recebe o alocador por ponteiro, a quantidade de objetos, o tamanho
de cada objeto e o alinhamento. Devolve `void *`, ou `NULL` quando não puder
satisfazer a solicitação. Tamanho, alinhamento e algoritmo são responsabilidades
da implementação. A forma tipada `alloc(a, T, n)` pode ser escrita pelo módulo
com parâmetro `type T` apagado; usa o mecanismo geral de tamanho, alinhamento
e conversão do retorno.

`from_parent` recebe primeiro a filha e depois o pai, por ponteiro. Os demais
parâmetros pertencem ao módulo. O retorno `bool` informa sucesso; na falha,
a filha fica sem região utilizável. Na construção bem-sucedida, sua validade
depende da validade do pai.

`reset` invalida os recursos anteriores da região. O protocolo não determina
se a implementação devolve memória ao sistema ou limpa objetos. `restore`,
quando declarado, invalida os recursos descartados pela restauração; a
representação da marca pertence ao módulo.

**2. Procedência**

A procedência é informação de tradução, associada a símbolos e construções
reconhecidos. Não exige campos no descritor nem registros em execução.

1. Uma construção reconhecida sobre um `array` de armazenamento automático
   registra origem local e seu escopo de duração. Um vetor de duração estática
   não é classificado como local apenas por ser declarado dentro de função.
2. `from_parent` registra a dependência da filha e transmite a procedência
   conhecida do pai.
3. Uma operação reconhecida de alocação ou construção de contêiner transmite
   a procedência de sua origem somente nas formas que o contrato de análise
   já acompanha. Isso não autoriza rastreamento geral de ponteiros.
4. Uma reatribuição sem procedência reconhecível torna a origem desconhecida.
   Origem desconhecida não equivale a origem comprovadamente válida.
5. Retornar um contêiner cuja procedência conhecida é armazenamento local
   produz `region-escape`. A regra independe da implementação do alocador.
6. O núcleo não deduz procedência de corpos C opacos, headers ou expressões
   arbitrárias. A procedência de uma construção sobre ponteiro cru permanece
   desconhecida quando sua origem não puder ser registrada pelas formas
   reconhecidas.

Para aplicar essas regras a bibliotecas do programa, o contrato de construção
deve tornar explícita a posição da origem e a posição do produto. A construção
sobre vetor conhecido, como `from_array`, tem esse papel; o nome da arena não
o tem. A forma declarativa geral para expressar esses papéis deve ser fechada
antes de incorporar este texto à spec. Reconhecer funções arbitrárias pelo
nome, sem contrato, não é suficiente.

**3. Hierarquia e invalidação**

1. Invalidar o pai invalida suas filhas e, transitivamente, as regiões
   derivadas delas.
2. Resetar uma filha não revalida armazenamento invalidado pelo pai.
3. Sair do escopo de um descritor não devolve recursos ao pai. A duração do
   descritor não determina a duração do armazenamento.
4. O núcleo registra dependências entre símbolos conhecidos estabelecidas por
   construções reconhecidas. Não avalia o resultado booleano do construtor:
   registra a dependência que existirá caso ele tenha sucesso.
5. No mesmo escopo, uma operação reconhecida sobre filha cuja dependência
   permanece conhecida, depois de invalidação do pai, produz
   `child-region-after-invalidation`. O próprio `reset(child)` é uso.
6. Uma nova construção reconhecida da filha substitui sua relação anterior.
7. `restore` exige uma política lexical explícita. Como proposta mínima,
   considerar todas as filhas conhecidas invalidadas; essa política é
   conservadora e pode recusar uma filha que a implementação tenha preservado.
   Não se interpreta o valor de uma marca nem o histórico de alocações em
   execução.

**4. Inicialização e construção**

Inicialização padrão é uma propriedade declarada pelo tipo, independente dos
protocolos Alocável e Hierarquizável.

1. Uma definição sem inicializador escrito recebe o inicializador padrão
   declarado pelo tipo, quando houver.
2. Um inicializador explícito prevalece.
3. Declarações `extern` e campos de agregados não recebem inicialização
   inserida. A inicialização do agregado continua sendo responsabilidade do
   programa.
4. A regra deve contemplar vetores do tipo e duração estática, preservando os
   casos hoje admitidos para arena. A forma de composição do inicializador
   para vetores precisa ser especificada no backend.
5. O núcleo substitui o inicializador declarado; não avalia expressões C nem
   insere chamada de construtor ou cleanup.
6. Os construtores são operações explícitas do módulo. Inicializador padrão
   não adquire armazenamento nem prolonga seu tempo de vida.

A arena declara `{0}` como seu padrão porque sua implementação define esse
estado como vazio. Outro tipo pode declarar outro padrão ou exigir
inicialização explícita. A sintaxe da declaração do padrão ainda está aberta;
não é uma palavra nova aprovada por esta proposta.

Exemplo de uso pretendido, com os mecanismos de declaração ainda a definir:

```keel
array u8 storage[4096];
arena a;  // The type declares {0} as its default initializer.
if (!arena.from_array(a, storage)) return false;

arena child;
if (!arena.from_parent(child, a, 1024)) return false;
```

As condições de alinhamento e de tipo efetivo do respaldo continuam no
contrato da Base e no backend. Escrever um vetor local explicitamente não
dispensa essas condições.

**5. Implementação padrão e custo**

`keel.arena` é a implementação padrão dos contratos na Base. Seu layout,
algoritmo linear, construtores, marcas, capacidade e comportamento na falta
de espaço pertencem ao contrato da biblioteca.

Os protocolos não impõem despacho dinâmico, campos adicionais nem controle de
procedência em execução. A resolução emite chamadas C diretas; as
verificações de procedência e hierarquia são de tradução. As operações e
checagens executadas pelo módulo têm o custo documentado pela implementação.

**6. Diagnósticos e limites**

| Diagnóstico atual | Destino proposto |
| --- | --- |
| `nonconstant-arena-stack` | adiado junto com `from_stack` |
| `arena-from-array-not-u8` | exigência de assinatura do construtor da Base; não é uma garantia de procedência |
| `arena-escape` | `region-escape`, conservando o alcance lexical atual |
| `child-arena-after-reset` | `child-region-after-invalidation`, conservando o alcance lexical e explicitando a política de `restore` |
| `byref-param` | preservar mediante mecanismo geral de passagem por referência, independente da alocação |
| `alloc-overflow` | checagem da implementação de `alloc`; preservar o comportamento documentado de debug e o retorno `NULL` na falha |

A assinatura de `from_array` na Base exige `array u8`. O mecanismo geral de
parâmetro `array` fornece a extensão; a compatibilidade do elemento é
validada conforme as regras keel e C aplicáveis. Mover a exigência para a
assinatura pode mudar a origem e a identificação do diagnóstico, e isso deve
ser declarado na revisão do catálogo.

Arena hoje é um tipo comum com restrição especial de passagem por valor.
Remover esse privilégio sem perder `byref-param` exige generalizar a
propriedade `byref` para tipos comuns. Exigir ponteiro nas operações do
protocolo não proíbe, por si só, passagem por valor em outras funções.
A sintaxe dessa generalização permanece aberta.

O programa mantém o armazenamento válido, respeita as marcas e não usa dados
descartados por `reset` ou `restore`. A verificação não acompanha aliases,
cópias, parâmetros de saída ou efeitos interprocedurais. Ponteiros derivados
usados depois da invalidação permanecem fora da garantia quando essas formas
escapam do reconhecimento atual. Não se promete segurança geral de memória.

#### Impacto editorial e condições de incorporação

| Documento / seção | Alteração necessária |
| --- | --- |
| Spec §5.1 | incluir protocolos consumidos por verificações; acrescentar Alocável e Hierarquizável; retirar o privilégio pelo nome `arena` |
| Spec §5.2 | substituir o contrato específico pelo contrato geral de regiões; mover a descrição concreta para a documentação da Base |
| Spec §§2.2 e 4.2 | declarar a forma geral de inicializador padrão, depois de escolhida sua sintaxe |
| Spec §§2.2 e 4.3 | generalizar `byref` para tipos comuns, depois de escolhida sua sintaxe |
| Spec contrato de análise e resolução (§§1.3, 2.3 e 4.4) | explicitar os papéis de origem e produto usados para transmitir procedência |
| Spec §6.2 | generalizar escape e invalidação; rever os diagnósticos de `from_stack`, `from_array` e overflow |
| Spec §§6.3–6.5 | separar garantias gerais de regiões das condições particulares do respaldo da arena |
| Base | documentar a arena como implementação padrão; preservar checagens de capacidade, overflow, alinhamento e estado vazio |
| Backend | substituir reconhecimento pelo nome da arena pelos contratos gerais; especificar inicializadores e procedência |
| Rationale | registrar o motivo da generalização, os limites mantidos e o adiamento de `from_stack` |

Exemplos de `arena.alloc` podem permanecer na spec: passam a exemplificar uma
biblioteca. `buffer.clone` e `slice.clone` podem continuar recebendo arena;
generalizá-los para qualquer Alocável depende do futuro `bound`.

**Critério de aceite:** conseguir retirar o reconhecimento de
`keel.arena` pelo nome, preservando escape de origem local conhecida,
invalidação de filhas, inicialização padrão e restrição de passagem por valor
por mecanismos gerais. As exceções adiadas ou alterações de diagnóstico devem
ser explícitas. Essa proposta não acrescenta análise geral de C.

**Ainda a fechar:** declaração dos papéis de procedência (proposta na
subseção seguinte); sintaxe do
inicializador padrão; aplicação de `byref` a tipos comuns; política lexical
de `restore`; localização do contrato concreto da arena na documentação da
Base. Não se deve apresentar esses pontos como já resolvidos por uma simples
troca de nomes.

#### Papéis de procedência na assinatura (2026-09-30)

**Status: proposta para discussão, não normativa.** Responde ao primeiro ponto
de "Ainda a fechar": como um módulo declara a origem e o produto de uma
construção, sem que o núcleo reconheça funções pelo nome. As grafias `child`,
`parent` e `invalidates` são provisórias. (Uma versão anterior deste texto usava
`out` e `from`; `out` sugeria parâmetro de saída, que é uma questão de passagem,
e não da relação.)

A assinatura já carrega marcas lidas pela resolução: `type T` (§4.4) e `byref`
(§4.3). Os papéis são da mesma família: **a assinatura declara, e o núcleo
verifica os pontos de uso.** Os papéis nomeiam a relação que existe entre o que
a chamada recebe e o que ela produz: uma dependência de pai para filho.

```keel
module keel.arena;

pub bool from_array (child arena *a, parent array u8 v);
pub bool from_parent(child arena *a, parent arena *p, size_t n);
pub bool from_memory(child arena *a, u8 *p, size_t n);   // no `parent`
pub child T *alloc  (parent arena *a, type T, size_t n);
pub void reset      (invalidates arena *a);
pub void restore    (invalidates arena *a, size_t m);
```

1. **Filha.** É o parâmetro marcado `child`, ou o retorno marcado `child`. Um
   verbo pode ter várias filhas (`split(parent a, child b, child c)`), e cada
   uma depende de todos os pais.
2. **Pai.** A filha depende do argumento marcado `parent`: sua memória vive nele
   e ela morre quando ele é invalidado. Numa chamada com símbolo conhecido nessa
   posição, o núcleo registra a procedência do pai na filha e uma aresta de
   dependência da filha para o pai. Vários pais unem as procedências.
3. **Raiz.** A única raiz é o `array` do núcleo. Ele é local se a duração é
   automática, e a duração vem da declaração. Todo o resto vem por transmissão
   pelo `parent`. Um molde (`keel_code`) que declara `array unsigned char st[N]` e chama
   `from_array` recebe a procedência local por esta regra, sem regra própria: é
   assim que o `from_stack` adiado poderia voltar como biblioteca.
4. **`invalidates`.** A operação invalida o argumento e, transitivamente, tudo
   o que depende dele pelas arestas. O uso posterior de uma filha conhecida, no
   mesmo escopo, produz `child-region-after-invalidation`. Para `restore`, vale
   a política conservadora da proposta acima.
5. **Estados da procedência.** Dois: *local conhecida* e *sem garantia*. Só o
   primeiro produz `region-escape` no retorno. O segundo cobre a filha sem pai
   declarado (como em `from_memory`), a reatribuição sem procedência, o
   argumento que não é símbolo conhecido e a expressão C. Sem garantia não
   equivale a válida. Distinguir a origem externa declarada da perdida só teria
   consumidor num modo estrito ("avise quando a origem for desconhecida"), e
   fica como extensão.
6. **Confiança.** Os papéis são declaração do autor do módulo. Corpo C opaco
   não é verificado: uma assinatura que mente não é detectada. A garantia vale
   só para o que a declaração afirma.

**Papéis e protocolos.** Os protótipos de um protocolo levam os papéis, e a
conformidade exige os mesmos papéis: quem chama o verbo pelo protocolo depende
deles na análise. A **pertença** a um protocolo, porém, não se deduz dos papéis: um
protocolo é o conjunto de verbos que alguém exige (por exemplo, `alloc` e `reset` para o
Alocável em grupo), e um verbo com papéis pode não estar em nenhum. O núcleo deixa de
casar `alloc` e `reset` pelo nome da arena: os papéis dão a análise, e o protocolo
dá o contrato.

**Efeitos sobre o catálogo.** `arena-from-array-not-u8` vira a checagem
comum do tipo do parâmetro `array u8`. `buffer.clone(parent arena *a, …)`
declara o papel na assinatura, sem depender do `bound`; generalizar o tipo de
`a` segue dependendo dele. Alias, struct, retorno indireto e efeito
interprocedural continuam fora do alcance.

**Palavras de papel e o mar de C.** `child`, `parent` e `invalidates` ficam na
posição de modificador da assinatura de um verbo, e o cgen as apaga na emissão.
Não há modificador com esses nomes em C, e uma macro homônima não altera o que
keel lê (keel não expande macros) nem o C que sai (o compilador nunca vê a
palavra). Por isso as palavras de papel **não** entram na regra
`define-over-keel-name`, e o programa não perde esses nomes.

**`parent` único, ou um papel por espécie.** O `parent` reúne dois fatos: o
armazenamento (a localidade que alimenta o escape) e a validade (a aresta que
alimenta a invalidação). Para a arena, os dois coincidem. Divergem quando há
dependência sem memória emprestada, como uma sessão filha sem `alloc`, e
quando há memória emprestada sem invalidação, como um `slice` sobre `static`.
Ficou o `parent` único: o primeiro caso é hipotético hoje, e um papel só de
validade (nome provisório) entra depois sem quebrar assinaturas, com `parent`
continuando a valer pelos dois.

Abertos:

- **`from_stack`.** Como molde (`keel_code`), na seção seguinte.
- **Verbos que só invalidam**, mas cujo argumento não é o receptor (por
  exemplo, `clear(b)` sobre um `buffer` com `slice` derivados): entram pelo
  mesmo `invalidates`, e o caso das visões sobre `buffer` fica registrado como
  vizinho, sem estar incluído.

#### Verificação em execução da invalidação: campos explícitos (2026-09-30)

**Status: proposta para discussão, não normativa.** Segunda camada, ao lado da
verificação de tradução dos papéis: a tradução pega o caso lexical, e a
verificação em execução pega o que a análise não alcança (alias, struct,
interprocedural), pelos verbos do descritor.

Não exige mecanismo no núcleo. `alloc`, `reset` e os construtores já são
código do módulo, e o módulo faz a checagem no corpo, com `KEEL_CHECK`, como
faz com `alloc-overflow`. O modelo é o do `extent`: o módulo **declara** numa
lista os campos de controle e escreve as atualizações no corpo dos verbos. Nada
é gerado por conta própria. No máximo, o núcleo confere que os campos citados
existem.

A lista é ordenada, pai primeiro e filha depois, e separa o que cada lado
guarda (leitura confirmada em 2026-09-30):

```keel
control (parent: epoch; child: link, stamp);
```

O pai tem a `epoch`, incrementada pelas operações `invalidates`. A filha tem o
ponteiro `link` para o pai e o `stamp` com a época dele na construção. Na arena,
pai e filha são o mesmo tipo, e os três campos moram no mesmo `struct`:

```c
/* arena descriptor with the optional control fields */
typedef struct keel_arena {
    size_t top, cap;
    u8    *ptr;
    struct keel_arena *link;     /* optional (child) */
    size_t epoch;                /* optional (parent): reset and restore increment it */
    size_t stamp;                /* optional (child): parent's epoch at construction */
} keel_arena;
/* every verb of the child: */
KEEL_CHECK(a->link == NULL || a->link->epoch == a->stamp,
           "child-region-after-invalidation");
```

- **Campo opcional por módulo.** O módulo escolhe presença sempre, só em debug
  ou nenhuma. Para a arena filha, o custo (um ponteiro e dois contadores num
  descritor raro) é baixo, mesmo em release. Para `slice` e `buffer`, que são
  descritores por valor, ubíquos e quentes, dois campos a mais dobram o
  tamanho: em visões, o campo fica em debug ou desligado.
- **Layout.** Um campo só em debug muda o layout entre debug e release, e
  objetos misturados quebram. É uma decisão do módulo, e vale registrá-la na
  documentação dele.
- **Limites.** Só o uso pelos verbos do descritor é coberto. O `T *` devolvido
  por `alloc` é ponteiro cru de C, e nenhum descritor o cobre. O ponteiro do
  pai fica pendurado se as arenas moram num vetor que realoca.
- **`restore`.** Incrementar a época em todo `restore` dá falso positivo em
  filha que foi preservada, e em execução isso é um `abort` num programa
  correto, o que é pior que o erro de tradução conservador. A versão precisa
  compara o fim da filha com o topo do pai, e não está desenhada.

Abertos: a sintaxe da lista de campos de controle; se os papéis devem citá-los;
a política de `restore` em execução.

#### Handle: ponteiro com geração (2026-09-30)

**Status: ideia a estudar, não normativa. Não está decidido que valha o esforço.**
Fecha a lacuna que a verificação em execução deixava: o `T *` devolvido por
`alloc` é ponteiro cru, e nenhum descritor o cobre. Um modificador `handle`
encapsula **um ponteiro mais a geração em que ele nasceu**, e é **filho** da arena
pelos papéis de procedência. Com isso, passa a ser coberto pelos dois mecanismos
que já temos:

```keel
module keel.handle type T;

modifier handle { T *ptr; keel_arena *link; size_t stamp; }
control (child: link, stamp);                       // same list as for a child arena

pub child handle make(parent arena *a);             // allocates one T in `a`
pub T get(handle h);                                // checks the generation
```

**Características.**

- **É um struct, e não um ponteiro.** Não admite aritmética de ponteiros: não há
  `h + 1`, e isso vem da própria forma, sem regra extra.
- **Sem operadores.** Não há `*h` nem `&x` sobre o handle. Tratar a referência com
  os operadores de ponteiro repetiria o que se fez com `[]` (a sobrecarga de
  `x[i]` por protocolo), só que com `*` e `&`, e faria o handle parecer um ponteiro
  ou uma referência de C++, o que ele não é. O acesso é por verbos (`get`, `set`,
  `ptr`). Por isso o nome é `handle`, e não `reference`.
- **Por valor.** Copiar um handle copia a geração, e a validade não depende da
  cópia.

**Cobertura.**

- **Estático.** Os papéis registram o handle como filho da arena. Um `reset(a)`
  seguido do uso do handle no mesmo escopo é `child-region-after-invalidation`,
  como para qualquer filha.
- **Em execução.** O handle guarda o dono (`link`) e a época dele na criação
  (`stamp`). `get` confere a época antes de desreferenciar, como na verificação por
  campos explícitos.
- **Custo, escolhido pelo programador.** O ponteiro cru custa 8 bytes e não é
  verificado. O handle custa três palavras e uma comparação por acesso, e os campos
  de controle podem ser só de debug ou desligados por módulo.

**Limites.**

- **O ponteiro obtido do handle escapa.** Um `ptr(h)` que devolve `T *` volta a ser
  ponteiro cru. Pelos papéis, esse `T *` pode ser declarado filho do handle (`pub
  child T *ptr(parent handle h)`), e a cadeia é seguida na análise lexical, mas não
  em execução.
- **`restore`.** A época sobe no `reset`. No `restore(m)` o problema é o mesmo da
  filha: sobe-se a época e há falso positivo nos handles anteriores à marca, ou
  compara-se o offset com o topo e perde-se o handle obsoleto depois de novas
  alocações. A versão precisa não está desenhada.
- **Dono concreto.** O módulo do handle conhece o tipo da arena (`keel_arena
  *link`). Generalizar o dono para qualquer alocador esbarra no parâmetro de tipo
  opaco (spec §4.3, regra 7), e um campo não pode ser do tipo de um protocolo (seria
  despacho dinâmico). Na primeira versão, o handle é da arena.
- **Cooperação entre módulos.** `make` precisa ler a época da arena, e o acesso a
  campo de instância de outro módulo é diagnosticado (`instance-field-access`). A
  arena teria de expor a época por um verbo público.

**Ideia: `*` e `->` como açúcar sobre o handle.** Como o acesso é por verbo, `*h`
poderia ser reescrito para `*handle.ptr(h)` e `h->f` para `handle.ptr(h)->f`, com o
`ptr` expandido em linha. É a mesma forma de `x[i]`, que já é açúcar sobre `ptr(x,
i)` (e o verbo `ptr(x)` de aridade um já existe nas instâncias, backend §2.1.1).

- **`&h` não muda.** Seria ambíguo (o endereço do struct, para passar por
  referência, ou o do ponteiro que ele guarda), mas o primeiro é o de C e não pede
  reescrita; o segundo é `handle.ptr(h)`, por verbo. Keel nunca reescreve `&`.
- **O ponteiro devolvido é `ref`.** `T *ref p = handle.ptr(h)`: o `ref` da spec
  (§4.2) já proíbe aritmética (`ref-arithmetic`), exige inicialização, desaparece no
  C, e dá ao ponteiro um símbolo conhecido. Com o papel `child` em `ptr`, esse
  símbolo é filho do handle e, por ele, da arena, então o uso depois do `reset` é
  seguido na análise lexical. A restrição vale para o símbolo declarado, "sem seguir
  cópias do endereço" (spec §4.2, item 15): copiar para um `T *` cru escapa.
- **Custo no núcleo.** O açúcar acrescenta ao núcleo uma terceira sintaxe de
  operador (o `*` unário e o `->`), ao lado do índice `[i, j]` e da região
  `[a..b]`, que são as duas que restam na síntese acima. O `*` unário é ambíguo com
  a multiplicação e com o declarador (`T *h`, com `T` um tipo C que keel não
  resolve), e um molde não serve: ele tem a forma de chamada, e não reescreve
  operador. Em contrapartida, o custo da comparação de geração fica escondido atrás
  de um `*`, como o da verificação de limite atrás de um `x[i]`.
- **Inclinação.** Não acrescentar o açúcar: `handle.ptr(h)` já dá o mesmo sem
  ampliar o núcleo, e a síntese do núcleo reduzido diz que as duas sintaxes de
  operador que restam são o índice e a região. Só reavaliar se o uso mostrar que o
  verbo é pesado demais.

**Desenho preferido: índice geracional sobre um alocador de slots.** O handle com
ponteiro é a versão sobre arena. O ponto de chegada é o que o DOD já pratica: uma
referência por **índice e geração** a um alocador de slots (`pool`), no estilo de
uma entidade. O handle passa a guardar só `{ index, gen }`, sem ponteiro:

```keel
module keel.pool type T;

modifier pool byref { /* slot array, per-slot generation, free list */ }
modifier entity { u32 index; u32 gen; }                   // no pointer: 8 bytes

pub child entity alloc(parent pool *p);                   // takes a slot
pub void free(parent pool *p, invalidates entity e);      // frees ONE element
pub void reset(invalidates pool *p);                      // invalidates the whole pool
pub T *ref ptr(parent pool *p, entity e);                 // checks the generation
```

- **Por que melhora o handle.** A referência custa 8 bytes, e não três palavras;
  não guarda endereço, então sobrevive ao crescimento do array de slots (o ponteiro
  do handle ficaria pendurado); e é relocável e serializável, como os elos por
  índice da entrada sobre estruturas flat.
- **`free` e `reset` são verbos diferentes.** `reset` é o recolhimento em lote, o
  mesmo dos protocolos de alocação e de hierarquia (`reset(invalidates T *a)`):
  invalida a região e tudo o que depende dela. A liberação de **um** elemento é o
  `free`, com `invalidates` sobre o **argumento** `e`, e não sobre o pool. Dar o
  mesmo nome às duas (`reset(p)` e `reset(p, e)`) é possível pela aridade (backend
  §2.1.1, sufixo), mas o mesmo verbo com raios de invalidação tão diferentes
  esconde o custo, e keel escolheu o lote para o `reset` (a filosofia é a de muitos).
- **Estático.** `invalidates` sobre `e` faz do uso de `e` depois do `free`, no mesmo
  escopo, um `child-region-after-invalidation`, e o mesmo vale para os `T *ref`
  derivados dele. Cópias de `e` não são seguidas na análise lexical.
- **Em execução.** `ptr` compara a geração do slot com a de `e`: pega o uso de uma
  cópia depois do `free`, o que a análise lexical não alcança. É a mesma divisão
  em duas camadas do resto. A detecção é exata por elemento, e não grossa por
  época da região.
- **Protocolos.** O `pool` é Alocável individualmente (`alloc` e `free`) e, por ter
  `reset`, também Alocável em grupo. Os dois protocolos não se compõem: cada um é o
  que algum lugar exige. Não há `realloc`, por enquanto. O `alloc` do pool devolve a
  entidade, um tipo **do implementador**; o protocolo só precisaria nomeá-lo se
  algum código genérico tivesse de escrevê-lo (tipo associado, ver a entrada de
  protocolo nominal).
- **Custo, especificado por módulo.** Alocar e liberar em tempo constante com lista
  livre; uma comparação por `ptr`; `reset` em tempo constante com uma época do pool
  junto da geração do slot, ou linear, subindo a geração de cada slot. A geração de
  32 bits dá a volta depois de 2^32 liberações do mesmo slot, e o comportamento
  nessa volta deve ser especificado (ou alargar a geração).

Limites: o `T *ref` obtido por `ptr` tem a mesma restrição do handle: copiar para
um `T *` cru escapa da análise; e a arena não tem esse desenho, porque não libera
elemento individual.

**A estudar.** Se o ganho compensa o esforço: o handle com ponteiro só cobre o uso
pelos verbos dele, em troca de três palavras por ponteiro, e a cultura de DOD de
keel já prefere índices a ponteiros. O índice geracional acima custa menos por
referência e cobre mais, e é o desenho que eu estudaria primeiro.

#### Moldes: a marca `keel_code` (2026-09-30)

**Status: proposta para discussão, não normativa.** O estudo do que mudaria na
documentação e no código está em [`mold-impacto.md`](mold-impacto.md).

**Por quê.** A biblioteca precisa escrever o que hoje o cgen faz à mão, como o
vetor `keel__st<N>` do `from_stack` (backend §5.4), sem que o programador
recorra a macros. A spec não expande macros e exige que as construções existam
antes da expansão (§1.2): uma macro parametrizada com `arena.alloc(...)` dentro
**não é traduzida**, e o seu resultado escapa da análise. Restringir a injeção
a declarações de `array` empurraria as pessoas para as macros.

**Nome.** O conceito é o **molde** (`mold`, nos identificadores e nos
diagnósticos), e a marca que o programador escreve é `keel_code`. "Template"
já nomeia o módulo genérico no código do cgen (`islands.h`, `instances.c`), e
"macro" já nomeia, na spec, a macro do pré-processador C. O prefixo `keel_` é
reservado (backend §2.3, item 3): o programa não declara identificadores com
ele, e `#define keel_code` já é `define-over-keel-name`.

**O que é um molde.** Uma função marcada com `keel_code` não é uma função C: é
um molde de **substituição em fichas** no ponto de chamada. Não gera função
nem símbolo no C. O que o corpo contém é keel, e não C colado: depois da
expansão, o chamador é analisado como qualquer outro código keel, com símbolos,
ilhas e papéis de procedência.

```keel
keel_code bool from_stack(child arena *a, dim N) {
    alignas(alignof(max_align_t)) array unsigned char st[N];
    return from_array(a, st);
}

keel_code void greet(dim N, char *name) {
    array char buf[N] = {0};
    size_t len = snprintf(buf, N, "%s %s", "Hello", name);
    return len;
}
```

Uma chamada `arena.from_stack(t, 4096)` ou `greet(30, name)` é uma ilha, e
seu resultado é o corpo expandido.

**Parâmetros.** Cada parâmetro é ligado conforme o seu tipo:

| Parâmetro | Ligação na chamada |
| --- | --- |
| `type T` | as fichas do tipo, conferidas como tipo |
| `dim N` | as fichas do argumento, conferidas como constante conhecida (`nonconstant-dim`); o mesmo contrato do `dim` de módulo |
| `keel_code C` | as fichas do argumento, tal como estão: um fragmento keel |
| com papel (`child`, `parent`) | só identificador, substituído como está, para a análise ver o símbolo do chamador |
| valor comum | ligado **uma vez** a um local com o tipo declarado, na ordem dos parâmetros |

`type`, `dim` e `keel_code` são substituídos porque só existem na tradução. O
valor comum se comporta como o de uma função: é avaliado uma vez, e o corpo
pode modificar a sua cópia.

**Expansão.** A chamada `m.f(args)` que resolve para um molde é expandida nesta
ordem:

1. Dividir os argumentos nas vírgulas de topo e conferir a aridade.
2. Ligar os parâmetros, como na tabela.
3. Copiar as fichas do corpo e substituir os parâmetros. Só fichas
   identificador são substituídas: strings e comentários são fichas inteiras e
   ficam intactos, e o identificador logo depois de `.` ou `->` nunca é
   substituído.
4. Renomear os locais declarados no corpo para `keel__<local><N>`, de forma
   igual em todo o corpo, com o contador `N` das demais construções (backend
   §2.3). Duas chamadas ao mesmo molde no mesmo bloco não colidem. O local `st`
   do `from_stack` dá `keel__st0`, como o golden 001 já tem.
5. Marcar cada ficha com a **origem**: `corpo` (vem do molde) ou `argumento`
   (vem do chamador). Fichas de corpo resolvem seus nomes no módulo do molde, e
   fichas de argumento resolvem no chamador. Assim o molde funciona mesmo que o
   chamador tenha importado o módulo com outro apelido, ou não tenha importado
   o que o corpo chama.
6. Reescrever o `return`: as fichas do corpo antes dele formam o **prelúdio**, e
   a expressão do `return` substitui a chamada.
7. Inserir o prelúdio no início do statement que contém a chamada, no mesmo
   bloco, respeitando a regra de posição abaixo.
8. Analisar as fichas geradas no ponto de uso, como código keel comum. Os locais
   viram símbolos (um `array` local vira símbolo `array`), os papéis registram
   procedência, e uma chamada a outro molde se expande recursivamente.

`greet(30, name);` dentro de `fn` fica:

```keel
char *keel__name0 = name;                  // value parameter, bound once
array char keel__buf0[30] = {0};           // N := 30, local renamed
size_t keel__len0 = snprintf(keel__buf0, 30, "%s %s", "Hello", keel__name0);
(void)(keel__len0);                        // statement position: the value is discarded
```

E `if (!arena.from_stack(t, 4096)) return fail;` fica:

```keel
alignas(alignof(max_align_t)) array unsigned char keel__st0[4096];
if (!arena.from_array(t, keel__st0)) return fail;
```

Depois da expansão, o `from_array` tem os papéis `child` e `parent`: o `array`
local é uma raiz local, e a procedência chega a `t`. O escape sai da regra
geral, sem regra própria do `from_stack`.

**O `return` do molde.** Só pode haver um, no fim e no nível de topo do corpo.
Ele não retorna do chamador: é o valor da expansão. Em posição de statement o
valor é descartado (`(void)(E);`). Um molde sem `return` só vale como
statement. O `return`, `break` e `continue` escritos dentro de um fragmento
`keel_code` continuam sendo do chamador, então o expansor reescreve o `return`
do corpo **antes** de substituir os fragmentos.

**Posição da chamada.** Içar o prelúdio antes do statement só é correto se a
chamada é avaliada uma vez, sem condição:

- **Prelúdio só de declarações de `array` sem inicializador:** não executa
  nada, e pode subir em qualquer posição de um bloco, inclusive na condição de
  um `if`, como no golden 001.
- **Prelúdio executável:** só em statement de expressão, inicializador de
  declaração, operando de `return` e condição de `if` ou `switch`. Não vale no
  operando direito de `&&`, `||` ou `?:`, na condição ou no incremento de um
  laço.
- **Sub-statement sem chaves** (`if (c) ok = arena.from_stack(a, N);`): o
  prelúdio exigiria um bloco sintético, e o `array` morreria no fim dele,
  enquanto a arena guarda um ponteiro para ele. Se o prelúdio declara algo, é
  diagnóstico (`mold-position`).
- Fora de corpo de função (inicializador de arquivo) não há onde inserir o
  prelúdio: só vale molde sem prelúdio.

**Limites da expansão.**

- **Ciclo.** Só entram na cadeia as chamadas vindas de fichas de **corpo**. `f`
  chamando `g` e `g` chamando `f` é `circular-mold`, detectável na declaração.
  Uma chamada escrita dentro de um **argumento** (`f(f(x))`) não conta: é
  finita, escrita por quem chama.
- **Profundidade.** Um limite fixo de aninhamento de fichas de corpo
  (`mold-depth`).
- **Tamanho.** Um limite do total de fichas que uma chamada gera (`mold-size`),
  porque `f` chamando `g` duas vezes, e `g` chamando `h` duas vezes, cresce
  exponencialmente sem ser cíclico.

**Diagnósticos.** Os erros apontam para a chamada, com o traço "na expansão de
`m.f`", e o `#line` do código expandido leva ao ponto de uso (backend §6).
Propostas: `circular-mold`, `mold-depth`, `mold-size`, `mold-position`,
`mold-return` (mais de um `return`, `return` fora do fim, ou valor pedido de um
molde sem `return`) e `mold-argument` (papel com argumento que não é
identificador, ou tipo de argumento que não confere). O `nonconstant-dim`
existente cobre o `dim`, e o `nonconstant-arena-stack` sai com o `from_stack`
especial.

**Consistência com a rationale.** A rationale ("Macros e sintaxe de keel") diz
que keel evita colagem de fichas, avaliação duplicada de argumentos e outras
técnicas de macro. O molde respeita isso: não há `##` nem `#`, o valor comum é
avaliado uma vez, e o nome local e a resolução são higiênicos. O que pode
duplicar código é o parâmetro `keel_code` usado mais de uma vez, e isso deve ser
dito na documentação.

**Fragmentos e a forma com bloco.** O argumento de expressão de um parâmetro
`keel_code` cabe na posição normal de argumento (`f(a, x + 1)`). Um fragmento
de statements pede a forma fixa `f(a) { bloco }`, que ainda está aberta na
seção seguinte (`construct`). Ela vem depois do molde de expressão, porque é
onde o `construct` completo começa; o molde com fragmento é o mecanismo que o
`construct` usaria para `foreach`, `walk` e `apply`.

Ordem sugerida: papéis de procedência, depois o molde com prelúdio de expressão
(o `from_stack`), depois o fragmento `keel_code` e a forma com bloco. Decidido em
2026-09-30: o molde vem **antes do M5**, e o corpo executável em posição de
expressão é **recusado** (sem função auxiliar).

Abertos:

- **Grafia** da marca `keel_code` como qualificador da função e como tipo de
  parâmetro (a mesma palavra nos dois usos).
- **Categoria do fragmento** (expressão ou statements), inferida pela posição de
  uso no corpo, com erro se houver usos conflitantes, ou escrita.
- **Nome dos locais.** `keel__<local><N>` reproduz o `keel__st0` do golden 001,
  mas divide o espaço de nomes com as construções que já usam `keel__c<N>`,
  `keel__n<N>`, `keel__f<N>`, `keel__l<N>`, `keel__e<N>`, `keel__m<N>` e
  `keel__rv<N>` (backend §2.3): um local de molde com um desses nomes colide.
  A proposta é diagnosticar o local de molde com nome reservado
  (`mold-reserved-name`) e redistribuir o `st` da tabela, hoje só do
  `from_stack`, para "local `st` de molde". Falta conferir o escopo do
  contador `N` (por função, ou por unidade).
- **`from_array` sobre `unsigned char`.** O `from_stack` emite `unsigned char`
  de propósito (armazenamento de tipo-caractere, backend §5.4, item 2), e o
  `from_array` exige `array u8` (`arena-from-array-not-u8`). Como molde, o
  `from_stack` chamaria `from_array` sobre `array unsigned char`. Ou o
  `from_array` aceita tipo-caractere, ou o molde usa um verbo interno sem essa
  checagem; hoje o tratamento especial do parser contorna isso.
- **Ordem entre prelúdios** de vários moldes no mesmo statement: proposta,
  da esquerda para a direita, na ordem textual.

### Construções definidas por módulo (`construct`)

**Nota (2026-09-30).** Com o molde (`keel_code`, acima), a **expansão** que esta
seção propunha já existe. O que o molde não cobre é a **forma** de chamada:
`foreach (T x, size_t i : c) { corpo }` não tem a forma `m.f(args)`, então
`foreach` e `walk` continuam no núcleo, e já funcionam na v0. Esta seção fica
como extensão futura do molde: a forma `f(args) { bloco }` para laços definidos
pelo usuário. Não é um recurso separado do molde.

Hoje `foreach`, `walk`, `parallel`, `match` e `else` são do núcleo, e
cada uma tem os seus diagnósticos escritos à mão. A ideia: um módulo declara a
construção como um molde de substituição de código, com parâmetros que exigem
protocolo, e a construção passa a ser um símbolo conhecido como qualquer outro.
Exportada por um módulo e importada, ela faz a ilha (spec §2.3): o parser
reconhece o símbolo, e o uso é uma ilha que se expande pelo molde.

Um esboço, com o mesmo espírito do protocolo nominal (§2 acima):

```keel
construct foreach(T x, size_t i : Indexable c) { body } {
    size_t n = length(c);
    for (size_t i = 0; i < n; i++) {
        T x = get(c, i);
        body
    }
}
```

`Indexable c` é o mesmo mecanismo do `bound` do protocolo nominal: o parâmetro
da construção exige o protocolo, do mesmo modo que o parâmetro de tipo do módulo
genérico. Não é preciso nomear o módulo do contêiner (`M.length(c)`): o verbo
sem qualificador é resolvido pelo tipo de `c` (spec §4.4, passo 1). O módulo só
faria falta para nomear um tipo dele, como o `cursor` de `walk`.

**Direção.** Se existem genéricos e metadados, uma metaprogramação simples pode
deixar a linguagem menor: o núcleo passa a entender `module`, `modifier`,
`construct` e `protocol`, com seus acessórios (`import`, `dim`, `type`, `tags`),
e o resto sai como biblioteca. A lista do que o núcleo conhece pelo nome (spec
§5.1) encolhe na mesma direção da arena. Isto revisa a recusa de "gerar keel" da
§4 abaixo: lá o que se recusou foi gerar módulos a partir de reflexão sobre
campos; aqui o molde só substitui texto, e a resolução dos verbos vem depois da
expansão, sobre os tipos dos argumentos.

O que substituição simples não cobre:

- **Avaliação única e higiene.** `foreach` obtém contêiner e comprimento na
  entrada. O molde precisa de temporários que o usuário não vê, e de garantir
  que `c` é avaliado uma vez, senão `foreach (x : f())` chama `f` a cada volta.
- **Forma do binder.** O binder por valor ou por endereço escolhe `get` ou
  `ptr`. No molde, são duas formas casadas por padrão sintático (`T x` contra
  `T *x`), ou uma condicional. É casamento de sintaxe, e não substituição.
- **`parallel`.** O corpo é extraído para uma função de worker, com gerente, e
  `return`, `break` e `worker-exit` mudam de sentido.
- **`match` e `else`.** Trazem verificação de exaustividade, `default` com
  `KEEL_CHECK` e fluxo por `goto`. É análise.

O molde cobriria os laços, `foreach` e `walk`. As outras três seguem no núcleo,
ou pedem um segundo mecanismo, além da substituição.

É também o caminho de volta do `apply`, retirado da v0 em 2026-09-29
(rationale, "Acesso e travessia"): como molde de biblioteca, escrito sobre
`foreach` e importado de um módulo, ele não reserva palavra no mar de C, e a
assinatura da função aplicada (com ou sem índice, contexto como captura) pode
ser decidida junto com o que lhe dá sentido, como uma função anônima.

Abertos:

- **Forma sintática fixa.** O parser precisa reconhecer o uso antes de saber o
  que ele faz: algo como `palavra ( binders : expressões ) { bloco }`, com o
  molde parametrizando o que está dentro.
- **Diagnósticos.** Devem sair do protocolo (o parâmetro não cumpre o
  protocolo), e um molde não deveria poder emitir diagnóstico livre. Os de hoje
  do `foreach` e do `walk` passariam a ser os da conformidade.
- **Mapeamento de linhas.** O código expandido aponta para o uso da construção,
  e não para o molde (backend §6).
- **Ordem de implementação.** O molde primeiro (antes do M5), e o protocolo
  nominal depois do M5; a forma com bloco vem por último, e só se fizer falta.
- **Tipo do elemento.** Hoje o usuário escreve o tipo do binder. Inferi-lo pelo
  protocolo exigiria tipo associado (o `Item` do Rust), que é outro recurso, e
  fica fora enquanto o binder for tipado por quem escreve.
- **Custo no núcleo.** Substituir o que hoje é código de tradução por um
  expansor é trabalho grande, e a v0 já depende das construções como estão.

---

## 3. Leituras e digressões

Registradas para não se perderem; nenhuma pede ação.

**keel como linguagem DOD.** `extent`, `buffer`/`slice`,
`foreach`/`walk`/`parallel` com `partition`, e `tags`/`match` formam,
na prática, uma linguagem orientada a projeto de dados sobre C. Cogitou-se, como
digressão, o nome **cdod** (C + DOD). Mudar o nome toca licença, documentos,
repositório e ferramentas: é decisão à parte, não de passagem.

**Leitura ECS.**

| ECS | keel |
| --- | --- |
| entidade | índice num `extent`/`buffer`/`slice` |
| componente | coluna de `extent struct` |
| travessia | `foreach`/`walk`/`parallel` sobre `slice` |
| presença de componente | `bitbuffer(1)` (§1) |
| query / filtro / redução | sem módulo; `foreach` cobre o caso simples |

Índice geracional não é lacuna: pressupõe alocador com free-list e reuso de
slot, e a arena não gerencia elemento individual — a invalidação é lógica, e a
memória se libera com a arena inteira.

**Vocabulário de domínio.** Os nomes da base são genéricos de propósito;
`import keel.buffer as component types;` já veste o vocabulário que o programa
quiser, sem mecanismo novo.

---

## 4. Recusadas

| Ideia | Motivo |
| --- | --- |
| `keel/basetypes.k` — modificadores num módulo, verbos em outro, para evitar o ciclo de headers | O modificador deixaria de morar no módulo dos seus verbos, e a resolução por tipo (spec §4.4), a instanciação e os protocolos supõem que mora. O corte `.type.h` + `.h` (backend §4.3.2) resolve o ciclo sem mexer na linguagem. |
| `.proto.h` — terceiro header, só de protótipos | A ordem das seções do `.h` já põe todo protótipo antes de todo corpo; o arquivo não comprava nada (rationale, "Dois headers, tipo e uso"). |
| `soa` com verbos sintetizados (`keel.soa_from`, `keel.push`, `keel.slice_of`…) | O `soa struct` é do usuário, `len`/`cap` inclusive; verbos que não cabem em `module`/`modifier` comum seriam código escondido. Ficou a forma mínima, hoje `extent` (spec §4.11), com `slice.from` como ponte. |
| Gerar, na declaração de um `soa struct`, um módulo keel com os verbos, nomeado pela tag | Seria gerar keel, não C: metaprogramação, um nível acima da linguagem. Os verbos são keel comum que o programa pode escrever num módulo próprio; escrevê-los uma vez para qualquer `soa` exigiria reflexão sobre os campos, que o parâmetro de tipo opaco exclui. Se um dia existir, é ferramenta externa que gera `.k`, como o `transform`. |
| `soa` heterogêneo por inversão de declarador de um struct existente | Inverter declarador C em geral é frágil (array, ponteiro a função, bitfield); a marca `array` na própria declaração resolve sem inversão. |
| Projeção parcial de `T` (subconjunto de campos como tipo novo) | Exigiria keel conhecer os campos de `T` para gerar um tipo menor — a mesma recusa do `soa` heterogêneo genérico. |
| Protocolo sobre `T` no corpo do genérico, na v0 | Exigiria dizer onde mora o verbo sem import; é o protocolo nominal (§2), adiado. A v0 fixou `T` opaco. |
