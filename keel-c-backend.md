# keel — Backend C

**Documento normativo.** Especifica como as construções da linguagem (`keel-spec.md`) são materializadas em C23.

Este documento existe porque **a linguagem não é o lowering**. `defer` é definido como cleanup léxico na saída do escopo, em ordem inversa de registro — isso é a linguagem, e não muda. Que a v0 já tenha estado presa a extensão do GCC, e que hoje se resolva varrendo os pontos de saída e injetando o corpo em cada um conforme o escopo, é assunto **deste** arquivo, e nada disso altera a definição do `defer`. Um segundo backend deve as mesmas obrigações; pode pagá-las de outro jeito — **onde a linguagem não tiver nomeado a forma**. O `parallel` é a exceção, e ela está escrita: a linguagem §4.7 nomeia o OpenMP, porque o lowering alternativo por threads da libc precisaria do tipo de cada captura e a invariante da linguagem §1.3 proíbe conhecê-lo (§5.9.1).

A divisão vale nos três documentos, e é um critério só:

> **A linguagem é dona do que depende apenas do fonte. O backend é dono do que depende do alvo. A ferramenta é dona do que depende da invocação** (`cgen-tool-spec.md`).

**Alvo:** C23 normativo, sem extensões de compilador.
**Prefixo reservado:** `keel_`.

---

## 1. Obrigações do backend

Um backend C conforme deve, para cada módulo processado:

1. Produzir, no mínimo, um `.c` e um `.h` (§4.1).
2. Formar os símbolos pela regra de nomes do §2, de modo que a identidade nominal da linguagem sobreviva à tradução.
3. Materializar os tipos primitivos e provar, no próprio C gerado, as garantias que a linguagem dá sobre eles (§3).
4. Emitir `#line` de modo que todo diagnóstico do compilador C caia na linha do `.k` que o usuário escreveu (§6).
5. Gerar código que satisfaça o princípio 2 da linguagem: código que um humano assinaria.

A obrigação 5 tem uma única concessão deliberada, registrada no §6.

---

## 2. Nomes

### 2.1 Do nome canônico ao símbolo C

A linguagem fixa o **nome canônico** de cada tipo no ponto de declaração (`keel-spec.md` §4.3). O backend o materializa trocando `.` por `_`:

| Nome canônico | Símbolo C |
| --- | --- |
| tipo da camada zero (`i32`, `char`, `const char`) | mesma grafia; qualificador prefixa com `_` — `const_char` |
| `M.nome` | `M_nome` |
| `M.mod` aplicado a `A` | `M_mod_<A canônico>` |
| `M.mod` aplicado a `A₁ … A_n` — módulo de vários parâmetros | `M_mod_<A₁>_…_<A_n>`, na ordem da linha `module` |
| `M.mod(D)` aplicado a `A` — modificador com `dim` parametrizado | `M_mod_<D>_<A canônico>` |
| `M.Tipo.CONST` — constante de enum nomeado | `M_Tipo_CONST` |

**A grafia gerada nunca contém `__` nem começa por `_` seguido de maiúscula.** O
C reserva as duas formas à implementação, e a regra de qualificador acima chega
nelas sozinha: `_Atomic u32` daria `_Atomic_u32`, e `keel_buffer__Atomic_u32`
tem `__` no meio. **Qualificador que começa por `_` perde o underscore inicial e
baixa a caixa** — é a única normalização de grafia do mangling, e existe para
essa colisão:

```plain
buffer const char    →  keel_buffer_const_char
buffer _Atomic u32   →  keel_buffer_atomic_u32
```

A perda de fidelidade é nominal e não cria ambiguidade: `atomic` não é grafia
válida de tipo no fonte (§4.2 da linguagem), então nada mais pode produzir esse
componente.

Módulo hierárquico achata o `.`: `net.http` dá `net_http_`. O argumento carrega a própria qualificação, e é isso que faz o nome ser o mesmo em toda parte — a condição para que dois módulos atribuam entre si a mesma instância.

```keel
module geom;
typedef struct { float x, y; } Point;
```

```c
typedef struct { float x, y; } geom_Point;
```

O `keel_` de `buffer` e `slice` **não é prefixo especial**: é o `M_` da terceira linha, com `M` sendo o módulo `keel` da camada zero. A mesma regra serve modificador embutido e modificador escrito pelo usuário.

```plain
buffer i32              →  keel_buffer_i32
buffer geom.Point       →  keel_buffer_geom_Point
buffer slice char       →  keel_buffer_slice_char
coll.stack i32          →  coll_stack_i32
kv.map i32 geom.Point   →  kv_map_i32_geom_Point
```

A representação interna de tipo é uma árvore, e o mangling cai dela — o prefixo aparece só na raiz:

```plain
buffer slice i32  →  buffer( slice( c_type("i32") ) )  →  keel_buffer_slice_i32
```

**Sufixo de aridade.** Uma função que pertence a um modificador pode existir em mais de uma aridade — `ptr(x)` e `ptr(x,i)`, `push(x)` e `push(x,v)`. Como o nome mangled é o mesmo, ele ganha um sufixo:

> A forma que recebe **apenas o contêiner** não leva sufixo; as demais levam **o número de argumentos além dele**.

```plain
ptr(b)          →  keel_buffer_i32_ptr
ptr(b,i)        →  keel_buffer_i32_ptr1
push(b)         →  keel_buffer_i32_push
push(b,v)       →  keel_buffer_i32_push1
ptr(m,i,j)      →  mat_matrix_f32_ptr2
```

**Com `dim`, o sufixo conta índices do call site, não argumentos do C** (linguagem §4.9). É a única exceção à frase acima:

```plain
ptr(t)                →  tens_tensor_2_f32_ptr    /* a base                 */
ptr(t,(size_t[2]){…}) →  tens_tensor_2_f32_ptr2   /* rank cheio: dois índices */
```

O acessor de rank cheio recebe **um** argumento além do contêiner — o vetor —, e ainda assim leva o sufixo `2`, porque o ponto de chamada escreveu dois índices. É o que faz o nome dizer o rank, e o que alinha o caso `dim` com o rank fixo, em que `mat_matrix_f32_ptr2` sai de dois índices escritos por extenso. **Não há acessor parcial** (linguagem §4.9), então não há par a desempatar — o que a exceção compra é legibilidade do símbolo, não unicidade.

**Fora de `dim`, o sufixo é o da regra geral, e é o que serve o rank fixo** (linguagem §4.9): `mat_matrix_f32_ptr1` e `mat_matrix_f32_ptr2` saem de `ptr(m,i)` e `ptr(m,i,j)`, dois acessores escritos por extenso, sem literal composto e sem exceção nenhuma no emissor.

Nada disso é resolução de sobrecarga: a aridade está escrita no call site e contar argumentos é sintático — nenhum tipo de argumento é examinado. É por isso que a regra não reabre o que a regra de fechamento da linguagem §4.9 fecha, e por isso ela vale igual para modificador embutido e do usuário (linguagem §4.9).

Os três espaços de identificador do C são prefixados, porque os três aparecem no `.h` e os três colidem entre módulos:

```keel
module sim;

typedef struct { f32 x, y; } Vec2;
struct No { struct No *prox; };
enum Estado { PARADO, ANDANDO };
enum { MAX = 64 };
```

```c
typedef struct { f32 x, y; } sim_Vec2;
struct sim_No { struct sim_No *prox; };
enum sim_Estado { sim_Estado_PARADO, sim_Estado_ANDANDO };
enum { sim_MAX = 64 };
```

A **constante de enum** é a que mais importa: ela vive no espaço de identificadores comuns e é definida no header. Sem prefixo, dois módulos que declarem `PARADO` não podem ser importados pelo mesmo terceiro.

E ela leva **dois** níveis, não um: o escopo da constante é o enum, não o módulo (linguagem §4.3), de modo que `Estado.PARADO` e `Tarefa.PARADO` do mesmo módulo não se encontram no `.h`. Enum sem nome não tem escopo próprio e fica com o prefixo do módulo, como qualquer outro símbolo.

- Vale só em escopo de arquivo. `enum` em escopo de bloco não aparece em header nenhum e não é tocado.
- Nada de novo é exigido do parser além de **ler o corpo do `enum`** para colher os nomes. A reescrita já existe — é a mesma que troca `Point` por `geom_Point` —, e por isso `ANDANDO = PARADO + 1` sai certo sem tratamento especial: dentro do corpo os dois nomes são nus e os dois são reescritos para o símbolo escopado.
- Dentro de `extern_c` nada disso vale: ali os nomes são do C.

### 2.2 Normalização do argumento

Antes de manglar, a sequência de tokens do argumento é normalizada:

1. Espaçamento colapsado.
2. Nome de `<stdint.h>` reduzido à grafia keel (`int32_t` → `i32`).
3. Qualificadores movidos para antes do tipo, em ordem fixa: `const`, `volatile`, `_Atomic` — e `_Atomic` grafado `atomic` no símbolo, pela regra do §2.1.
4. `M.Nome` → `M_Nome`. **Exceto `keel.X`, que reduz a `X`** — a camada zero não tem prefixo no C gerado, e é isso que faz `buffer keel.i32` e `buffer i32` serem a mesma instância.
5. Argumento que é instância de modificador é normalizado recursivamente.
6. Argumento de `dim` reduzido à forma decimal mínima: zeros à esquerda caem, de modo que `tensor(03)` e `tensor(3)` sejam a **mesma** instância.

```plain
buffer int32_t     →  keel_buffer_i32     /* mesma instância de buffer i32 */
buffer char const  →  keel_buffer_const_char
buffer keel.i32    →  keel_buffer_i32
buffer _Atomic u32 →  keel_buffer_atomic_u32
```

**O prefixo do objeto não entra na normalização.** `const buffer i32` e
`_Atomic buffer i32` são a instância `keel_buffer_i32` num objeto qualificado: o
qualificador sai no declarador do C, como o usuário o escreveu, e nenhuma
instância nova é gerada (linguagem §3.1). Só o qualificador do **argumento**
manga, porque só ele muda o tipo do elemento.

`ref` não entra na mangling: `buffer i32 *ref` e `buffer i32 *` são a mesma instância.

`restrict` não aparece em mangling nenhum: ele não é qualificador de contêiner em keel (linguagem §4.2), e em declarador C comum atravessa verbatim, sem instância a nomear. Já o argumento de `dim` **entra**, e tem que entrar: `tensor(2) f32` e `tensor(3) f32` são tipos diferentes, com structs de tamanhos diferentes. Módulo sem `dim` não tem numeral a carregar — `mat_matrix_f32` (linguagem §4.9).

**O que entra é o valor, e nunca a grafia.** `tensor(3) f16` e `tensor(DIM) f16`, com `DIM` valendo 3, dão o mesmo `tens_tensor_3_f16` — mesmo nome, mesma struct, mesmo header, byte a byte. É o que a linguagem §4.9 exige, e é o que mantém o §7.1 de pé: com a grafia no nome, dois módulos que declarassem `DIM` com valores diferentes pediriam o mesmo arquivo com conteúdos diferentes, e o header de instância deixaria de ser função das entradas. **O backend não resolve o símbolo** — recebe o valor já resolvido pela linguagem e o escreve.

**No C gerado sai a grafia keel, não a de `<stdint.h>`.** A decisão está forçada: o corpo de função é copiado verbatim, então um `i32 x = 5;` escrito pelo usuário chega ao `.c` como `i32` de qualquer forma — o `typedef` do prelúdio é necessário em qualquer cenário. Emitir `int32_t` nas structs geradas criaria duas grafias para o mesmo tipo dentro do mesmo programa. E pelo princípio 3, a mensagem do compilador C deve referir o nome que o usuário escreveu: `expected i32 * but argument is of type f32 *` lê direto contra o fonte.

### 2.3 Namespaces e nomes reservados

| Categoria | Prefixo |
| --- | --- |
| Instância de modificador do módulo `M` e suas funções | `M_` — `keel_` para os da camada zero |
| Tipo, função e variável do usuário no módulo `M` | `M_` |
| Temporários gerados | `keel__t<N>` |
| Rótulos de cleanup | `keel__cl<N>` |
| Structs de captura de `defer` | `keel__c<N>` |
| Vetores de `arena_from_stack` | `keel__st<N>` |
| Tipo da camada zero (`i32`, `f32`, `size_t`, …) | nenhum — mesma grafia no fonte e no C |

Identificadores começando com `keel_` no fonte do usuário são reservados, e usá-los é erro.

### 2.4 Limite de comprimento

**Nome gerado acima de 255 caracteres é erro.**

**Por que existe um teto.** Não é conformidade: é colisão. Onde o linker só
considera os primeiros *N* caracteres, duas instâncias distintas cujos nomes só
diferem depois do corte **colidem em silêncio no link** — sem erro, sem aviso, e
com um dos dois símbolos vencendo. É a pior classe de falha que a identidade
nominal pode produzir, e é ela que o teto existe para transformar em diagnóstico.
Truncar com hash não serve: mataria a legibilidade que a identidade nominal existe
para preservar.

**Por que 255, e não o número do padrão.** O mínimo que o C garante é bem menor —
e menor do que a edição anterior deste documento afirmava:

| | Interno / macro | **Externo** |
| --- | --- | --- |
| C89/C90 | 31 | **6** |
| C99 em diante, incluindo C23 | 63 | **31** |

Praticamente todo nome que este backend gera é **externo** — símbolo `pub` com
prefixo de módulo —, então o número aplicável seria **31**, não 63. Com 31,
`keel_buffer_geom_Point` (22 caracteres) já estaria a uma composição de estourar,
e a álgebra de modificadores do §4.3 da linguagem seria inutilizável na prática.

E o mínimo do padrão não descreve nenhum compilador real. O levantamento está no
Anexo deste documento; o resumo é que **o pior caso prático é 255**, do IAR
Embedded Workbench, e que todo o resto ou garante 255 ou não impõe limite algum.
Adotar o mínimo do padrão seria pagar por um alvo que não existe.

> **Nota de projeto — de onde veio o 63.** Ele estava aqui por um erro de leitura
> do padrão: 63 é a significância de identificador **interno**, e o texto o
> atribuía ao externo. O erro era conservador, então nunca produziu programa
> errado — só proibia nomes que todo compilador aceita. Fica registrado porque a
> correção move um número que outras seções orçavam (linguagem §4.3, §1.7).

