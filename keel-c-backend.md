# keel — Backend C

> © 2026 Faculdade de Engenharia Elétrica e de Computação, Universidade
> Federal do Amazonas (FEEC/UFAM). Licenciado sob
> [CC BY-SA 4.0](LICENSE-DOCS.md) ([tradução](LICENSE-DOCS.pt.md)) — este
> documento é prosa sobre a linguagem, não código; o `cgen` e a Base keel têm
> licença própria, ver [`LICENSE.md`](LICENSE.md) e a nota do §4.2. Escrita e
> revisão tiveram auxílio de Claude Opus e Claude Sonnet (Anthropic), sob
> direção humana.

**Documento normativo.** Especifica como as construções da linguagem ([`keel-spec.md`](keel-spec.md)) são materializadas em C.

Este documento existe porque **a linguagem não é o lowering**. `defer` é definido como cleanup léxico na saída do escopo, em ordem inversa de registro — isso é a linguagem, e não muda. Que a v0 já tenha estado presa a extensão do GCC, e que hoje se resolva varrendo os pontos de saída e injetando o corpo em cada um conforme o escopo, é assunto **deste** arquivo, e nada disso altera a definição do `defer`. Um segundo backend deve as mesmas obrigações; pode pagá-las de outro jeito — **onde a linguagem não tiver nomeado a forma**. O `parallel` é o caso em que isso mais aparece: a linguagem §4.8 deixa o mecanismo em aberto, e este documento especifica dois — série e OpenMP —, escolhidos pela invocação. O terceiro que a linguagem permitiria, um pool de threads da libc, não está aqui porque precisaria do tipo de cada captura, e a invariante da linguagem §1.3 proíbe conhecê-lo (§5.9.1).

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

A linguagem fixa o **nome canônico** de cada tipo no ponto de declaração (linguagem §4.2). O backend o materializa trocando `.` por `_`:

| Nome canônico | Símbolo C |
| --- | --- |
| tipo da camada zero (`i32`, `char`, `const char`) | mesma grafia; qualificador prefixa com `_` — `const_char` |
| `M.nome` | `M_nome` |
| Tipo C `arena`, fornecido pelo header da base | `arena` (§5.4) |
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
//keel
module geom;
typedef struct { float x, y; } Point;
```

```c
//C gerado
typedef struct geom_Point { float x, y; } geom_Point;
```

A tag repete o nome manglado mesmo quando o fonte escreveu a struct sem tag. É o §4.3.1 que a exige, para que a declaração adiantada seja sempre escrevível; ela é invisível no C, porque o espaço de tags é separado e o tipo é o mesmo tipo.

`buffer` é o modificador declarado no módulo `keel.buffer`; com o alias
`buffer`, sua escrita qualificada é `buffer.buffer`. O mesmo vale para
`outcome.outcome`, no alias do módulo `keel.outcome`. A escrita abreviada vem
do import com `types` e não confunde a identidade do módulo com a do modificador.

Na formação do nome de emissão, o nome do modificador igual ao último
componente do módulo não se repete, conforme a regra de encurtamento herdada
da spec original §1.7, hoje sem contrapartida na linguagem: ela é de emissão, e
mora aqui. Por isso `keel.buffer.buffer` aplicado a `i32` produz
`keel_buffer_i32`; `keel.outcome.outcome` aplicado a `i32` produz
`keel_outcome_i32`. Essa regra de nomes não modifica a representação nem o
contrato do tipo ao qual o modificador foi aplicado.

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

> O sufixo existe **apenas quando o verbo é declarado em mais de uma aridade**. Entre elas, a forma que recebe **apenas o contêiner** não leva sufixo; as demais levam **o número de argumentos além dele**. Verbo de aridade única não leva sufixo, quantos argumentos tenha.

O sufixo é a **última** parte do símbolo: primeiro o módulo e o modificador,
depois os argumentos de tipo, e o sufixo por último.

```plain
ptr(b)          →  keel_buffer_i32_ptr
ptr(b,i)        →  keel_buffer_i32_ptr1
push(b)         →  keel_buffer_i32_push
push(b,v)       →  keel_buffer_i32_push1
ptr(m,i,j)      →  mat_matrix_f32_ptr2

get(b,i)        →  keel_buffer_i32_get      /* aridade única: sem sufixo */
set(b,i,v)      →  keel_buffer_i32_set      /* idem, com dois além do contêiner */
```

**Com `dim`, o sufixo conta índices do call site, não argumentos do C** (linguagem §4.3). É a única exceção à frase acima:

```plain
ptr(t)                →  tens_tensor_2_f32_ptr    /* a base                 */
ptr(t,(size_t[2]){…}) →  tens_tensor_2_f32_ptr2   /* rank cheio: dois índices */
```

O acessor de rank cheio recebe **um** argumento além do contêiner — o vetor —, e ainda assim leva o sufixo `2`, porque o ponto de chamada escreveu dois índices. É o que faz o nome dizer o rank, e o que alinha o caso `dim` com o rank fixo, em que `mat_matrix_f32_ptr2` sai de dois índices escritos por extenso. **Não há acessor parcial** (linguagem §4.3), então não há par a desempatar — o que a exceção compra é legibilidade do símbolo, não unicidade.

**Fora de `dim`, o sufixo é o da regra geral, e é o que serve o rank fixo** (linguagem §4.3): `mat_matrix_f32_ptr1` e `mat_matrix_f32_ptr2` saem de `ptr(m,i)` e `ptr(m,i,j)`, dois acessores escritos por extenso, sem literal composto e sem exceção nenhuma no emissor.

Nada disso é resolução de sobrecarga: a aridade está escrita no call site e contar argumentos é sintático — nenhum tipo de argumento é examinado. É por isso que a regra não reabre o que a regra de fechamento da linguagem §4.3 fecha, e por isso ela vale igual para modificador embutido e do usuário (linguagem §4.9).

Os três espaços de identificador do C são prefixados, porque os três aparecem no `.h` e os três colidem entre módulos:

```keel
//keel
module sim;

typedef struct { f32 x, y; } Vec2;
struct No { struct No *prox; };
enum State { STOPPED, WALKING };
enum { MAX = 64 };
```

```c
//C gerado
typedef struct sim_Vec2 { f32 x, y; } sim_Vec2;
struct sim_No { struct sim_No *prox; };
enum sim_State { sim_State_STOPPED, sim_State_WALKING };
enum { sim_MAX = 64 };
```

A **constante de enum** é a que mais importa: ela vive no espaço de identificadores comuns e é definida no header. Sem prefixo, dois módulos que declarem `STOPPED` não podem ser importados pelo mesmo terceiro.

E ela leva **dois** níveis, não um: o escopo da constante é o enum, não o módulo (linguagem §4.2), de modo que `State.STOPPED` e `Task.STOPPED` do mesmo módulo não se encontram no `.h`. Enum sem nome não tem escopo próprio e fica com o prefixo do módulo, como qualquer outro símbolo.

- Vale só em escopo de arquivo. `enum` em escopo de bloco não aparece em header nenhum e não é tocado.
- Nada de novo é exigido do parser além de **ler o corpo do `enum`** para colher os nomes. A reescrita já existe — é a mesma que troca `Point` por `geom_Point` —, e por isso `WALKING = STOPPED + 1` sai certo sem tratamento especial: dentro do corpo os dois nomes são nus e os dois são reescritos para o símbolo escopado.
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
instância nova é gerada (linguagem §2.2). Só o qualificador do **argumento**
manga, porque só ele muda o tipo do elemento.

`ref` não entra na mangling: `buffer i32 *ref` e `buffer i32 *` são a mesma instância.

`restrict` não aparece em mangling nenhum: ele não é qualificador de contêiner em keel (linguagem §4.2), e em declarador C comum atravessa verbatim, sem instância a nomear. Já o argumento de `dim` **entra**, e tem que entrar: `tensor(2) f32` e `tensor(3) f32` são tipos diferentes, com structs de tamanhos diferentes. Módulo sem `dim` não tem numeral a carregar — `mat_matrix_f32` (linguagem §4.9).

**O que entra é o valor, e nunca a grafia.** `tensor(3) f16` e `tensor(DIM) f16`, com `DIM` valendo 3, dão o mesmo `tens_tensor_3_f16` — mesmo nome, mesma struct, mesmo header, byte a byte. É o que a linguagem §4.3 exige, e é o que mantém o §7.1 de pé: com a grafia no nome, dois módulos que declarassem `DIM` com valores diferentes pediriam o mesmo arquivo com conteúdos diferentes, e o header de instância deixaria de ser função das entradas. **O backend não resolve o símbolo** — recebe o valor já resolvido pela linguagem e o escreve.

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
> correção move um número que outras seções orçavam (linguagem §4.3, e a regra de encurtamento do §2.1).

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
cblas_dgemv(..., ptr(xs), ...);      /* the C library expects double * */
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
typedef _Float32 f32;      /* NO */
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
/* keel/f16.h — included only when the module names f16 */
#include "keel.type.h"

#if !defined(__FLT16_MANT_DIG__)
#  error "keel: f16 requires _Float16, which this target does not offer"
#endif
typedef _Float16 f16;
static_assert(sizeof(f16) == 2, "keel: f16 requires 16-bit binary16 on this target");
```

```c
/* keel/bf16.h — included only when the module names bf16 */
#include "keel.type.h"

#if !defined(__BFLT16_MANT_DIG__) && !defined(__ARM_BF16_FORMAT_ALTERNATIVE)
#  error "keel: bf16 requires __bf16, which this target does not offer"
#endif
typedef __bf16 bf16;
static_assert(sizeof(bf16) == 2, "keel: bf16 requires 16 bits on this target");
```

**Um header por formato, e é por isso que a guarda não incomoda ninguém.** Se os
dois `typedef` morassem no prelúdio (§4.2), todo projeto num alvo sem binary16
deixaria de compilar por um tipo que não usa. E se morassem no mesmo arquivo, um
módulo que só nomeia `f16` pararia pela guarda de `bf16` — que é o caso comum,
porque `_Float16` e `__bf16` não chegam juntos aos alvos. **Cada arquivo é
incluído só quando o módulo nomeia o seu tipo**, e o custo recai exatamente sobre
quem pediu. É a mesma mecânica de `keel/keel_arena.h`, e pela mesma razão.

O diagnóstico `specific-format-unavailable` é a versão do cgen dessa parada, para o caso em que o alvo é
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

### 4.1 Um `.c` por invocação, e nenhum alvo além dele

**O que a invocação compila** — o `.k` da linha de comando, ou a instância pedida
por `--instance` (ferramenta §4.3) — produz sempre **três** arquivos: dois
headers e um `.c`, ainda que o `.c` contenha apenas o `#include` do próprio `.h`
e que um header contenha apenas as guardas. É o que permite ao build usar uma
regra de padrão `%.o: %.c` sem exceção. Não existe alvo sem fonte correspondente
para o build descobrir por glob, manifesto ou arquivo agregador.

**O que a invocação só alcança** — os módulos importados, inclusive o `keel`
implícito, e as instâncias que os usos pedem — produz **os dois headers, e
nenhum `.c`**. Headers não compilam e não geram objeto, então não são alvos — é
justamente por isso que a invariante se sustenta. O `.c` de um módulo importado é
escrito pela invocação **dele**, na regra que o build já tem para ele.

> **Todo corpo fora de linha termina no `.c` da invocação, e em nenhum outro.** O
> do próprio módulo, e o das instâncias que ele declara com `instance` (§4.4).

O corte dos dois headers é do §4.3.2, e a razão dele é evitar ciclo de inclusão.

```text
app/cfg.k  →  app/app_cfg.type.h                tipos
              app/app_cfg.h                     protótipos e corpos inline → o que se inclui para usar
              app/app_cfg.c                     corpos fora de linha       → app_cfg.o
              keel/keel_buffer_f32.type.h  .h   instância                  → (nenhum objeto)
```

#### O nome do arquivo é o símbolo; o diretório é o módulo

Os componentes-**pai** do nome do módulo viram diretórios; o nome do arquivo é o
símbolo manglado do §2.1, com o argumento quando há:

| Declarado | Diretório | Arquivo |
| --- | --- | --- |
| `module keel;` | — | `keel.h` |
| `module keel.arena;` | `keel/` | `keel_arena.h` |
| `module net.http;` | `net/` | `net_http.h` |
| `module lst;` | — | `lst.h` |
| instância `buffer i32` de `keel.buffer` | `keel/` | `keel_buffer_i32.h` |
| instância `stack i32` de `stack` | — | `pilha_stack_i32.h` |

