# keel — manual (not so quick start)

> **Versão descrita: keel 0.1.1** (`cgen 0.1.1`, tag `v0.1.1`). Se a sua versão
> for outra, desconfie dos detalhes: a spec e o `cgen --cgen-version` valem
> mais que este texto.

Este não é um dos quatro documentos normativos (`keel-spec.md`,
`keel-rationale.md`, `keel-c-backend.md`, `cgen-tool-spec.md`): é um manual
para quem está aprendendo keel, derivado deles. Começa na ideia da linguagem,
passa pelas construções, pelos protocolos e pelos papéis, mostra como compilar
e o que sai, e termina na referência da API da base. Pode ficar defasado em
relação ao repositório; quando o manual e a spec divergirem, vale a spec.
Cada capítulo aponta o trecho dela que o sustenta.

**Como ler.** Os capítulos 1 a 7 formam um percurso, nessa ordem. O capítulo 8
é consulta. Os exemplos marcados *(verificado)* foram compilados com o `cgen`
0.1.1 ao escrever este texto; os trechos de C gerado são saída real, só
reformatada.

| # | Capítulo |
| --- | --- |
| 1 | [A ideia: ilhas num mar de C](#1-a-ideia-ilhas-num-mar-de-c) |
| 2 | [Primeiro programa: compilar e ver o que sai](#2-primeiro-programa-compilar-e-ver-o-que-sai) |
| 3 | [A linguagem em um passeio](#3-a-linguagem-em-um-passeio) |
| 4 | [Protocolos](#4-protocolos) |
| 5 | [Papéis: de onde vem o armazenamento](#5-papéis-de-onde-vem-o-armazenamento) |
| 6 | [Módulos genéricos e instâncias](#6-módulos-genéricos-e-instâncias) |
| 7 | [O `cgen` por dentro: opções, build e diagnósticos](#7-o-cgen-por-dentro-opções-build-e-diagnósticos) |
| 8 | [Referência da API da base](#8-referência-da-api-da-base) |

---

## 1. A ideia: ilhas num mar de C

Um arquivo `.k` é C. Dentro dele há **ilhas**: construções que keel reconhece
pela forma léxica (`module`, `defer`, `foreach`, `match`…) e pelos símbolos
que ela mesma registrou. O mar em volta é copiado byte a byte para o C
gerado.

Três consequências que valem desde o início:

- **keel não analisa as suas expressões C** nem resolve tipos de headers.
  Quem valida tipo é sempre o compilador C. Quando keel não consegue decidir
  algo, recusa em vez de adivinhar.
- **O dialeto de C é assunto seu.** `nullptr`, `typeof`, o que o seu compilador
  aceitar: keel não olha.
- **Macros não participam.** keel roda antes do pré-processador C; o que ela
  precisa reconhecer tem de estar escrito no fonte, não vir de expansão.

A regra que sustenta o resto: **toda construção tem uma tradução para C escrita
e legível**, e o custo dela está documentado. Você nunca precisa adivinhar o
que foi gerado; e este manual mostra o C de cada ideia importante.

> Referências: [README](README.md#por-que-existir) ·
> [spec cap. 1–2](keel-spec.md) ·
> [Rationale: fronteira com C](keel-rationale.md#fronteira-com-c-e-conflitos-léxicos).

---

## 2. Primeiro programa: compilar e ver o que sai

Crie `src/ola.k` *(verificado)*:

```keel
module ola;
import_c <stdio.h>;

pub int main(int argc, char **argv) {
    array char message[] = "Ola, keel!";
    puts(message);
    return 0;
}
```

- `module ola;` abre o módulo. O nome tem de concordar com o caminho do
  arquivo a partir da raiz (`src/net/http.k` declara `module net.http;`).
- `import_c <stdio.h>;` entrega o header ao compilador C. `puts` continua
  sendo C: keel não precisa conhecer a assinatura.
- `array char message[]` é um vetor C comum, marcado com `array` para que keel
  o registre (a dimensão e a verificação de índice dependem disso).
- `pub` exporta; sem `pub` nem `priv`, a declaração é `pub`.

### Compilar

O `cgen` é um driver no molde do `gcc`: transpila e chama o compilador C sobre
o que gerou. Trocar `gcc` por `cgen` numa linha de compilação é a forma
pretendida de uso.

```sh
cgen -I src src/ola.k -o ola    # transpila, compila e liga
./ola                           # Ola, keel!
```

`-I src` diz onde estão as raízes de módulos. Uma função de entrada pública
(`main` do módulo) basta: ao ligar, o `cgen` gera sozinho a unidade com o
`main` do C.

### O que ele escreveu

Por padrão a saída vai para `./gen` (`--dest-dir` muda):

```plain
gen/ola.c            ← o módulo traduzido
gen/ola.h            ← a interface pública (protótipos)
gen/ola.type.h       ← os tipos do módulo
gen/main_ola.c       ← o main do C, que chama ola_main
gen/keel.h           ← o prelúdio: i8…u64, f32, f64, KEEL_CHECK…
gen/keel.type.h
```

E o `ola.c` é isto (sem o cabeçalho e os `#include`):

```c
int ola_main(int argc, char **argv) {
    char message[] = "Ola, keel!";
    puts(message);
    return 0;
}
```

```c
/* main_ola.c */
int main(int argc, char **argv) { return ola_main(argc, argv); }
```

Duas regras de nome para guardar:

- **O símbolo C é o caminho do módulo + o nome**: `pub int main` do módulo
  `ola` é `ola_main`; do módulo `app.stats`, `app_stats_main`. Escrever `.k`
  nunca exige prever o nome final de nada: dentro de keel você escreve `main`.
- **O arquivo gerado leva o nome do símbolo do módulo**, não do caminho:
  `src/net/http.k` produz `gen/net/net_http.c` (e `.h`, `.type.h`).

Para só **ver o C**, sem chamar o compilador:

```sh
cgen --stop-after=gen -I src src/ola.k
```

> Referências: [spec §1–§4.1](keel-spec.md) ·
> [backend (nomes e artefatos)](keel-c-backend.md) ·
> [cgen-tool-spec](cgen-tool-spec.md). O capítulo 7 detalha a invocação.

---

## 3. A linguagem em um passeio

Cada subseção traz o essencial de uma construção e o C que ela gera. As
referências levam ao contrato completo.

### 3.1 Módulos, imports e visibilidade

```keel
module app.stats;
import keel.buffer as buffer types;   // traz o módulo; "types" injeta os nomes de tipo
import keel.slice  as slice;          // traz o módulo; nada é injetado
import_c <stdio.h>;                   // header C
```

- `import M as nome` dá um qualificador curto; chamadas escrevem
  `buffer.push(b, 1)`.
- `types` injeta sem qualificador os **nomes de tipo** do módulo: depois de
  `import keel.buffer as buffer types;` escreve-se `buffer i32 b;` em vez de
  `buffer.buffer i32 b;`.
- Dois módulos com `types` que injetem o mesmo nome (aqui, `cursor` de
  `buffer` e de `slice`) dão `duplicate-injected-name`. A saída é importar um
  deles sem `types` e escrever o tipo qualificado (`slice.slice i32 s`).
- A base inteira exige import explícito. O único implícito é o prelúdio `keel`
  (tipos primitivos e protocolos).
- `pub` e `priv` valem antes de declarações de arquivo. `priv` tira da
  interface (do `.h`) e não muda sozinho o *linkage* C.
- `extern_c { … }` delimita C que atravessa sem tratamento nenhum.

Os tipos primitivos são `i8…i64`, `u8…u64`, `f16`, `f32`, `f64`, `bf16`, com
largura e representação fixas (não são aliases de `int`). `bool`, `char`,
`size_t` e `ptrdiff_t` conservam o contrato do C.

### 3.2 `array`: vetor C com dimensão registrada

```keel
array i16 dados[3] = {1, 2, 3};
dados[1] = 20;                 // índice verificado contra 3 (ver --checks)
```

`array` não é um tipo novo: é o marcador que faz keel registrar a dimensão. Em
`foreach` e em `x[a..b]` isso é o que permite ao tradutor saber o comprimento.

### 3.3 Modificadores: `buffer`, `slice`, `outcome`, `tagged`

Um **modificador** atua sobre o tipo que o segue e define representação e
operações:

```keel
buffer i32 data;          // sequência que cresce até a capacidade, com dono
slice  const char text;   // vista de extensão fixa; não possui nada
outcome i32 r;            // um i32 mais um código de resultado
tagged Cycle void state;  // uma etiqueta de um conjunto declarado
```

`buffer T` e `slice T` são o par memória/visão: um possui o armazenamento e
pode crescer; o outro descreve um trecho e não possui nada. Um modificador
`byref` (como `buffer`) é sempre passado por ponteiro; os demais passam por
valor.

### 3.4 `defer`: limpeza que acompanha o escopo

```keel
pub int first(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return EOF;
    defer fclose(file);       // corre em todas as saídas, em ordem reversa
    return fgetc(file);
}
```

```c
int reading_first(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return EOF;

    { int keel__rv0 = fgetc(file);
    fclose(file);
    return keel__rv0; }
}
```

Não há runtime: o tradutor varre os pontos de saída e injeta o corpo em cada
um. A expressão do `return` é avaliada **antes** da limpeza (por isso o
temporário `keel__rv0`).

### 3.5 Travessia: `foreach`, `walk` e intervalos

```keel
range indices = 0..2;                       // meio-aberto: 0 e 1
foreach (size_t i : indices) { /* … */ }    // um binder: Countable
foreach (i32 x, size_t i : xs) { /* … */ }  // valor + índice: length + get
foreach (i32 *p, size_t i : xs) { *p += 1; }// ponteiro + índice: length + ptr
walk (i32 *p, buffer.cursor c : xs) { /* … */ }  // cursor: begin/has_next/next
```

`foreach` conta de `0` a `length`; `walk` usa um cursor e não pede
comprimento. O C de um `foreach` sobre um `slice` *(verificado)*:

```c
{ keel_slice_i32 keel__c0 = s;
  size_t keel__n0 = keel_slice_i32_length(keel__c0);
  for (size_t i = 0; i < keel__n0; i++) {
      i32 x = keel_slice_i32_get(keel__c0, i);
      total += x; } }
```

Qual contêiner serve em cada forma é assunto do capítulo 4: **protocolos**.
Fatiar usa a mesma ideia: `b[0..3]` chama `as_slice`, e `x[a..]`, `x[..b]` e
`x[..]` completam a forma.

### 3.6 Resultado: `outcome` e a cauda `else`

`outcome T` é um resultado **final**, de dois estados, com um valor associado.
A extração é sempre explícita:

```keel
pub outcome i16 accept(i16 entry) {
    outcome i16 r = {0};
    if (entry < 0) return outcome.fail(r, entry);
    return outcome.win(r, entry);
}

pub i16 get_value(i16 entry) {
    outcome i16 res = accept(entry) else 0;   // falhou? vira sucesso com valor 0
    return outcome.value(res);
}
```

A cauda `else` trata a falha no ponto da declaração: `else <valor>` usa o
valor padrão (exige `Winnable`), e `else return -1;` age sobre a falha (exige
`Failable`). Nunca há conversão automática de `outcome i16`
para `i16`.

### 3.7 Tags e `match` *(verificado)*

```keel
module cycle;
import keel.tagged as tagged types;

pub tags Cycle [WAIT, END];          // conjunto fechado; constantes em caixa alta

pub int run(void) {
    tagged Cycle void state = {0};   // valor etiquetado por Cycle, sem valor associado
    tagged.mark(state, WAIT);        // estado inicial
    int steps = 0;
    bool finished = false;

    while (!finished) {              // o laço é do programa; match não laça
        match (state) {              // exaustividade verificada
            WAIT:
                steps++;
                if (steps == 3) tagged.mark(state, END);
            END:
                finished = true;
        }
    }
    return steps;                    // 3
}
```

`tagged Cycle void` é um valor etiquetado por `Cycle`; com `void` no lugar do
tipo do valor, só a etiqueta existe. `tagged.mark` escreve a etiqueta, e
`match` despacha sobre ela (`tagged` atende `Taggable`, §4). Uma mudança de
etiqueta só é vista no próximo despacho do laço.

O `match` gera um `switch` que só escolhe um rótulo; os corpos dos braços ficam
fora dele, para preservar `break` e `continue` do usuário. Não há passagem
automática de um braço ao seguinte, e um valor fora do conjunto é um erro de
execução (`tag-out-of-range`), ligado por padrão.

### 3.8 Estado cooperativo: `corot`

`corot` é o estado de uma função que se chama em ciclos, com três estados:
andamento, sucesso, falha. O valor que ela produz viaja pelo contexto (um
parâmetro), não pelo retorno:

```keel
priv corot count(i16 *counter, i16 limit) {
    corot r = {0};
    ++*counter;
    if (*counter < limit) return corot.again(r);   // ONGOING
    return corot.win(r);                            // SUCCESS
}
```

`keel.routine` compõe participantes assim (`routine.seq`, `routine.par`, §8).
`outcome` e `corot` **não** são intercambiáveis: um é resultado final, o outro,
progresso.

### 3.9 `parallel`: execução particionada

```keel
parallel sum ALL (size_t w : 0..4; slice.slice i32 part : xs; (totals)) {
    u32 t = 0;
    walk (i32 *p, slice.cursor c : part) { t += (u32)*p; }
    totals[w] = t;
}
if (parallel.ok(sum)) { /* … */ }
```

O bloco chama `partition` do contêiner uma vez por worker (quem divide é o
**verbo**, não uma fórmula embutida) e declara um símbolo de controle (`sum`)
que se consulta depois. Percorrer a parte é escolha do corpo. A política `ALL`
exige que ninguém falhe; `ANY N` aceita `N` vitórias e permite interromper os
demais com `win;`/`fail;` e `parallel.interrupted(nome)`. Captura escalar é
**cópia por worker**: escrever nela é `captured-write`; a saída vai num
contêiner indexado por posição disjunta (`totals[w]`).

O mecanismo (série ou OpenMP) é escolha da invocação (`--parallel-lowering`)
e não muda a semântica.

### 3.10 Um programa de ponta a ponta *(verificado)*

Arena, buffer, fatia e travessia juntos, em `src/app/stats.k`:

```keel
module app.stats;
import keel.arena  as arena  types;
import keel.buffer as buffer types;
import keel.slice  as slice;
import keel.array  as array;

pub int main(int argc, char **argv) {
    array u8 storage[256];
    arena a = {0};
    arena.from_array(a, storage);          // a região vive no vetor local

    i32 *raw = arena.alloc(a, i32, 4);     // 4 × i32 dentro da arena
    buffer i32 b = buffer.from(raw, 4);    // buffer vazio sobre esse espaço
    buffer.push(b, 10);
    buffer.push(b, 20);
    buffer.push(b, 12);

    slice.slice i32 s = b[0..3];           // vista dos 3 primeiros
    i32 total = 0;
    foreach (i32 x, size_t i : s) total += x;
    return total == 42 ? 0 : 1;
}
```

```sh
cgen -I src src/app/stats.k -o stats && ./stats; echo $?    # 0
```

O C gerado para o miolo:

```c
keel_arena a = {0};
keel_arena_from_array(&a, storage, 256);

i32 *raw = (i32 *)keel_arena_alloc2(&a, sizeof(i32), _Alignof(i32), 4);
keel_buffer_i32 b = keel_buffer_i32_from(raw, 4);
keel_buffer_i32_push1(&b, 10);
/* … */
keel_slice_i32 s = keel_buffer_i32_as_slice2(&b, 0, 3);
```

Repare em dois fatos: `buffer` é `byref`, então vai por ponteiro (`&b`); e
`arena.alloc(a, i32, 4)` virou `sizeof`/`_Alignof` na chamada: o tipo escrito
vira dois números e a função de arena é uma só para todos os tipos (§6).

---

## 4. Protocolos

Esta é a ideia que mais distingue keel de um gerador de código com tipos
privilegiados. Uma construção do núcleo não conhece uma lista de tipos
abençoados: **procura verbos, por nome e aridade**. Declare os verbos e o seu
tipo participa.

keel não tem `implements`. Um tipo atende um protocolo quando o módulo que o
declara tem os verbos certos: **conformidade estrutural**, verificada no uso.
Não há registro a fazer, marcação a escrever nem permissão a pedir, e keel não
distingue um módulo da base de um módulo seu ao resolver.

### 4.1 Os protocolos da base

Moram no prelúdio `keel`, visíveis sem import:

| Protocolo | Exige | Usado por |
| --- | --- | --- |
| `Countable` | `first`, `limit` | `foreach` de um binder |
| `IndexPtr` | `length`, `ptr` (produto `child`) | `x[i]`; `foreach` de dois binders, binder por ponteiro |
| `IndexGet` | `length`, `get` | `foreach` de dois binders, binder por valor |
| `Sliceable` | `length`, `as_slice` | `x[a..b]`; `slice.of` |
| `Traversable` | `begin`, `has_next`, `next` | `walk` |
| `Partitionable` | `partition` | `parallel` |
| `Taggable` | `tag` | `match` sobre valor marcado |
| `Failable` | `failed` | `else` sem valor |
| `Winnable` | `win` (com default) | `else` com valor |

### 4.2 Fazendo o seu tipo entrar em `walk` *(verificado: golden 019)*

Uma lista encadeada comum não tem `length` nem `ptr`, e é percorrível do mesmo
jeito: `walk` só pede `begin`, `has_next` e `next`.

```keel
module lst;

pub typedef struct Node { i32 v; struct Node *link; } Node;
pub typedef struct { Node *head; } List;
pub typedef struct { Node *current; } cursor;

pub inline cursor begin(List *l)               { cursor c = { l->head }; return c; }
pub inline bool   has_next(List *l, cursor *c) { return c->current != NULL; }
pub inline i32   *next(List *l, cursor *c) {
    i32 *p = &c->current->v;
    c->current = c->current->link;
    return p;
}

pub i32 sum(List *l) {
    i32 t = 0;
    walk (i32 *p, lst.cursor c : l) { t += *p; }
    return t;
}
```

Nenhum desses verbos vem da base; o protocolo é resolvido pelos símbolos do
módulo. Como não há sobrecarga por tipo C, **um módulo dá um só `begin`** a um
receptor: o verbo pertence ao módulo.

### 4.3 Declarando um protocolo próprio

```keel
pub protocol Countable type C {
    type index;                 // tipo associado
    index first(C *x);
    index limit(C *x);
}

protocol Both [IndexGet, Sliceable];   // composição: a união dos verbos
```

Dentro das chaves, `type nome;` declara um tipo associado e cada
`retorno verbo(parâmetros);` é um verbo exigido, sem corpo. Um protocolo **não
emite C**. Composição significa conjunção.

### 4.4 Funções sobre protocolo

Um parâmetro cujo tipo é um protocolo torna a função genérica sobre ele, como
uma interface de Java, mas resolvida na tradução, sem vtable nem objeto:

```keel
pub inline i32 span(Countable items) { return limit(items) - first(items); }

void inspect(Traversable C items) {
    walk (C.elem *v, C.cursor c : items) { use(*v); }
}
```

- Cada combinação de tipos concretos usada numa chamada gera **uma instância**
  da função, com o tipo concreto na assinatura C (por ponteiro se for
  `byref`).
- O corpo só pode chamar sobre o parâmetro os verbos do protocolo, mesmo que o
  tipo concreto tenha outros.
- `C.nome` (com o identificador entre o protocolo e o parâmetro) é o tipo
  associado ligado ao tipo concreto.
- Um tipo que não atende é `protocol-not-satisfied`.

*Estado do cgen 0.1.1.* O caso coberto pelo golden é uma função
`pub inline` sobre um tipo comum do próprio módulo (caso 065). Nos meus testes,
uma função `pub inline` do programa chamada com um `buffer` ou `slice` da base
não gerou o corpo da instância, e a ligação falhou com símbolo indefinido.
Enquanto isso não estiver coberto, para tipos da base prefira os verbos
diretos (`buffer.push`, `slice.length`…) ou o `foreach` no próprio ponto de
uso. Relato à parte, ainda sem cartão.

> Referências: [spec §4.4, §4.14, §5.1](keel-spec.md#51-protocolos) ·
> [Rationale: contrato nomeado](keel-rationale.md#contrato-nomeado-conformidade-estrutural).

---

## 5. Papéis: de onde vem o armazenamento

Papéis são quatro palavras na assinatura de uma função: `parent`, `child`,
`invalidates` e `consumes`. Dizem ao tradutor **de onde vem o armazenamento** de
um valor e **quais operações o invalidam**. Com isso o `cgen` pega, na
tradução, dois erros clássicos: usar um valor depois que o dono dele foi
resetado, e devolver algo que aponta para a pilha da própria função.

Não geram campos, parâmetros nem código: a chamada sai como uma chamada C
comum. São contextuais e só valem antes do tipo de um parâmetro (`child`
também antes do tipo de retorno); fora disso, `role-position`.

### 5.1 Os quatro papéis

| Papel | Significado |
| --- | --- |
| `parent` | O argumento é a **origem** do armazenamento. |
| `child` | O argumento ou retorno é um **produto** que depende dos `parent` da chamada. |
| `invalidates` | A chamada invalida os produtos do argumento; o argumento continua válido. |
| `consumes` | A chamada invalida o argumento, seus produtos e seus descendentes. |

As assinaturas da arena, na base:

```keel
pub bool from_array(child arena *a, parent array u8 storage[size_t n]);
pub child T *alloc(parent arena *a, type T, size_t count);
pub void reset(invalidates arena *a);
pub void restore(invalidates arena *a, size_t mark);
```

`invalidates` e `consumes` são dois papéis porque descartam coisas
diferentes: resetar uma arena descarta os produtos, mas o descritor segue em
uso; liberar uma entidade torna inválida também a própria entidade. A
distinção é **declarada**, nunca deduzida do nome `reset` ou `free`.

### 5.2 Como a análise raciocina

Cada símbolo conhecido guarda o conjunto dos símbolos de que depende e dois
estados: *origem local* e *inválido*.

1. Uma chamada torna cada produto `child` dependente de todos os argumentos
   `parent`. O produto é o argumento na posição `child`, ou o símbolo que
   recebe o retorno `child` numa declaração ou atribuição simples.
2. Um `array` automático é *origem local*. Dependência e origem local se
   propagam transitivamente.
3. `invalidates` invalida os produtos do argumento e seus descendentes;
   `consumes` invalida também o argumento.
4. Usar um símbolo inválido é `child-region-after-invalidation`; retornar um
   símbolo cuja procedência inclui origem local da função é `region-escape`.

O papel pertence à **posição** na assinatura, não ao símbolo: o mesmo símbolo
pode ser produto de uma chamada e origem de outra.

### 5.3 Os dois erros, de verdade *(verificado)*

```keel
module app.bad;
import keel.arena as arena types;
import keel.array as array;

pub int use_after_reset(void) {
    array u8 storage[64];
    arena a = {0};
    arena.from_array(a, storage);       // a depende de storage (local)
    i32 *p = arena.alloc(a, i32, 4);    // p depende de a
    arena.reset(a);                     // p inválido; a continua válido
    return *p;                          // ← erro
}

pub i32 *escape(void) {
    array u8 storage[64];
    arena a = {0};
    arena.from_array(a, storage);
    i32 *p = arena.alloc(a, i32, 4);
    return p;                           // ← erro
}
```

```plain
src/app/bad.k:11:13: error: `p` is used after `reset` invalidated the storage it comes from [child-region-after-invalidation]
src/app/bad.k:19:12: error: `p` is returned, and its storage comes from an `array` local to the function: it ends with the function [region-escape]
```

### 5.4 Papéis nas construções

As construções valem como a chamada do verbo correspondente, com o contêiner
na posição `parent`:

- `x[a..b]` chama `as_slice`; o `slice` resultante depende de `x`.
- `foreach` de binder **por ponteiro** chama `ptr`; o binder por valor é cópia
  e não gera dependência.
- `walk` usa `begin` e `next`; `parallel`, `partition`.

Por isso uma fatia de um `buffer` clonado numa arena fica inválida quando a
arena reseta, sem que você escreva um papel sequer: os verbos da base já os
declaram (tabela em §8.3).

### 5.5 Papéis e protocolos

Um papel escrito no protótipo de um protocolo é parte do contrato. O
implementador que não escreve papéis **herda** os do protótipo:

```keel
pub cursor begin(List *l);            // herdado: parent l, child retorno
pub i32   *next(List *l, cursor *c);  // herdado: parent l, child retorno
```

Escrever papéis diferentes dos do contrato é `protocol-role-mismatch`.

### 5.6 O que a análise não promete

- É **lexical**: não segue o fluxo do C, chamadas opacas, cópias, campos,
  aliases nem ponteiros crus. Depois de `f = e;`, invalidar `e` não invalida
  `f`. Ausência de diagnóstico não prova validade.
- Atribuição sem papel descarta as dependências: o símbolo fica *sem
  garantia*, o que é diferente de *válido*.
- O resultado da chamada não conta: a relação vale mesmo se a operação falhar.
- keel **não confere o corpo** da função contra os papéis que ela declara. O
  programa responde pela veracidade do contrato.
- Formas cruas (`slice.from(T, p, n)`, `buffer.from(p, cap)`) não têm
  `parent`: a procedência é afirmada pelo programa.

> Referências: [spec §4.12](keel-spec.md) ·
> [Rationale: papéis e núcleo mínimo](keel-rationale.md#papéis-e-núcleo-mínimo).

---

## 6. Módulos genéricos e instâncias

Um módulo pode ter parâmetros na linha `module`: `type T`, `dim N`, `tags E`.
Cada combinação de argumentos usada vira uma **instância**, com os próprios
arquivos e símbolos. É assim que a base é escrita (`keel.buffer type T`).

```keel
module coll type T;

pub constexpr i32 EMPTY = -1;            // não cita T nem stack: emitida UMA vez

pub modifier stack byref { size_t cap, len; T *ptr; }

pub inline size_t length(stack *s) { return s->len; }   // da instância; sai inteira no header

pub bool push(stack *s, T v) {           // pub sem inline: o corpo precisa de dono
    if (s->len == s->cap) return false;
    s->ptr[s->len++] = v;
    return true;
}
```

```keel
module instances;
import coll;
instance coll.stack i32;     // o corpo de push mora AQUI
```

Uso: `stack i32 s; push(s, 7);` com `import coll types;`. O C gerado nomeia a
instância `coll_stack_i32` (`coll_stack_i32.type.h`, `.h`) e `EMPTY` fica
`coll_EMPTY`, sem o argumento.

Três regras que mordem:

- **`instance` decide onde o corpo mora; não gera a instância.** A geração
  segue o **uso**. Sem uma linha `instance` em algum módulo do programa, uma
  função `pub` sem `inline` de um genérico não tem corpo e a ligação falha.
- **Um módulo genérico não compila sozinho** (`generic-source-without-instance`).
  Entra no build por uso ou por `--instance`:

  ```sh
  cgen -c --instance "coll.stack i32" -o coll_stack_i32.o
  ```
- **`type T` de uma função tem duas naturezas, decididas pela declaração.**
  *Seleção*, quando `T` é o `type` da linha `module`: o tipo escrito na
  chamada escolhe a instância (`slice.from(f32, p, n)` →
  `keel_slice_f32_from(p, n)`). *Apagado*, quando `T` não é parâmetro do
  módulo: vira dois `size_t` (tamanho e alinhamento) e a função é uma só no C
  (`arena.alloc(a, i32, n)` → `keel_arena_alloc2(a, sizeof(i32), _Alignof(i32), n)`).

Dentro de um módulo genérico, `T` é opaco: um valor de tipo `T` não participa
de `foreach`, `walk`, `match` nem indexação.

`extent struct nome [dim, cap] { … }` declara uma struct de colunas
(`array T *campo`) com dois inteiros de controle e acesso verificado, para
tabelas com mais de um campo por posição que `buffer` e `slice` não cobrem.

> Referências: [spec §4.3, §4.4, §4.11](keel-spec.md) ·
> [backend §5.15, §5.19](keel-c-backend.md).

---

## 7. O `cgen` por dentro: opções, build e diagnósticos

### 7.1 Formas de invocar

```sh
cgen -I src -c src/main.k -o main.o -O2 -Wall   # compila um módulo
cgen main.o geom.o -o prog                       # sem .k na linha: só liga
cgen -I src ola.k -o ola                         # módulo com entrada: compila e liga
cgen --stop-after=gen -I src --dest-dir gen src/mod.k   # só vê o C
```

- **Um fonte `.k` por invocação.** Fontes `.c`, `.o` e `.a` na mesma linha
  passam direto ao compilador C.
- **`-c` compila uma unidade**: escreve o `.c` do módulo pedido e os headers
  (`.type.h` e `.h`) dele e de tudo o que ele importa, nunca o `.c` de um
  módulo importado. *Importar não é compilar.* Cada módulo tem a sua própria
  invocação.
- **A entrada do programa**: se a invocação liga e o `.k` tem função de
  entrada pública, o `main` do C é gerado sozinho. `--main <módulo>` escolhe a
  entrada explicitamente; o argumento é nome de módulo, não caminho.
- **Raízes de módulo**: `-I <dir>` (repetível, a ordem importa). O `.k` tem de
  estar sob exatamente uma raiz, e o caminho relativo concordar com o `module`.

### 7.2 Opções próprias

Nunca chegam ao compilador C; todo o resto da linha vai a ele, na ordem.

| Opção | Efeito | Padrão |
| --- | --- | --- |
| `-I <dir>` | raiz de módulos (também repassada ao `cc`) | `.` |
| `--dest-dir <dir>` | raiz da saída gerada | `./gen` |
| `--stop-after=<lex\|parse\|gen>` | para após a fase; `gen` não chama o `cc` | não para |
| `--main <módulo>` | gera o `main` do C chamando a entrada do módulo | automático ao ligar |
| `--instance "<M.mod> <args>"` | compila uma instância, no lugar de um `.k` | — |
| `--checks=<on\|off>` | `off` passa `-DKEEL_CHECKS=0`; o C gerado é o mesmo | `on` |
| `--line=<on\|off>` | emite `#line` apontando para o `.k` | `on` |
| `--cc=<programa>` | compilador C a invocar | `cc` |
| `--profile=<auto\|c11\|c23>` | padrão C do gerado; `auto` lê o `-std=` | `auto` |
| `--parallel-lowering=<auto\|serie\|openmp>` | mecanismo de `parallel` | `auto` |
| `--base-dir <dir>` | raiz da base | `<exe>/../lib/base` |
| `--pedantic-names` | teto de nome gerado de 31/63 caracteres | desligado |
| `-f` | reescreve o gerado mesmo idêntico | desligado |
| `--cgen-version`, `--cgen-help` | versão e ajuda do cgen | — |

Lidas sem serem retiradas: `-I`, `-std=` (perfil), `-fopenmp` (lowering) e
`-MMD`/`-MD`/`-MF`/`-MT`/`-MQ` (depfile). Não há arquivo de configuração nem
variável de ambiente: a saída depende só da linha de comando.

### 7.3 O que sai, em detalhe

Para o programa do §3.10 *(verificado)*:

```plain
gen/main_app_stats.c            ← o main do C (só ao ligar, ou com --main)
gen/app/app_stats.c             ← o módulo
gen/app/app_stats.h             ← interface pública
gen/app/app_stats.type.h        ← tipos do módulo
gen/keel.h, gen/keel.type.h     ← prelúdio: i8…u64, KEEL_CHECK, keel_index…
gen/keel/keel_arena.{h,type.h}
gen/keel/keel_buffer_i32.{h,type.h}     ← a instância buffer i32
gen/keel/keel_slice_i32.{h,type.h}      ← a instância slice i32
gen/keel/keel_outcome_*.{h,type.h}      ← instâncias que a base usa por dentro
```

- **`.type.h`** guarda os tipos (`typedef struct keel_slice_i32 { size_t len; i32 *ptr; } …`);
  **`.h`** guarda protótipos e funções `inline`; **`.c`** guarda os corpos.
- **Cada instância tem os seus arquivos**: `keel_buffer_i32`, `keel_slice_i32`.
  É o preço da generalidade sem runtime: um `buffer u8` e um `buffer i32` não
  compartilham código.
- **O código da base é copiado como fonte** para o C gerado do seu projeto.
  Por isso a base tem uma exceção de licença: o seu programa não é arrastado ao
  copyleft por transitividade de transpilação (veja o README).
- **`#line`**: o C aponta para a linha do `.k`. Erros do compilador C e o
  depurador falam em `.k`, e `--line=off` desliga.
- **Idempotência**: os headers são determinísticos e escritos de forma atômica,
  sem tocar arquivo idêntico.

### 7.4 Verificações em tempo de execução

O C gerado tem `KEEL_CHECK(cond, "id")` em índice de `array`, `tag-out-of-range`,
alvo de `routine.par` e afins: imprime `arquivo:linha: keel: id` e aborta.
`--checks=off` desliga **sem mudar o C gerado**, só passa `-DKEEL_CHECKS=0`
ao compilador C; com a verificação desligada a condição não é avaliada.

### 7.5 Em um build

```make
CC      = cgen
KFLAGS  = -I src
OBJS    = $(SRCS:.k=.o)

%.o : %.k
	$(CC) $(KFLAGS) $(CFLAGS) -MMD -c $< -o $@

prog: $(OBJS)
	$(CC) $^ -o $@

-include $(OBJS:.o=.d)
```

O depfile (`-MMD`) lista, numa regra só, o que o compilador C reporta mais
todo `.k` alcançável por `import`: editar um módulo importado refaz quem
depende dele. O `cgen` acrescenta `-I <dest-dir>` à chamada do `cc`.

### 7.6 Diagnósticos e código de saída

O formato é o do gcc, com a posição no `.k` (nunca no gerado) e o nome estável
do diagnóstico entre colchetes:

```plain
src/app/bad.k:11:13: error: `p` is used after `reset` invalidated the storage it comes from [child-region-after-invalidation]
```

`-Wno-<nome>` silencia um `warning` ou `info` da linguagem (`error` não se
desliga); `-Werror` torna `warning` em parada. É assim que se cala o `info`
`injected-names`, que lista o que `types` injetou. `-W…` com nome que não é de
keel passa ao compilador C.

| Código | Situação |
| --- | --- |
| 0 | sucesso, com ou sem `warning` |
| 1 | pelo menos um `error` da linguagem; nada foi escrito |
| 2 | erro da ferramenta (invocação) |
| *n* | o compilador C saiu com *n* ≠ 0 (por sinal *s*, 128+*s*) |

Erros de invocação, todos com código 2 e sem posição: `multiple-sources`,
`generic-source-without-instance`, `invalid-instance`, `source-not-found`,
`source-outside-roots`, `module-not-found`, `missing-entry-point`,
`invalid-option`, `write-failure`.

> Referências: [cgen-tool-spec](cgen-tool-spec.md) ·
> [backend](keel-c-backend.md) · [`tools/cgen/diag-messages.md`](tools/cgen/diag-messages.md).

---

## 8. Referência da API da base

A base são onze módulos: o prelúdio `keel` (tipos primitivos e protocolos) e
dez módulos `keel.*`. Nenhum módulo `keel.*` é importado implicitamente: só o
prelúdio. O contrato completo — regras, erros, casos especiais — está em
[`keel-spec.md` capítulo 5](keel-spec.md#5-protocolos-e-módulos-da-base); esta
referência resume e remete para lá. Para a fonte das funções, veja
[`base/`](base/) ou, condensado,
[`design/keel-llm-context.md`](design/keel-llm-context.md#3-a-api-da-base). O
que vem depois da base (v1, v2+) está, sem valor normativo, em
[`design/possibilidades.md`](design/possibilidades.md).

### 8.1 Convenção de grafia da base

| O que a base declara | Grafia |
| --- | --- |
| tipos, modificadores, verbos | minúscula — `arena`, `slice`, `corot`, `has_next` |
| conjuntos de tags | maiúscula inicial — `Status` |
| constantes de tag e de enum | caixa alta — `SUCCESS`, `ONGOING`, `FAILED` |

### 8.2 `keel.arena` — memória por região

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

### 8.3 `keel.buffer`, `keel.slice`, `keel.range`, `keel.array`

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

#### `buffer`

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

#### `slice`

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `slice.from(T, p, n)` | `slice T` | vista crua dos `n` elementos em `p` |
| `slice.of(x[, a, c \| r])` | `slice T` | vista de qualquer `Sliceable` (`buffer`, `slice`, `array`, ou tipo do programa) |
| `slice.length(s)` | `size_t` | elementos da vista |
| `slice.get(s, i)`, `slice.set(s, i, v)` | `T`, — | lê/escreve o elemento `i` |
| `slice.ptr(s)`, `slice.ptr(s, i)` | `T *` | início, ou endereço do elemento `i` |
| `slice.at(s, i)` | `outcome T` | elemento `i`, ou `NONE` fora do comprimento |
| `slice.clone(a, s)` | `outcome slice T` | cópia em uma `arena` |

#### `range`

| Chamada | Devolve | O que faz |
| --- | --- | --- |
| `range.of(a, b)`, ou `a..b` | `range` | intervalo `[a, b)`; invertido vira vazio |
| `range.first(r)`, `range.limit(r)` | `size_t` | os limites |
| `range.length(r)` | `size_t` | `limit - first` |
| `range.get(r, i)` | `size_t` | `first + i` |

#### `keel.array`

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

### 8.4 `keel.tagged` — valor rotulado por um conjunto declarado

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

### 8.5 `keel.outcome` e `keel.corot` — os dois formatos de resultado

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

#### `outcome`

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

#### `corot`

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

### 8.6 `keel.routine` — composição cooperativa sobre uma tabela

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

### 8.7 `keel.parallel` — o símbolo de controle de um bloco `parallel`

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

#### Papéis dos verbos da base

Os verbos têm os papéis abaixo; a tabela faz parte das assinaturas
([spec §5.3](keel-spec.md#53-keelbuffer-keelslice-keelrange-e-keelarray)).

| Verbo / família | Parâmetro `parent` | Produto `child` |
| --- | --- | --- |
| `buffer.ptr`, `slice.ptr` | receptor | retorno |
| `buffer.as_slice`, `slice.as_slice`, `array.as_slice` | receptor / vetor | retorno |
| `buffer.begin`, `slice.begin` | receptor | cursor |
| `buffer.next`, `slice.next` | receptor | endereço |
| `*.partition` | receptor | parte |
| `slice.of` | origem | vista |
| `buffer.of(v)` sobre `array` | vetor | buffer |
| `buffer.clone`, `slice.clone` | arena de destino | resultado |
| `buffer.push`, `buffer.pop` | receptor | endereço |


---

## Ver também

- [`design/keel-llm-context.md`](design/keel-llm-context.md) — a fonte das
  mesmas funções, mais a gramática completa; feito para alimentar uma IA,
  mas lê-se também.
- [`README.md`](README.md#keel-por-exemplos) — sete pares keel/C comentados.
- [`keel-spec.md` capítulo 5](keel-spec.md#5-protocolos-e-módulos-da-base) —
  o contrato normativo: regras numeradas, catálogo de erros, casos
  especiais, exemplos com o C gerado.