**`--pedantic-names` baixa o teto para 63** (ferramenta §4.1), para quem mira alvo
fora do levantamento. Não existe flag para *subir*: 255 já é o topo do que se pode
prometer sem saber qual é o linker.

**O 63 não é o mínimo do padrão, e a flag não promete ser.** O mínimo para nome
externo é 31, e a 31 a álgebra de modificadores do §4.3 da linguagem seria
inutilizável — `keel_buffer_geom_Point` já tem 22. A flag existe para o alvo
cujo linker se conhece mal, não para reproduzir a garantia do padrão, e 63 é o
teto abaixo do qual nenhum toolchain do Anexo foi encontrado.

É esse teto, e nenhuma regra própria, que limita a profundidade de aninhamento de
modificadores — e a 255 ele deixa de ser restrição sentida: `mat_matrix_keel_buffer_geom_Point`
tem 33 caracteres.

---

## 3. Tipos primitivos

A linguagem define os primitivos por **largura e formato**, sem referência a tipo do C. Este backend os materializa assim:

| Definição da linguagem | Materialização |
| --- | --- |
| `i8`…`i64` — inteiro com sinal, complemento de dois, N bits | `int8_t` … `int64_t` |
| `u8`…`u64` — inteiro sem sinal, N bits | `uint8_t` … `uint64_t` |
| `f32` `f64` — IEEE 754 binary32 / binary64 | `float` / `double`, com `static_assert` |
| `f16` — IEEE 754 binary16 | `_Float16`, com guarda de disponibilidade (§3.2) |
| `bf16` — bfloat16 | `__bf16`, com guarda de disponibilidade (§3.2) |
| `bool` `char` | `bool` / `char` |
| `size_t` `ptrdiff_t` `uintptr_t` | idem |

> **Nota de projeto.** A separação entre definição e materialização é deliberada. `f32` **é** binary32; `typedef float f32;` é como *este* backend entrega isso, e o `static_assert` que o acompanha é prova de obrigação **do backend**, não parte da definição do tipo. Um backend cujo alvo já tenha binary32 nativo não deve nada e não emite guarda nenhuma. Sem a separação, trocar de backend obrigaria a reescrever a definição dos primitivos — e definição de primitivo não deveria se mover porque a saída mudou.

### 3.1 Ponto flutuante — a regra

> `fN` mapeia para o tipo por **palavra-chave** quando existe um com o formato certo; `_FloatN` só onde não existe alternativa.

O motivo não é disponibilidade — GCC e Clang suportam `_Float32`/`_Float64`/`_Float128` bem. É que o C os define como **tipos distintos** de `float`/`double`/`long double`, mesmo com representação idêntica. Com inteiros não há esse problema: `int32_t` é `typedef` de um tipo existente, então `int32_t *` e `int *` são compatíveis quando coincidem. Com ponto flutuante não há saída equivalente:

```keel
buffer f64 xs;
cblas_dgemv(..., ptr(xs), ...);      /* biblioteca C espera double * */
```

```c
/* f64 = _Float64 */  error: passing '_Float64 *' to parameter of type 'double *'
/* f64 = double   */  /* compila */
```

Ponteiro é o caso fatal: não há conversão implícita entre `_Float64 *` e `double *`, e todo `ptr(x)` entregue a biblioteca C exigiria cast — exatamente o que um programa que descreve layout com precisão não deveria precisar fazer. Some-se que a promoção de argumento variádico é `float` → `double`: `_Float32` não promove, então `printf("%f", x)` com `f32 = _Float32` é UB silencioso.

A garantia de formato não se perde: é verificada no C gerado, por `static_assert` no prelúdio. Numa plataforma onde `float` não seja binary32 a compilação **para**, com mensagem, em vez de gerar código errado em silêncio. E o ganho teórico de `_Float32` some justamente onde ele seria necessário: numa plataforma em que `float` não é binary32, `_Float32` quase certamente também não existe.

`f16` cai do outro lado da regra, e **entrou assim**: não há palavra-chave para binary16, então ele é `_Float16` e a objeção de compatibilidade de ponteiro não se aplica — não existe `float`-de-16-bits com que confundir. `bf16` é `__bf16` pela mesma razão e com uma a mais: não é IEEE, não é typedef de nada, e nenhum alvo lhe dá palavra-chave. `f128` continua fora: `long double` **não** é binary128 — são 80 bits estendidos no x86-64 e double-double no PowerPC, coincidindo só no AArch64 Linux.

Os dois estreitos custam uma advertência que os largos não custam: **`_Float16` e `__bf16` são tipos distintos de tudo**, então `ptr(xs)` de um `buffer f16` entrega `_Float16 *`, e a biblioteca C que espera `__fp16 *` ou `uint16_t *` exige cast. É o preço que o §3.1 evita para `f64` e não tem como evitar aqui, porque a alternativa — `u16` com reinterpretação na mão — perde a aritmética inteira e o `static_assert` que prova o formato.

O que **não** se faz é condicionar o `typedef` ao alvo:

```c
#ifdef __STDC_IEC_60559_TYPES__
typedef _Float32 f32;      /* NÃO */
#else
typedef float f32;
#endif
```

`f32` teria identidade de tipo diferente conforme a plataforma, e o mesmo fonte compilaria num alvo e falharia noutro por incompatibilidade de ponteiro — pior que qualquer das duas escolhas fixas.

### 3.2 Formatos estreitos — a guarda de disponibilidade

`_Float16` e `__bf16` não existem em todo alvo, e a §3.1 acaba de proibir a
saída óbvia: **`typedef` condicional está fora**, porque faria a identidade do
tipo variar com a plataforma. A regra que sobra é a única que preserva
identidade única:

> O `typedef` é **incondicional**. O que é condicional é a **parada**: num alvo
> sem o formato, a compilação para com mensagem, antes do `typedef`.

```c
/* keel/f16.h — incluído apenas quando o módulo nomeia f16 */
#include "keel/prelude.h"

#if !defined(__FLT16_MANT_DIG__)
#  error "keel: f16 exige _Float16, que este alvo nao oferece"
#endif
typedef _Float16 f16;
static_assert(sizeof(f16) == 2, "keel: f16 exige binary16 de 16 bits neste alvo");
```

```c
/* keel/bf16.h — incluído apenas quando o módulo nomeia bf16 */
#include "keel/prelude.h"

#if !defined(__BFLT16_MANT_DIG__) && !defined(__ARM_BF16_FORMAT_ALTERNATIVE)
#  error "keel: bf16 exige __bf16, que este alvo nao oferece"
#endif
typedef __bf16 bf16;
static_assert(sizeof(bf16) == 2, "keel: bf16 exige 16 bits neste alvo");
```

**Um header por formato, e é por isso que a guarda não incomoda ninguém.** Se os
dois `typedef` morassem no prelúdio (§4.2), todo projeto num alvo sem binary16
deixaria de compilar por um tipo que não usa. E se morassem no mesmo arquivo, um
módulo que só nomeia `f16` pararia pela guarda de `bf16` — que é o caso comum,
porque `_Float16` e `__bf16` não chegam juntos aos alvos. **Cada arquivo é
incluído só quando o módulo nomeia o seu tipo**, e o custo recai exatamente sobre
quem pediu. É a mesma mecânica de `keel/arena.h`, e pela mesma razão.

O diagnóstico **103** é a versão do cgen dessa parada, para o caso em que o alvo é
conhecido na invocação; o `#error` é a rede para quando não é. Os dois dizem a
mesma coisa, e nenhum dos dois deixa passar.

> **Nota de projeto — por que não `u16` com reinterpretação.** A tentação é
> definir `f16` como `u16` e converter na mão, o que compila em todo lugar. Ela
> troca uma falha de compilação visível por duas perdas silenciosas: `+` passa
> a somar padrões de bits, e `buffer f16` deixa de ser distinguível de
> `buffer u16` — dois tipos que a identidade nominal (§2.2) existe para separar.
> Um formato numérico que não soma não é um formato numérico.

---

## 4. Artefatos

### 4.1 Um `.c` e um `.h` por módulo

Cada `.k` produz exatamente um `.c` e, portanto, um `.o` — e **nenhum alvo de compilação além disso**. O `.c` é sempre gerado, ainda que contenha apenas o `#include` do próprio `.h`, o que permite ao build usar uma regra de padrão `%.o: %.c` sem exceção. Não existe alvo sem `.k` correspondente para o build descobrir por glob, manifesto ou arquivo agregador.

A invariante sobrevive aos módulos genéricos porque a instância continua sendo header-only, e porque a única forma de obter definição fora de linha — `instance` — mora num `.k` que o usuário escreveu, com o `.o` e a regra que ele mesmo declarou.

Isso **não** quer dizer que o par seja o único arquivo gerado: as instâncias geram headers próprios. Headers não compilam e não geram objeto, então não são alvos — é justamente por isso que a invariante se sustenta.

```text
modulo.k  →  modulo.h                  interface
                modulo.c                  corpo              → modulo.o
                keel/keel_buffer_f32.h    instância          → (nenhum objeto)
```

**O que vai em cada arquivo:**

O `.h` recebe definições de tipo, protótipos de função, declarações `extern` de variáveis, os `#include` das instâncias usadas na interface e os `import_c`.
O `.c` recebe corpos de função, definições de variáveis e os blocos `extern_c`.

Quem decide para qual dos dois cada declaração vai é `pub`/`priv` (linguagem §4.1). A tabela do posicionamento é deste documento, porque é ela que fala de arquivo:

| Escrita no `.k` | `.h` | `.c` |
| --- | --- | --- |
| função (`pub` implícito) | protótipo | corpo |
| função `pub inline` | `static inline` + corpo | — |
| variável (`pub` implícito) | `extern T var;` | `T var = ...;` |
| tipo (`pub` implícito) | definição | — |
| função `priv` | — | corpo, com `static` |
| variável `priv` | — | definição, com `static` |
| tipo `priv` | — | definição |

O mapeamento nem é monotônico: `pub inline` sai como `static inline` no `.h`. Público no keel virou `static` no C.

Variável pública **nunca** vai para o `.h` como `static`. Isso compila e linka, mas produz uma cópia independente por unidade de tradução: um módulo escreve, outro lê e não enxerga nada, sem erro nem aviso.

`const` não precisa de exceção: `pub const float PI = 3.14f;` sai como `extern const float PI;` no `.h` e a definição no `.c` — símbolo único, sem duplicação.

### 4.2 Headers fixos

Três headers são **fixos, idênticos em todo projeto, não gerados**. Eles são a materialização do módulo `keel` da camada zero neste backend: é o backend C que deve os `typedef` e as guardas, porque é ele que não tem os tipos nativamente.

```c
/* keel/prelude.h — incluído no topo de todo .h de módulo */
#include <stdint.h>
#include <stddef.h>
#include <float.h>
typedef int8_t  i8;   typedef uint8_t  u8;
typedef int16_t i16;  typedef uint16_t u16;
typedef int32_t i32;  typedef uint32_t u32;
typedef int64_t i64;  typedef uint64_t u64;
typedef float   f32;  typedef double   f64;

static_assert(FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128
              && sizeof(f32) == 4, "keel: f32 exige IEEE 754 binary32 neste alvo");
static_assert(FLT_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024
              && sizeof(f64) == 8, "keel: f64 exige IEEE 754 binary64 neste alvo");
```

**Os asserts provam o formato, e não o tamanho.** `sizeof == 4` sozinho não
distingue binary32 de um float de 32 bits que não é IEEE — e o §3.1 promete o
formato, não a largura. Radix, dígitos de mantissa e expoente máximo fixam
binary32 e binary64 sem depender de `__STDC_IEC_559__`, que é opcional e que
vários alvos com IEEE de verdade não definem por causa de exceções e
arredondamento. Este é o **único lugar onde `<stdint.h>` e `<float.h>` aparecem**:
todo o resto do C gerado usa a grafia keel.

**Só a camada zero é fixa, e é o prelúdio da linguagem que decide isso.** `keel` é
o único módulo implícito (linguagem §1.3): `arena`, `buffer`, `slice`, `range`,
`corot` e `outcome` **se importam**, e por isso os seus headers chegam pela regra
geral de import — o `.h` do módulo, mais os headers de instância que ele origina.
`keel/arena.h` continua existindo e continua sendo C comum, mas ele é o `.h` do
módulo `keel.arena`, incluído porque alguém escreveu o `import`, e não porque o
backend o injeta em toda unidade.

A consequência prática é boa: um módulo que não usa arena não vê `keel/arena.h`,
e um projeto que tenha uma `arena` própria não colide com nada — que é
exatamente o que a linguagem comprou ao tirar a base do prelúdio.

Os `static_assert` são a contrapartida do mapeamento de ponto flutuante (§3.1). Este é o **único lugar onde `<stdint.h>` aparece**: todo o resto do C gerado usa a grafia keel. Se o projeto já define `u8` ou `i32` com tipo compatível, a redeclaração de `typedef` é legal desde o C11; uma definição conflitante vira erro do compilador C sobre o prelúdio — visível, não silencioso.

### 4.3 Headers de instância

Cada instância de modificador gera **um header, sem `.c`**:

```plain
keel/keel_buffer_i32.h
keel/keel_slice_geom_Point.h
coll/coll_stack_i32.h
```

Include guard derivado do nome mangled; as funções saem `static inline`. Não há objeto de instância, não há símbolo externo e portanto não há discussão de duplicidade — é o mesmo caminho que klib e stb_ds seguem para o mesmo tipo de código.

O header de instância inclui o do argumento quando este é outra instância, e o `.h` do módulo que define o tipo quando o argumento é tipo de usuário. Ele é autossuficiente: não depende de ordem de inclusão. Não há ciclo possível, porque o tipo é escrito por extenso — a profundidade é finita e a ordem é topológica por construção.

```keel
buffer slice char lines;
```

```c
/* keel/keel_buffer_slice_char.h */
#include "keel/keel_slice_char.h"

typedef struct keel_buffer_slice_char {
    size_t           cap;
    size_t           len;
    keel_slice_char *ptr;
} keel_buffer_slice_char;
```

O header de instância é incluído pelo `.h` do módulo quando o tipo aparece na interface, e pelo `.c` quando é só de uso interno. Como todo módulo importado traz seu `.h`, os headers de instância chegam por transitividade.