**A regra é uma só para módulo e para instância**, e é o que ela compra que a
justifica ([justificativa: o nome do arquivo gerado é o símbolo](keel-rationale.md#o-nome-do-arquivo-gerado-é-o-símbolo)):
o header de um tipo passa a ser **função do nome do tipo**, sem que
se precise saber de qual dos dois ele veio. `keel_arena` mora em
`keel_arena.h` como `keel_buffer_i32` mora em `keel_buffer_i32.h`. Quem precisa
emitir um `#include` a partir de um nome de tipo — o próprio cgen, ao resolver a
regras 2 e 3 do §4.3.2 — concatena, em vez de consultar uma tabela.

O preço é a repetição em `net/net_http.h`, e ela é deliberada: o diretório existe
para agrupar, o nome para identificar, e um não substitui o outro. Sem ela,
`#include "net/http.h"` traria `net_http_get` de um arquivo cujo nome não o
nomeia, e a derivação acima deixaria de existir.

**O caminho do fonte é outra regra, e a assimetria é de propósito.** O `.k` mora
no caminho do módulo — `module app.cfg;` em `app/cfg.k` —, sob pena de
`module-path-mismatch` (linguagem §4.1). Ele pode se dar a esse luxo porque
**declara o próprio nome na primeira linha**: o caminho é redundante e serve de
conferência. O header gerado não declara nada — o nome do arquivo é o único
identificador que ele tem, e por isso carrega o símbolo inteiro.

**O que vai em cada arquivo:**

O `.type.h` recebe os `import_c`, as definições de tipo, as declarações adiantadas que os campos por ponteiro exigem e os `constexpr` de módulo.
O `.h` recebe os protótipos de função e as declarações `extern` de variáveis, depois os corpos `inline`, e traz o `.type.h` por inclusão: é o arquivo que se inclui para usar o módulo.
O `.c` recebe os corpos fora de linha, as definições de variáveis e os blocos `extern_c`.

Quem decide em qual deles cada declaração vai é `pub`/`priv` (linguagem §4.1) **e a camada** (§4.3.1). A tabela do posicionamento é deste documento, porque é ela que fala de arquivo:

| Escrita no `.k` | `.type.h` | `.h` | `.c` |
| --- | --- | --- | --- |
| função (`pub` implícito) | — | protótipo | corpo |
| função `pub inline` | — | protótipo, e depois `static inline` + corpo | — |
| variável (`pub` implícito) | — | `extern T var;` | `T var = ...;` |
| `constexpr` de módulo | definição (§9.2) | — | — |
| tipo (`pub` implícito) | definição | — | — |
| função `priv` | — | — | corpo, com `static` |
| variável `priv` | — | — | definição, com `static` |
| tipo `priv` | definição | — | — |
| `import_c` | `#include` | — | — |
| `extern_c { … }` | — | — | o conteúdo, intacto |

O mapeamento nem é monotônico: `pub inline` sai como `static inline` no `.h`. Público no keel virou `static` no C.

**Tipo `priv` vai para o `.type.h` como o público**, e não para o `.c`: o `.type.h` é L1, e um tipo privado que aparecesse só no `.c` não poderia ser campo de nada que o módulo declare. A privacidade é do keel, que recusa o nome fora do módulo; o arquivo não é o mecanismo.

Função `pub inline` leva protótipo **e** corpo no `.h`, nessa ordem, e não só o corpo. A seção de protótipos vem antes de qualquer corpo, e é o que deixa os corpos do módulo se chamarem em qualquer ordem, inclusive por recursão mútua (regra 2 do §4.3.2).

Variável pública **nunca** vai para o `.h` como `static`. Isso compila e linka, mas produz uma cópia independente por unidade de tradução: um módulo escreve, outro lê e não enxerga nada, sem erro nem aviso.

`const` não precisa de exceção: `pub const float PI = 3.14f;` sai como `extern const float PI;` no `.h` e a definição no `.c` — símbolo único, sem duplicação.

**`import_c` vai para o `.type.h`, a camada mais baixa**, porque é o único lugar de onde todas as outras o enxergam, e um tipo do módulo pode precisar do header: `pub struct Log { FILE *f; };` não compila se `<stdio.h>` chega depois do `.type.h`, e `FILE` não admite declaração adiantada. O header é de fora, não é gerado, e a regra 1 do §4.3.2 não fala dele. O preço é um contrato que o keel não verifica, porque não abre o header (linguagem §4.1): **o header de um `import_c` não inclui gerado do próprio módulo**. Se incluir, o ciclo passa pela guarda com o módulo pela metade, e a falha é do compilador C, pelo princípio 3 — é o mesmo ciclo que o C já tem entre dois headers que se incluem.

**`extern_c` vai para o `.c`, inteiro.** O conteúdo é opaco e pode misturar tipo com corpo de função; num header, os corpos dariam definição múltipla, e o keel não tem como separar um do outro sem entender o C. Um tipo declarado ali é, portanto, privado do `.c`. **Tipo C que atravessa a interface mora num header, e entra por `import_c`**:

```keel
import_c "legacy.h";            // typedef struct legacy legacy_t;  → .type.h
pub void use(legacy_t *x);      // the type arrives before the prototype

extern_c {                      // → .c: private C, no mangling
    static legacy_t cache;
    void legacy_init(void) { /* ... */ }
}
```

### 4.2 O prelúdio: `keel.k`

> **Licença.** A Base keel — `base/` no repositório, e o que dela é distribuído sob
> `lib/base` (ferramenta §8) — é fonte **copiado**, não vinculado, para dentro do C
> de todo projeto: por isso ela sai da GPLv3 comum e ganha uma exceção que
> isenta esse texto gerado. Ver [`LICENSE.md`](LICENSE.md),
> [`LICENSE.pt.md`](LICENSE.pt.md) e o argumento em
> [rationale](keel-rationale.md#por-que-a-base-é-copyleft-com-exceção-e-não-gpl-simples-nem-mit).
> Esta nota não é normativa.

`keel` é `module keel;` (linguagem §5.1): não declara verbo nenhum, só os
nomes de tipo primitivos — `i8`..`u64`, `f32`, `f64`. **É um módulo como
qualquer outro**, e o único privilégio dele é o import implícito (linguagem
§4.1). É fonte keel de verdade, `keel.k`, e mora na raiz da base porque o nome
do módulo é o caminho; pela mesma regra do §4.1, os gerados saem na raiz do
destino, sem diretório.

Importado, gera os dois headers do §4.3.2, um deles quase vazio:
`keel.type.h` (L0+L1, os `typedef` e as guardas C que o backend deve porque o
alvo não tem os tipos nativamente) e `keel.h` (L2+L3, só o `#include` do
`.type.h`: não há protótipo nem corpo, porque não há verbo). Compilado diretamente — `cgen keel.k`,
o que só acontece no desenvolvimento da própria base — gera também `keel.c`,
com o `#include` do `.h` e nada mais.

```c
/* keel.type.h — included at the top of every module .type.h */
#include <stdint.h>
#include <stddef.h>
#include <float.h>
typedef int8_t  i8;   typedef uint8_t  u8;
typedef int16_t i16;  typedef uint16_t u16;
typedef int32_t i32;  typedef uint32_t u32;
typedef int64_t i64;  typedef uint64_t u64;
typedef float   f32;  typedef double   f64;

static_assert(FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128
              && sizeof(f32) == 4, "keel: f32 requires IEEE 754 binary32 on this target");
static_assert(FLT_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024
              && sizeof(f64) == 8, "keel: f64 requires IEEE 754 binary64 on this target");
```

**Gerado, mas não variável.** `keel.type.h` não é hand-maintained fora do
pipeline, é a saída de compilar `keel.k` — só que, como `keel.k` nunca muda de
projeto para projeto e o backend nunca insere `#if` de versão (§9.1), o
resultado é determinístico: mesmo conteúdo, byte a byte, toda vez, para um
dado perfil. Essa estabilidade é o que antes se chamava de "fixo"; a diferença
é que agora há um fonte `.k` real do qual esse header é, de fato, função —
não uma exceção às regras de artefato, é o caso mais simples delas.

**Os asserts provam o formato, e não o tamanho.** `sizeof == 4` sozinho não
distingue binary32 de um float de 32 bits que não é IEEE — e o §3.1 promete o
formato, não a largura. Radix, dígitos de mantissa e expoente máximo fixam
binary32 e binary64 sem depender de `__STDC_IEC_559__`, que é opcional e que
vários alvos com IEEE de verdade não definem por causa de exceções e
arredondamento. Este é o **único lugar onde `<stdint.h>` e `<float.h>` aparecem**:
todo o resto do C gerado usa a grafia keel.

**Só a camada zero vem do prelúdio, e é a linguagem que decide isso.** `keel` é
o único módulo implícito (linguagem §4.1): `arena`, `buffer`, `slice`, `range`,
`tagged`, `outcome`, `corot`, `routine` e `parallel` **se importam**, e por isso os
seus headers chegam pela regra geral de import — o `.h` do módulo, mais os headers de instância que ele origina, cada um na camada que a regra de inclusão do §4.3.2 pedir.
`keel/keel_arena.h` continua existindo e continua sendo C comum, mas ele é o `.h` do
módulo `keel.arena`, incluído porque alguém escreveu o `import`, e não porque o
backend o injeta em toda unidade.

A consequência prática é boa: um módulo que não usa arena não vê `keel/keel_arena.h`,
e um projeto que não importe esse módulo pode usar sua própria `arena` — que é
exatamente o que a linguagem comprou ao tirar a base do prelúdio.

Se o projeto já define `u8` ou `i32` com tipo compatível, a redeclaração de `typedef` é legal desde o C11; uma definição conflitante vira erro do compilador C sobre `keel.type.h` — visível, não silencioso.

### 4.3 Headers de instância

Cada instância de modificador gera **dois headers, e nenhum `.c`** — salvo quando
é ela o que a invocação compila, por `--instance` (§4.1):

```plain
keel/keel_buffer_i32.type.h        keel/keel_buffer_i32.h
keel/keel_slice_geom_Point.type.h  keel/keel_slice_geom_Point.h
coll/coll_stack_i32.type.h         coll/coll_stack_i32.h
```

O corte é o do §4.3.2, o mesmo de todo módulo. Include guard derivado do nome
mangled; as funções saem `static inline`; não há objeto de instância nem símbolo
externo.

O header de instância alcança o argumento de uma das duas formas do §4.3.1:
**inclui o `.type.h`** do argumento quando este aparece por valor, e **escreve a
declaração adiantada** quando aparece por ponteiro. Ele é autossuficiente e não
depende de ordem de inclusão.

```keel
//keel
buffer slice char lines;
```

```c
//C gerado
/* keel/keel_buffer_keel_slice_char.type.h */
typedef struct keel_slice_char keel_slice_char;   /* the name is enough: the field is a pointer */

typedef struct keel_buffer_keel_slice_char {
    size_t           cap;
    size_t           len;
    keel_slice_char *ptr;
} keel_buffer_keel_slice_char;
```

O elemento é guardado por ponteiro, então o `.type.h` de fora não inclui o de
dentro. Vale para os aninhamentos comuns — `buffer slice T`, `slice buffer T`,
`buffer outcome T`.

Quem inclui o `.h` de um módulo recebe, por transitividade, os tipos, os
protótipos e os corpos das instâncias que ele usa.

**Quem instancia é quem usa**, não quem declara o tipo argumento. Dois módulos que
usam `buffer i32` geram o mesmo header, byte a byte (§7.1), e a segunda escrita é
no-op.

#### 4.3.1 Camadas de emissão

O que uma declaração emitida pode precisar de um tipo, e o que basta em cada caso:

| Precisa de | Basta | Onde ocorre |
| --- | --- | --- |
| nome | `typedef struct X X;` ou `struct X;` | campo `X *`, parâmetro ou retorno `X *` |
| layout | a definição do agregado | campo por valor, parâmetro ou retorno por valor, `sizeof`, variável local |
| assinatura | o protótipo | chamada |
| corpo | a definição da função | chamada de `static inline`, na mesma unidade de tradução |

Daí as quatro camadas. **A camada determina o que a declaração pode alcançar:**

| Camada | Conteúdo | Alcança |
| --- | --- | --- |
| L0 nome | declaração adiantada | — |
| L1 layout | `struct`, `union`, `enum`, `typedef`, `constexpr` de módulo | L0, L1 |
| L2 assinatura | protótipos, `extern` | L0, L1 |
| L3 corpo | corpos de função, definições de variável | L0, L1, L2, L3 |

Toda aresta desce de camada, salvo L1→L1 e L3→L3.

**L1→L1 é contenção de layout**: campo por valor. Campo por ponteiro é L1→L0.
O grafo de L1→L1 tem de ser acíclico; um ciclo é tipo de tamanho infinito.

```keel
//keel — finite L1→L1: the outer field is by value, the inner one by pointer
outcome buffer outcome i32 r;
```

```plain
keel_outcome_keel_buffer_keel_outcome_i32  ──L1→L1──▶  keel_buffer_keel_outcome_i32  ──L1→L0──▶  keel_outcome_i32
          { i32 code; <buffer> value; }                  { cap, len; <outcome> *ptr; }
```

```keel
//keel — cyclic L1→L1: layout-cycle
module list;
import keel.outcome as outcome types;
pub struct No { i32 v; outcome No prox; };
```

```plain
lista_No ──L1→L1──▶ keel_outcome_lista_No ──L1→L1──▶ lista_No
```

Quando a cadeia é toda do fonte, quem diagnostica é o compilador C (princípio 3).
Quando ela **atravessa instância de modificador**, como acima, keel a diagnostica:
`layout-cycle`, `error`, com a cadeia na mensagem (linguagem §4.3).

**L3→L3 é chamada entre corpos**, e pode ser cíclica dentro de um módulo: é a
recursão mútua. Entre arquivos a linguagem não a produz — o grafo de import é
acíclico, e a instância não chama o módulo do argumento, porque o parâmetro de
tipo é opaco (linguagem §4.3). Em qualquer caso, basta que o protótipo da função
chamada preceda o corpo que a chama na unidade de tradução, e o §4.3.2 fixa a
ordem que o garante.

```keel
//keel — cyclic L3→L3 inside the module
pub inline bool par(u32 n)   { return n == 0 ? true  : odd(n - 1); }
pub inline bool odd(u32 n) { return n == 0 ? false : par(n - 1); }
```

As duas arestas que atravessam instância podem correr em sentidos opostos entre
o mesmo par, desde que em camadas diferentes. É o caso de `buffer T` com
`outcome buffer T`:

```plain
keel_outcome_keel_buffer_T  ──L1→L1──▶  keel_buffer_T          campo v, por valor
keel_buffer_T               ──L3→L3──▶  keel_outcome_keel_buffer_T   clone chama outcome.win
```

`constexpr` de módulo é L1: pode dimensionar um campo — `i32 itens[MAX];` — e
ser argumento de `dim` (linguagem §4.3).

**L0 é emitido por valor, não por referência.** O arquivo que precisa apenas do
nome escreve a declaração adiantada ele mesmo, em vez de incluir o arquivo que a
possui. `typedef` idêntico repetido é legal desde o C11 (§4.2).

Para que L0 seja sempre escrevível, **todo agregado gerado leva tag igual ao nome
manglado**, inclusive os que o usuário escreveu sem tag:

```keel
//keel
typedef struct { f32 x, y; } Vec2;
```

```c
//C gerado
typedef struct sim_Vec2 { f32 x, y; } sim_Vec2;
```

`enum` não tem L0: seu layout não depende de tipo nenhum.

[Justificativa: dois headers, tipo e uso](keel-rationale.md#dois-headers-tipo-e-uso).

#### 4.3.2 Os três artefatos

**Todo módulo e toda instância geram dois headers; o que a invocação compila gera
também o `.c`** (§4.1). A fronteira de arquivo fica entre L1 e o resto:

| Arquivo | Camada | Conteúdo |
| --- | --- | --- |
| `modulo.type.h` | L0 + L1 | os `import_c`, as declarações adiantadas de que os campos `X *` precisam, as definições dos tipos concretos e os `constexpr` de módulo |
| `modulo.h` | L2 + L3 | os protótipos e as declarações `extern`, e depois os corpos `static inline`; inclui o próprio `.type.h` |
| `modulo.c` | L3 | os corpos fora de linha e as definições de variável |

```text
modulo.k  →  modulo.type.h                     tipos
             modulo.h                          protótipos e corpos inline → o que se inclui para usar
             modulo.c                          corpos fora de linha       → modulo.o
             keel/keel_buffer_f32.type.h  .h   instância                  → (nenhum objeto)
```

**`modulo.h` é o único include de uso.** É o que o `.c` do usuário escreve. O
`.type.h` existe para os headers gerados; nenhum dos dois é alvo de compilação
(§4.1).

Três regras de inclusão, e nada além delas:

1. **`.type.h` inclui apenas `.type.h`** entre os gerados, e só do tipo que
   aparece **por valor**. Tipo por ponteiro leva declaração adiantada escrita no
   próprio arquivo. Os `import_c` são de fora (§4.1).
2. **`.h` tem quatro seções, nesta ordem:**
   1. o `#include` do próprio `.type.h` e do `.type.h` de todo tipo que apareça
      por valor nas suas assinaturas ou nos seus corpos;
   2. os protótipos e as declarações `extern`;
   3. o `#include` do `.h` de cada módulo ou instância cujas funções os corpos
      chamam;
   4. os corpos.
3. **`.c` inclui o próprio `.h` e o `.h` de cada módulo ou instância que ele usa.**

O `.type.h` nunca inclui `.h`, e nada inclui `.c`.

O caso de borda do §4.3.1, `outcome buffer outcome i32`, sai assim — abreviado:
sem o diretório `keel/`, só os verbos `get` e `clone`, e sem a declaração
adiantada de `keel_arena`:

```c
/* keel_outcome_i32.type.h */
typedef struct keel_outcome_i32 { i32 code; i32 value; } keel_outcome_i32;

/* keel_buffer_keel_outcome_i32.type.h — rule 1: by pointer, L0 */
typedef struct keel_outcome_i32 keel_outcome_i32;
typedef struct keel_buffer_keel_outcome_i32 {
    size_t cap, len; keel_outcome_i32 *ptr;
} keel_buffer_keel_outcome_i32;

/* keel_outcome_keel_buffer_keel_outcome_i32.type.h — rule 1: by value */
#include "keel_buffer_keel_outcome_i32.type.h"
typedef struct keel_outcome_keel_buffer_keel_outcome_i32 {
    i32 code; keel_buffer_keel_outcome_i32 value;
} keel_outcome_keel_buffer_keel_outcome_i32;

/* keel_buffer_keel_outcome_i32.h — regra 2 */
#include "keel_buffer_keel_outcome_i32.type.h"                  /* 2.1 */
#include "keel_outcome_i32.type.h"                              /*     get devolve T   */
#include "keel_outcome_keel_buffer_keel_outcome_i32.type.h"     /*     clone devolve   */
static inline keel_outcome_i32                                  /* 2.2 */
    keel_buffer_keel_outcome_i32_get(keel_buffer_keel_outcome_i32 *b, size_t i);
static inline keel_outcome_keel_buffer_keel_outcome_i32
    keel_buffer_keel_outcome_i32_clone(keel_arena *a, keel_buffer_keel_outcome_i32 *b);
#include "keel_outcome_keel_buffer_keel_outcome_i32.h"          /* 2.3 clone chama win */
/* 2.4 corpos */

/* keel_outcome_keel_buffer_keel_outcome_i32.h — does not include keel_buffer_…h */
#include "keel_outcome_keel_buffer_keel_outcome_i32.type.h"
/* prototypes, bodies */
```

Entrando por qualquer um dos `.h`, os tipos chegam completos antes dos
protótipos, e os protótipos antes dos corpos.

A linguagem não produz ciclo L3→L3 entre arquivos (§4.3.1), mas a ordem não
depende disso. Havendo um, a seção 2.2 vem antes da 2.3 nos dois lados, então
cada arquivo declara as próprias funções antes de alcançar o outro:

```c
/* foo.h */                              /* sorted_foo.h */
#include "foo.type.h"                    #include "sorted_foo.type.h"
#include "sorted_foo.type.h"             #include "foo.type.h"
int  foo_cmp(foo, foo);   /* 2.2 */      void sorted_foo_sort(sorted_foo *);  /* 2.2 */
#include "sorted_foo.h"   /* 2.3 */      #include "foo.h"                     /* 2.3 */
/* bodies: they call sorted_foo_sort */     /* bodies: they call foo_cmp */
```

Entrando por `foo.h`, o `foo.h` de volta é pulado pela guarda, e `foo_cmp` já
está declarado quando o corpo de `sorted_foo_sort` o chama. Entrando por
`sorted_foo.h`, o mesmo, com os papéis trocados.

O programador nunca escreve esses `#include`: o cgen os deriva das chamadas que
já resolveu (linguagem §4.4). Em `extern_c` o cgen não olha nomes; quem escrever
`keel_buffer_i32_push` lá dentro não ganha o include (§4.5).

**L3 se parte por ligação:**

> `modulo.h` recebe os corpos que precisam existir uma vez **por unidade de
> tradução** — `static inline`. `modulo.c` recebe os que precisam existir uma vez
> **no programa**.

O `.h` de instância é, assim, o `.c` da instância, replicável, e não precisa de
regra de build. **Isso não dispensa o §4.4**: função `pub` sem `inline` num
módulo genérico sai `extern`, e `instance` é a única forma de dar dono ao corpo.

**Instância sobre o tipo em definição** sai como qualquer outra. A linguagem
permite `slice Val` dentro de `struct Val` (linguagem §4.3):

```keel
//keel
pub struct Val { Kind tag; slice Val items; };
```

```c
//C gerado
/* keel/keel_slice_val_Val.type.h */
struct val_Val;                                 /* the name is enough: the field is a pointer */

typedef struct keel_slice_val_Val {
    size_t           len;
    struct val_Val  *ptr;
} keel_slice_val_Val;

/* val.type.h */
#include "keel/keel_slice_val_Val.type.h"       /* items is by value */

struct val_Val { val_Kind tag; keel_slice_val_Val items; };

/* keel/keel_slice_val_Val.h */
#include "keel/keel_slice_val_Val.type.h"
#include "val.type.h"                           /* get returns Val by value */
```

O `.h` da instância alcança `Val` pelo `val.type.h`, e não pelo `val.h`.

O backend não depende de declaração adiantada no fonte: com a tag sintética do
§4.3.1, emite L0 para todo agregado. Quem escrever `slice Val` sem nunca definir
`Val` leva erro de tipo incompleto do compilador C no ponto de uso (princípio 3).

### 4.4 Definição fora de linha de instância

Função `pub` **sem** `inline` num módulo genérico sai `extern` no `.h` da instância, e o corpo precisa de um dono. `instance` é a declaração que assume essa posse (linguagem §4.3):

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
/* extern definitions of each instance's non-inline functions */
```

É a disciplina de definição única do C, explicitada: o header da instância é o `extern int g;`, e `instance` é o `int g;`. E é `instances.k` — um fonte que o usuário escreveu — que dá ao build o `.o` e a regra, o que mantém a invariante do §4.1 intacta.

- **Não altera o header da instância.** Ele é função do genérico e do argumento, e é byte a byte idêntico para todo mundo. Por isso **o modo é propriedade do genérico, não do uso**: `pub inline` gera header-only, `pub` gera `extern`, e nenhum dos dois depende de existir ou não um `instances.k` em algum lugar da árvore.
- **Instância duplicada é erro de link.** Dois módulos declarando a mesma `instance` dão símbolo duplicado, e o cgen compila um módulo por vez — genuinamente não vê. É a mesma falha de definir o mesmo global em dois `.c`, e o idioma é um `instances.k` por projeto.
- **Instância faltando também é erro de link.** Usar um genérico não-inline sem que ninguém tenha declarado a `instance` falha em `coll_stack_i32_length`. É a taxa ergonômica que justifica o default ser inline.

**Instância pré-compilada.** A posse também pode vir da invocação:
`cgen -c --instance "coll.stack i32"` compila a instância como o que a invocação
compila (ferramenta §4.3), e o corpo fora de linha vai para `coll/coll_stack_i32.c`,
que o build compila pela mesma `%.o: %.c`. É `instance` escrita na linha de
comando, e as duas falhas de link acima valem igual para ela.

**A base não tem definição fora de linha.** Os módulos dela entram no programa só
por import, e módulo importado não gera `.c` (§4.1): um corpo fora de linha da
base não teria `.c` onde morar nem regra que o compilasse. Pré-declarar
`instance` dentro do próprio módulo genérico funciona para genérico do usuário e
**não** funciona para a base, que é exatamente o conjunto de módulos para o qual o
usuário não escreveu regra nenhuma. Módulo da base é inteiramente `pub inline`.

#### 4.4.1 Declaração que não menciona parâmetro

A linguagem emite **uma vez, no módulo**, toda declaração de um genérico que não
mencione **nem parâmetro nem modificador** do módulo (linguagem §4.3). Ela não
pertence a instância nenhuma, e por isso não vai para o header de instância:

```keel
module keel.outcome type T;
pub modifier outcome { i32 code; T value; }
pub constexpr i32 OK   = 0;
pub constexpr i32 NONE = INT32_MIN;
```

```c
/* keel/keel_outcome.type.h — from the module, once; under C23 */
constexpr i32 keel_outcome_OK   = 0;
constexpr i32 keel_outcome_NONE = (-2147483647 - 1);

/* keel/keel_outcome_i32.type.h — per instance */
typedef struct keel_outcome_i32 { i32 code; i32 value; } keel_outcome_i32;
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
- **A instância alcança o que é do genérico pelas regras do §4.3.2**, e nunca o
  contrário: a constante e o cursor (§5.11) são L1 e moram no `.type.h` do
  genérico, e o `.h` da instância inclui esse `.type.h` quando os corpos usam a
  constante ou uma assinatura usa o cursor por valor. A constante é do
  genérico; a struct é da instância, e mora no `.type.h` dela.

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

`<modifier> <argument>` é um especificador de tipo, e o lowering é troca de nome, **local**. O backend nunca precisa da gramática de declaradores do C:

```keel
buffer i32 x;
buffer i32 *x;
buffer i32 x[10];
buffer i32 (*f)(void);
void (*g)(slice char s);
typedef slice u8 (*Reader)(i32);
```

```c
keel_buffer_i32 x;
keel_buffer_i32 *x;
keel_buffer_i32 x[10];
keel_buffer_i32 (*f)(void);
void (*g)(keel_slice_char s);
typedef keel_slice_u8 (*Reader)(i32);
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

`spec-c` (linguagem §2.2) é **copiado verbatim, na posição em que foi
escrito**, e não participa de nada mais: não entra no mangling (§2.2), não muda a
instância, não é reordenado.

```keel
alignas(64) array f32 canal[1024];
static      buffer i32 pool;
_Atomic     buffer u32 shared;
buffer _Atomic u32 counters;
[[maybe_unused]] slice char s;
```

```c
alignas(64) f32 canal[1024];
static      keel_buffer_i32 pool;
_Atomic     keel_buffer_u32 shared;
keel_buffer_atomic_u32 counters;
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

static inline keel_buffer_i32 keel_buffer_i32_from(i32 *p, size_t n) {
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

A chamada de builtin é reescrita para a função mangled correspondente. **A forma do parâmetro é o bit `byref`** (linguagem §4.3, com a adaptação da §4.4): instância `byref` recebe o **endereço** do contêiner; instância que não é — `slice`, `view`, `range` — recebe **cópia**. É por isso que `as_slice` devolve `keel_slice_i32` e `keel_slice_i32_length` o consome direto, sem `&`:

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
keel_slice_char_length(*keel_buffer_slice_char_ptr(&lines, 3))
keel_buffer_i32_push1(keel_buffer_buffer_i32_ptr(&grid, 3), 42)
*keel_buffer_i32_ptr(keel_buffer_buffer_i32_ptr(&grid, 3), 7) = 5
```

O par `&*` colapsa na geração; não sai `&*` no `.c`.

```keel
array i32 v[2,3,4];
v[1,2,3] = 0;
```

```c
i32 v[2][3][4];
v[1][2][3] = 0;
```

Sob `--checks`, cada índice escrito sobre `array` é precedido de um `assert`
contra a dimensão declarada correspondente — é o `array-index-out-of-bounds`.
O número vem da tabela, e não de `sizeof`: em parâmetro multidimensional o
`sizeof` não está disponível (§5.2), e nas dimensões de índice 1 em diante o
número declarado **é** o tipo C emitido. A dimensão 0 de um parâmetro é a
exceção, e é o ponto a entender: `[static d0]` não é verificado pelo C, então
o `assert` ali afirma o contrato declarado, não a extensão recebida. Ele nunca
acusa falso — ultrapassar o `d0` escrito é defeito qualquer que seja o vetor
que chegou —, mas também não alcança o chamador que entregou menos do que
prometeu. Esse é pego do lado da chamada, na tradução, pelo
`array-argument-wrong-dimension` (linguagem §4.2).

Quando índice e dimensão são ambos decimais conhecidos, não há `assert`: a
tradução já recusou, e é o `array-index-above-dimension` (linguagem §4.5).

#### 5.3.1 Açúcar sobre modificador com `dim`

Quando o modificador declara `dim N` (linguagem §4.3), a emissão do açúcar
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
   por aridade (linguagem §4.3), e o açúcar da §5.3 o alcança pela regra
   geral: nenhum literal composto é emitido, e a ramificação desta subseção nem é
   consultada.

**O `for` do acessor tem limite constante depois da substituição**, então o
compilador C o desenrola e o literal desaparece por SROA. Vale conferir uma vez,
porque é a premissa do desenho:

```c
/* -O2, x86-64: the body of _ptr3 collapses into */
t->ptr + idx0 * t->steps[0] + idx1 * t->steps[1] + idx2 * t->steps[2]
```

Em `-O0` não colapsa: o vetor é escrito na pilha e o laço roda. É o custo
declarado na linguagem §4.3, e não há mitigação de backend para ele — nem
deveria haver, porque a alternativa seria o backend gerar o que a linguagem
decidiu não gerar.

### 5.4 `arena`

A arena é o `.h` do módulo `keel.arena` — C comum, utilizável inclusive a partir de código que não passa pelo keel. Ela chega ao módulo pelo `import`, como qualquer outro (§4.2).

A forma abaixo é a emissão de `keel/arena.k`, e é dele que ela sai — nomes,
campos e corpo (§7.2). O que este documento fixa são as quatro propriedades
listadas a seguir, não a grafia.

```c
/* keel/keel_arena — the typedef in the .type.h, the bodies in the .h */
typedef struct keel_arena {
    size_t  top;
    size_t  cap;
    u8     *ptr;
} keel_arena;

[[nodiscard]] static inline void *keel_arena_alloc(keel_arena *a, size_t n,
                                                   size_t sz, size_t align) {
    if (sz == 0 || n > SIZE_MAX / sz) return NULL;        /* alloc-overflow */
    size_t need = n * sz;
    uintptr_t base = (uintptr_t)(a->ptr + a->top);
    size_t pad = (size_t)(-(uintptr_t)base & (align - 1));
    size_t avail = a->cap - a->top;
    if (pad > avail || need > avail - pad) return NULL;
    a->top += pad + need;
    return a->ptr + a->top - need;
}
```

`alloc` é verbo `pub` como qualquer outro. O programa escreve `arena.alloc(a, T, n)`
e o backend materializa o `sizeof`, o `alignof` e o cast; a função recebe os
números, nunca o tipo.

Quatro coisas nessa função são normativas, e as quatro vêm da linguagem §4.4:

1. **Contagem e tamanho do elemento entram separados**, e o produto é feito aqui. É a forma do `calloc`, e existe para que `n * sizeof(T)` que transborda devolva `NULL` em vez de uma região pequena que o programa acredita ser grande. **É o único ponto do backend que emite essa multiplicação.**
2. **A soma final não transborda**, porque é escrita como `need > avail - pad` e nunca como `pad + need > avail`.
3. **O alinhamento é do endereço, não do deslocamento**, e é o que dispensou o campo `base_align` que esta struct já teve. Alinhar `top` só serviria se `ptr` já estivesse alinhado — e keel não tem como saber se está, porque `alignas(64)` é copiado verbatim e nunca avaliado (linguagem §1.3). Alinhando o endereço que se vai entregar, a base pode estar em qualquer lugar e **toda alocação sai alinhada**, inclusive de tipo sobre-alinhado sobre um `array u8` nu.
4. **`[[nodiscard]]`**, porque o `NULL` é o único canal de falha.

**`uintptr_t` aparece uma vez e não fabrica ponteiro.** Ele calcula o **número** de bytes de padding; o endereço devolvido sai de `a->ptr + a->top`, aritmética de ponteiro dentro do próprio vetor. É a diferença entre uma conversão de valor definida-pela-implementação e uma travessia de ponteiro por inteiro, e só a primeira acontece aqui.

**O `T` nunca chega à biblioteca.** Quem carrega o tipo é o verbo, que não é função e sim reescrita: ele materializa o `sizeof`, o `alignof` e o cast que um humano escreveria à mão.

```keel
arena a;
arena.alloc(a, Particle, 100)
```

```c
keel_arena a = {0};
(sim_Particle *)keel_arena_alloc(&a, 100, sizeof(sim_Particle), alignof(sim_Particle))
```

O `= {0}` na declaração é normativo: é ele que faz arena não inicializada ter `capacity == 0` e todo `alloc` nela falhar limpo — com `cap` zerado, `avail` é zero e a primeira comparação já recusa.

**Os quatro construtores.** Todos devolvem `bool`, verdadeiro quando a capacidade resultante é maior que zero:

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
- **`from_parent` recorta com `keel_arena_alloc(&parent, n, 1, 1)`** — alinhamento 1, porque a filha alinha as próprias alocações. Ela não herda nem precisa herdar alinhamento nenhum.

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
sem tipo declarado, é ele que sai, e a guarda da linguagem §6.3 fica sem uso —
sem que uma linha da §4.4 da linguagem mude. É a mesma separação do §3
entre `f32` **ser** binary32 e `typedef float f32;` ser como este backend
entrega binary32.
- **`from_memory`** toma ponteiro e tamanho crus e assume `max_align_t`, porque a origem é `malloc` ou `mmap`. Região de linker script com alinhamento menor é responsabilidade de quem a declarou, e é o que a regra 3 da linguagem §4.4 já diz.

Com tamanho constante não é preciso VLA. Some o `#ifdef __STDC_NO_VLA__`, some o lowering duplo, some a flag de compilador que o forçaria, e some o modo de falha não testável.

`clone` combina os verbos que já existem: aloca no destino pelo comprimento da origem e copia, e devolve `outcome` pela regra do modo de falha (linguagem §4.4): o que ele devolve é descritor, não ponteiro, e descritor não tem sentinela. Vale igual para `at` (linguagem §5.3), e nenhum dos dois precisa de tratamento próprio no backend: a instância de `outcome` sai pelo §5.14 como qualquer outra.

```keel
outcome slice geom.Point out = slice.clone(a, slice.of(tmp)) else return -1;
```

```c
keel_outcome_keel_slice_geom_Point out = keel_slice_geom_Point_clone(&a, keel_buffer_geom_Point_as_slice(&tmp)); if (keel_outcome_keel_slice_geom_Point_failed(out)) return -1;
```

A função da instância faz `keel_arena_alloc` mais a cópia dos elementos, e devolve `code != OK` quando a alocação falha. **Ela não refaz a checagem de transbordamento**: o comprimento da origem já coube na memória uma vez.

**`at` é a única função de acesso com teste em release.** Ela é total (linguagem §5.3), então o `if` é semântica e não verificação — não depende de `--checks` e não some. Ela devolve **`outcome T`**, e não ponteiro, pela regra de modo de falha da linguagem §4.4; a emissão está no §5.13, que é onde ela mora.

`get`, `set` e `ptr(x,i)` continuam sem teste em release, pela regra do §5.2: elas têm pré-condição, e a verificação de debug existe para revelar quem a violou.

### 5.5 `defer`

> **A definição do `defer` é da linguagem** (linguagem §4.6): cleanup léxico, na saída do escopo, em ordem inversa de registro, sem pilha em runtime. **Como isso acontece em C é deste documento**, e já mudou uma vez — a v0 chegou a depender de extensão do GCC antes de passar à varredura de pontos de saída com injeção por escopo. A definição não mudou junto.

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
    if (!fp) return -1;                     /* nothing to clean up: the defer is not registered yet */
    struct { FILE *fp; } keel__c0 = { fp };  /* [now]: copy at the registration point */

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

Em `return expr;`, `expr` é avaliada para um temporário gerado **antes** de o cleanup rodar, e o temporário é retornado depois. Ele é declarado com o **tipo de retorno escrito na função**, copiado como sequência de token da produção `decl-function` (linguagem §2.2) — `size_t f(…)` dá `{ size_t keel__rv0 = expr; … }`, e o `*` de `char *f(…)` está no declarador, que também está capturado.

> **O `auto` do C23 saiu daqui**, e por isso este lowering é o mesmo nos dois perfis (§9). Enquanto não havia produção de função na gramática, o backend genuinamente não tinha o tipo em lugar nenhum. O que o `auto` acrescentava era a conversão de lvalue, inofensiva num temporário inicializado uma vez e devolvido em seguida. O que ele escondia era o caso do declarador que enterra o nome — `int (*f(void))[10]` não tem corrida contígua de tokens que seja o tipo de retorno —, e esse caso passou a ser o error 120 da linguagem, em vez de um lowering que só funcionava sob C23.

**Captura.** A lista de `[now]` já vem com os tipos escritos (linguagem §4.6), então ela **é** a lista de membros: o backend copia cada entrada verbatim para uma struct local gerada no ponto de registro, e o corpo referencia as cópias.

```keel
defer [now int fd, FILE *out] { report(out, fd); }
```

```c
struct { int fd; FILE *out; } keel__c0 = { fd, out };
/* ... at each exit point: */
app_report(keel__c0.out, keel__c0.fd);
```

Nada é interpretado: a entrada da captura entra como membro sem uma reescrita. **O `typeof_unqual` saiu junto com o `auto`**, pelo mesmo motivo — o tipo agora está escrito, e escrito ele serve aos dois perfis. Sem `[now]`, o corpo referencia as variáveis diretamente e não há struct.

#### 5.5.1 `defer` é a última passagem de fluxo

> **Toda construção que termina um escopo é baixada antes do `defer`.** O que a
> varredura de saídas enxerga são os **cinco terminadores do C** — `return`,
> `break`, `continue`, `goto` e o fim natural do bloco — e nada de keel.

É ordem de emissão, não semântica: a definição continua sendo a da linguagem
§4.6. E é possível porque a redução já é total — todo terminador de keel já cai
num dos cinco, e as seções que o fazem são estas:

| Escrito | Já baixa para | |
| --- | --- | --- |
| `win` · `fail` | `goto keel__end<N>` | §5.9, regra 8 |
| `break` no nível do braço de `match` | `goto keel__m<N>_end` | §5.6, regra 4 |
| cláusula `else` | `if (…failed(x)) return …;` | §5.12 |
| corpo de `foreach` e de `parallel` | `for` comum — o `break` e o `continue` do usuário são C ordinário | §5.7, §5.9 |

**O ganho não é a contagem, é o fecho.** Doze formas viram cinco, mas o que
importa é que as cinco **não podem crescer**: construção nova que termine escopo
tem de baixar para um dos cinco, porque em C não há um sexto. O `defer` nunca
aprende palavra nova, e é a invariante da linguagem §1.3 aplicada para dentro do gerador.

**A ordem também decide um caso, e não só simplifica.** `parallel.interrupted` é
**consulta**, e não saída: devolve `bool` onde foi escrita, sem salto e sem
cleanup. Ela não aparece nesta tabela porque não termina escopo nenhum — e é
por isso que a lista de terminadores encolheu de três verbos de `parallel` para
dois. Quem sai é quem salta.

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
despacho do `match` e o `if` da cláusula `else` são código sem cleanup. A ordem é
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
/* ladder: 3 registrations, 6 exits */
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
sentido não é lowering. A linguagem recusa o sombreamento (linguagem §4.6), e é essa recusa
que autoriza o backend a ter duas formas. **Se ela caísse, só a escada seria
correta**, e o corpo de laço voltaria a custar a variável de ação.

**A condição não precisa de caso especial para nenhuma construção**, e é o §5.5.1
que a torna assim: depois da expansão, o corpo de um `parallel` tem saídas que são
`goto`, então a condição dá falso e o inline sai por dedução — não por uma linha
escrita a respeito de `parallel`. O mesmo vale para `match`. **Escrever a condição
em termos dos cinco terminadores do C é o que a mantém com um caso só.**

### 5.6 `tags` e `match`

O conjunto de tags sai como `enum`; o despacho sai como **salto por `goto` e blocos rotulados**. Nunca como um `switch` com o corpo do usuário dentro dele.

```keel
//keel
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
typedef enum ast_Kind {
    ast_Kind_LIT,            /* 0 — ordinal of the written position */
    ast_Kind_ADD,
    ast_Kind_MUL
} ast_Kind;

void ast_eval(keel_tagged_ast_Kind_ast_Node *n) {
    switch (n->tag) {                                 /* jumps only: nothing of the user's here */
    case ast_Kind_LIT: goto keel__m0_LIT;
    case ast_Kind_ADD: goto keel__m0_ADD;
    case ast_Kind_MUL: goto keel__m0_MUL;
    default:           goto keel__m0_end;
    }
    keel__m0_LIT: {
        ast_leaf(keel_tagged_ast_Kind_ast_Node_value(n));
    }
    goto keel__m0_end;
    keel__m0_ADD:
    keel__m0_MUL: {
        ast_binary(keel_tagged_ast_Kind_ast_Node_value(n));
    }
    keel__m0_end: ;
}
```

Sete regras de emissão, e cada uma existe por um motivo concreto:

1. **O `switch` de despacho contém apenas saltos.** Nenhum código do usuário mora dentro dele, e é isso que faz `break` e `continue` do usuário ligarem ao laço ou `switch` dele. Um `switch` com o corpo dentro reservaria `break` para o despacho.
2. **O rótulo vai antes da chave de abertura**, e as chaves são o escopo do braço que a linguagem §4.9 exige — não um detalhe de emissão. Saltar para um rótulo interno entraria no meio do escopo e **os inicializadores das declarações não rodariam** — legal em C, e o tipo de bug que ninguém encontra. Com o rótulo fora, o salto entra pelo topo e declaração de braço se comporta normalmente.
3. **Cada bloco de braço é seguido de `goto <m>_end`.** É o fim de braço da linguagem §4.9, e é o que elimina fallthrough. No último braço o salto é omitido: ele cairia na linha seguinte, e ninguém escreveria isso à mão (princípio 2). **Rótulos consecutivos empilham antes da mesma chave** — `keel__m0_ADD: keel__m0_MUL: { … }` —, que é como o braço compartilhado da linguagem §4.9 se materializa sem duplicar o corpo e sem reabrir fallthrough.
4. **`break` no nível do braço é `goto <m>_end`**, de qualquer profundidade de escopo dentro do braço — a razão de o despacho ser por rótulo. Um `break` que pertença a laço ou `switch` escrito pelo usuário dentro do braço **não é reescrito**: quem decide é a estrutura C que o contém, reconhecida pela linguagem §2.2. `return` e os demais pontos de saída pertencem à função e atravessam como sempre, com o cleanup do §5.5.
5. **O operando é lido uma vez, pelo verbo `tag`.** Sobre uma instância de `tagged` isso é o acesso ao campo, e sai como tal; sobre outro tipo que declare `tag` — `corot`, por exemplo — sai a chamada do verbo, e o `switch` é sobre o valor devolvido. O campo da etiqueta é `i32`, e não o `enum`: é o que mantém a largura estável na ABI e o que permite ao `corot` participar sem mudar de representação.
   **Na escrita a assimetria aparece no C:** um parâmetro declarado com o nome do parâmetro `tags` sai com o tipo do `enum` — `void keel_tagged_ast_Kind_ast_Node_mark(… , ast_Kind e)` —, e a atribuição ao campo é a conversão usual de `enum` para `i32`. A verificação de pertinência é da tradução (linguagem §4.3); o C não a faria, porque enum e int se convertem em silêncio.
6. **Não há verbo de transição no despacho.** `tagged.mark(n, MUL)` é chamada comum, e o que keel faz na constante nua é a reescrita de escopo de enum (§2.1). Escrever a etiqueta não redespacha: o `switch` já executou.
7. **`default:` sai sempre**, saltando para o fim. Etiqueta fora de faixa é possível quando o valor vem de memória — `memset`, arquivo, rede. Sob `--checks`, um `assert` o precede, e é o `tag-out-of-range`. Ele não é braço: a exaustividade já foi verificada na tradução, sobre a lista declarada.

**O laço é do usuário, e o backend não o emite.** Um `match` executa um braço por passagem; repetir é `while` escrito no fonte. É a decisão da linguagem §4.9 de não ter opinião sobre a política de avanço, e para o backend significa que não há nada a gerar em volta do despacho.

**Os rótulos levam um contador por função** — `keel__m<N>_<TAG>` —, e não o nome da construção, porque `match` não tem nome no fonte. O contador reinicia por função (`ferramenta §6.1`), então inserir um `match` antes de outro renomeia os rótulos dos seguintes **dentro daquela função**. Isso não alcança o `.h` nem símbolo de link: o custo é o `.c` daquela unidade diferir, e ele já ia diferir porque a função foi editada.

**O `enum` vem da declaração `tags`**, e não do corpo do `match`. `pub` o põe no `.h`, `priv` no `.c`, pela regra normal de posicionamento (§4.1). O nome é o do §2.1 — `M_<conjunto>`, com as constantes `M_<conjunto>_<tag>`, **exatamente como qualquer enum nomeado do módulo**. **A ordem das constantes vem da lista declarada**, e um valor escrito é emitido literalmente: é a diferença entre um valor de tag que é contrato e um que é consequência da ordem de edição, e importa porque este enum atravessa o `.h` e pode estar gravado em memória, arquivo ou rede — a mesma razão de o `default:` existir.

**Mapeamento de linhas.** O despacho é a maior região injetada da linguagem: uma linha de fonte — o `match … {` — vira de três a N+2 linhas de saída. Diverge, e portanto ressincroniza com um `#line` logo depois, uma vez. Do primeiro rótulo em diante o corpo é copiado e volta a mapear 1:1, pela regra 2 do §6.

### 5.7 `foreach` e `apply`

Cabeçalho numa linha, corpo copiado, nenhuma diretiva `#line`.

```keel
foreach (i32 v, size_t i : xs) {
    sum += v * pesos[i];
}
```

```c
{ keel_buffer_i32 *keel__c0 = &xs; size_t keel__n0 = keel_buffer_i32_length(keel__c0); for (size_t i = 0; i < keel__n0; i++) { i32 v = keel_buffer_i32_get(keel__c0, i);
    sum += v * pesos[i];
} }
```

Quatro regras de emissão:

1. **O contêiner vai para um ponteiro temporário e o comprimento para um `size_t`**, ambos antes do `for`. É o que dá a avaliação única da linguagem §4.7, e é o que um humano escreveria ao perceber que `length` seria rechamado.
2. **O binder de índice é a própria variável do laço**, e por isso a linguagem §4.7 o exige sempre: o `for` gerado leva o nome que o usuário escolheu, e a família `keel__i<N>` só sobra no `apply`, que não tem binder para escrever.
3. **Binder por valor gera `get`; por ponteiro gera `ptr` de um índice.** O bloco extra em volta existe para os temporários morrerem no fim, e é o que permite `foreach` aninhado sem colisão de nome.
4. **Tudo até a abertura do corpo cabe numa linha.** É o que faz o corpo mapear 1:1 e dispensa ressincronizar — ao contrário do despacho do `match` (§5.6), que não tem como caber.

### 5.8 Ponto de entrada

A função de entrada de um módulo é função comum e sai manglada como qualquer outra. O `main` do C sai em **unidade separada**, gerada quando a ferramenta recebe `--main <módulo>`:

```c
/* gen/main_net_http.c */
#include "net/net_http.h"
int main(int argc, char **argv) { return net_http_main(argc, argv); }
```

Ela não vai dentro do `.c` do módulo, porque então o conteúdo gerado dependeria da flag de invocação e o critério de timestamp deixaria de significar o que significa.

---

### 5.9 `parallel`

O que sai são **o gestor, um laço de workers e a chamada de partição** — e a propriedade que organiza a seção é que o gestor é o mesmo nos dois lowerings especificados: a diretiva é a única diferença entre eles (§5.9.1).

```keel
//keel
f32 dt = 1.0f / 60.0f;

parallel step ALL (size_t w : 0..4; slice Particle part : ps; (dt)) {
    foreach (Particle *p, size_t i : part) { p->v += dt * p->a; }
}
if (parallel.failed(step)) handle();
```

```c
//C gerado
keel_parallel_control step = { .workers = 4, .target = 0 };
{   keel_buffer_sim_Particle *keel__c0 = &ps;

    #pragma omp parallel for num_threads(4) default(none) \
            shared(keel__c0, step) firstprivate(dt)
    for (size_t w = 0; w < 4; w++) {
        keel_slice_sim_Particle part =
            keel_buffer_sim_Particle_partition(keel__c0, 4, w);
        { keel_slice_sim_Particle *keel__c1 = &part;
          size_t keel__n1 = keel_slice_sim_Particle_length(keel__c1);
          for (size_t i = 0; i < keel__n1; i++) {
            sim_Particle *p = keel_slice_sim_Particle_ptr(keel__c1, i);
            p->v += dt * p->a;
          } }
        keel__end0: ;
    }
}
if (keel_parallel_failed(&step)) sim_handle();
```

Nove regras de emissão:

1. **O símbolo de controle é declarado fora do bloco do gestor**, com o nome escrito no fonte e o tipo `keel_parallel_control`. Tem que ser fora: a linguagem §4.8 o torna legível **depois** do bloco, e um objeto declarado dentro do bloco injetado morreria com ele. Os campos constantes — quantos workers, qual alvo — são escritos no inicializador, e não em atribuições depois.
2. **A diretiva reparte o laço dos workers, e nada mais.** Não há aritmética de faixa no gestor: quem divide é `partition`, chamada uma vez por worker, dentro do corpo do laço e antes do corpo do usuário. É a linguagem §4.8, e é o que permite particionar um contêiner cuja divisão o backend não conhece.
3. **A parte é ligada ao binder por valor**, com o tipo que o módulo declarou como produto de `partition` — aqui `keel_slice_sim_Particle`. Percorrer é do corpo, que é código comum: o `foreach` acima saiu pela regra do §5.7, sobre a parte, e não sobre o todo.
4. **`num_threads(k)` sai sempre**, com o literal. É o que materializa "uma thread por parte"; sem ele o número de partes continuaria certo, mas duas rodariam na mesma thread, o que a linguagem permite e ninguém escreveria à mão.
5. **`default(none)` é obrigatório**, e é ele que cumpre a promessa da linguagem §4.8: um local não listado na captura vira **erro do compilador C nomeando a variável**, na linha do `.k`. Sem a cláusula, ele entraria como `shared` em silêncio e o programa teria corrida.
6. **Os temporários gerados e o símbolo de controle entram nas cláusulas junto com os do usuário.** `default(none)` exige atributo para tudo que a região toca, inclusive `keel__c0` e `step` — e o backend os lista porque escreveu os nomes.
7. **Captura escalar é `firstprivate`; instância `byref` é `shared`.** É a disciplina da linguagem §4.8 traduzida uma para uma. Escrever num escalar capturado já é o error `captured-write` na linguagem, então o backend não precisa de `const` para proibi-lo — e é uma diferença de custo real: com um struct de argumentos, o tipo de cada captura teria que ser escrito, e o backend não o conhece (§5.5).
8. **`win` e `fail` saltam para o fim do corpo do worker.** Viram `goto keel__end<N>`, com o rótulo dentro do bloco estruturado da iteração — nunca `break`, nunca `return`, nunca saída da região. `return` do usuário é o error `return-in-parallel` na linguagem, exatamente porque não teria como sair daqui: sob OpenMP o GCC recusa a região com `invalid branch to/from OpenMP structured block`. Eles são pontos de saída de escopo, então os `defer` registrados no corpo — inclusive em travessias aninhadas — saem antes do salto, pela regra do §5.5.
   **O fim natural não grava nada**, e é por isso que o rótulo pode ficar na última linha do corpo: quem sai por verbo já contabilizou antes de saltar, e quem chega ao fim apenas termina. Contabilizar o fim natural como vitória tornaria `ANY` satisfeito por workers que não acharam nada (linguagem §4.8).
9. **Os contadores e a bandeira vivem no símbolo de controle**, nunca em `static`. Um bloco `parallel` numa função chamada duas vezes começa zerado nas duas, e `static` também tornaria o bloco não reentrante.

**O tipo do símbolo é do módulo, e sai no header dele** — `keel.parallel` é módulo comum (linguagem §5.7), e o gestor apenas escreve nos seus campos:

```c
/* keel/keel_parallel — the typedef in the .type.h, the bodies in the .h */
#include <stdatomic.h>

typedef struct keel_parallel_control {
    _Atomic u32  wins, fails;
    _Atomic bool flag;
    u32          workers, target;    /* target 0 = ALL */
} keel_parallel_control;

static inline bool keel_parallel_interrupted(keel_parallel_control *c) {
    return atomic_load_explicit(&c->flag, memory_order_relaxed);
}
static inline u32  keel_parallel_wins(keel_parallel_control *c) {
    return atomic_load_explicit(&c->wins, memory_order_relaxed);
}
static inline bool keel_parallel_failed(keel_parallel_control *c) {
    return atomic_load_explicit(&c->fails, memory_order_relaxed) > 0;
}
static inline bool keel_parallel_ok(keel_parallel_control *c) {
    return c->target ? keel_parallel_wins(c) >= c->target
                     : atomic_load_explicit(&c->fails, memory_order_relaxed) == 0;
}
```

**Sob política diferente de `ALL`**, a vitória também arma a bandeira, e o alvo sai como literal porque a linguagem exige constante (§4.8):

```c
/* win;  — sob ANY, alvo 1 */
if (atomic_fetch_add_explicit(&step.wins, 1, memory_order_relaxed) + 1 >= 1)
    atomic_store_explicit(&step.flag, true, memory_order_relaxed);
goto keel__end0;

/* fail; */
atomic_fetch_add_explicit(&step.fails, 1, memory_order_relaxed);
goto keel__end0;

/* parallel.interrupted(passo) */
atomic_load_explicit(&step.flag, memory_order_relaxed)
```

A terceira linha é a diferença de modelo em relação à revisão anterior: **`interrupted` é consulta, não saída**. Ela devolve `bool` onde foi escrita, não salta, não grava status e não dispara cleanup; o que o worker faz com a resposta é código dele. Sob `ALL` a bandeira nunca é armada, e a consulta é sempre falsa — legal, e sem caso especial na emissão.

**Os campos `target` e `workers` são escritos uma vez, no inicializador, e lidos pelos verbos.** O caminho de `win` compara com o literal, que o compilador dobra; os verbos `ok` e `wins` da linguagem §5.7 leem os campos, porque são funções do módulo e não têm o literal à mão.

**`relaxed` basta em todos.** A bandeira é dica: vê-la tarde custa iterações, não corretude. O que precisa estar visível ao pai é o que o worker escreveu, e quem sincroniza isso é a barreira implícita no fim do `omp parallel for` — no lowering em série, a própria ordem do programa.

**`<stdatomic.h>` chega com `keel/keel_parallel.type.h`**, e não depende do OpenMP: `_Atomic` é qualificador de linguagem, e gcc e clang o baixam para instrução ou para builtin `__atomic_*`. Não há alvo em que se tenha o compilador e falte o atômico. Em execução serial as operações continuam corretas, sem contenção.

**Mapeamento de linhas.** O gestor é região injetada e diverge, como o despacho do `match`; ressincroniza com um `#line` uma vez, depois dele. Dentro do corpo do worker vale a regra do §5.7.

`apply(T, c, fn, …)` é o mesmo laço com o corpo fixo, e os argumentos de contexto atravessam opacos, na ordem escrita, depois do elemento e do índice:

```keel
apply(i32, xs, doubler);
apply(Node *, p->ns, visit, pool, sb);
```

```c
{ keel_buffer_i32 *keel__c0 = &xs; size_t keel__n0 = keel_buffer_i32_length(keel__c0); for (size_t keel__i0 = 0; keel__i0 < keel__n0; keel__i0++) { m_doubler(keel_buffer_i32_get(keel__c0, keel__i0), keel__i0); } }
{ keel_buffer_ast_Node *keel__c1 = &p->ns; size_t keel__n1 = keel_buffer_ast_Node_length(keel__c1); for (size_t keel__i1 = 0; keel__i1 < keel__n1; keel__i1++) { lift_visit(keel_buffer_ast_Node_ptr(keel__c1, keel__i1), keel__i1, pool, sb); } }
```

`fn` recebe elemento e índice, nessa ordem, e depois o contexto. A chamada é direta: se `fn` é função keel, sai manglada; se vem de `extern_c`, sai como está. O backend **não conta nem examina os argumentos de contexto** — quem confere o parâmetro é o compilador C, pelo princípio 3.

#### 5.9.1 Os dois lowerings, e de quem é o OpenMP

> **OpenMP é obrigação do compilador, pedida por flag.** O keel **emite** a diretiva; não traz agendador, não traz pool de threads, não linka runtime próprio e não tem camada dependente de sistema operacional.

É a frase que decide tudo o mais desta subseção, e ela vale a pena porque a conclusão oposta é tentadora: como `parallel` é a única construção com dependência de plataforma, parece natural o keel resolvê-la — vendorizando threads, ou trazendo um shim. Resolver significaria distribuir o primeiro artefato do keel que não é C portável, com superfície de porte crescendo a cada alvo novo.

**Este backend especifica dois lowerings, e os dois emitem o mesmo gestor:**

| Lowering | O que muda | Quando é escolhido |
| --- | --- | --- |
| `openmp` | o `#pragma omp parallel for` acima do laço de workers | `-fopenmp` na linha, ou pedido explícito |
| `serie` | nada além da ausência da diretiva: os workers correm um depois do outro | ausência de `-fopenmp` |

Quem escolhe é a invocação, pela regra da `ferramenta §4.8`, porque só ela sabe o que o alvo oferece. **A escolha não muda a semântica**: a linguagem §4.8 diz que a execução serial das partes em ordem crescente é uma das execuções permitidas, e é exatamente essa que o segundo lowering entrega.

Disso saem três consequências:

- **Não há diagnóstico de indisponibilidade.** Ele existia quando o lowering era um só: sem OpenMP, o programa saía do caminho previsto e isso merecia aviso. Com dois lowerings especificados, a ausência de OpenMP seleciona o outro, e selecionar não é desviar. Quem exige paralelismo pede o mecanismo pela flag, e é a ferramenta que recusa a invocação quando ele não está disponível.
- **O código é o mesmo, tirando a linha da diretiva.** Não há emissão condicional dentro do gestor, não há `#ifdef` em torno dos laços e não existe um gestor serial separado a manter. É a diferença que mais paga nesta seção: um caminho de código, não dois.
- **Não há `<threads.h>`, `thrd_create`, thunk nem struct de argumentos.** É a não-obrigação, e ela vale registrar porque foi uma alternativa considerada: um lowering por threads da libc precisaria montar um struct com **o tipo de cada captura**, e o backend não conhece esses tipos — a invariante da linguagem §1.3 proíbe que conheça. `firstprivate(dt)` não precisa de tipo nenhum. É por isso que um terceiro lowering por pool de threads, que a linguagem permite, não está especificado aqui: ele custaria conhecer os tipos das capturas, e não custaria menos por ser escrito depois.

### 5.10 `keel.routine`

A composição deixou de ser construção: `seq` e `par` são funções de um módulo da base (linguagem §5.6), e saem pelas regras do §5.2, como qualquer função de instância. Esta seção registra só o que é próprio delas.

```keel
//keel
array routine.slot Ctx steps[2] = {
    { .f = prepare, .ctx = &ctx },
    { .f = measure,    .ctx = &ctx },
};
outcome u32 r = routine.par(slice.of(steps), 1);
```

```c
//C gerado
typedef keel_corot (*keel_routine_Ctx)(app_Ctx *);

typedef struct keel_routine_slot_Ctx {
    keel_routine_Ctx f;
    app_Ctx         *ctx;
    keel_corot       state;
} keel_routine_slot_Ctx;

keel_routine_slot_Ctx app_steps[2] = {
    { app_prepare, &app_ctx, {0} },
    { app_measure,    &app_ctx, {0} },
};
keel_outcome_u32 r =
    keel_routine_par_Ctx(keel_slice_keel_routine_slot_Ctx_of(app_steps, 2), 1);
```

Quatro regras de emissão:

1. **O `typedef` da participante é da instância**, e sai no header de instância (§4.3) com o nome canônico do módulo mais o argumento — `keel_routine_Ctx`. A regra de encurtamento do §2.1 se aplica: o nome do tipo coincide com o último componente do módulo e não se repete.
2. **O campo de estado é um `corot`**, não um inteiro nu. É a struct de um campo do §5.14, e o inicializador agregado sem terceiro membro a deixa zerada — que é `ONGOING`, e é o estado inicial correto sem escrita.
3. **A chamada da participante é indireta e sai como está**: `s.ptr[i].f(s.ptr[i].ctx)`. O backend não a inspeciona, não a insere em linha e não a envolve em nada. É o custo declarado de a composição ser biblioteca (linguagem §5.6).
4. **Nenhuma região é injetada.** Não há gestor, não há laço gerado em volta da chamada, não há rótulo de saída e não há reafirmação de código: o corpo de `par` e de `seq` é keel comum, e foi emitido quando o módulo foi instanciado. Por isso esta seção não tem regra de mapeamento de linhas — o que existe é o `#line` normal das funções do módulo (§6).

**O que sumiu, e por que vale registrar.** As duas seções anteriores deste documento — a emissão de `coseq` e a de `copar` — descreviam gestores injetados, com laço, rótulo de finalização e reafirmação do código de resultado depois do corpo do usuário. Nada disso sobrevive: o que era emissão virou fonte, e o fonte é keel que o próprio backend já sabe traduzir. É o teste do §1 funcionando no sentido bom — construção que a biblioteca alcança sai do backend junto com a linguagem.

### 5.11 Cursor e partição

Os dois protocolos que a linguagem §5.1 exige de `walk` e de `parallel` saem como funções de instância comuns (§5.2). O que é próprio deles é onde o tipo do cursor mora e o que a partição devolve.

```c
/* keel/keel_buffer.type.h — from the module, not the instance */
typedef struct keel_buffer_cursor { size_t i; } keel_buffer_cursor;

/* keel/keel_buffer_i32.h — from the instance; the prototypes, then the bodies */
static inline keel_buffer_cursor keel_buffer_i32_begin(keel_buffer_i32 *b);
static inline bool  keel_buffer_i32_has_next(keel_buffer_i32 *b, keel_buffer_cursor *c);
static inline i32  *keel_buffer_i32_next(keel_buffer_i32 *b, keel_buffer_cursor *c);
static inline keel_slice_i32 keel_buffer_i32_partition(keel_buffer_i32 *b,
                                                       size_t k, size_t w);
```

Três regras:

1. **O cursor é do módulo, não da instância.** Ele guarda uma posição e não menciona o parâmetro de tipo, então cai na regra da linguagem §4.3 — declaração que não menciona parâmetro nem modificador é emitida uma vez — e sai no `.type.h` do módulo, ao lado do `typedef` do modificador. É o que faz `buffer.cursor` ser escrito sem argumento no fonte, e o que evita um tipo de cursor por instância com layout idêntico.
2. **`next` devolve o endereço do elemento**, e por isso `walk` sobre a base usa binder por ponteiro. Quem quiser cópia escreve a indireção no corpo; o contrário — devolver cópia e pedir endereço — não teria como ser escrito.
3. **`partition` devolve o produto declarado**, e para `buffer T` e `slice T` isso é `keel_slice_T` construído sem chamada: `{ hi - lo, base + lo }`. O passo é o teto de `n/k`, calculado sem transbordamento intermediário, e a parte de índice alto pode sair vazia. A instância de `slice T` é arrastada pela instanciação de `buffer T`, pela regra recursiva do §4.3.

### 5.12 Cláusula `else`

O lowering é uma linha, e é o que a linguagem §4.10 define: a declaração, ou a atribuição, sai como estava, seguida de um `if` cujo teste vem do verbo `failed` da instância. São **duas formas**, e o backend as recebe já separadas pelo parser (linguagem §4.7) — ele não olha para o operando.

**Forma de saída** — o statement entra no `if`, verbatim:

```keel
//keel
outcome Cfg c = cfg.le(path) else return -1;
outcome u32 n = cfg.port(path) else { log(path); return -1; }
r = cfg.le(path) else break;
```

```c
//C gerado
keel_outcome_cfg_Cfg c = cfg_le(path); if (keel_outcome_cfg_Cfg_failed(c)) return -1;
keel_outcome_u32 n = cfg_port(path); if (keel_outcome_u32_failed(n)) { log(path); return -1; }
r = cfg_le(path); if (keel_outcome_cfg_Cfg_failed(r)) break;
```

**Forma de default** — o próprio resultado e a expressão de default são argumentos do `win` da instância:

```keel
//keel
outcome string name = login() else string.from("(noname)");
```

```c
//C gerado
keel_outcome_keel_string name = app_login(); if (keel_outcome_keel_string_failed(name)) keel_outcome_keel_string_win1(&name, keel_string_from("(noname)"));
```

Cinco regras de emissão:

1. **O teste é sempre a chamada a `failed` da instância**, pelo despacho normal do §5.2 — nunca `if (!x)`. Ponteiro não é falível (linguagem §4.10), então não há segundo caso a emitir, e o backend não classifica tipo nenhum.
2. **Sai numa linha só**, declaração e `if`, pela regra 2 do §6. É o que faz o corpo continuar mapeando 1:1 e dispensa ressincronizar — ao contrário do `match` e do `parallel`, que não têm como caber. Vale para as duas formas: a de default acrescenta a chamada de ajuste ao próprio objeto.
3. **O operando é copiado verbatim nas duas formas.** Nada é sintetizado dentro dele: não há desembrulho, não há conversão, não há `return` implícito. O que muda é **onde** ele é colado — dentro do `if` na forma de saída, como segundo argumento de `M_win1(&resultado, …)` na de default.
4. **Na forma de default, o receptor de `win` é o próprio símbolo.** A chamada recebe seu endereço e o valor de default; o verbo ajusta o objeto. Não há reatribuição obrigatória da cópia retornada, temporário, literal composto nem escrita direta de campo na expansão de `else`. Um tipo falível sem `win` é error `else-default-without-win` e não chega ao backend.
5. **Nenhum temporário é criado.** O símbolo — declarado ali, ou declarado antes e atribuído aqui — é o que a cláusula lê e o que ela repara, e é ele que já está em escopo.

`corot` não participa do protocolo: declara `faulted`, não `failed`. O predicado emitido é `keel_corot_faulted`, com teste `code > 0`. Não há exclusão adicional baseada na presença de `ongoing`. `outcome.failed` continua testando `code != 0`.

### 5.13 `at` — o acessor verificado

`at` é o único verbo da base cuja checagem sobrevive ao release, e a emissão diz isso sem `#ifdef`:

```keel
//keel
outcome i32 v = buffer.at(xs, idx) else return -1;
```

```c
//C gerado
static inline keel_outcome_i32 keel_buffer_i32_at(keel_buffer_i32 *b, size_t i) {
    keel_outcome_i32 r = {0};
    return i < b->len ? keel_outcome_i32_win1(&r, b->ptr[i]) : keel_outcome_i32_none(&r);
}

keel_outcome_i32 v = keel_buffer_i32_at(&xs, idx); if (keel_outcome_i32_failed(v)) return -1;
```

Duas regras:

1. **A comparação não é condicional de build.** Ao contrário das checagens de `get`, `set` e `ptr` (§5.3), que saem entre as macros de `debug`, esta é código comum da instância. É o que a linguagem §5.3 promete, e a promessa é o motivo de o verbo existir.
2. **Fora de faixa sai por `none`, não por um código.** O produtor é o do §5.14, e nenhum valor de erro é inventado aqui — o backend não tem catálogo de erro e não deve ganhar um.

### 5.14 `outcome` e `corot`

`keel.outcome` é o módulo; `outcome` é o modificador que ele declara.
Aplicar esse modificador a `T` emite uma struct com o valor desse tipo e seu
código de resultado, acompanhada das funções que interpretam e ajustam o
código e o valor. A emissão usa a substituição do §4.3 e a partição do §4.4.1:
representação e operações para cada aplicação, constantes uma vez no módulo.

```keel
//keel
pub outcome u32 port(const char *path);
```

```c
//C gerado
typedef struct keel_outcome_u32 { i32 code; u32 value; } keel_outcome_u32;

static inline bool keel_outcome_u32_failed(keel_outcome_u32 e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_u32_ok(keel_outcome_u32 e)     { return e.code == keel_outcome_OK; }

static inline keel_outcome_u32 keel_outcome_u32_win(keel_outcome_u32 *r) {
    r->code = keel_outcome_OK;
    return *r;
}
static inline keel_outcome_u32 keel_outcome_u32_win1(keel_outcome_u32 *r, u32 v) {
    r->code = keel_outcome_OK;
    r->value = v;
    return *r;
}
static inline keel_outcome_u32 keel_outcome_u32_fail(keel_outcome_u32 *r, i32 c) {
    r->code = c;
    return *r;
}
static inline keel_outcome_u32 keel_outcome_u32_none(keel_outcome_u32 *r) {
    r->code = keel_outcome_NONE;
    return *r;
}

keel_outcome_u32 cfg_port(const char *path);
```

Quatro regras:

1. **`code` vem primeiro no layout**, e `OK` é zero. É o que faz `{0}` e `memset` deixarem um `outcome T` válido com valor presente, sem código — a mesma propriedade que o `ONGOING` zero dá ao `corot` (linguagem §5.5).
2. **`failed` é comparação com zero, não com uma lista.** Qualquer código diferente de `OK` é falha, então acrescentar código de erro novo não toca a função — e é o que permite ao programa usar o `code` como `errno`, como enum próprio, ou como o que quiser.
3. **`NONE` é uma constante como `OK`, e não um estado a mais.** As duas saem uma vez pela partição do §4.4.1; `failed` não as distingue, e nenhum código do backend as compara entre si. O que separa ausência de erro é o `code` que o programa lê, não a emissão (linguagem §4.10).
4. **Os verbos de escrita recebem o objeto por endereço.** `win(r)` ajusta o
   código; `win(r,v)` ajusta código e valor; `fail(r,c)` e `none(r)` ajustam
   somente o código. Todos devolvem `*r` depois da escrita. A forma sem valor
   preserva o campo associado: não há inicialização implícita desse campo.
   A instância vem do primeiro argumento, sem consulta ao destino da chamada.

`outcome.win(r, v)` traduz para `keel_outcome_T_win1(&r, v)` quando `r` é
objeto, ou para `keel_outcome_T_win1(r, v)` quando já é ponteiro. A aridade
adicional usa o sufixo da §2.1. O retorno é da função inline; uma chamada
isolada continua na função chamadora. Para encerrá-la, o fonte escreve
`return outcome.win(r, v);`. A avaliação dos argumentos ocorre uma vez, como
em uma chamada C comum. Predicados e getters recebem o valor para consulta;
o setter recebe endereço, conforme sua assinatura escrita.

**`corot` tem o mesmo layout de um `outcome void` e outra leitura do zero.** Ele é tipo, e não modificador (linguagem §5.5): sai **uma vez** no `.type.h` do módulo, sem header de instância e sem sufixo de argumento.

```c
/* keel/keel_corot — the typedefs in the .type.h, the bodies in the .h */
typedef struct keel_corot { i32 code; } keel_corot;

typedef enum keel_corot_Status {
    keel_corot_Status_SUCCESS = -1,
    keel_corot_Status_ONGOING =  0,
    keel_corot_Status_FAILED  =  1
} keel_corot_Status;

static inline bool keel_corot_ok(keel_corot r)      { return r.code <  0; }
static inline bool keel_corot_ongoing(keel_corot r) { return r.code == 0; }
static inline bool keel_corot_faulted(keel_corot r) { return r.code >  0; }
static inline i32  keel_corot_code(keel_corot r)    { return r.code; }

static inline i32 keel_corot_tag(keel_corot r) {
    return r.code < 0 ? keel_corot_Status_SUCCESS
         : r.code > 0 ? keel_corot_Status_FAILED
                      : keel_corot_Status_ONGOING;
}

static inline keel_corot keel_corot_win(keel_corot *r)   { r->code = -1; return *r; }
static inline keel_corot keel_corot_again(keel_corot *r) { r->code =  0; return *r; }
static inline keel_corot keel_corot_fault(keel_corot *r, i32 c) { r->code = c; return *r; }
```

Seis consequências para a emissão:

1. **`{0}` é ONGOING**, e é o que faz um slot da tabela de `keel.routine` (§5.10) e um agente inteiro nascerem prontos sem código de inicialização.
2. **Os produtores são verbos, e não saltos.** `corot.win(r)`, `corot.again(r)` e `corot.fault(r,c)` recebem o objeto por endereço, escrevem o código — `-1`, `0` e `c` — e devolvem `*r`, exatamente como os de `outcome`. Não emitem `return`: sair da função é `return corot.win(r);` escrito no fonte, e esse `return` recebe o cleanup do §5.5 como qualquer outro.
3. **Não há campo de valor, e não há aridade com valor.** O que uma passagem produz pertence ao contexto que o programa passou (linguagem §5.5), e por isso o tipo tem um campo só.
4. **Não há `keel_costatus`.** Um enum de três valores não descreveria FAILURE, que é uma região; expor a constante seria mentira. O `enum` acima é outra coisa: são três **nomes** para as três regiões, e a ponte entre eles é `tag`.
5. **`tag` normaliza o código para a tag, e é o que põe `corot` no `match`** (§5.6). Ele não é o código: dois códigos de falha diferentes dão a mesma tag.
6. **`corot.fault` exige código positivo.** Código conhecido zero ou negativo é recusado na tradução pelo diagnóstico `invalid-fault-code`; expressões C não avaliadas por keel têm essa positividade como pré-condição. A emissão avalia o operando uma vez, sem normalizar o sinal — normalizar transformaria um erro do programa em um estado que ele não pediu.

**A struct de um campo não custa nada, e é o que o argumento da linguagem §5.5 pressupõe.** Struct de um `i32` é classificada como INTEGER no SysV x86-64 e volta em `EAX`; em AArch64 volta em `X0`; em ARM32 e RISC-V, no primeiro registrador de retorno. O que ela compra é o compilador C recusando `if (r)`, `r == 0` e a mistura com um código de `outcome` — barreira que um `typedef` de inteiro não daria.

**Instância `void` de `outcome`.** O campo associado é omitido; código e predicados permanecem:

```c
typedef struct keel_outcome_void { i32 code; } keel_outcome_void;

static inline bool keel_outcome_void_failed(keel_outcome_void r) {
    return r.code != 0;
}
```

Para `outcome void`, `win(r)`, `fail(r,c)` e `none(r)` recebem o objeto por
endereço, ajustam seu código e o devolvem por valor. Não existe `win(r,v)`.
A forma C de `win` é:

```c
static inline keel_outcome_void keel_outcome_void_win(keel_outcome_void *r) {
    r->code = 0;
    return *r;
}
```

**Setter de valor.** A aridade adicional usa o sufixo da §2.1 e recebe o
resultado por endereço. A escrita não altera o código:

```c
static inline void keel_outcome_i32_value1(keel_outcome_i32 *r, i32 v) {
    r->value = v;
}
```

A forma de default de `else` chama `win` com o objeto e o valor de default,
conforme a §5.12. Não pode ser substituída somente pelo setter, pois também
precisa estabelecer o código de sucesso. As grafias antigas de produção e
consulta cooperativas não são emitidas como aliases.


## 6. Mapeamento de linhas

`#line` é pegajosa: fixa o número da linha seguinte e o compilador segue incrementando sozinho. Isso permite enunciar a regra como uma **invariante mecânica**, em vez de uma lista de casos:

> O gerador mantém dois contadores — a linha do `.k` que está sendo traduzida e a linha do arquivo de saída. **Sempre que os dois divergem, emite-se `#line`.** Enquanto andam juntos, nada é necessário.

Daí decorre o comportamento de cada região:

| Região | Diverge? | `#line` |
| --- | --- | --- |
| Texto copiado — corpo de função, bloco `extern_c` | não, se as quebras forem preservadas | uma na entrada da região |
| Transliteração 1:1 — `import`, `import_c` viram `#include` | não, se as linhas em branco forem preservadas | uma na entrada da região |
| Declaração levada a header — tipo, protótipo, `extern`, `constexpr` de módulo | sim: o header as reúne fora da ordem e do espaçamento do fonte | uma antes de cada declaração que não seja a linha seguinte da anterior |
| Expansão de builtin ocupando mais de uma linha | sim | ressincroniza depois |
| Código injetado — struct de instância, cleanup de `defer`, temporário de `return` | sim | ressincroniza depois |
| Gestor de `parallel` e despacho de `match` | sim | ressincroniza uma vez, depois do bloco |
| Cláusula `else` — declaração mais `if`, nas duas formas | não, cabe numa linha | nenhuma |
| `at` — chamada de instância | não | nenhuma |
| Sintético — `#include` do próprio `.type.h`/`.h`, do prelúdio, dos headers de instância | — | **nenhuma**: falha ali é bug de ferramenta ou de build, não erro do usuário |

A linha de `import_c` merece nota, porque é o caso que o critério "copiado versus gerado" deixaria escapar: `#include <tgmath.h>` é gerado, não copiado, mas é tradução um-para-um de uma linha que o usuário escreveu, e falha com frequência — nome errado, `-I` faltando, header que só existe em outra plataforma. Sem a diretiva, `fatal error: tgmath.h: No such file or directory` aponta para um `.h` que ninguém escreveu.

Quatro regras de emissão decorrem da invariante:

1. **Texto copiado nunca é reindentado nem reformatado.** Reformatar destrói o alinhamento e, com ele, a posição de todo diagnóstico do compilador C naquela região. A mesma condição se estende à região de imports: preservar as linhas em branco é o que faz uma diretiva só cobrir o bloco inteiro.

2. **Em corpo de função, o lowering de uma linha de fonte ocupa preferencialmente uma linha de saída.** Uma chamada de builtin longa sai numa linha só em vez de quebrada em três, e uma expansão de dois statements sai numa linha só — porque quebrar custa um `#line` a cada statement e, sem ele, todo o resto do corpo passa a apontar para a linha errada. Com a regra, o corpo inteiro mapeia 1:1 a partir da diretiva única da regra 3.

   O custo é linha gerada mais longa que a que um humano escreveria. É a **única concessão deliberada ao princípio 2** desta especificação, e ela se paga: é o que faz cada erro do compilador C cair na linha certa do `.k`, que é a razão de o princípio 3 funcionar.

3. **O `.c` preserva a estrutura de linhas do fonte.** Toda linha do `.k` tem a sua linha no `.c`: o que vai para header — tipos, protótipos, `constexpr` de módulo —, os `import` e `import_c`, a linha `module` e os comentários deixam linha vazia. Os `#include` sintéticos vêm antes, e um único `#line 1` abre o fonte; daí em diante, só as expansões de várias linhas pedem ressincronização. Linha vazia a mais é C comum; se incomodar, a alternativa é trocar cada sequência delas por um `#line`, e a invariante vale igual.

4. **Comentários não chegam ao C.** Cada comentário vira espaços da mesma largura, com as quebras de linha preservadas, e o espaço que sobra no fim da linha é cortado: o de fim de linha some, e o do meio preserva a coluna do que vem depois. A exceção é `extern_c`, cujo conteúdo é C e atravessa intacto (linguagem §2.4).

A string do arquivo é o caminho normalizado do módulo, com a extensão `.k`.

### 6.1 Diagnóstico dentro de instância

Erro do compilador C no corpo de `coll_stack_i32_length` aponta, por `#line`, o fonte do genérico — `coll.k`, na linha da função. É o certo: toda posição é no fonte keel que o usuário escreveu.

Só que essa posição sozinha não diz **qual** instanciação quebrou, e um genérico que funciona em `i32` e falha em `geom.Point` fica ilegível. Por isso a instância carrega a posição do primeiro uso que a criou, e o diagnóstico ganha uma linha de contexto:

```plain
coll.k:14:12: error: invalid operands to binary + [...]
sim.k:7:1: note: in the instantiation of coll.stack at geom.Point
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
| `.type.h` de um módulo | o fonte daquele `.k` **e o `.type.h` dos módulos que ele importa** — dos tipos deles, não da interface |
| `.h` e `.c` de um módulo | o fonte daquele `.k` **e a interface pública dos módulos que ele importa**, transitivamente |
| header de instância de modificador **embutido** | **apenas o próprio nome** |
| header de instância de modificador **do usuário**, e o `.c` de instância pedida por `--instance` | o nome **e** o corpo do módulo genérico |
| unidade de ponto de entrada | o nome do módulo pedido na invocação |

**A primeira linha é a nova, e ela é mais estreita de propósito.** O `.type.h` de
`A` não depende da interface de `B`: depende só dos tipos de `B` que `A` menciona
por valor. Mexer numa assinatura de `B` não muda o `.type.h` de `A`, e é essa
estreiteza que o §7.3 cobra em recompilação poupada.

**A segunda linha tem duas metades, e a segunda delas é fácil de perder.** O C de `A`
depende de `B` porque a tradução consulta a interface de `B` em dois lugares que a
linguagem já nomeia: o `&` de adaptação vem do **parâmetro declarado no callee**
(linguagem §4.4), e o despacho decide entre verbo de tipo e função de módulo
lendo a assinatura de lá.

```keel
/* b.k */  pub void consume(slice i32 s);     →  A emit  b_consume(s)
/* b.k */  pub void consume(slice i32 *s);    →  A emit  b_consume(&s)
```

Editar `b.k` muda o `.c` de `A` sem que `A.k` seja tocado. **O critério de
atualização tem de refletir isso**, e é ferramenta §5 que o escreve.

A terceira linha é a que paga o custo da repetição. Como todo módulo que usa o tipo gera os headers, numa árvore grande a mesma instância é considerada muitas vezes:

> O conteúdo de um header de instância de modificador embutido é **função pura do próprio nome**, e o nome é o nome do arquivo. Se ele existe e foi escrito pela mesma versão do gerador, é necessariamente idêntico — basta um `stat`.

É o que separa esses headers dos do módulo: o `.type.h`/`.h`/`.c` de um módulo depende do **conteúdo** do fonte, e por isso precisa de comparação; a instância de `buffer` depende apenas do **nome**. Dentro de uma invocação, cada instância é considerada uma vez só, por memoização.

**Instância de modificador do usuário não tem essa propriedade**, e é a única coisa que os módulos genéricos custam aqui: editar o `push` de `coll.k` muda `coll_stack_i32.h` sem mudar o nome dele. O critério de atualização correspondente é ferramenta §5.1.

O número de instâncias distintas é limitado pelo fonte, não pelo número de módulos — um projeto real tem dezenas, não milhares —, então o custo em regime é alguns `stat` por invocação e nenhuma escrita.

### 7.3 Consequência: o depfile é transitivo

As funções da instância saem inline num header, então toda unidade de tradução que a inclui **embute o código**. Editar `coll.k` tem que retriggar não só quem escreveu `stack i32`, mas todo módulo que alcance esse header por transitividade. O formato e a emissão são de ferramenta §4.5; a razão é esta linha.

**O corte do §4.3.2 limita esse alcance só no caminho de layout.** Editar um
corpo ou uma assinatura muda o `.h` e mais nada; um header gerado que só precisa
do tipo alcança o `.type.h`, que não mudou. Quem chama inclui o `.h` (regras 2 e
3), então recompila — é o preço de não haver um header só de protótipos
([justificativa](keel-rationale.md#dois-headers-tipo-e-uso)). Quem quiser a
granularidade fina num `.c` escrito à mão ainda pode incluir só o `.type.h`: o
arquivo existe, só não é o padrão.

A ressalva que sobra é a mesma do make: se o próprio gerador mudar, os gerados ficam obsoletos sem que timestamp nenhum acuse, e a saída é apagar o diretório de destino.

---

## 8. Diagnósticos deste documento

| Identificador | Diagnóstico | Sev. |
| --- | --- | --- |
| `name-too-long` | Nome gerado acima do teto de comprimento (255, ou 63 sob `--pedantic-names`) | `error` |
| `reserved-name` | Identificador do usuário no espaço reservado `keel_` | `error` |
| `set-out-of-length` | `set` com índice fora de `length` | `debug` |
| `instance-field-access` | Acesso direto a campo de instância de modificador, fora do módulo que a declara | `warning` |
| `tag-out-of-range` | Etiqueta fora da lista declarada do conjunto | `debug` |
| `range-index-out-of-bounds` | Intervalo cujos limites violam `a <= b <= length(x)` | `debug` |
| `array-index-out-of-bounds` | Índice de `array` fora da dimensão declarada | `debug` |
| `specific-format-unavailable` | Módulo usa `f16` ou `bf16` e o alvo não oferece o formato | `error` |
| `alloc-overflow` | `arena.alloc` cujo `n * sizeof(T)` não cabe em `size_t` | `debug` |
| `layout-cycle` | Cadeia de tipos que se contêm por valor atravessando instância de modificador | `error` |

Os identificadores são os do [catálogo da spec](keel-spec.md#62-catálogo). A spec define a condição normativa; esta tabela reúne os casos relacionados ao backend.

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
> conjunto de programas (linguagem §6.3). Recusar sob um e aceitar sob o outro é
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
//keel
priv constexpr int K = 1 << 4;
```

```c
//C gerado
#define app_K ((int)(1 << 4))
static const int app_K__chk = (1 << 4);      /* checks the restriction and the constancy */
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
`-Woverflow` sob C11. Está registrado na linguagem §6.3, e é o único item de lá
que fala do perfil.

**Escopo de bloco pede o par.** Em escopo de arquivo o símbolo já leva o prefixo do
módulo, e nada mais no arquivo casa com ele. Em escopo de bloco não há prefixo
(linguagem §4.2), e duas funções do mesmo arquivo podem declarar `constexpr size_t
N` com valores diferentes. **A macro não leva o nome do usuário:** ela recebe um
nome gerado no espaço reservado, e o backend **reescreve os usos** dentro do bloco.

```keel
//keel
void f(struct S *s) {
    constexpr size_t N = 8;
    array char buf[N];
    s->N = 1;
}
```

```c
//C gerado
static void app_f(struct S *s) {
#define keel__N_0 ((size_t)8)
    static const size_t keel__N_0__chk = 8;
    char buf[keel__N_0];
    s->N = 1;
#undef keel__N_0
}
```

Três regras, e as três existem por um caso concreto:

1. **O nome é `keel__<símbolo>_<ordinal do block>`.** O prefixo reservado (§2)
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

## 10. Decisões de emissão

Sete pontos que estiveram abertos enquanto o lowering se firmava. Ficam aqui
com o motivo, como decisão registrada — não como alternativa em aberto.

| | Decisão | Por quê |
| --- | --- | --- |
| 1 | O arquivo de instância mora em `keel/`, diretório fixo | os dois headers e o `.c` de uma instância são função do símbolo dela e de mais nada (§7.2); um diretório por módulo multiplicaria cópias byte a byte idênticas só para deduplicá-las depois no build |
| 2 | Os campos das structs geradas **não** levam prefixo | o prefixo é do nome do tipo — `keel_buffer_i32`, `keel_slice_geom_Point` —, e é ele que carrega a identidade. `xs.keel_len` não compraria nada que a linguagem §5.3 e o warning `instance-field-access` já não digam: layout não é interface |
| 3 | O índice de `array` é verificado por dimensão, e a dimensão 0 de parâmetro é contrato | §5.3. Decimal conhecido contra decimal conhecido recusa na tradução (`array-index-above-dimension`); o resto é `assert` sob `--checks` (`array-index-out-of-bounds`); e o chamador que entrega menos do que promete é pego na chamada (`array-argument-wrong-dimension`) |
| 4 | Composição cooperativa é biblioteca, não emissão | `seq`, `par` e `mask` são funções de `keel.routine`, emitidas como qualquer instância (§5.10), e o estado por slot é um `corot` no próprio registro. Não há gestor injetado, região de finalização nem reafirmação de código |
| 5 | `mask` devolve `u64`, e o limite é 64 slots | o mapeamento é de 64 bits, daí 64 entradas; o excedente é o `debug` `mask-above-64-slots`. Largura arbitrária volta junto com `bitslice bool`, e aí será tipo, não conveniência |
| 6 | As cláusulas do `#pragma` saem em ordem fixada: os temporários do gestor na ordem de emissão, o símbolo de controle, depois as capturas na ordem escrita, repartidas entre `shared` e `firstprivate` pela espécie | `default(none)` obriga a listar todo símbolo tocado, e o §7.1 exige saída byte a byte idêntica. Os temporários de `foreach` no corpo não entram na lista: são declarados dentro do laço do worker e já são privados por construção (§5.9) |
| 7 | A base fica toda `pub inline`; não há `.c` por instância distribuído pronto | em laço quente o inline é o que mantém o código junto do dado, e isso é objetivo da linguagem, não detalhe de emissão. O preço — `--instance` sobre a base emitir `redundant-instance` sempre — é consequência anunciada ([rationale](keel-rationale.md#prelúdio-e-base-mínima)) |
