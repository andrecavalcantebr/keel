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

### Protocolos de alocação e de hierarquia: Alocável e Hierarquizável

Hoje o núcleo trata a arena pelo nome: registra a procedência (spec §5.2, regra
10) e emite `child-arena-after-reset` só para os verbos de `keel.arena`. Dois
protocolos tirariam a arena da lista de pontos que o núcleo conhece pelo nome, e
abririam a porta para outros alocadores (um pool, um `slab`, uma pilha sobre
arena). São independentes: um módulo pode querer ser hierarquizável sem alocar.

| Protocolo | Verbos | Papel |
| --- | --- | --- |
| Alocável | `alloc(a, n, sz, al)` e `reset(a)`; opcionais `mark` e `restore` | entrega memória em lote e a recolhe em lote |
| Hierarquizável | `from_parent(filho, pai, …)` | uma região feita de outra: invalidar o pai invalida as filhas |

Um verbo pode estar nos dois protocolos: `reset` é o recolhimento em Alocável, e
é o que se propaga às filhas em Hierarquizável. O papel de cada verbo vem da
pertença ao conjunto, e não de uma marca nele: um módulo que só tem `reset`,
sem `alloc`, não é alocador, e o `reset` dele não conta. A arena é os dois
protocolos juntos; um pool sem filhas é só Alocável; uma sessão com sessões
filhas, sem `alloc`, é só Hierarquizável.

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
  que é o `bound` do protocolo nominal (§2 acima).
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
| Alocável | `alloc(a, n, sz, al)` e `reset(a)`; opcionais `mark` e `restore` | reservar armazenamento e invalidar alocações em lote |
| Hierarquizável | `from_parent(child, parent, …)` e `reset(region)`; opcional `restore` | estabelecer dependência entre regiões e invalidar derivadas |

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
   pelo `parent`. Um verbo inline que declara `array u8 tmp[N]` e chama
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

**Protocolos definidos pelos papéis.** Hierarquizável é o módulo com um verbo
`child T` + `parent T` do mesmo tipo e um `invalidates T`. Alocável é o módulo
com um verbo de retorno `child` cujo pai é `T` e um `invalidates T`. O núcleo
deixa de casar `alloc` e `reset` pelo nome. Isso substitui, nesta proposta, a
definição por lista de verbos da tabela dos protocolos.

**Efeitos sobre o catálogo.** `arena-from-array-not-u8` vira a checagem
comum do tipo do parâmetro `array u8`. `buffer.clone(parent arena *a, …)`
declara o papel na assinatura, sem depender do `bound`; generalizar o tipo de
`a` segue dependendo dele. Alias, struct, retorno indireto e efeito
interprocedural continuam fora do alcance.

**`parent` único, ou um papel por espécie.** O `parent` reúne dois fatos: o
armazenamento (a localidade que alimenta o escape) e a validade (a aresta que
alimenta a invalidação). Para a arena, os dois coincidem. Divergem quando há
dependência sem memória emprestada, como uma sessão filha sem `alloc`, e
quando há memória emprestada sem invalidação, como um `slice` sobre `static`.
Ficou o `parent` único: o primeiro caso é hipotético hoje, e um papel só de
validade (nome provisório) entra depois sem quebrar assinaturas, com `parent`
continuando a valer pelos dois.

Abertos:

- **`from_stack`.** Como verbo inline, na seção seguinte.
- **Palavras contextuais.** `child`, `parent` e `invalidates` valem só na lista
  de parâmetros de um verbo, mas `parent` e `child` são identificadores comuns em
  C de árvore e lista. A spec dá erro para `#define` de palavra contextual
  (`define-over-keel-name`, §2.5): é preciso decidir se as palavras de papel
  entram nessa regra, e dizer que o programa perde esses nomes de macro se
  entrarem. Conferir também contra o mar de C (spec §1.4).
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

A lista seria ordenada, pai primeiro e filha depois, separando o que cada lado
guarda. Uma leitura, a confirmar:

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