**Quem instancia é quem usa**, não quem declara o tipo argumento. Dois módulos que usam `buffer i32` geram o mesmo header independentemente; como a geração é determinística, o conteúdo é byte a byte idêntico e a segunda escrita vira no-op. É o determinismo que torna o compartilhamento correto sem exigir visão do grafo inteiro.

#### 4.3.1 Instância sobre o tipo em definição

A linguagem permite `slice Val` dentro de `struct Val` (linguagem §4.3) e
atribui a **ordem de emissão** a este documento. Ela é uma só, e cai da divisão
entre o que precisa da declaração e o que precisa da definição:

```keel
pub typedef struct Val Val;
pub struct Val { Kind tag; slice Val itens; };
```

```c
/* val.h */
typedef struct val_Val val_Val;                 /* 1. declaração adiantada   */

typedef struct keel_slice_val_Val {             /* 2. struct da instância    */
    size_t    len;
    val_Val  *ptr;                              /*    basta a declaração     */
} keel_slice_val_Val;

struct val_Val { val_Kind tag; keel_slice_val_Val itens; };   /* 3. definição */

static inline size_t keel_slice_val_Val_length(...)           /* 4. verbos    */
```

Quatro obrigações:

1. **A declaração adiantada vem do fonte**, não do backend. O `typedef struct Val
   Val;` é do C e o usuário o escreve; sem ele quem diagnostica é o compilador C,
   pelo princípio 3. O backend não sintetiza declaração que o usuário não pediu.
2. **A struct da instância precede a definição do agregado.** Ela guarda `T *`, e
   ponteiro para tipo incompleto é legal — é o que quebra o ciclo.
3. **Os verbos vêm depois da definição**, porque os que passam `T` por valor
   precisam do tamanho.
4. **O header da instância deixa de ser autossuficiente neste caso**, e é a única
   exceção à frase do §4.3: ele não pode incluir `val.h`, que o incluiria de
   volta. A saída é a mesma do C de sempre — as três partes saem **no `.h` do
   módulo que define o tipo**, na ordem acima, e o arquivo separado não é gerado.
   A instância continua sendo função do nome; o que muda é onde ela mora.

A condição que dispara a exceção é sintática e local: o argumento do modificador
é um tipo declarado **neste mesmo módulo** e a instância aparece dentro do corpo
dele. Fora disso vale o §4.3 sem emenda.

### 4.4 Definição fora de linha de instância

Função `pub` **sem** `inline` num módulo genérico sai `extern` no header da instância, e o corpo precisa de um dono. `instance` é a declaração que assume essa posse (linguagem §4.9):

```keel
module instances;
import coll;

instance coll.stack i32;
instance coll.stack geom.Point;
```

```c
/* gen/instances.c — os corpos */
#include "coll/coll_stack_i32.h"
#include "coll/coll_stack_geom_Point.h"
/* definições extern das funções não-inline de cada instância */
```

É a disciplina de definição única do C, explicitada: o header da instância é o `extern int g;`, e `instance` é o `int g;`. E é `instances.k` — um fonte que o usuário escreveu — que dá ao build o `.o` e a regra, o que mantém a invariante do §4.1 intacta.

- **Não altera o header da instância.** Ele é função do genérico e do argumento, e é byte a byte idêntico para todo mundo. Por isso **o modo é propriedade do genérico, não do uso**: `pub inline` gera header-only, `pub` gera `extern`, e nenhum dos dois depende de existir ou não um `instances.k` em algum lugar da árvore.
- **Instância duplicada é erro de link.** Dois módulos declarando a mesma `instance` dão símbolo duplicado, e o cgen compila um módulo por vez — genuinamente não vê. É a mesma falha de definir o mesmo global em dois `.c`, e o idioma é um `instances.k` por projeto.
- **Instância faltando também é erro de link.** Usar um genérico não-inline sem que ninguém tenha declarado a `instance` falha em `coll_stack_i32_length`. É a taxa ergonômica que justifica o default ser inline.

**A stdlib keel não tem definição fora de linha.** Pré-declarar `instance` dentro do próprio módulo genérico funciona para genérico do usuário e **não** funciona para a stdlib, que é exatamente o conjunto de módulos para o qual o usuário não escreveu regra nenhuma: um `gen/keel.c` não-vazio precisaria de `keel.o` na linha de link, e o alvo sem fonte volta inteiro. Módulo da stdlib é inteiramente `pub inline`.

#### 4.4.1 Declaração que não menciona parâmetro

A linguagem emite **uma vez, no módulo**, toda declaração de um genérico que não
mencione **nem parâmetro nem modificador** do módulo (linguagem §4.9). Ela não
pertence a instância nenhuma, e por isso não vai para o header de instância:

```keel
module keel.outcome type T;
pub modifier outcome { i32 code; T v; }
pub constexpr i32 OK   = 0;
pub constexpr i32 NONE = INT32_MIN;
```

```c
/* keel/outcome.h — o .h do módulo, uma vez; sob C23 */
constexpr i32 keel_outcome_OK   = 0;
constexpr i32 keel_outcome_NONE = (-2147483647 - 1);

/* keel/keel_outcome_i32.h — por instância */
typedef struct keel_outcome_i32 { i32 code; i32 v; } keel_outcome_i32;
```

Duas consequências de emissão:

- **O nome não leva o argumento** — `keel_outcome_OK`, não
  `keel_outcome_i32_OK`. É a regra do §2.1 aplicada ao que a linguagem já decidiu
  que não é da instância.
- **`constexpr` de módulo segue o §9.2 como qualquer outro**, e não `static const`:
  o que a linguagem promete é valer **onde o C exige expressão constante**, e
  `static const` não vale — `case outcome.OK:` não compilaria. Sob C11 as duas
  linhas saem como macro de nome já manglado, sem o tratamento de escopo de bloco,
  porque o prefixo do módulo já as torna únicas no arquivo.
- **O header de instância inclui o `.h` do genérico**, e não o contrário. A
  constante é dele; a struct é da instância.

A verificação é léxica — o nome do parâmetro ou de um modificador aparece, ou não
aparece, na sequência de tokens da declaração —, então o backend não classifica
nada: recebe a partição pronta da linguagem e emite cada metade no seu arquivo.

### 4.5 Tipos gerados são opacos

O layout das structs de instância aparece no header porque **tem** que aparecer: são tipos de valor e o compilador C precisa do tamanho. Aparecer não é ser interface.

> **O nome mangled de um tipo gerado e o layout dos seus campos não são interface suportada, e não são ABI.** O acesso é pelos builtins, e só.

O motivo é substantivo: se os campos fossem contrato, nada mais garantiria as invariantes que os builtins existem para manter — `push` respeitando `cap`, `len` nunca ultrapassando a capacidade. Bastaria um `xs.len++` para tudo voltar a ser vetor cru com struct em volta.

Nada impede fisicamente que alguém escreva o nome mangled ou toque o campo, inclusive de dentro de um `extern_c`. É uso não suportado, e o layout **vai** mudar entre versões. Quando muda, o critério de atualização regenera tudo que o cgen gerou; a única superfície que não cicatriza sozinha é o que foi escrito à mão.

Acesso direto a campo de símbolo conhecido como instância é detectável no fonte keel e gera **warning** nomeando o builtin equivalente.

---

## 5. Lowering das construções

### 5.1 Declarações: substituição local de nome

`<modificador> <argumento>` é um especificador de tipo, e o lowering é troca de nome, **local**. O backend nunca precisa da gramática de declaradores do C:

```keel
buffer i32 x;
buffer i32 *x;
buffer i32 x[10];
buffer i32 (*f)(void);
void (*g)(slice char s);
typedef slice u8 (*Leitor)(i32);
```

```c
keel_buffer_i32 x;
keel_buffer_i32 *x;
keel_buffer_i32 x[10];
keel_buffer_i32 (*f)(void);
void (*g)(keel_slice_char s);
typedef keel_slice_u8 (*Leitor)(i32);
```

`array` e `ref` **somem**:

```keel
array i32  v[100];
array i32  m[2,3,4];
i32 *ref   p = ptr(xs, 3);
```

```c
i32 v[100];
i32 m[2][3][4];
i32 *p = keel_buffer_i32_ptr(&xs, 3);
```

Parâmetro `array` multidimensional recebe `[static d0]`, que documenta o contrato e faz GCC/Clang avisarem sobre `NULL` e sobre vetor menor na dimensão 0:

```keel
void f(array i32 v[2,3,4]);
```

```c
void f(i32 v[static 2][3][4]);
```

#### 5.1.1 Prefixo do C

`spec-c` (linguagem §3.1) é **copiado verbatim, na posição em que foi
escrito**, e não participa de nada mais: não entra no mangling (§2.2), não muda a
instância, não é reordenado.

```keel
alignas(64) array f32 canal[1024];
static      buffer i32 pool;
_Atomic     buffer u32 compartilhado;
buffer _Atomic u32 contadores;
[[maybe_unused]] slice char s;
```

```c
alignas(64) f32 canal[1024];
static      keel_buffer_i32 pool;
_Atomic     keel_buffer_u32 compartilhado;
keel_buffer_atomic_u32 contadores;
[[maybe_unused]] keel_slice_char s;
```

As duas últimas linhas são o par que justifica a regra existir. `_Atomic` antes do
modificador qualifica o **descritor** e sai onde estava; dentro do argumento ele
qualifica o **elemento**, e aí é a instância que muda de nome. O backend não
decide qual é qual — a linguagem já separou por posição, e aqui só se emite.

**O prefixo não altera o posicionamento do §4.1.** `static` é ligação e `pub`/`priv`
é arquivo (linguagem §4.1): uma variável `pub static` já é error 15 na
linguagem, e o backend nunca vê o caso.

### 5.2 Containers: struct e funções `static inline`

```c
typedef struct keel_buffer_i32 {
    size_t  cap;
    size_t  len;
    i32    *ptr;
} keel_buffer_i32;

typedef struct keel_slice_i32 {
    size_t  len;
    i32    *ptr;
} keel_slice_i32;

static inline keel_buffer_i32 keel_buffer_i32_as(i32 *p, size_t n) {
    return (keel_buffer_i32){ .cap = p ? n : 0, .len = 0, .ptr = p };
}
static inline keel_buffer_i32 keel_buffer_i32_of(i32 *p, size_t n) {
    return (keel_buffer_i32){ .cap = p ? n : 0, .len = p ? n : 0, .ptr = p };
}
static inline i32 *keel_buffer_i32_push(keel_buffer_i32 *b) {
    if (b->len == b->cap) return NULL;
    return &b->ptr[b->len++];
}
static inline i32 *keel_buffer_i32_push1(keel_buffer_i32 *b, i32 v) {
    if (b->len == b->cap) return NULL;
    b->ptr[b->len] = v;
    return &b->ptr[b->len++];
}
static inline i32 *keel_buffer_i32_pop(keel_buffer_i32 *b) {
    if (b->len == 0) return NULL;
    return &b->ptr[--b->len];
}
```

`push(x)` e `push(x,v)` geram funções distintas — `_push` e `_push1`, pela regra de aridade do §2.1 — em vez de uma variádica: são duas assinaturas C normais, e o compilador C confere cada uma.

A chamada de builtin é reescrita para a função mangled correspondente, que recebe o **endereço** do container:

```keel
length(w->ps)
push(pts, (geom.Point){3.0f, 4.0f})
as_slice(xs, 2, 7)
```

```c
keel_buffer_sim_Particle_length(&w->ps)
keel_buffer_geom_Point_push1(&pts, (geom_Point){3.0f, 4.0f})
keel_buffer_i32_as_slice2(&xs, 2, 7)
```

**O sufixo de aridade vale para `as_slice` como para qualquer outro**, e é o que
impede a colisão: `slice.of(xs)` dá `_as_slice` e `slice.of(xs,2,7)` dá
`_as_slice2` — dois argumentos além do contêiner. Sem o sufixo, as duas
assinaturas chegariam ao mesmo símbolo, e C não tem sobrecarga:
`error: conflicting types for 'as_slice'`.

Todo builtin que pode falhar é gerado com `[[nodiscard]]`: ignorar o retorno de `push` ou de `alloc` vira warning do compilador C.

Verificação de limites em `get`, `set` e `ptr(x,i)` é emitida em build de debug e ausente em release. A chave é da ferramenta.

**`array` no despacho.** Sobre símbolo `array` unidimensional, `length` e `capacity` saem como `sizeof(v)/sizeof(<elem>)` — o tipo do elemento vem da tabela, não de `*(v)`. Sobre multidimensional, o idioma `sizeof(v)/sizeof(*(v))` daria a primeira dimensão, então o backend emite `sizeof(v)/sizeof(i32)`. `dim(v,k)` sai como literal. Em parâmetro multidimensional, `length` **não** pode usar `sizeof` e emite o produto literal das dimensões.

### 5.3 Açúcar de indexação

`x[i]` é `*ptr(x,i)`. O lowering é sempre por **função `static inline`, nunca por macro** — é o que garante que cada argumento seja avaliado exatamente uma vez, pelas regras normais de chamada de função, e é por isso que `x[i++]` incrementa `i` uma vez só.

```keel
length(lines[3])
push(grid[3], 42)
grid[3][7] = 5
```

```c
keel_slice_char_length(keel_buffer_slice_char_ptr(&lines, 3))
keel_buffer_i32_push1(keel_buffer_buffer_i32_ptr(&grid, 3), 42)
*keel_buffer_i32_ptr(keel_buffer_buffer_i32_ptr(&grid, 3), 7) = 5
```

O par `&*` colapsa na geração; não sai `&*` no `.c`.

```keel
array i32 v[2,3,4];
v[1,2,5] = 0;
```

```c
i32 v[2][3][4];
v[1][2][5] = 0;
```

#### 5.3.1 Açúcar sobre modificador com `dim`

Quando o modificador declara `dim N` (linguagem §4.9), a emissão do açúcar
conta os índices escritos e ramifica. É **a única ramificação** que `dim`
acrescenta ao emissor:

```keel
tensor(3) f32 t;
tensor(2) f32 m;

f32 x = t[i,j,k];
f32 y = m[i,j];
```

