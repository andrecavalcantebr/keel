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

Os protocolos da spec §5.1 são estruturais: quem declara `begin`/`has_next`/
`next` participa de `walk`, e o contrato está só na documentação. Dar-lhes nome
quase não pede peça nova:

```keel
module Traversable type T;          // prototypes over T only: that is the contract

module keel.buffer type T protocol Traversable;   // the module declares that it complies

module stats type C bound Traversable;         // the generic requires it (bound)
pub f64 media(C *c) {
    f64 s = 0; size_t n = 0;
    walk (f64 *x, cursor k : c) { s += *x; n++; }
    return n ? s / n : 0;
}
```

O que se ganha:

1. **Algoritmo genérico escrito pelo usuário sobre um protocolo** — o único
   ganho de expressividade. Hoje o parâmetro de tipo é opaco (spec §4.3,
   `protocol-on-parameter`), então `media` teria de ser escrita uma vez por
   contêiner. Com o bound, o `walk` sobre `C` se resolve na instanciação: o
   bound garante que os verbos existem, e o nome canônico da instância
   (`keel.buffer.buffer f64`) diz em que módulo moram — sem import novo.
2. **Conformidade verificada no implementador**: o cgen confere, em
   `keel.buffer`, que os protótipos do contrato estão lá. Hoje a falta só
   aparece no uso, no código de outra pessoa.

O que não se ganha: bound na construção (`walk(Traversable ...)`) é
redundante — a construção já é o protocolo; o bound só tem lugar na linha
`module`. Despacho dinâmico e sobrecarga por protocolo ficam fora.

A coerência — em que módulo mora a implementação, o problema que a *orphan
rule* do Rust resolve — já está dada: os verbos moram no módulo do
modificador.

Motivo de ficar para depois: complexidade no núcleo para um ganho que a base e
as construções (`walk`, `foreach`, `parallel`) não pedem, e que a
documentação cobre enquanto o usuário não escreve algoritmos genéricos.

---

## 3. Leituras e digressões

Registradas para não se perderem; nenhuma pede ação.

**keel como linguagem DOD.** `extent`, `buffer`/`slice`,
`foreach`/`walk`/`apply`/`parallel` com `partition`, e `tags`/`match` formam,
na prática, uma linguagem orientada a projeto de dados sobre C. Cogitou-se, como
digressão, o nome **cdod** (C + DOD). Mudar o nome toca licença, documentos,
repositório e ferramentas: é decisão à parte, não de passagem.

**Leitura ECS.**

| ECS | keel |
| --- | --- |
| entidade | índice num `extent`/`buffer`/`slice` |
| componente | coluna de `extent struct` |
| travessia | `foreach`/`walk`/`apply`/`parallel` sobre `slice` |
| presença de componente | `bitbuffer(1)` (§1) |
| query / filtro / redução | sem módulo; `apply` cobre o caso simples |

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
