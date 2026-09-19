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
- Serve de **lote de tamanho fixo** para gather/scatter sobre um `soa` —
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
  007 já tem um `pilha.stack` do usuário.

### `keel.strbuf` / `keel.string`

Typedefs de conveniência, não modificadores novos:

```keel
typedef buffer char strbuf;
typedef slice const char string;
```

O trabalho real é o conjunto de funções de string — comparação, concatenação,
formatação — que os verbos de `buffer`/`slice` não cobrem. Par memória/visão:
`strbuf` produz `string`.

### `keel.bitbuffer(W)` / `keel.bitslice(W)`

Array compacto de inteiros de `W` bits; `T` é o tipo de interface de
`get`/`set` (qualquer inteiro, na prática sem sinal). `bitbuffer(1)` é o
bitset clássico, sem tipo à parte.

```keel
module keel.bitbuffer dim W type T;
pub modifier bitbuffer byref { u8 *palavras; size_t len, cap; }
// get/set convertem entre o empacotamento de W bits e o T da chamada
```

```keel
bitbuffer(3) u8 grupos;
bitbuffer(1) u8 marcados;   // bitmask
```

Uso já à vista: a tabela de estados de `keel.routine` (spec §5.6) poderia ser
`bitbuffer(1)`, e a presença de componente numa leitura ECS (§3) também.

### `keel.slice.from(T, p, range)`

Extensão de `keel.slice` (spec §5.3): aceitar um `range` no lugar da contagem,
para descrever `[a,b)` sobre um ponteiro cru. Útil com `soa`, quando o
programa já tem o intervalo em mãos.

### Cooperativo: `keel.atomic`, `keel.chan`, `keel.barrier`

Completam o conjunto cooperativo. Com `corot`, `outcome`, `tags`/`match` e
`seq`/`par` já se montam rotinas que pausam e retomam no mesmo ponto, ao estilo
do Duff's device; estes três dão comunicação e sincronização.

- **`keel.atomic type T`** — `modifier atomic byref { _Atomic T v; }`, com
  `init`/`load`/`store`/`swap`. `byref` evita sincronizar com uma cópia.
  Diagnóstico `atomics-indisponivel` sob `__STDC_NO_ATOMICS__`, no molde de
  `formato-estreito-indisponivel`.
- **`keel.chan type T`** — `send`/`recv` como `corot`. Duas variantes:
  cooperativa (índice comum, mesmo fio, sem atomics) e SPSC entre threads
  (`acquire`/`release` nos dois índices). MPMC fica fora. Nome em aberto:
  `chan` (combina com `corot`) ou `channel`.
- **`keel.barrier`** — `arrive(b, total) -> corot`, `ONGOING` até completar;
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
  é invalidação lógica (ou entra numa lista livre de índices), sem `free`;
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
- a relação com `soa`: a árvore flat pode guardar os elos em colunas.

### Protocolo nominal

Os protocolos da spec §5.1 são estruturais: quem declara `begin`/`has_next`/
`next` participa de `walk`, e o contrato está só na documentação. Dar-lhes nome
quase não pede peça nova:

```keel
module Traversable type T;          // só protótipos sobre T: é o contrato

module keel.buffer type T [protocol Traversable];   // o módulo declara que cumpre

module stats type C [protocol Traversable];         // o genérico exige (bound)
pub f64 media(C *c) {
    f64 s = 0; size_t n = 0;
    walk (f64 *x, cursor k : c) { s += *x; n++; }
    return n ? s / n : 0;
}
```

O que se ganha:

1. **Algoritmo genérico escrito pelo usuário sobre um protocolo** — o único
   ganho de expressividade. Hoje o parâmetro de tipo é opaco (spec §4.3,
   `protocolo-sobre-parametro`), então `media` teria de ser escrita uma vez por
   contêiner. Com o bound, o `walk` sobre `C` se resolve na instanciação: o
   bound garante que os verbos existem, e o nome canônico da instância
   (`keel.buffer.buffer f64`) diz em que módulo moram — sem import novo.
2. **Conformidade verificada no implementador**: o cgen confere, em
   `keel.buffer`, que os protótipos do contrato estão lá. Hoje a falta só
   aparece no uso, no código de outra pessoa.

O que não se ganha: bound na construção (`walk([Traversable] ...)`) é
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

**keel como linguagem DOD.** `soa`, `buffer`/`slice`,
`foreach`/`walk`/`apply`/`parallel` com `partition`, e `tags`/`match` formam,
na prática, uma linguagem orientada a dados sobre C. Cogitou-se, como
digressão, o nome **cdod** (C + DOD). Mudar o nome toca licença, documentos,
repositório e ferramentas: é decisão à parte, não de passagem.

**Leitura ECS.**

| ECS | keel |
| --- | --- |
| entidade | índice num `soa`/`buffer`/`slice` |
| componente | campo de `soa struct` |
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
| `soa` com verbos sintetizados (`keel.soa_from`, `keel.push`, `keel.slice_of`…) | O `soa struct` é do usuário, `len`/`cap` inclusive; verbos que não cabem em `module`/`modifier` comum seriam código escondido. Ficou a forma mínima (spec §4.11), com `slice.from` como ponte. |
| Gerar, na declaração de um `soa struct`, um módulo keel com os verbos, nomeado pela tag | Seria gerar keel, não C: metaprogramação, um nível acima da linguagem. Os verbos são keel comum que o programa pode escrever num módulo próprio; escrevê-los uma vez para qualquer `soa` exigiria reflexão sobre os campos, que o parâmetro de tipo opaco exclui. Se um dia existir, é ferramenta externa que gera `.k`, como o `transform`. |
| `soa` heterogêneo por inversão de declarador de um struct existente | Inverter declarador C em geral é frágil (array, ponteiro a função, bitfield); a marca `array` na própria declaração resolve sem inversão. |
| Projeção parcial de `T` (subconjunto de campos como tipo novo) | Exigiria keel conhecer os campos de `T` para gerar um tipo menor — a mesma recusa do `soa` heterogêneo genérico. |
| Protocolo sobre `T` no corpo do genérico, na v0 | Exigiria dizer onde mora o verbo sem import; é o protocolo nominal (§2), adiado. A v0 fixou `T` opaco. |