```c
f32 x = *tens_tensor_3_f32_ptr3(&t, (size_t[3]){i, j, k});
f32 y = *tens_tensor_2_f32_ptr2(&m, (size_t[2]){i, j});
```

Quatro obrigações:

1. **O literal composto é emitido no ponto da indexação**, com tipo `size_t[N]` e
   os índices na ordem escrita. Cada índice é copiado verbatim, e continua
   avaliado exatamente uma vez — a garantia da §5.3 não muda, porque inicializador
   de literal composto tem a mesma regra de avaliação única que argumento de
   função. `t[i++, j]` incrementa `i` uma vez.
2. **A extensão do literal é o bloco que o contém**, que é o que o C garante e o
   que basta: ele vive até o fim da expressão de chamada, e o acessor não guarda
   o ponteiro.
3. **`k ≠ N` não chega aqui** — é o error 104 da linguagem, decidido antes de
   qualquer emissão. O emissor tem um caso só, e não ramifica por aridade.
4. **Modificador sem `dim` não passa por aqui.** O rank fixo declara um acessor
   por aridade (linguagem §4.9), e o açúcar da §5.3 o alcança pela regra
   geral: nenhum literal composto é emitido, e a ramificação desta subseção nem é
   consultada.

**O `for` do acessor tem limite constante depois da substituição**, então o
compilador C o desenrola e o literal desaparece por SROA. Vale conferir uma vez,
porque é a premissa do desenho:

```c
/* -O2, x86-64: o corpo de _ptr3 colapsa em */
t->ptr + idx0 * t->passos[0] + idx1 * t->passos[1] + idx2 * t->passos[2]
```

Em `-O0` não colapsa: o vetor é escrito na pilha e o laço roda. É o custo
declarado na linguagem §4.9, e não há mitigação de backend para ele — nem
deveria haver, porque a alternativa seria o backend gerar o que a linguagem
decidiu não gerar.

### 5.4 `arena`

A arena é o `.h` do módulo `keel.arena` — C comum, utilizável inclusive a partir de código que não passa pelo keel. Ela chega ao módulo pelo `import`, como qualquer outro (§4.2).

```c
/* keel/arena.h */
typedef struct keel_arena {
    size_t         cap;
    size_t         top;
    unsigned char *buf;
} keel_arena;

[[nodiscard]] static inline void *keel_arena_alloc_n(keel_arena *a, size_t n,
                                                     size_t sz, size_t align) {
    if (n > SIZE_MAX / sz) return NULL;                   /* diagnóstico 109 */
    size_t need = n * sz;
    uintptr_t base = (uintptr_t)(a->buf + a->top);
    size_t pad   = (size_t)(((base + (align - 1)) & ~(uintptr_t)(align - 1)) - base);
    size_t livre = a->cap - a->top;
    if (pad > livre || need > livre - pad) return NULL;
    a->top += pad + need;
    return a->buf + a->top - need;
}
```

Quatro coisas nessa função são normativas, e as quatro vêm da linguagem §4.4:

1. **Contagem e tamanho do elemento entram separados**, e o produto é feito aqui. É a forma do `calloc`, e existe para que `n * sizeof(T)` que transborda devolva `NULL` em vez de uma região pequena que o programa acredita ser grande. **É o único ponto do backend que emite essa multiplicação.**
2. **A soma final não transborda**, porque é escrita como `need > livre - pad` e nunca como `pad + need > livre`.
3. **O alinhamento é do endereço, não do deslocamento**, e é o que dispensa o campo `base_align` que esta struct tinha. Alinhar `top` só serviria se `buf` já estivesse alinhado — e keel não tem como saber se está, porque `alignas(64)` é copiado verbatim e nunca avaliado (linguagem §1.3). Alinhando o endereço que se vai entregar, a base pode estar em qualquer lugar e **toda alocação sai alinhada**, inclusive de tipo sobre-alinhado sobre um `array u8` nu.
4. **`[[nodiscard]]`**, porque o `NULL` é o único canal de falha.

**`uintptr_t` aparece uma vez e não fabrica ponteiro.** Ele calcula o **número** de bytes de padding; o endereço devolvido sai de `a->buf + a->top`, aritmética de ponteiro dentro do próprio vetor. É a diferença entre uma conversão de valor definida-pela-implementação e uma travessia de ponteiro por inteiro, e só a primeira acontece aqui.

**O `T` nunca chega à biblioteca.** Quem carrega o tipo é o verbo, que não é função e sim reescrita: ele materializa o `sizeof`, o `alignof` e o cast que um humano escreveria à mão.

```keel
arena a;
arena.alloc(a, Particle, 100)
```

```c
keel_arena a = {0};
(sim_Particle *)keel_arena_alloc_n(&a, 100, sizeof(sim_Particle), alignof(sim_Particle))
```

O `= {0}` na declaração é normativo: é ele que faz arena não inicializada ter `capacity == 0` e todo `alloc` nela falhar limpo. Com `base_align` zerado, ele falha já na primeira comparação.

**Os quatro construtores.** Todos devolvem `bool` e todos preenchem `base_align`:

```keel
alignas(64) array u8 memo[65536];
arena a;  arena.from_array(a, memo);
arena s;  arena.from_parent(s, a, 4096);
arena t;  arena.from_stack(t, 4096);
arena h;  arena.from_memory(h, mem, cap);
```

```c
alignas(64) u8 memo[65536];
keel_arena a = {0};  keel_arena_from_array(&a, memo, sizeof memo);
keel_arena s = {0};  keel_arena_from_parent(&s, &a, 4096);
keel_arena t = {0};  alignas(alignof(max_align_t)) unsigned char keel__st0[4096];
                     keel_arena_from_array(&t, keel__st0, sizeof keel__st0);
keel_arena h = {0};  keel_arena_from_memory(&h, mem, cap);
```

- **Nenhum construtor recebe alinhamento**, e é a consequência de a alocação alinhar o endereço. A tentativa anterior era `alignof(<símbolo>)` para levar o `alignas` do usuário ao descritor, e ela **não é C**: `alignof` exige nome de tipo, e o GCC recusa com `ISO C does not allow 'alignof (expression)'`. Não havia substituto — keel copia `alignas(64)` verbatim e não avalia o argumento —, e a saída foi tirar a necessidade em vez de procurar a grafia.
- **`from_stack` é o único construtor sem função C própria**: ele gera o vetor no frame e chama `keel_arena_from_array`. A origem "pilha" está no vetor emitido, não numa inicialização diferente. O `alignas(alignof(max_align_t))` continua saindo, mas agora é **economia e não correção**: sem ele a arena funciona igual, e apenas gasta até `alignof(max_align_t) - 1` bytes de padding na primeira alocação.
- **`from_parent` recorta com `keel_arena_alloc_n(&pai, n, 1, 1)`** — alinhamento 1, porque a filha alinha as próprias alocações. Ela não herda nem precisa herdar alinhamento nenhum.

#### 5.4.1 O respaldo de tipo-caractere

A linguagem nomeia uma suposição e passa o conserto para cá (linguagem §4.4, §7.1):

> **Armazenamento de tipo-caractere, suficientemente alinhado, serve de respaldo
> para os objetos que a arena entrega.**

Ela é necessária porque o C não oferece alternativa. Objeto com tipo declarado
tem esse tipo como **tipo efetivo**, e só armazenamento **alocado** recebe tipo
pela escrita — então `from_array` e `from_stack`, que respaldam em objeto
declarado, estão fora do que o padrão promete, e `from_memory` sobre `malloc` ou
`mmap` está dentro. **Não há terceira rota**, e as candidatas foram descartadas
por razão e não por gosto: união faz da união o tipo efetivo; `max_align_t[]`
troca um tipo declarado por outro; e emitir a união dos tipos alocados exigiria
saber quais são, que é o princípio 7 da linguagem.

**O que este backend faz, então, são três coisas, e nenhuma esconde a quarta:**

1. **Emite armazenamento de tipo-caractere e nada mais** — `unsigned char` em
   `from_stack`, `array u8` do usuário em `from_array`. É o tipo de acesso que
   todo compilador real põe no topo da árvore de aliasing, e por isso o que
   estreita a suposição ao mínimo: nenhuma reordenação é habilitada por ele.
2. **Resolve o alinhamento na base**, e não na alocação — `alignas` do usuário
   chega pelo `alignof` do símbolo, e `from_stack` sobre-alinha sempre. Uma das
   duas armadilhas do respaldo é de alinhamento, e essa é fechada de verdade.
3. **Nomeia o remédio de build** para o alvo em que a suposição não se sustente:
   `-fno-strict-aliasing` na compilação do C gerado (ferramenta §4.1). Não é
   padrão, e não deve ser: nenhuma reprodução foi obtida em GCC 13 a `-O3` em
   nenhuma direção, e ligá-lo por omissão custaria otimização a todo programa
   para pagar um risco que não se mediu.

**Um backend com rota conforme não deve nada.** Se o alvo oferece armazenamento
sem tipo declarado, é ele que sai, e a guarda da linguagem §7.1 fica sem uso —
sem que uma linha da §4.4 da linguagem mude. É a mesma separação do §3
entre `f32` **ser** binary32 e `typedef float f32;` ser como este backend
entrega binary32.
- **`from_memory`** toma ponteiro e tamanho crus e assume `max_align_t`, porque a origem é `malloc` ou `mmap`. Região de linker script com alinhamento menor é responsabilidade de quem a declarou, e é o que a regra 3 da linguagem §4.4 já diz.

Com tamanho constante não é preciso VLA. Some o `#ifdef __STDC_NO_VLA__`, some o lowering duplo, some a flag de compilador que o forçaria, e some o modo de falha não testável.

`clone` combina os verbos que já existem: aloca no destino pelo comprimento da origem e copia, e devolve `outcome` pela regra do modo de falha (linguagem §4.11): o que ele devolve é descritor, não ponteiro, e descritor não tem sentinela. Vale igual para `at` (linguagem §4.5), e nenhum dos dois precisa de tratamento próprio no backend: a instância de `outcome` sai pelo §5.14 como qualquer outra.

```keel
outcome slice geom.Point out = slice.clone(a, slice.of(tmp)) else return -1;
```

```c
keel_outcome_keel_slice_geom_Point out = keel_slice_geom_Point_clone(&a, keel_buffer_geom_Point_as_slice(&tmp)); if (keel_outcome_keel_slice_geom_Point_failed(out)) return -1;
```

A função da instância faz `keel_arena_alloc_n` mais a cópia dos elementos, e devolve `code != OK` quando a alocação falha. **Ela não refaz a checagem de transbordamento**: o comprimento da origem já coube na memória uma vez.

**`at` é a única função de acesso com teste em release.** Ela é total (linguagem §4.5), então o `if` é semântica e não verificação — não depende de `--checks` e não some. Ela devolve **`outcome T`**, e não ponteiro, pela regra de modo de falha da linguagem §4.11; a emissão está no §5.13, que é onde ela mora.

`get`, `set` e `ptr(x,i)` continuam sem teste em release, pela regra do §5.2: elas têm pré-condição, e a verificação de debug existe para revelar quem a violou.

### 5.5 `defer`

> **A definição do `defer` é da linguagem** (linguagem §4.7): cleanup léxico, na saída do escopo, em ordem inversa de registro, sem pilha em runtime. **Como isso acontece em C é deste documento**, e já mudou uma vez — a v0 chegou a depender de extensão do GCC antes de passar à varredura de pontos de saída com injeção por escopo. A definição não mudou junto.

O backend varre o escopo, coleta os pontos de saída — fim natural do bloco, `return`, `break`/`continue` que deixam o escopo, `goto` para fora — e emite o corpo de cada `defer` pendente **naquele escopo**, na ordem inversa. Não há registro em runtime e não há flag.

Duas coisas organizam o resto da seção e estão adiante: **quando a varredura roda** (§5.5.1 — depois de toda construção de keel que termina escopo, de modo que ela só vê os cinco terminadores do C) e **em que forma o cleanup sai** (§5.5.2 — escada de rótulos ou inline, por condição léxica).

```keel
priv int process(arena *a, const char *path) {
    FILE *ref fp = fopen(path, "r");
    if (!fp) return -1;
    defer [now FILE *fp] (fclose(fp));

    arena s;
    arena.from_parent(s, a, 4096);
    defer (arena.reset(s));

    if (!parse(s, fp)) return -2;
    return 0;
}
```

```c
#line 24 "app/main.k"
static int app_main_process(keel_arena *a, const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;                     /* nada a limpar: o defer ainda não foi registrado */
    struct { FILE *fp; } keel__c0 = { fp };  /* [now]: cópia no ponto de registro */

    keel_arena s = {0};
    keel_arena_from_parent(&s, a, 4096);


    if (!parse(&s, fp)) { keel_arena_reset(&s); fclose(keel__c0.fp); return -2; }
    keel_arena_reset(&s); fclose(keel__c0.fp); return 0;
}
```

Três coisas nesse par são normativas, e as três sustentam o §6:

- o `defer` de expressão **não emite nada** no ponto de registro, então a linha de fonte vira linha em branco na saída;
- o cleanup de cada ponto de saída sai **numa linha só**, pela regra 2 do §6;
- o `return -1` não recebe cleanup, porque naquele ponto nenhum `defer` havia sido registrado.

Em `return expr;`, `expr` é avaliada para um temporário gerado **antes** de o cleanup rodar, e o temporário é retornado depois. Ele é declarado com o **tipo de retorno escrito na função**, copiado como sequência de token da produção `decl-funcao` (linguagem §3.1) — `size_t f(…)` dá `{ size_t keel__rv0 = expr; … }`, e o `*` de `char *f(…)` está no declarador, que também está capturado.

> **O `auto` do C23 saiu daqui**, e por isso este lowering é o mesmo nos dois perfis (§9). Enquanto não havia produção de função na gramática, o backend genuinamente não tinha o tipo em lugar nenhum. O que o `auto` acrescentava era a conversão de lvalue, inofensiva num temporário inicializado uma vez e devolvido em seguida. O que ele escondia era o caso do declarador que enterra o nome — `int (*f(void))[10]` não tem corrida contígua de tokens que seja o tipo de retorno —, e esse caso passou a ser o error 120 da linguagem, em vez de um lowering que só funcionava sob C23.

