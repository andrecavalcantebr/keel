# keel — biblioteca padrão

Este não é um dos quatro documentos normativos (`keel-spec.md`,
`keel-rationale.md`, `keel-c-backend.md`, `cgen-tool-spec.md`): é um guia de
consulta, derivado deles, para quem está aprendendo a API da base. O
contrato completo — regras, erros, casos especiais — está em
[`keel-spec.md` capítulo 5](keel-spec.md#5-protocolos-e-módulos-da-base); este
arquivo resume e remete para lá. Para a fonte das funções (o que cada uma faz
por dentro), veja [`base/`](base/) ou, condensado, [`design/keel-llm-context.md`](design/keel-llm-context.md#3-a-api-da-base).

A base são onze módulos: o prelúdio `keel` (tipos primitivos e protocolos) e
dez módulos `keel.*`. Nenhum módulo `keel.*` é importado implicitamente — só
o prelúdio é. O que vem depois da base (v1, v2+) está, sem valor normativo,
em [`design/possibilidades.md`](design/possibilidades.md).

## Protocolos: como um tipo entra numa construção

keel não tem `implements`. Um tipo atende um protocolo quando o módulo que o
declara tem os verbos certos, com o nome e a aridade certos — conformidade
estrutural, verificada no uso. Os protocolos da base vivem no prelúdio
`keel`:

| Protocolo | Exige | Usado por |
| --- | --- | --- |
| `Countable` | `first`, `limit` | `foreach` de um binder sobre objeto |
| `IndexPtr` | `length`, `ptr` (produto `child`) | `x[i]`; `foreach` de dois binders, binder por ponteiro |
| `IndexGet` | `length`, `get` | `foreach` de dois binders, binder por valor |
| `Sliceable` | `length`, `as_slice` | `x[a..b]`; `slice.of` |
| `Traversable` | `begin`, `has_next`, `next` | `walk` |
| `Partitionable` | `partition` | `parallel` |
| `Taggable` | `tag` | `match` sobre valor marcado |
| `Failable` | `failed` | `else` sem `win` |
| `Winnable` | `win` (com default) | `else` com `win` |

Um tipo do programa que declare `begin`/`has_next`/`next` entra em `walk`
sem nenhuma outra declaração — é o que o exemplo 019 do golden (uma lista
encadeada comum) e o exemplo de `lst` da spec mostram. Detalhe completo:
[spec §5.1](keel-spec.md#51-protocolos).

## Convenção de grafia da base

| O que a base declara | Grafia |
| --- | --- |
| tipos, modificadores, verbos | minúscula — `arena`, `slice`, `corot`, `has_next` |
| conjuntos de tags | maiúscula inicial — `Status` |
| constantes de tag e de enum | caixa alta — `SUCCESS`, `ONGOING`, `FAILED` |

## `keel.arena` — memória por região

Um bloco linear: `alloc` avança um topo alinhado; `mark`/`restore` voltam a
um ponto; `reset` esvazia tudo. Não há `free` individual — a invalidação é
por região inteira.

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `arena.from_array(a, v)` | `bool` | usa como região o `array u8` `v` |
| `arena.from_parent(a, pai, n)` | `bool` | reserva `n` bytes no pai, usa como região da filha |
| `arena.from_memory(a, p, n)` | `bool` | usa como região os `n` bytes externos em `p` |
| `arena.alloc(a, T, n)` | `T *` | reserva `n` objetos de `T`; `NULL` sem espaço |
| `arena.alloc(a, n, sz, al)` | `void *` | forma crua: `n` objetos de `sz` bytes, alinhados a `al` |
| `arena.mark(a)` | `size_t` | topo atual |
| `arena.restore(a, m)` | — | volta ao topo `m` |
| `arena.reset(a)` | — | volta ao topo zero |
| `arena.length(a)` | `size_t` | bytes ocupados |
| `arena.capacity(a)` | `size_t` | capacidade total |

Essenciais: toda `arena` sem inicializador nasce `{0}` (vazia, capacidade
zero); `reset`/`restore` não devolvem memória ao sistema nem limpam
objetos; resetar o pai invalida as filhas e tudo que delas deriva,
transitivamente; `arena` não sincroniza entre threads — recorte uma arena
por worker antes de um `parallel`. Contrato completo, erros e casos
especiais: [spec §5.2](keel-spec.md#52-keelarena).

## `keel.buffer`, `keel.slice`, `keel.range`, `keel.array`

Quatro módulos para a mesma família de problema — sequência, trecho,
intervalo, vetor cru — com os mesmos protocolos de acesso, cursor e
partição.

- **`buffer T`** — sequência de comprimento variável, com dono do
  armazenamento (`byref`: sempre por ponteiro).
- **`slice T`** — trecho de extensão fixa, vista sobre armazenamento de
  outra coisa; passa por valor (não é `byref`).
- **`range`** — intervalo meio-aberto de `size_t`; sem `ptr` nem cursor,
  porque não guarda elemento.
- **`keel.array`** — a família de verbos sobre um `array T` unidimensional
  comum (não é tipo nem modificador: a instância é escolhida pelo tipo do
  elemento).

### `buffer`

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `buffer.of(v)` | `buffer T` | sobre o `array` `v`, cheio |
| `buffer.of(p, n)` | `buffer T` | cheio, sobre `n` elementos em `p` |
| `buffer.from(p, cap)` | `buffer T` | vazio, sobre `cap` elementos em `p` |
| `buffer.length(b)`, `buffer.capacity(b)` | `size_t` | em uso, reservado |
| `buffer.get(b, i)`, `buffer.set(b, i, v)` | `T`, — | lê/escreve o elemento `i` |
| `buffer.ptr(b)`, `buffer.ptr(b, i)` | `T *` | início, ou endereço do elemento `i` |
| `buffer.push(b)`, `buffer.push(b, v)` | `T *` | acrescenta; `NULL` se cheio |
| `buffer.pop(b)` | `T *` | retira o último; `NULL` se vazio |
| `buffer.clear(b)` | — | comprimento zero, mesma capacidade |
| `buffer.at(b, i)` | `outcome T` | elemento `i`, ou `NONE` fora do comprimento, em toda build |
| `buffer.as_slice(b[, a, c \| r])` | `slice T` | vista do comprimento atual, de `[a,c)`, ou dos limites de um `range` |
| `buffer.clone(a, b)` | `outcome buffer T` | cópia em uma `arena` |

### `slice`

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `slice.from(T, p, n)` | `slice T` | vista crua dos `n` elementos em `p` |
| `slice.of(x[, a, c \| r])` | `slice T` | vista de qualquer `Sliceable` (`buffer`, `slice`, `array`, ou tipo do programa) |
| `slice.length(s)` | `size_t` | elementos da vista |
| `slice.get(s, i)`, `slice.set(s, i, v)` | `T`, — | lê/escreve o elemento `i` |
| `slice.ptr(s)`, `slice.ptr(s, i)` | `T *` | início, ou endereço do elemento `i` |
| `slice.at(s, i)` | `outcome T` | elemento `i`, ou `NONE` fora do comprimento |
| `slice.clone(a, s)` | `outcome slice T` | cópia em uma `arena` |

### `range`

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `range.of(a, b)`, ou `a..b` | `range` | intervalo `[a, b)`; invertido vira vazio |
| `range.first(r)`, `range.limit(r)` | `size_t` | os limites |
| `range.length(r)` | `size_t` | `limit - first` |
| `range.get(r, i)` | `size_t` | `first + i` |

### `keel.array`

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `array.length(v)` | `size_t` | a extensão do binder |
| `array.get(v, i)`, `array.set(v, i, x)` | `T`, — | lê/escreve o elemento `i` |
| `array.ptr(v)`, `array.ptr(v, i)` | `T *` | início, ou endereço do elemento `i` |
| `array.as_slice(v, a, b)` | `slice T` | vista de `[a, b)` |
| `array.at(v, i)` | `outcome T` | elemento `i`, ou `NONE` fora da extensão |

`begin`, `has_next`, `next` e `partition` existem nos quatro (menos `ptr`e
cursor em `range`), mas o programa não os chama: quem chama é `walk` e
`parallel`. Nenhuma operação de `buffer`/`slice` invalida nada por si — a
invalidação vem de quem é dono do armazenamento (uma `arena`, por exemplo),
e se propaga por procedência (`parent`/`child`): uma fatia de um `buffer`
clonado numa `arena` fica inválida quando essa `arena` reseta. Contrato
completo, papéis de procedência, erros e casos especiais:
[spec §5.3](keel-spec.md#53-keelbuffer-keelslice-keelrange-e-keelarray).

## `keel.tagged` — valor rotulado por um conjunto declarado

```keel
module keel.tagged tags E type T;
pub modifier tagged { i32 tag; T value; }
```

| Chamada | Devolve | O que faz | Com `T` `void` |
| --- | --- | --- | --- |
| `tagged.tag(t)` | `E` | etiqueta corrente | disponível |
| `tagged.value(t)` | `T` | cópia do valor | omitido |
| `tagged.mark(t, E)` | — | escreve a etiqueta, preserva o valor | disponível |
| `tagged.set(t, E, v)` | — | escreve etiqueta e valor | omitido |

Atende `Taggable` (`tag`), o que permite `match` direto sobre um
`tagged`. Com argumento `void`, o campo de valor e os verbos que o
mencionam somem por conta própria — não há forma especial a escrever.
Contrato completo: [spec §5.4](keel-spec.md#54-keeltagged).

## `keel.outcome` e `keel.corot` — os dois formatos de resultado

Dois modificadores para dois problemas diferentes, que por isso **não** são
intercambiáveis:

- **`outcome T`** — resultado **final**, de dois estados (válido/falha), com
  um valor associado de tipo `T`. É o que uma função normal devolve.
- **`corot`** — estado **cooperativo**, de três estados, sem valor
  associado. É o que uma função que se chama em ciclos devolve; o valor que
  ela produz viaja pelo contexto (um parâmetro), não pelo retorno.

| Código | `outcome` | `corot` |
| --- | --- | --- |
| `< 0` | falha ou ausência (`failed`) | sucesso (`ok`) |
| `= 0` | resultado válido (`ok`) | andamento (`ongoing`) |
| `> 0` | falha ou ausência (`failed`) | falha cooperativa (`faulted`) |

### `outcome`

| Chamada | Devolve | O que faz | Com `T` `void` |
| --- | --- | --- | --- |
| `outcome.win(r)` | `outcome T` | código zero, valor preservado | disponível |
| `outcome.win(r, v)` | `outcome T` | código zero, valor `v` | omitido |
| `outcome.fail(r, c)` | `outcome T` | código `c` (≠ 0), valor preservado | disponível |
| `outcome.none(r)` | `outcome T` | código `outcome.NONE`, valor preservado | disponível |
| `outcome.ok(r)`, `outcome.failed(r)` | `bool` | sucesso, falha |
| `outcome.code(r)` | `i32` | o código |
| `outcome.value(r)` | `T` | cópia do valor | omitido |
| `outcome.value(r, v)` | — | escreve só o valor | omitido |

Extração do valor é **sempre explícita** (`outcome.value(r)`); nunca há
conversão automática — é o tema do exemplo 4 do README.

### `corot`

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `corot.win(r)` | `corot` | escreve `SUCCESS` |
| `corot.again(r)` | `corot` | escreve `ONGOING` |
| `corot.fault(r, c)` | `corot` | código de falha `c` (> 0) |
| `corot.ok(r)`, `corot.ongoing(r)`, `corot.faulted(r)` | `bool` | consulta |
| `corot.code(r)` | `i32` | o código |
| `corot.tag(r)` | `Status` | `SUCCESS`/`ONGOING`/`FAILED`, para `match` |

Os cinco produtores (`win`, `fail`, `none`, `again`, `fault`) ajustam o
objeto recebido e devolvem a cópia ajustada — por isso `return corot.win(r);`
funciona numa linha. Contrato completo:
[spec §5.5](keel-spec.md#55-keeloutcome-e-keelcorot).

## `keel.routine` — composição cooperativa sobre uma tabela

```keel
array routine.slot Ctx steps[3] = {
    { .f = prepare, .ctx = &ctx },
    { .f = measure, .ctx = &ctx },
    { .f = write,   .ctx = &ctx },
};
outcome u32 r = routine.par(slice.of(steps), 2);
```

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `routine.seq(s)` | `outcome u32` | executa os slots em sequência; a primeira falha encerra |
| `routine.par(s, alvo)` | `outcome u32` | executa em ciclos, até a política de `alvo` se resolver |
| `routine.mask(s)` | `u64` | bit `i` ligado quando o slot `i` terminou em `SUCCESS` |
| `routine.state(slot)` | `corot` | estado final do slot |
| `routine.code(slot)` | `i32` | código do estado |

Uma **participante** é uma função `corot (*)(C *ctx)`; o que ela produz
viaja pelo `ctx`, não pelo retorno. `par` chama, em cada ciclo, todo slot
ainda `ONGOING`; a política (`alvo`, zero = todos) é julgada **ao fim** do
ciclo, então mais slots podem terminar do que o mínimo pedido naquele ciclo.
Não há threads: `par` é cooperativa, não paralela. Contrato completo:
[spec §5.6](keel-spec.md#56-keelroutine).

## `keel.parallel` — o símbolo de controle de um bloco `parallel`

```keel
parallel update ALL (size_t w : 0..4; slice i32 part : xs) { /* … */ }
if (parallel.failed(update)) handle();
```

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `parallel.interrupted(n)` | `bool` | pedido de interrupção ativo (só definido *dentro* do worker) |
| `parallel.ok(n)` | `bool` | a política foi satisfeita |
| `parallel.failed(n)` | `bool` | algum worker terminou por `fail` |
| `parallel.wins(n)` | `u32` | quantos workers terminaram por `win` |

`n` é o nome que o próprio bloco `parallel` declara — o programa não cria
esse símbolo, só o consulta. `parallel` não percorre o contêiner por si:
chama `partition` uma vez por worker, e o corpo escolhe como percorrer a
parte (tipicamente `walk`). Contrato completo:
[spec §5.7](keel-spec.md#57-keelparallel).

## Ver também

- [`design/keel-llm-context.md`](design/keel-llm-context.md) — a fonte das
  mesmas funções, mais a gramática completa; feito para alimentar uma IA,
  mas lê-se também.
- [`README.md`](README.md#keel-por-exemplos) — sete pares keel/C comentados.
- [`keel-spec.md` capítulo 5](keel-spec.md#5-protocolos-e-módulos-da-base) —
  o contrato normativo: regras numeradas, catálogo de erros, casos
  especiais, exemplos com o C gerado.