Abertos: a sintaxe da lista de campos de controle e se a leitura acima é a
pretendida; se os papéis devem citá-los; a política de `restore` em execução.

#### Verbos inline e o marcador `keel_code` (2026-09-30)

**Status: proposta para discussão, não normativa.** Nasce da pergunta de como
a biblioteca escreve o que hoje o cgen faz à mão, como o vetor `keel__st<N>`
do `from_stack` (backend §5.4), sem que o programador recorra a macros. A
spec não expande macros e exige que as construções existam antes da expansão
(§1.2): uma macro parametrizada com `arena.alloc(...)` dentro **não é
traduzida**, e o seu resultado escapa da análise. Restringir a injeção a
declarações de `array` empurraria as pessoas para as macros.

A ideia é injetar **código keel**, e não texto C: o corpo é analisado como keel
na declaração do módulo, com símbolos e ilhas reconhecidos, e substituído
estaticamente no ponto de uso. É o mesmo espírito de `type T`, que injeta um
tipo, e de `array`, marcador do núcleo.

**Verbo inline.** Mantém a chamada `m.f(args)` e a resolução normal (§4.4):

```keel
pub inline bool from_stack(child arena *a, keel_const size_t N) {
    array u8 tmp[N];               // keel `array`, not pasted C
    return from_array(a, tmp);
}
```

- O corpo é keel. `keel_const size_t N` é um parâmetro de valor conhecido na
  tradução, como `type T` é um de tipo. A grafia é provisória. Não pode ser o
  `constexpr` de C23, que só vale para objetos e é erro em parâmetro de função.
  A alternativa é reaproveitar o contrato do `dim` (literal decimal, ou
  `constexpr` de inicializador decimal conhecido; a spec diz que keel não
  calcula expressões, §4.2 e §4.3).
- A injeção tem a forma **prelúdio + expressão**: as declarações sobem para o
  escopo do chamador, antes do statement que contém a chamada, e a expressão
  substitui a chamada.
- **Posição de uso.** Prelúdio só de declarações de `array` sem inicializador
  sobe em qualquer posição, porque não executa nada. Prelúdio com código
  (`size_t n = length(c)`) quebraria a ordem de avaliação em `a && f()`:
  proponho valer só em posição de statement, com diagnóstico em posição de
  expressão.
- **Procedência.** O `array` injetado é uma raiz local pelo núcleo, e o
  `parent` do `from_array` a transmite: o escape sai da regra geral, sem regra
  própria do `from_stack`.
- **Recursão.** Inline chamando inline precisa ser acíclico, como o
  `circular-generic`.
- Os diagnósticos do código expandido apontam para o ponto de uso (backend §6).
  A emissão tem de reproduzir a do golden, como `keel__st0` hoje.

**Marcador `keel_code`.** É um parâmetro cujo argumento é um **fragmento keel**
(um bloco ou uma expressão), no molde de `type T` e de `array`: o terceiro
marcador de parâmetro. O nome diz que **não é código C**, e o prefixo `keel_`
já é reservado (backend §2.3, item 3): o programa não declara identificadores com
ele, e `#define keel_code` já é `define-over-keel-name`, então o marcador não
colide com nome de usuário nem de macro. Só faz falta na forma com bloco
(`foreach`, `walk`, `apply`), que pede a sintaxe fixa
`palavra ( binders : expressões ) { bloco }` da seção seguinte. Fica para depois
do verbo inline, porque é onde o `construct` completo começa.

Ordem sugerida: papéis de procedência, depois o verbo inline, depois o
marcador `keel_code` e a forma com bloco. Enquanto isso, o `from_stack`
continua como está na v0.

Abertos: a grafia de `inline` e de `keel_const`; se o prelúdio admite
statements além de declarações (e onde); como o argumento `keel_code` é
delimitado no ponto de uso.

### Construções definidas por módulo (`construct`)

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
- **Ordem de implementação.** Protocolo nominal primeiro, com `bound`; a
  construção é o terceiro consumidor, depois do `bound` de módulo genérico.
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