**Captura.** A lista de `[now]` já vem com os tipos escritos (linguagem §4.7), então ela **é** a lista de membros: o backend copia cada entrada verbatim para uma struct local gerada no ponto de registro, e o corpo referencia as cópias.

```keel
defer [now int fd, FILE *saida] { relata(saida, fd); }
```

```c
struct { int fd; FILE *saida; } keel__c0 = { fd, saida };
/* ... em cada ponto de saída: */
app_relata(keel__c0.saida, keel__c0.fd);
```

Nada é interpretado: a entrada da captura entra como membro sem uma reescrita. **O `typeof_unqual` saiu junto com o `auto`**, pelo mesmo motivo — o tipo agora está escrito, e escrito ele serve aos dois perfis. Sem `[now]`, o corpo referencia as variáveis diretamente e não há struct.

#### 5.5.1 `defer` é a última passagem de fluxo

> **Toda construção que termina um escopo é baixada antes do `defer`.** O que a
> varredura de saídas enxerga são os **cinco terminadores do C** — `return`,
> `break`, `continue`, `goto` e o fim natural do bloco — e nada de keel.

É ordem de emissão, não semântica: a definição continua sendo a da linguagem
§4.7. E é possível porque a redução já é total — todo terminador de keel já cai
num dos cinco, e as seções que o fazem são estas:

| Escrito | Já baixa para | |
| --- | --- | --- |
| `win` · `fail` · `interrupted` | `goto keel__fim<N>` | §5.9, regra 6 |
| `cobreak` | `goto <fsm>_end` | §5.6, regra 4 |
| `cowin` · `cofail` · `coagain` | `return` | §5.6, regra 4 |
| cláusula `else` | `if (…failed(x)) return …;` | §5.12 |
| corpo de `foreach` e de `parallel` | `for` comum — o `break` e o `continue` do usuário são C ordinário | §5.7, §5.9 |

**O ganho não é a contagem, é o fecho.** Doze formas viram cinco, mas o que
importa é que as cinco **não podem crescer**: construção nova que termine escopo
tem de baixar para um dos cinco, porque em C não há um sexto. O `defer` nunca
aprende palavra nova, e é a invariante da linguagem §1.3 aplicada para dentro do gerador.

**A ordem também corrige um caso, e não só simplifica.** `interrupted;` é saída
**condicional** — só salta com a bandeira ligada. Baixado antes, ele *é*
`if (…) { st[w] = INTERRUPTED; goto keel__fim<N>; }`, e a regra genérica do `goto`
põe o cleanup **dentro** do `if`, onde ele pertence. Sem a ordem, "os três verbos
são pontos de saída" precisaria de uma ressalva escrita para o único dos três que
pode não sair.

**Três obrigações que a ordem cria**, e nenhuma é grande:

1. **Token gerado carrega a marca de gerado.** Os diagnósticos de `defer` — 36,
   38 e o warning 47 — são sobre o que o usuário escreveu. A varredura passa a ver
   `goto` que o próprio backend emitiu, e **nenhum deles pode acusar**. Um bit por
   token resolve, e é a disciplina de sempre: não diagnosticar a própria saída.
2. **A posição sobrevive à expansão.** O §6 é uma invariante sobre dois
   contadores, e ela só vale se o fluxo de tokens que chega ao `defer` ainda
   carregar a linha do `.k`. A expansão produz token posicionado, nunca texto.
3. **O error 122 da linguagem roda antes.** Ele é sobre o `return` que o usuário
   escreveu no corpo do `parallel`, e depois da expansão o corpo continua com esse
   `return` intacto — mas a checagem é da linguagem e vem antes de qualquer
   emissão, o que a mantém falando de fonte.

**Não há ponto fixo.** Nenhuma expansão injeta `defer`: o gestor do `parallel`, o
despacho do `cofsm` e o `if` da cláusula `else` são código sem cleanup. A ordem é
uma passagem, não uma iteração — que é a condição para ela simplificar em vez de
apenas mudar de lugar o problema.

#### 5.5.2 Duas formas de emissão

O cleanup sai de duas maneiras, e **a condição é léxica**:

> **Se toda saída do escopo é `return` ou o fim natural**, o cleanup baixa para
> **escada de rótulos** — um rótulo por registro, em ordem inversa, e cada saída
> salta para o que corresponde a quantos registros estão pendentes.
> **Se alguma saída é `break`, `continue` ou `goto`**, o cleanup baixa **inline**
> em cada uma delas.

```c
/* escada: 3 registros, 6 saídas */
    if (a==1) { rv=1; goto keel__e2; }
    ...
keel__e2: s3(r);
keel__e1: s2(q);
keel__e0: s1(p);
    return rv;
```

**As duas são o que um humano escreveria naquele lugar, e é esse o critério.**
Ninguém repete o cleanup em seis saídas de uma função — escreve a escada, que é o
idioma clássico do C e é exatamente a ordem inversa de registro sem tradução.
E ninguém escreve escada num corpo de laço: ali as saídas vão para três lugares
diferentes — `continue` cai no incremento, `break` deixa o laço, `return` deixa a
função —, um rótulo não serve aos três, e o que sobra é uma variável de ação
despachada no fim do corpo, que ninguém assina. Medido em GCC 13 a `-O2`:

| | inline | escada |
| --- | --- | --- |
| 3 registros × 6 saídas, escopo de função | 86 instr. | **51** |
| 1 registro × 3 saídas, escopo de função | 23 instr. | **15** |
| 1 registro em corpo de laço, saídas mistas | **59** | 60, **mais uma variável em runtime** |

O *tail merging* do compilador C não apaga a diferença, e é por isso que a
condição vale a pena: onde a escada ganha, ganha de 35% a 40%; onde ela não
ganha, ela custa.

**As duas formas só são equivalentes porque o error 123 da linguagem existe.** A
escada emite o corpo do `defer` **uma vez**, no fim do escopo do registro; o inline
o cola **em cada saída**, que pode estar dentro de um escopo mais interno. Sob
sombreamento as duas ligariam a símbolos diferentes — e um lowering que muda
sentido não é lowering. A linguagem recusa o sombreamento (linguagem §4.7), e é essa recusa
que autoriza o backend a ter duas formas. **Se ela caísse, só a escada seria
correta**, e o corpo de laço voltaria a custar a variável de ação.

**A condição não precisa de caso especial para nenhuma construção**, e é o §5.5.1
que a torna assim: depois da expansão, o corpo de um `parallel` tem saídas que são
`goto`, então a condição dá falso e o inline sai por dedução — não por uma linha
escrita a respeito de `parallel`. O mesmo vale para `cofsm`. **Escrever a condição
em termos dos cinco terminadores do C é o que a mantém com um caso só.**

### 5.6 `cofsm`

O bloco de estados baixa para **despacho por `goto` e blocos rotulados**. Nunca para um `switch` com o corpo do usuário dentro dele.

```keel
pub cofsm ciclo [ST1, ST2, ST3];

pub corot i32 passo(arena *a, Agente *ag) {
    while (1) {
        cofsm ciclo (ag->s) {
            ST1:
                corot i32 r = fn1(a);
                if (corot.failed(r))  cofail(1);
                if (corot.ongoing(r)) coagain(0);
                ag->s = ST3; cobreak;
            ST2:
                if (ag->n >= 20) coagain(0);
                ag->s = ST1;
            ST3:
                cowin(0);
        }
    }
}
```

```c
typedef enum ag_ciclo {
    ag_ciclo_ST1,            /* 0 — estado inicial */
    ag_ciclo_ST2,
    ag_ciclo_ST3
} ag_ciclo;

keel_corot_i32 ag_passo(keel_arena *a, ag_Agente *ag) {
    while (1) {
    switch (ag->s) {                                  /* só saltos: nada do usuário aqui */
    case ag_ciclo_ST1: goto keel__ciclo_ST1;
    case ag_ciclo_ST2: goto keel__ciclo_ST2;
    case ag_ciclo_ST3: goto keel__ciclo_ST3;
    default:           goto keel__ciclo_end;
    }
    keel__ciclo_ST1: {
        keel_corot_i32 r = ag_fn1(a);
        if (r.code > 0) return (keel_corot_i32){ 1, 0 };
        if (r.code == 0) return (keel_corot_i32){ 0, 0 };
        ag->s = ag_ciclo_ST3; goto keel__ciclo_end;
    }
    goto keel__ciclo_end;
    keel__ciclo_ST2: {
        if (ag->n >= 20) return (keel_corot_i32){ 0, 0 };
        ag->s = ag_ciclo_ST1;
    }
    goto keel__ciclo_end;
    keel__ciclo_ST3: {
        return (keel_corot_i32){ -1, 0 };
    }
    keel__ciclo_end: ;
    }
}
```

Seis regras de emissão, e cada uma existe por um motivo concreto:

1. **O `switch` de despacho contém apenas saltos.** Nenhum código do usuário mora dentro dele, e é isso que faz `break` e `continue` do usuário ligarem ao laço ou `switch` dele. Um `switch` com o corpo dentro reservaria `break` para a fsm.
2. **O rótulo vai antes da chave de abertura**, e as chaves são o escopo do
   estado que a linguagem §4.8 exige — não um detalhe de emissão. Saltar para um rótulo interno entraria no meio do escopo e **os inicializadores das declarações não rodariam** — legal em C, e o tipo de bug que ninguém encontra. Com o rótulo fora, o salto entra pelo topo e declaração de estado se comporta normalmente.
3. **Cada bloco de estado é seguido de `goto <fsm>_end`.** É o fim de bloco de estado da linguagem §4.8, e é o que elimina fallthrough. No último estado o salto é omitido: ele cairia na linha seguinte, e ninguém escreveria isso à mão (princípio 2).
4. **`cobreak` é `goto <fsm>_end`**, de qualquer profundidade — a razão de o despacho ser por rótulo. **`cowin`, `cofail` e `coagain` são `return`**, não saltos: eles saem da função, não do bloco (linguagem §4.8), e por isso não interagem com o despacho.
5. **Não há verbo de transição.** `ag->s = ST3;` é atribuição do usuário e atravessa opaca; o que keel faz é reescrever a constante nua para `ag_ciclo_ST3`, pela regra de escopo de enum (§2.1).
6. **`default:` sai sempre**, saltando para o fim. Estado fora de faixa é possível quando o estado vem de memória — `memset`, arquivo, rede. Sob `--checks`, um `assert` o precede.

**O laço é do usuário, e o backend não o emite.** O `while (1)` acima veio do fonte; sem ele, o `cofsm` executa um estado por chamada. É a decisão da linguagem §4.8 de não ter opinião sobre a política de avanço, e para o backend significa que não há nada a gerar em volta do despacho.

Os rótulos levam o nome da fsm, não um contador: `keel__ciclo_ST1`. É o que mantém os símbolos gerados estáveis quando outra fsm é inserida antes dela no arquivo — sem isso, uma edição no topo reescreveria o arquivo inteiro e recompilaria quem não precisava, contra a regra 2 de `ferramenta §6`.

**A declaração e a operação são formas separadas** (linguagem §4.8), e isso simplifica a emissão: o `enum` sai do `decl-maquina`, não da cabeça do `cofsm`. `pub` o põe no `.h`, `priv` no `.c`, pela regra normal de posicionamento (§4.1) — não há mais inferência a partir de onde o tipo é usado. Na forma local — `cofsm mm [ISSO, AQUILO] state;` dentro de uma função — o `enum` e a variável saem juntos, ali, como qualquer declaração de bloco.

O nome é o do §2.1 — `M_<fsm>`, com as constantes `M_<fsm>_<estado>`, **exatamente como qualquer enum nomeado do módulo**. **A ordem das constantes vem sempre da lista declarada**, que é obrigatória: é a diferença entre um valor de estado que é contrato e um que é consequência do layout do corpo, e importa porque este enum atravessa o `.h` e pode estar gravado em memória, arquivo ou rede — a mesma razão de o `default:` existir.

**Mapeamento de linhas.** O despacho é a maior região injetada da linguagem: uma linha de fonte — o `cofsm … {` — vira de três a N+2 linhas de saída. Diverge, e portanto ressincroniza com um `#line` logo depois, uma vez. Do primeiro rótulo em diante o corpo é copiado e volta a mapear 1:1, pela regra 2 do §6.

### 5.7 `foreach` e `apply`

Cabeçalho numa linha, corpo copiado, nenhuma diretiva `#line`.

```keel
foreach (i32 v, size_t i : xs) {
    soma += v * pesos[i];
}
```

```c
{ keel_buffer_i32 *keel__c0 = &xs; size_t keel__n0 = keel_buffer_i32_length(keel__c0); for (size_t i = 0; i < keel__n0; i++) { i32 v = keel_buffer_i32_get(keel__c0, i);
    soma += v * pesos[i];
} }
```

Quatro regras de emissão:

1. **O contêiner vai para um ponteiro temporário e o comprimento para um `size_t`**, ambos antes do `for`. É o que dá a avaliação única da linguagem §4.7, e é o que um humano escreveria ao perceber que `length` seria rechamado.
2. **O binder de índice é a própria variável do laço**, e por isso a linguagem §4.7 o exige sempre: o `for` gerado leva o nome que o usuário escolheu, e a família `keel__i<N>` só sobra no `apply`, que não tem binder para escrever.
3. **Binder por valor gera `get`; por ponteiro gera `ptr` de um índice.** O bloco extra em volta existe para os temporários morrerem no fim, e é o que permite `foreach` aninhado sem colisão de nome.
4. **Tudo até a abertura do corpo cabe numa linha.** É o que faz o corpo mapear 1:1 e dispensa ressincronizar — ao contrário do despacho do `cofsm` (§5.6), que não tem como caber.

### 5.8 Ponto de entrada

A função de entrada de um módulo é função comum e sai manglada como qualquer outra. O `main` do C sai em **unidade separada**, gerada quando a ferramenta recebe `--main <módulo>`:

```c
/* gen/main_net_http.c */
#include "net/http.h"
int main(int argc, char **argv) { return net_http_main(argc, argv); }
```

Ela não vai dentro do `.c` do módulo, porque então o conteúdo gerado dependeria da flag de invocação e o critério de timestamp deixaria de significar o que significa.

---

### 5.9 `parallel`

É a maior emissão do backend, e a única que depende de recurso fora do C. O que sai são **dois laços aninhados e uma diretiva** — e a propriedade que organiza a seção inteira é que o C emitido **não muda** conforme o OpenMP exista ou não.

```keel
f32 dt = 1.0f / 60.0f;

parallel passo ALL (size_t w : 0..4; Particle *p, size_t i : ps; (dt)) {
    p->v += dt * p->a;
}
```

```c
{   keel_buffer_sim_Particle *keel__c0 = &ps;
    size_t keel__n0 = keel_buffer_sim_Particle_length(keel__c0), keel__k0 = 4;
    size_t keel__s0 = (keel__n0 + keel__k0 - 1) / keel__k0;
    keel_parstatus keel__st0[4] = {0};

    #pragma omp parallel for num_threads(4) default(none) \
            shared(keel__c0, keel__n0, keel__s0, keel__st0) firstprivate(dt)
    for (size_t w = 0; w < 4; w++) {
        size_t keel__lo = w * keel__s0;
        size_t keel__hi = (w + 1) * keel__s0 < keel__n0 ? (w + 1) * keel__s0 : keel__n0;
        for (size_t i = keel__lo; i < keel__hi; i++) { sim_Particle *p = keel_buffer_sim_Particle_ptr(keel__c0, i);
            p->v += dt * p->a;
        }
        keel__st0[w] = keel_parstatus_SUCCESS;
        keel__fim0: ;
    }
}
```

Oito regras de emissão:

1. **A diretiva reparte o laço dos workers, não a travessia.** A aritmética da faixa fica dentro do corpo, e é a fórmula normativa da linguagem §4.7. É por isso que a partição não depende de `schedule` e o mesmo programa dá as mesmas faixas em qualquer implementação.
2. **`num_threads(k)` sai sempre**, com o literal. É o que materializa "uma thread por faixa"; sem ele o número de faixas continuaria certo, mas duas rodariam na mesma thread, o que a linguagem permite e ninguém escreveria à mão.
3. **`default(none)` é obrigatório**, e é ele que cumpre a promessa da linguagem §4.7: um local não listado na captura vira **erro do compilador C nomeando a variável**, na linha do `.k`. Sem a cláusula, ele entraria como `shared` em silêncio e o programa teria corrida.
4. **Os temporários gerados entram nas cláusulas junto com os do usuário.** `default(none)` exige atributo para tudo que a região toca, inclusive `keel__c0` e `keel__n0` — e o backend os lista porque escreveu os nomes.
5. **Captura escalar é `firstprivate`; instância `byref` é `shared`.** É a disciplina da linguagem §4.7 traduzida uma para uma. Escrever num escalar capturado já é o error **114** na linguagem, então o backend não precisa de `const` para proibi-lo — e é uma diferença de custo real: com um struct de argumentos, o tipo de cada captura teria que ser escrito, e o backend não o conhece (§5.5).
6. **Os três verbos saltam para o fim do corpo do worker.** `win`, `fail` e `interrupted` viram `goto keel__fim<N>`, com o rótulo dentro do bloco estruturado da iteração — nunca `break`, nunca `return`, nunca saída da região. `return` do usuário é o error **122** na linguagem, exatamente porque não teria como sair daqui: o GCC recusa a região com `invalid branch to/from OpenMP structured block`.
   **O rótulo vem depois da gravação do status**, e não antes: quem chega ao fim natural grava `SUCCESS` e cai no rótulo; quem saltou já gravou o próprio status e o rótulo não o toca. Invertido, `fail` e `interrupted` seriam apagados por um `SUCCESS` que ninguém pediu.
7. **O status por worker vai num vetor local**, indexado por `w`, e os predicados `ok`, `failed` e `interrupted` da linguagem §4.7 saem como uma varredura serial depois do bloco.
8. **Contador e bandeira são locais do gestor**, nunca `static`. Um bloco `parallel` numa função chamada duas vezes tem que começar zerado nas duas, e `static` também tornaria o bloco não reentrante.

**Sob política diferente de `ALL`**, entram as duas variáveis atômicas, também locais:

```c
_Atomic size_t keel__vit0 = 0;
_Atomic bool   keel__intr0 = false;

/* win; */
if (atomic_fetch_add_explicit(&keel__vit0, 1, memory_order_relaxed) + 1 >= 1)
    atomic_store_explicit(&keel__intr0, true, memory_order_relaxed);
keel__st0[w] = keel_parstatus_SUCCESS; goto keel__fim0;

/* interrupted; */
if (atomic_load_explicit(&keel__intr0, memory_order_relaxed)) {
    keel__st0[w] = keel_parstatus_INTERRUPTED; goto keel__fim0;
}

/* fail; */
keel__st0[w] = keel_parstatus_FAILURE; goto keel__fim0;
```

A política é constante (linguagem §4.7), então o alvo sai como literal e o gestor não tem caso. Sob `ALL` as duas variáveis não existem, `interrupted` é erro da linguagem (**88**) e `win` é só o status mais o salto. Vencedor e interrompido **não são distinguidos em runtime**: são verbos diferentes, e cada um já sabe com que status sai.

**`relaxed` basta em todos.** A bandeira é dica: vê-la tarde custa iterações, não corretude. O que precisa estar visível ao pai é o que o worker escreveu, e quem sincroniza isso é a barreira implícita no fim do `omp parallel for`.

**Mapeamento de linhas.** O gestor é região injetada e diverge, como o despacho do `cofsm`; ressincroniza com um `#line` uma vez, depois dele. Dentro do laço interno vale a regra do §5.7: o cabeçalho cabe numa linha e o corpo mapeia 1:1.

`apply(T, c, fn, …)` é o mesmo laço com o corpo fixo, e os argumentos de contexto atravessam opacos, na ordem escrita, depois do elemento e do índice:

```keel
apply(i32, xs, dobra);
apply(Node *, p->ns, visita, pool, sb);
```

```c
{ keel_buffer_i32 *keel__c0 = &xs; size_t keel__n0 = keel_buffer_i32_length(keel__c0); for (size_t keel__i0 = 0; keel__i0 < keel__n0; keel__i0++) { m_dobra(keel_buffer_i32_get(keel__c0, keel__i0), keel__i0); } }
{ keel_buffer_ast_Node *keel__c1 = &p->ns; size_t keel__n1 = keel_buffer_ast_Node_length(keel__c1); for (size_t keel__i1 = 0; keel__i1 < keel__n1; keel__i1++) { lift_visita(keel_buffer_ast_Node_ptr(keel__c1, keel__i1), keel__i1, pool, sb); } }
```

`fn` recebe elemento e índice, nessa ordem, e depois o contexto. A chamada é direta: se `fn` é função keel, sai manglada; se vem de `extern_c`, sai como está. O backend **não conta nem examina os argumentos de contexto** — quem confere o parâmetro é o compilador C, pelo princípio 3.

#### 5.9.1 OpenMP, e de quem ele é

> **OpenMP é obrigação do compilador, pedida por flag.** O keel **emite** a diretiva; não traz agendador, não traz pool de threads, não linka runtime próprio e não tem camada dependente de sistema operacional.

É a frase que decide tudo o mais desta subseção, e ela vale a pena porque a conclusão oposta é tentadora: como `parallel` é a única construção com dependência de plataforma, parece natural o keel resolvê-la — vendorizando threads, ou trazendo um shim. Resolver significaria distribuir o primeiro artefato do keel que não é C portável, com superfície de porte crescendo a cada alvo novo.

**E aqui não é preciso, porque a diretiva é ignorável.** Sem `-fopenmp` o `#pragma` desconhecido é ignorado pelo compilador C, e o que sobra são os dois laços aninhados percorrendo o contêiner inteiro, uma faixa depois da outra. O programa compila, roda e dá o mesmo resultado — a linguagem §4.7 já diz que a ordem entre faixas não é definida, e a execução serial é um dos entrelaçamentos permitidos.

Disso saem três obrigações e uma não-obrigação:

- **O C emitido é o mesmo nos dois casos.** Não há emissão condicional, não há `#ifdef` em torno dos laços, e não existe um "lowering serial" separado a manter. É a diferença que mais paga nesta seção: um caminho de código, não dois.
- **`<stdatomic.h>` é incluído quando a política não é `ALL`**, e não depende de OpenMP: `_Atomic` é qualificador de linguagem, e gcc e clang baixam as operações para instrução ou para builtin `__atomic_*`. Não há alvo em que se tenha o compilador e falte o atômico. Em execução serial as operações continuam corretas e o resultado é o mesmo.
- **A guarda é `#warning`, nunca `#error`:**

```c
#ifndef _OPENMP
#  warning "keel: modulo usa `parallel` e o alvo nao oferece OpenMP: a travessia sai em serie"
#endif
```

- **Não há `<threads.h>`, `thrd_create`, thunk nem struct de argumentos.** É a não-obrigação, e ela vale registrar porque foi uma alternativa considerada: um lowering por threads da libc precisaria montar um struct com **o tipo de cada captura**, e o backend não conhece esses tipos — a invariante da linguagem §1.3 proíbe que conheça. `firstprivate(dt)` não precisa de tipo nenhum. Os dois lowerings não são intercambiáveis, e é essa a razão de a linguagem ter nomeado o OpenMP em vez de deixar a escolha aqui.

O diagnóstico 105 é a versão do cgen dessa advertência, quando o alvo é conhecido na invocação; o `#warning` é a rede para quando não é. Quem exige paralelismo de verdade transforma o warning em erro pela flag de build (ferramenta §3).

### 5.10 `coseq`

A cadeia baixa para **um `switch` sobre uma variável de etapa local**, e aqui o `switch` é legítimo — ao contrário do `cofsm` (§5.6), cujo corpo é código do usuário e onde um `break` dele mataria a máquina. O bloco de um `coseq` só contém chamadas (linguagem §4.8), então não há código do usuário dentro do despacho e não há `break` para colidir.

**Nada escapa além do resultado.** `coseq` é uma máquina completa: entra, roda e sai, sem devolver ONGOING. Por isso a variável de etapa é **local e não sobrevive**, não há enum a declarar fora, e o resultado é `outcome`, não `corot`.

```keel
coseq passo { ola(a, ag); autentica(a, ag); pronto(a, ag); }
if (outcome.failed(passo)) return -1;
```

```c
keel_outcome_i32 passo = { 0, 0 };
{   unsigned char keel__et = 0;
    for (;;) {
        switch (keel__et) {
        case 0: {
            keel_corot_i32 keel__r = ag_ola(a, ag);
            if (keel__r.code > 0) { passo = (keel_outcome_i32){ 1, 0 }; goto keel__passo_fim; }
            if (keel__r.code == 0) continue;            /* ONGOING: repete a mesma etapa */
            keel__et = 1; continue;
        }
        case 1: /* … idem, com código de falha 2, avançando para 2 … */
        case 2: {
            keel_corot_i32 keel__r = ag_pronto(a, ag);
            if (keel__r.code > 0) { passo = (keel_outcome_i32){ 3, 0 }; goto keel__passo_fim; }
            if (keel__r.code == 0) continue;
            passo = (keel_outcome_i32){ 0, keel__r.v };  /* última etapa: OK com o valor dela */
            goto keel__passo_fim;
        }
        default: passo = (keel_outcome_i32){ 1, 0 }; goto keel__passo_fim;
        }
    }
    keel__passo_fim: ;
}
```

Seis regras de emissão:

1. **O símbolo do nome é o resultado**, e ele fica **fora** do bloco, porque é interrogado depois dele (linguagem §4.8). É `outcome T`, e nasce `{0}` — que já é `OK`.
2. **A variável de etapa é local ao bloco**, e o tipo é o menor inteiro sem sinal que conte as etapas. Não há enum e não há constante nomeada: numa cadeia o rótulo seria número de série, e a linguagem não pede que ele exista.
3. **O código de falha é o ordinal da etapa, base um.** É de onde vem o "qual etapa quebrou" que o `outcome.code` entrega, e não custa campo novo.
4. **ONGOING repete a mesma etapa na mesma chamada.** É a consequência de `coseq` não ceder o controle, e é o que o `continue` sobre o `for (;;)` expressa. O custo — laço apertado sobre uma etapa lenta — é da construção, e está escrito na linguagem §4.8.
5. **`default:` existe pela exaustividade, e é inalcançável.** A razão é a do §5.6 — o C pede o ramo quando o valor pode não estar no conjunto e não há fallthrough —, mas a *situação* do §5.6 não se repete aqui: lá a variável de estado é do usuário e pode vir de memória; aqui `keel__et` é local gerado, recebe apenas os ordinais que a emissão escreve, e nada do programa alcança. Sai com falha porque um ramo tem de sair com alguma coisa, e o `assert` sob `--checks` documenta que o caminho não deveria existir.
6. **Nenhum `#line` dentro do despacho além do de cada `case`**: não há corpo de usuário a mapear, só chamadas que ele escreveu em uma linha cada.

### 5.11 `copar`

Não há enum e não há estado que sobreviva: o `copar` é **uma sequência de blocos guardados, mais duas contagens, dentro de um laço**. Os `corot` dos participantes são o estado (linguagem §4.8), e como a construção não cede o controle, eles são locais.

```keel
copar coleta ANY { parser(a, ag); tempo(a, ag); }
```

```c
keel_outcome_i32 coleta = { 0, 0 };
{   keel_corot_i32 keel__p = {0}, keel__t = {0};      /* {0} == ONGOING */
    size_t keel__ok = 0;
    for (;;) {
        if (keel__p.code == 0) {
            keel__p = ag_parser(a, ag);
            if (keel__p.code > 0) { coleta = (keel_outcome_i32){ 1, 0 }; goto keel__coleta_fim; }
            if (keel__p.code < 0) keel__ok++;
        }
        if (keel__t.code == 0) {
            keel__t = ag_tempo(a, ag);
            if (keel__t.code > 0) { coleta = (keel_outcome_i32){ 2, 0 }; goto keel__coleta_fim; }
            if (keel__t.code < 0) keel__ok++;
        }
        if (keel__ok >= 1) goto keel__coleta_fim;      /* ANY */
    }
    keel__coleta_fim: ;
}
```

Cinco regras de emissão:

1. **`{0}` é ONGOING**, e é por isso que os participantes não precisam de inicialização escrita. É a decisão de qual constante vale zero (linguagem §4.8) pagando aqui.
2. **Um bloco guardado por participante**, na ordem escrita. Guardar é o que impede chamar de novo quem já terminou — o erro que a versão à mão comete.
3. **A política é constante** (linguagem §4.8), então `keel__ok >= 1` sai com o número literal. Sob `ALL` o número é a contagem de participantes, conhecida na emissão.
4. **Falha sai cedo, e com o ordinal.** Falha não conta para a política e não interrompe por contagem — ela encerra o bloco direto, que é a diferença para o `win` do `parallel` (linguagem §4.7).
5. **Nenhum átomo, nenhuma thread.** É a diferença inteira para o §5.9: mesma política, mesmo vocabulário, e aqui tudo acontece numa chamada de função comum.

> **Não há segundo laço.** A linguagem §4.8 decidiu que quem sobrou simplesmente deixa de ser chamado: não há passagem de encerramento e não há bandeira de interrupção. O lowering acima é o completo — depois do `goto … _fim` vem o rótulo, e nada entre os dois.

---

### 5.12 Cláusula `else`

O lowering é uma linha, e é o que a linguagem §4.7 define: a declaração sai como estava, seguida de um `if` cujo teste vem do verbo `failed` da instância. São **duas formas**, e o backend as recebe já separadas pelo parser (linguagem §4.7) — ele não olha para o operando.

**Forma de saída** — o statement entra no `if`, verbatim:

```keel
outcome Cfg c = cfg.le(path) else return -1;
outcome u32 n = cfg.porta(path) else { log(path); return -1; }
```

```c
keel_outcome_cfg_Cfg c = cfg_le(path); if (keel_outcome_cfg_Cfg_failed(c)) return -1;
keel_outcome_u32 n = cfg_porta(path); if (keel_outcome_u32_failed(n)) { log(path); return -1; }
```

**Forma de default** — a expressão entra no `win` da instância, e o resultado é reatribuído ao próprio símbolo:

```keel
outcome string name = login() else string.from("(noname)");
```

```c
keel_outcome_keel_string name = app_login(); if (keel_outcome_keel_string_failed(name)) name = keel_outcome_keel_string_win(keel_string_from("(noname)"));
```

Cinco regras de emissão:

1. **O teste é sempre a chamada a `failed` da instância**, pelo despacho normal do §5.2 — nunca `if (!x)`. Ponteiro não é falível (linguagem §4.7), então não há segundo caso a emitir, e o backend não classifica tipo nenhum.
2. **Sai numa linha só**, declaração e `if`, pela regra 2 do §6. É o que faz o corpo continuar mapeando 1:1 e dispensa ressincronizar — ao contrário do `cofsm` e do `parallel`, que não têm como caber. Vale para as duas formas: a de default acrescenta uma atribuição, não uma linha.
3. **O operando é copiado verbatim nas duas formas.** Nada é sintetizado dentro dele: não há desembrulho, não há conversão, não há `return` implícito. O que muda é **onde** ele é colado — dentro do `if` na forma de saída, dentro dos parênteses de `M_win(…)` na de default.
4. **Na forma de default, o alvo da atribuição é o próprio símbolo.** Não se emite temporário, não se emite literal composto escrito à mão, e não se escreve campo: o valor bom sai do construtor da instância, que é o mesmo verbo que o programa chamaria (warning 48). Um tipo falível sem `win` é error 119 e não chega ao backend.
5. **Nenhum temporário é criado.** O símbolo declarado é o que a cláusula lê e o que ela repara, e é ele que já está em escopo.

**`corot` não aparece aqui**, e não é omissão: ele declara `failed`, mas declara também `ongoing`, e o protocolo da linguagem §4.7 exige o primeiro e proíbe o segundo. Nenhuma declaração de `corot` casa com a cláusula. O predicado dele é `keel_corot_T_failed`, e quem o chama é o programa, no `if` que escreveu.

### 5.13 `at` — o acessor verificado

`at` é o único verbo da base cuja checagem sobrevive ao release, e a emissão diz isso sem `#ifdef`:

```keel
outcome i32 v = buffer.at(xs, idx) else return -1;
```

```c
static inline keel_outcome_i32 keel_buffer_i32_at(keel_buffer_i32 *b, size_t i) {
    return i < b->len ? keel_outcome_i32_win(b->ptr[i]) : keel_outcome_i32_none();
}

keel_outcome_i32 v = keel_buffer_i32_at(&xs, idx); if (keel_outcome_i32_failed(v)) return -1;
```

Duas regras:

1. **A comparação não é condicional de build.** Ao contrário das checagens de `get`, `set` e `ptr` (§5.3), que saem entre as macros de `debug`, esta é código comum da instância. É o que a linguagem §4.5 promete, e a promessa é o motivo de o verbo existir.
2. **Fora de faixa sai por `none`, não por um código.** O produtor é o do §5.14, e nenhum valor de erro é inventado aqui — o backend não tem catálogo de erro e não deve ganhar um.

### 5.14 `outcome` e `corot`

`keel.outcome` é módulo genérico comum, e a emissão é a do §4.3 mais a partição do §4.4.1 — a struct por instância, as constantes uma vez.

```keel
pub outcome u32 porta(const char *path);
```

```c
typedef struct keel_outcome_u32 { i32 code; u32 v; } keel_outcome_u32;

static inline bool keel_outcome_u32_failed(keel_outcome_u32 e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_u32_ok(keel_outcome_u32 e)     { return e.code == keel_outcome_OK; }

static inline keel_outcome_u32 keel_outcome_u32_win(u32 v)  { return (keel_outcome_u32){ keel_outcome_OK, v }; }
static inline keel_outcome_u32 keel_outcome_u32_fail(i32 c) { return (keel_outcome_u32){ c, (u32){0} }; }
static inline keel_outcome_u32 keel_outcome_u32_none(void)  { return (keel_outcome_u32){ keel_outcome_NONE, (u32){0} }; }

keel_outcome_u32 cfg_porta(const char *path);
```

Quatro regras:

1. **`code` vem primeiro no layout**, e `OK` é zero. É o que faz `{0}` e `memset` deixarem um `outcome T` válido com valor presente, sem código — a mesma propriedade que o `ONGOING` zero dá ao `corot` (linguagem §4.8).
2. **`failed` é comparação com zero, não com uma lista.** Qualquer código diferente de `OK` é falha, então acrescentar código de erro novo não toca a função — e é o que permite ao programa usar o `code` como `errno`, como enum próprio, ou como o que quiser.
3. **`NONE` é uma constante como `OK`, e não um estado a mais.** As duas saem uma vez pela partição do §4.4.1; `failed` não as distingue, e nenhum código do backend as compara entre si. O que separa ausência de erro é o `code` que o programa lê, não a emissão (linguagem §4.10).
4. **Os três produtores são `inline` da instância**, e o campo de valor sai zerado nos dois que não recebem valor. É o que faz `fail` e `none` não dependerem de `T` ser inicializável de outra forma, e é por isso que o programa nunca escreve campo à mão (warning 48).

**`corot T` tem o mesmo layout e outra leitura do zero.** Não há enum de status: os três estados são regiões do `code`, testadas por sinal.

```c
typedef struct keel_corot_i32 { i32 code; i32 v; } keel_corot_i32;

static inline bool keel_corot_i32_ok(keel_corot_i32 r)      { return r.code <  0; }
static inline bool keel_corot_i32_ongoing(keel_corot_i32 r) { return r.code == 0; }
static inline bool keel_corot_i32_failed(keel_corot_i32 r)  { return r.code >  0; }
static inline i32  keel_corot_i32_code(keel_corot_i32 r)    { return r.code; }
```

Quatro consequências para a emissão:

1. **`{0}` é ONGOING**, e é o que faz um participante de `copar` (§5.11) e um agente inteiro nascerem prontos sem código de inicialização.
2. **`cowin(v)` emite `{ -1, v }`; `cofail(n)` emite `{ n, {0} }`; `coagain(v)` emite `{ 0, v }`.** Os três são `return`, nunca salto (§5.6).
3. **Não há `keel_costatus`.** Um enum de três valores não descreveria FAILURE, que é uma região; expor a constante seria mentira, e o backend não emite nenhuma.
4. **`cofail` com literal zero é recusado na tradução** — error 64 —, porque emitiria `{ 0, … }`, que é ONGOING.


## 6. Mapeamento de linhas

`#line` é pegajosa: fixa o número da linha seguinte e o compilador segue incrementando sozinho. Isso permite enunciar a regra como uma **invariante mecânica**, em vez de uma lista de casos:

> O gerador mantém dois contadores — a linha do `.k` que está sendo traduzida e a linha do arquivo de saída. **Sempre que os dois divergem, emite-se `#line`.** Enquanto andam juntos, nada é necessário.

Daí decorre o comportamento de cada região:

| Região | Diverge? | `#line` |
| --- | --- | --- |
| Texto copiado — corpo de função, bloco `extern_c` | não, se as quebras forem preservadas | uma na entrada da região |
| Transliteração 1:1 — `import`, `import_c` viram `#include` | não, se as linhas em branco forem preservadas | uma na entrada da região |
| Expansão de builtin ocupando mais de uma linha | sim | ressincroniza depois |
| Código injetado — struct de instância, cleanup de `defer`, temporário de `return` | sim | ressincroniza depois |
| Gestor de `parallel` e despacho de `cofsm`/`coseq` | sim | ressincroniza uma vez, depois do bloco |
| Cláusula `else` — declaração mais `if`, nas duas formas | não, cabe numa linha | nenhuma |
| `at` — chamada de instância | não | nenhuma |
| Sintético — `#include` do próprio `.h`, do prelúdio, dos headers de instância | — | **nenhuma**: falha ali é bug de ferramenta ou de build, não erro do usuário |

A linha de `import_c` merece nota, porque é o caso que o critério "copiado versus gerado" deixaria escapar: `#include <tgmath.h>` é gerado, não copiado, mas é tradução um-para-um de uma linha que o usuário escreveu, e falha com frequência — nome errado, `-I` faltando, header que só existe em outra plataforma. Sem a diretiva, `fatal error: tgmath.h: No such file or directory` aponta para um `.h` que ninguém escreveu.

Duas regras de emissão decorrem da invariante:

1. **Texto copiado nunca é reindentado nem reformatado.** Reformatar destrói o alinhamento e, com ele, a posição de todo diagnóstico do compilador C naquela região. A mesma condição se estende à região de imports: preservar as linhas em branco é o que faz uma diretiva só cobrir o bloco inteiro.

2. **Em corpo de função, o lowering de uma linha de fonte ocupa preferencialmente uma linha de saída.** Uma chamada de builtin longa sai numa linha só em vez de quebrada em três, e uma expansão de dois statements sai numa linha só — porque quebrar custa um `#line` a cada statement e, sem ele, todo o resto do corpo passa a apontar para a linha errada. Com a regra, o corpo inteiro mapeia 1:1 a partir de uma única diretiva na abertura da função.

   O custo é linha gerada mais longa que a que um humano escreveria. É a **única concessão deliberada ao princípio 2** desta especificação, e ela se paga: é o que faz cada erro do compilador C cair na linha certa do `.k`, que é a razão de o princípio 3 funcionar.

A string do arquivo é o caminho normalizado do módulo, com a extensão `.k`.

### 6.1 Diagnóstico dentro de instância

Erro do compilador C no corpo de `coll_stack_i32_length` aponta, por `#line`, o fonte do genérico — `coll.k`, na linha da função. É o certo: toda posição é no fonte keel que o usuário escreveu.

Só que essa posição sozinha não diz **qual** instanciação quebrou, e um genérico que funciona em `i32` e falha em `geom.Point` fica ilegível. Por isso a instância carrega a posição do primeiro uso que a criou, e o diagnóstico ganha uma linha de contexto:

```plain
coll.k:14:12: error: invalid operands to binary + [...]
sim.k:7:1: note: na instanciação de coll.stack em geom.Point
```

O `note:` é do cgen, não do compilador C, e é a única informação que o mapeamento de linhas sozinho não alcança.

---

## 7. Propriedades exigidas do conteúdo gerado

Estas são propriedades do **conteúdo**, e por isso deste documento. Como o conteúdo chega ao disco — escrita atômica, comparação antes de gravar, critério de timestamp — é da ferramenta (`cgen-tool-spec.md` §5 e §6).

### 7.1 Determinismo

> **Mesmas entradas, saída byte a byte idêntica, em qualquer máquina.**

É o que torna o modelo por arquivo seguro. Dois módulos compilados separadamente geram o mesmo header de instância, cada um por sua conta; como o conteúdo é idêntico, a segunda escrita vira no-op e a corrida sob `make -j` é inofensiva — tanto faz quem renomeia por último. Sem determinismo, os dois ficariam se sobrescrevendo em laço, retriggando compilação sem fim.

É também o que permite teste por comparação de saída. As fontes de variação que costumam quebrá-lo na implementação estão catalogadas em ferramenta §6.1.

### 7.2 De que cada arquivo é função

| Arquivo | É função de |
| --- | --- |
| `.h` e `.c` de um módulo | o fonte daquele `.k` **e a interface pública dos módulos que ele importa**, transitivamente |
| header de instância de modificador **embutido** | **apenas o próprio nome** |
| header de instância de modificador **do usuário** | o nome **e** o corpo do módulo genérico |
| unidade de ponto de entrada | o nome do módulo pedido na invocação |

**A primeira linha tem duas metades, e a segunda é fácil de perder.** O C de `A`
depende de `B` porque a tradução consulta a interface de `B` em dois lugares que a
linguagem já nomeia: o `&` de adaptação vem do **parâmetro declarado no callee**
(linguagem §4.11), e o despacho decide entre verbo de tipo e função de módulo
lendo a assinatura de lá.

```keel
/* b.k */  pub void consume(slice i32 s);     →  A emite  b_consume(s)
/* b.k */  pub void consume(slice i32 *s);    →  A emite  b_consume(&s)
```

Editar `b.k` muda o `.c` de `A` sem que `A.k` seja tocado. **O critério de
atualização tem de refletir isso**, e é ferramenta §5 que o escreve.

A segunda linha é a que paga o custo da repetição. Como todo módulo que usa o tipo gera o header, numa árvore grande a mesma instância é considerada muitas vezes:

> O conteúdo de um header de instância de modificador embutido é **função pura do próprio nome**, e o nome é o nome do arquivo. Se ele existe e foi escrito pela mesma versão do gerador, é necessariamente idêntico — basta um `stat`.

É o que separa esse header do par do módulo: o `.c`/`.h` depende do **conteúdo** do fonte, e por isso precisa de comparação; a instância de `buffer` depende apenas do **nome**. Dentro de uma invocação, cada instância é considerada uma vez só, por memoização.

**Instância de modificador do usuário não tem essa propriedade**, e é a única coisa que os módulos genéricos custam aqui: editar o `push` de `coll.k` muda `coll_stack_i32.h` sem mudar o nome dele. O critério de atualização correspondente é ferramenta §5.1.

O número de instâncias distintas é limitado pelo fonte, não pelo número de módulos — um projeto real tem dezenas, não milhares —, então o custo em regime é alguns `stat` por invocação e nenhuma escrita.

### 7.3 Consequência: o depfile é transitivo

As funções da instância saem inline num header, então toda unidade de tradução que a inclui **embute o código**. Editar `coll.k` tem que retriggar não só quem escreveu `stack i32`, mas todo módulo que alcance esse header por transitividade. O formato e a emissão são de ferramenta §4.5; a razão é esta linha.

A ressalva que sobra é a mesma do make: se o próprio gerador mudar, os gerados ficam obsoletos sem que timestamp nenhum acuse, e a saída é apagar o diretório de destino.

---

## 8. Diagnósticos deste documento

| # | Nome | Diagnóstico | Sev. |
| --- | --- | --- | --- |
| 14 | `nome-acima-do-teto` | Nome gerado acima do teto de comprimento (255, ou 63 sob `--pedantic-names`) | `error` |
| 18 | `nome-reservado` | Identificador do usuário no espaço reservado `keel_` | `error` |
| 23 | `set-fora-de-length` | `set` com índice fora de `length` | `debug` |
| 48 | `campo-de-instancia` | Acesso direto a campo de instância de modificador, fora do módulo que a declara | `warning` |
| 66 | `estado-fora-de-faixa` | Estado fora de faixa na variável de estado | `debug` |
| 79 | `recorte-fora-de-faixa` | Intervalo cujos limites violam `a <= b <= length(x)` | `debug` |
| 103 | `formato-estreito-indisponivel` | Módulo usa `f16` ou `bf16` e o alvo não oferece o formato | `error` |
| 108 | `openmp-indisponivel` | Módulo usa `parallel` e o alvo não oferece OpenMP — a travessia sai em série | `warning` |
| 109 | `alloc-overflow` | `arena.alloc` cujo `n * sizeof(T)` não cabe em `size_t` | `debug` |

A numeração é a da tabela única de `keel-spec.md` §2.4, onde estes aparecem com a origem `backend`. Ela é preservada aqui para que nenhuma referência quebre com a divisão dos documentos, e **a spec é a fonte**: divergência entre as duas tabelas é erro desta.

---

## Anexo — Levantamento de significância de identificador

Base factual do teto do §2.4. Levantado em 2026-08-24; os números são os que a
documentação de cada fornecedor publica, não medições.

### O que o padrão garante

| Edição | Interno / macro | Externo |
| --- | --- | --- |
| C89/C90 | 31 | 6 |
| C99, C11, C17, C23 | 63 | **31** |

Nenhuma edição posterior a C99 alterou estes números; C23 os mantém.

### O que os compiladores fazem

| Compilador | Significância | Observação |
| --- | --- | --- |
| GCC | interno: todos; externo: definido pelo linker | "para quase todos os alvos, todos os caracteres são significativos" |
| Clang / Arm Compiler 6 (`armclang`) | ilimitado | |
| TI Arm Clang, MSP430, TMS320C28x | ilimitado | documentado como *implementation-defined behavior* |
| MSVC | 2048 significativos; nomes externos 2047 | `/H` só **reduz**, e está obsoleta desde VS 2005 |
| Microchip XC16 / XC32 | sem limite imposto; ≥ 255 garantidos | |
| Microchip XC8 | C99: sem limite. C90 em PIC: 31 por padrão, extensível | modo C90 não se aplica: o keel gera C23 |
| **IAR Embedded Workbench** | **255** | **é o piso do levantamento** |

**Conclusão.** Entre toolchains capazes de C23, o pior caso é 255. O único número menor encontrado — 31, do XC8 — só existe em modo C90, que este backend nunca produz.

### Fontes

- [cppreference — *Identifier*, limites de tradução por edição do padrão](https://en.cppreference.com/c/language/identifier)
- [GCC — *Identifiers implementation*](https://gcc.gnu.org/onlinedocs/gcc/Identifiers-implementation.html)
- [Microsoft Learn — *Identifiers (C++)*](https://learn.microsoft.com/en-us/cpp/cpp/identifiers-cpp?view=msvc-170)
- [Microsoft Learn — */H (Restrict Length of External Names)*](https://learn.microsoft.com/en-us/cpp/build/reference/h-restrict-length-of-external-names?view=msvc-170)
- [Texas Instruments — *Arm C Implementation-Defined Behavior*](https://software-dl.ti.com/codegen/docs/tiarmclang/compiler_tools_user_guide/compiler_manual/c_cpp_language_implementation/c_implementation_defined_behavior.html)
- [Microchip — *The Number of Significant Initial Characters in an Identifier*](https://onlinedocs.microchip.com/oxy/GUID-BD1C16C8-7FA3-4D73-A4BE-241EE05EF592-en-US-6/GUID-CA7FD647-B8A0-497D-A3D4-931B200CFBB5.html)
- [IAR — *C/C++ Compiler Reference Guide*, 255 caracteres significativos](https://wwwfiles.iar.com/m32c/guides/EWM32C_CompilerReference.pdf)
- [SEI CERT C — *DCL23-C*, unicidade de identificadores mutuamente visíveis](https://wiki.sei.cmu.edu/confluence/display/c/DCL23-C.+Guarantee+that+mutually+visible+identifiers+are+unique)

---

## 9. Perfis de geração

Um **perfil** é o padrão C ao qual o código emitido se conforma. São dois — **C23**
e **C11** — e a regra que os governa é a do §1: um perfil só varia **onde a
linguagem não tiver nomeado a forma**.

> **O perfil não muda a linguagem.** Os dois aceitam e recusam exatamente o mesmo
> conjunto de programas (linguagem §7.1). Recusar sob um e aceitar sob o outro é
> não-conformidade nos dois.

### 9.1 O que varia

| No gerado | C23 | C11 |
| --- | --- | --- |
| `static_assert(c, m)` | igual | `_Static_assert(c, m)` — sempre com mensagem, que o C11 exige |
| `alignof` · `alignas` | igual | `_Alignof` · `_Alignas` |
| `bool` | igual | o prelúdio acrescenta `<stdbool.h>`, ao lado do `<stdint.h>` do §4.2 |
| `[[nodiscard]]` (§5.3) | igual | **omitido**: a alternativa seria `__attribute__`, e extensão de compilador está fora |
| `constexpr` (linguagem §4.2) | declaração verbatim | macro do símbolo manglado, mais a conferência do inicializador (§9.2) |

E o que **não** varia, porque a linguagem nomeou a forma: o `#pragma omp` do
`parallel`, o mangling do §2, o `#line` do §6, e — desde que o `auto` e o
`typeof_unqual` saíram — o lowering inteiro do `defer` (§5.5).

### 9.2 `constexpr` sob C11

```keel
priv constexpr int K = 1 << 4;
```

```c
#define app_K ((int)(1 << 4))
static const int app_K__chk = (1 << 4);      /* confere restrição e constância */
```

**O cast é obrigatório**, e é ele que preserva o sentido. Sem ele, quatro classes
de programa **válido** mudam de resultado em silêncio, o que seria o princípio 1
da linguagem quebrado:

| No fonte | Com `((T)…)` | Sem o cast |
| --- | --- | --- |
| `constexpr f32 K = 1;` … `K/2` | `0.5f` | `1/2` → `0` |
| `constexpr u8 B = 1;` … `sizeof B` | `1` | `sizeof(1)` → `4` |
| `constexpr size_t N = 1;` … `i < N`, `int i = -1` | falso | verdadeiro |
| `_Generic(K, float: a, int: b)` | ramo `float` | ramo `int` |

**O objeto de conferência devolve o que o cast escondeu.** Um cast torna bem
tipado o que não era, então `constexpr char *S = 10;` deixaria de ser
diagnosticado em lugar nenhum. `static const T … = INIT;` em escopo de bloco ou
de arquivo submete `INIT` a **duas** verificações que o cast dispensaria:

| | Porque |
| --- | --- |
| **restrição de tipo** | é inicialização, e vale a mesma regra de `(char *){10}` — violação, na linha da declaração |
| **constância** | o C exige que o inicializador de objeto com duração estática seja **expressão constante**, e é isso que recusa `constexpr int K = f();` |

A segunda é a razão de o `_Static_assert(sizeof((T){INIT}) > 0, …)` ter saído. O
literal composto em escopo de bloco **não** exige inicializador constante — o
`sizeof` conferia o tipo e deixava passar `constexpr int K = f();`, que o C23
recusa. Sob a forma antiga, o `K + K` do programa chamaria `f()` duas vezes, e o
perfil C11 aceitaria um programa que o C23 nega. Sob `static const` os dois
recusam.

O objeto de conferência é `static const`, nunca referenciado e sempre elidível; o
nome sai do símbolo mais o sufixo `__chk`, no espaço reservado do §2.

**Fica um resíduo, e ele é o único da tabela do §9.1 que custa diagnóstico:** o
C23 exige que o valor seja *exatamente representável* no tipo, e a inicialização
só faz a conversão. `constexpr u8 B = 300;` é error sob C23 e no máximo
`-Woverflow` sob C11. Está registrado na linguagem §7.3, e é o único item de lá
que fala do perfil.

**Escopo de bloco pede o par.** Em escopo de arquivo o símbolo já leva o prefixo do
módulo, e nada mais no arquivo casa com ele. Em escopo de bloco não há prefixo
(linguagem §4.2), e duas funções do mesmo arquivo podem declarar `constexpr size_t
N` com valores diferentes. **A macro não leva o nome do usuário:** ela recebe um
nome gerado no espaço reservado, e o backend **reescreve os usos** dentro do bloco.

```keel
void f(struct S *s) {
    constexpr size_t N = 8;
    array char buf[N];
    s->N = 1;
}
```

```c
static void app_f(struct S *s) {
#define keel__N_0 ((size_t)8)
    static const size_t keel__N_0__chk = 8;
    char buf[keel__N_0];
    s->N = 1;
#undef keel__N_0
}
```

Três regras, e as três existem por um caso concreto:

1. **O nome é `keel__<símbolo>_<ordinal do bloco>`.** O prefixo reservado (§2)
   garante que ele não colida com símbolo do usuário nem com macro vinda de
   header; o nome do usuário no meio é o que mantém o gerado legível, que é o
   princípio 2; o ordinal separa dois blocos irmãos que declarem o mesmo `N`.
2. **Os usos são reescritos, e keel só reescreve símbolo que ele mesmo declarou** —
   é a mesma operação do mangling do §2, aplicada em escopo de bloco. **`IDENT`
   precedido de `.` ou `->`, e designador `.x =`, não são reescritos**: são nome
   de membro, e a regra é a que a linguagem §4.3 já usa para resolver o `.`.
3. **Sem isso, `s->N` viraria `s->((size_t)8)`.** O pré-processador não sabe o que
   é membro, e a macro com o nome do usuário captura **toda** ocorrência do token
   no resto do bloco. Era o pior vazamento da forma anterior, e é erro do
   compilador C com mensagem que não aponta a causa.

O `#undef` no fim do bloco léxico deixa de ser necessário — nomes gerados não
colidem entre si nem com nada — e continua sendo emitido, porque limita a vida da
macro ao que o fonte dizia.

**Isto não é gerar truque de macro.** O que a linguagem existe para dispensar é a
macro que **constrói estrutura** — colagem de token, X-Macro, `TRY`/`CATCH` —, que
some do depurador e do diagnóstico. Uma constante nomeada não some de lugar nenhum,
e `#define MAX ((size_t)4096)` é o que um C99 bem escrito faz. O princípio 2 está
satisfeito por leitura direta.

**E nada disto existe sob C23**, que emite a declaração verbatim. A macro, o nome
gerado, a reescrita e o objeto de conferência são todos o preço de um perfil que
não tem a construção — e é a recusa do endereço (linguagem §4.2) que permite ao
C23 não pagar nenhum deles e ainda assim aceitar o mesmo conjunto de programas.

---

## 10. Questões abertas do backend

1. **Onde vive o arquivo de instância** — diretório fixo `keel/`, ou por módulo com dedup no build. Inalterada.
2. **Prefixo nos campos das structs geradas.** `xs.keel_len` em vez de `xs.len` não impede nada, mas torna a invasão legível e libera renomear sem discussão. A linguagem §4.5 — "um modificador que possui uma linearização não expõe o próprio armazenamento" — mais o warning 50 já dizem que layout não é interface, então o §4.5 responde: prefixar é coerente. O que resta é o atrito com o princípio 2, e ele é menor do que parecia.
3. **Bounds check por dimensão em `array` de parâmetro.** As dimensões vêm da tabela, não do `sizeof`; a dimensão 0 é uma promessa do chamador. O caso encolheu: a linguagem §4.2 hoje recusa `array T v[N]` 1D em parâmetro (errors 28 e 29), então o que sobra é o nD, onde as dimensões 1..n−1 são de fato conhecidas e só a 0 é promessa.
4. ~~A passagem de encerramento do `copar`.~~ **Fechada:** a linguagem §4.8 decidiu que não há passagem de encerramento — quem sobrou deixa de ser chamado, e a limpeza é do bloco que contém a máquina. O lowering do §5.11 já estava certo ao não a emitir, e nada entra entre o `goto … _fim` e o rótulo.
5. **Ordem das cláusulas no `#pragma` emitido.** `default(none)` obriga a listar todo símbolo tocado, e a lista cresce com os temporários do gestor. A ordem é irrelevante para o compilador e relevante para o determinismo (§7.1): fixar em "temporários do keel, depois capturas na ordem escrita" é o candidato, e falta confirmar contra um caso com aninhamento de `foreach` dentro do corpo.
