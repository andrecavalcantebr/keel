# keel — Especificação da Linguagem

Duas convenções neste documento:

- **Toda construção aparece em par:** o bloco `keel` e, logo abaixo, o C que ele
  produz. O par diz o sentido, não a grafia; a grafia é de `keel-c-backend.md`.
- **A razão de cada regra está em `keel-rationale.md`.** Este documento diz o que a linguagem é; o outro diz por que ela é assim.

---

## 1. O que é keel

keel é um co-processador de C simples, que transpila para **C23 padrão**, sem extensão de compilador. 

Ela insere sobre o C algumas abstrações que facilitam a programação, principalmente aquilo que faríamos com truques de macros. keel é a macro como linguagem e algumas pequenas coisas a mais.

### Princípios

Os princípios **são a régua do documento inteiro, e decidem as dúvidas que a spec não cobrir.** Cada seção
adiante é um deles em exercício.

1. **Não mudar o C.** Nenhum programa C válido muda de sentido ao passar por keel.
2. **Nada que não se faria à mão.** O gerado é código que um humano assinaria, e **se você não consegue prever o C que uma construção produz, a construção falhou**. Lowering que ninguém assinaria — uma camada de substituição textual, um pool escondido — não entra; e onde a regra é quebrada de propósito, o custo fica escrito.
3. **O compilador C é o verificador final.** keel não reimplementa o sistema de tipos do C: erro de tipo aparece no compilador C, mas com os nomes que você escreveu no fonte keel.
4. **Identidade nominal.** Dois tipos são o mesmo tipo se vêm da mesma declaração.
5. **Rejeitar em vez de adivinhar.** Diante do que não sabe traduzir com certeza, keel diagnostica; não heuristiza.
6. **keel não esconde layout, tempo de vida nem travessia.** São as três decisões que a orientação a dados exige do programador, e nenhuma construção pode tomar qualquer uma delas em silêncio. Onde keel escreve a travessia, ele obriga a escrever as decisões dela — `k`, a partição e a captura do `parallel` são escritos, nunca deduzidos.
7. **O que exigiria entender o C está fora.** Recurso que obrigue keel a saber o que um nome do C significa não entra na linguagem, por isso mesmo (§1.3).
8. **Núcleo é o que nenhum módulo keel poderia escrever.** O que a biblioteca alcança, a biblioteca escreve (§1.6).
9. **Açúcar entra pelo que carrega, não pelo que economiza.** Construção que só reescreve o que o C já expressa bem fica de fora, por barata que seja; entra a que o C expressa mal, ou não expressa. O que separa as duas é a garantia que a notação passa a sustentar, nunca o número de tokens que ela poupa.

**Os três últimos são a admissão, e valem nessa ordem:** o 7 pergunta se keel *pode*, e dá o teto — o que exigiria entender o C; o 8 pergunta se keel *precisa*, e dá o piso — o que a biblioteca alcança; o 9 pergunta se keel *deve*. Passar nos dois primeiros não admite nada: é o 9 que recusa `match`, que passa nos outros dois sem esforço, e é o 9 que admite a forma `a..b`, que passa neles pelo mesmo motivo — e que carrega os dois limites que o C não verifica. O que sobra depois dos três é keel.

### 1.1 O C é o alvo, não a fonte

**keel trata o C como alvo, e não como fonte.**
Escreve-se keel; sai C. O C que sai é o artefato — versionável, compilável por qualquer compilador C23 conforme, sem necessariamente a keel instalada. Para cada módulo keel são gerados `.c` e o `.h` que se teria escrito, se tivesse paciência para escrever sempre.

Deste fato saem os princípios 2, 3 e 7, e nenhum se repete aqui. O que dele sai e é sobre **este documento** é a convenção de redação: a imagem que keel pede que você mantenha é a do C gerado, e por isso cada construção é especificada em **pares keel/C**.

> Razão: `rationale §1.1`.

### 1.2 O que keel acrescenta

O alvo de keel é ser o mais *data oriented design* possível, sem um sistema completo de tipos: dados em blocos, tempo de vida explícito, sem `malloc` espalhado. Para isso o C não oferece três coisas:

| Falta | keel dá |
| --- | --- |
| contêiner que carrega os próprios limites | `buffer`, `slice` |
| memória por região, e não por objeto | `arena` |
| um lugar para pôr o cleanup | `defer` |

O resto do vocabulário fica em cima disso: travessia (`foreach`, `parallel`), máquina de estados (`cofsm`, `coseq`, `copar`), parametrização por tipo (`modifier`), e o conjunto das bibliotecas comuns (ex.: `string`, `strbuf`).

### 1.3 A invariante

keel é um **parser de ilhas**: há ilhas de keel num mar de C. O que é construção de keel é traduzido; o resto atravessa. **keel não precisa entender C** — um compilador não entende as macros do assembler: ele emite dentro delas, e keel faz o mesmo, reconhecendo as próprias construções e copiando o resto sem examinar. Dessa forma:

> **keel nunca precisa conhecer nomes de tipo do C.** Ele conhece apenas os símbolos que ele mesmo declarou e os nomes dos módulos importados. Todo o resto é sequência opaca de tokens, copiada verbatim.

```keel
FILE *fp = fopen(path, "rb");
float d  = sqrtf(dx*dx + dy*dy);
```

keel não sabe o que `FILE` é, nem `fopen`, nem `sqrtf`. As duas linhas saem idênticas. E como declaração e expressão são copiadas do mesmo jeito, a ambiguidade clássica do C — a que obriga um compilador C a manter tabela de `typedef` — não aparece aqui.

A invariante dá um teste, e ele decide o que entra na linguagem — é o princípio 7, e é aqui que ele nasce:

> **Recurso que exija de keel saber o que um nome do C significa está fora de keel, por isso mesmo.**

**Corolário**: **keel não tipa expressão.** O que sobra tipável é a posição de contêiner (§3.4), construída só sobre símbolos que keel mesmo declarou. keel só conhece os tipos declarados no fonte keel.

Um efeito colateral vale registrar: `#include <tgmath.h>` funciona. O `_Generic` do C23 é resolvido pelo compilador C, assim como `#embed` funciona como seria em C puro. **keel ganha os recursos do C porque não tenta entendê-los.**

### 1.4 Modelo de compilação

keel roda **antes do pré-processador**.

```plain
fonte.k → cgen → fonte.c + fonte.h → cpp → cc1 → objeto
```

Três consequências:

- keel não enxerga conteúdo de `#include`. Nenhum `typedef` ou macro vindo de header C participa de decisão nenhuma.
- Diretivas de pré-processador atravessam verbatim.
- Nenhum nome que keel precise resolver pode nascer de expansão de macro.

`cgen` (a ferramenta que executa o transpiler keel) é um **driver**: trocar `CC = gcc` por `CC = cgen` num Makefile existente deve produzir o mesmo executável, e é o teste de aceitação da ferramenta (vide `cgen-tool-spec.md`).

> Razão: `rationale §1.4`.

### 1.5 Metaprogramação de pré-processador

Esta seção é a consequência normativa do §1.4: keel roda antes do pré-processador, e é daí que sai tudo o que segue.

> **Nenhum nome que keel precise resolver pode nascer de expansão de macro. E construção cujo sentido só se forma depois que o pré-processador reescreve a estrutura do código não é suportado.**

Isso não proíbe nada: código de pré-processador continua funcionando como sempre, porque passa verbatim e quem o expande é o pré-processador C. O que a regra diz é que keel não participa.

Três consequências, todas normativas:

**Alternativas de um grupo condicional que discordam na contagem de delimitadores são error 41** `chaves-em-ramos`.

Um **grupo condicional** vai de `#if`, `#ifdef` ou `#ifndef` até o `#endif`, e tem
uma **alternativa** por ramo: `#elif`, `#elifdef`, `#elifndef` e `#else` abrem
alternativa, e a ausência de `#else` conta como alternativa vazia. Cada alternativa é varrida por conta própria
e produz um delta de `{}`, `[]` e `()`; **todas as do grupo têm de concordar**, e
o delta comum entra **uma vez** no contexto que cerca o grupo. Grupos aninham, e o
de dentro se resolve primeiro.

Disso o lexer precisa de uma coisa só: a **palavra** da diretiva, que lhe dá a
classe `TK_PPC_IF`, `TK_PPC_ELSE` ou `TK_PPC_END` (§3.6). A condição continua
opaca e **nunca é avaliada** — keel não pergunta se `MIPS` está definido,
só onde cada alternativa começa e termina. É o que preserva cross-compilation, e é
o princípio 5 na letra: não se escolhe ramo, exige-se que eles concordem.

```c
#ifdef MIPS
    if ((v = read(port))) {           /* alternativa A: +1 chave */
#else
    if ((v = digitalRead(port))) {    /* alternativa B: +1 chave */
#endif                                /* concordam: o grupo vale +1 */
        trata(v);
    }
```

```c
#ifdef DEBUG
    if (verbose) {                    /* alternativa A:  +1 chave */
#endif                                /* alternativa vazia: 0 → error 41 */
        registra(r);
#ifdef DEBUG
    }
#endif
```

**O segundo caso é comum, e a recusa é deliberada.** Ali a estrutura de blocos do
arquivo depende de `DEBUG`: keel teria de escolher um ramo para saber onde o corpo
da função termina, e escolher significa avaliar a condição. As duas saídas custam
pouco — pôr o `#ifdef` em volta de statements inteiros, ou tirar a variação da
estrutura e pô-la no valor:

```c
    if (DEBUG_VERBOSE && verbose) { registra(r); }   /* #define DEBUG_VERBOSE 0 */
```

A segunda ainda ganha o ramo desligado sendo verificado pelo compilador C, que o
`#ifdef` não dá. **Condicional que preserva a estrutura de blocos passa;
condicional que a altera, não.**

**Generics por concatenação de token não produzem tipos que keel conheça.** O idioma clássico — `#define T int`, `CAT2(a,b) a##b`, X-Macros — continua compilando como C; simplesmente é opaco.

**`#define` ou `#undef` de palavra contextual de keel, ou de qualquer nome no espaço reservado `keel_`, é error 42** `define-sobre-keel`: o `cpp` reescreveria o **código gerado**, produzindo erro do compilador C apontando para uma linha que o usuário não escreveu.

E o inverso também vale, e é o lado útil da regra: **o que o pré-processador do C faz bem, keel não refaz.** Embutir arquivo em tempo de compilação é `#embed` (C23+), e ele atravessa como qualquer diretiva.

> Razão: `rationale §1.5`.


### 1.6 O núcleo e as bibliotecas

O que keel entrega se reparte em cinco degraus, e **quem decide o degrau é o princípio 8**: nada está no núcleo por conveniência, só por impossibilidade.

| Categoria | O que é |
| --- | --- |
| **núcleo** | `module`, `import`, `pub`/`priv`, a qualificação `.`, a justaposição de modificador, `array`, `constexpr`, `ref`, `defer`, `cofsm`, `coseq`, `copar`, `foreach`, `apply`, `parallel`, o intervalo `a..b`, `type`, `dim`, `modifier`, `instance`, `else` em opcionais/propagação de erros |
| **biblioteca privilegiada** | o módulo `keel.arena` |
| **biblioteca base** | `keel.buffer`, `keel.slice`, `keel.range`, `keel.corot`, `keel.outcome` |
| **stdlib** | `string`, `strbuf`, `tensor`, `view` — escritos em keel, importados |
| **biblioteca de usuário** | todo o resto |

Os três primeiros degraus respondem *quem consegue escrever*; os dois últimos, *quem precisa concordar*.

**A base são módulos comuns**, nomeados e despachados pela regra geral dos módulos genéricos (§4.9). `arena` é a única privilegiada, e por quatro verificações que são do parser: procedência do que é retornado (§4.4), `from_array` exigir um `array u8`, `from_stack` exigir tamanho constante, e uso de arena filha depois de `reset` no pai. **Essas quatro são a dívida**: `arena` quase passou no princípio 8 e não passou, e a categoria existe para que o custo fique escrito em vez de concedido.

> Razão: `rationale §1.6`.

### 1.7 O prelúdio

**Só a camada zero é implícita.** Todo arquivo começa como se tivesse escrito uma linha:

```keel
import keel types;
```

**A base se importa**, e cada linha é escrita por quem a usa:

```keel
import keel.arena   as arena   types;
import keel.buffer  as buffer  types;
import keel.slice   as slice   types;
import keel.range   as range   types;
import keel.corot   as corot   types;
import keel.outcome as outcome types;
```

O corte é o do §1.6, e está onde há diferença de espécie: os tipos de tamanho
fixo não são biblioteca substituível — o §4.2 faz da **grafia** a identidade do
tipo —, e o resto é. Quem tem `arena`, `outcome` ou `buffer` próprios não é
punido por um `import` que não escreveu, e um módulo que não usa arena não vê
`keel/arena.h` (`backend §4.2`).

Escrever a linha implícita é no-op legal. Cada uma faz duas coisas: o **alias** dá o qualificador curto, e `types` injeta o nome do tipo nu. Daí a regra:

| | Como se escreve |
| --- | --- |
| **tipo** — `i32`, `f32`, `buffer`, `slice`, `arena`, `range` | **nu** |
| **verbo** — `length`, `push`, `alloc`, `from_stack` | **qualificado**: `buffer.length(b)`, `arena.alloc(a,T,n)` |

`types` injeta **nome de tipo, e só** — o verbo leva sempre o módulo na frente, porque o leitor precisa saber de quem ele é. Sem exceção para a base: `buffer.push(pts, p)` e `geom.dist(a, b)` são a mesma forma.

**O açúcar não é verbo.** `x[i]`, `x[i,j]` e `x[a..b]` são sintaxe do núcleo e não levam prefixo (§4.6). Idem `foreach`, `defer`, `parallel` e a família cooperativa: são construções, não chamadas.

#### Suprimir a injeção

`import keel;` **sem `types`** suprime o tipo nu e passa a exigir `keel.i32`. É a saída para o projeto que já tem um `i32` ou uma `arena` próprios, e é a única — keel não negocia com um `typedef` que não enxerga.

**O alias substitui o nome, não o acrescenta:** `import keel as k;` dá `k.i32`, tira `keel.i32`, e por não trazer `types` tira também o `i32` nu. Quem quer prefixo curto **e** tipo nu escreve `import keel as k types;`.

Isso não quebra identidade de tipo porque **`M.X` reduz a `X` na normalização**: `buffer keel.i32` e `buffer i32` são a mesma instância, e a qualificação é puramente de fonte.

#### A regra de encurtamento

O modificador `buffer`, declarado em `keel.buffer`, daria `keel_buffer_buffer_i32` pela regra geral de nome canônico. Uma regra o traz de volta:

> **Nome de tipo igual ao último componente do módulo não se repete no nome canônico.**

Daí `keel_buffer_i32`, `keel_slice_char`, `keel_arena`. Ela não encurta nada que já existia; existe para a divisão em módulos não alongar. A única colisão que pode criar é **intra-módulo**, logo é diagnóstico local, nunca falha silenciosa de link.

> Razão: `rationale §1.6`.


---

## 2. keel por exemplos

Quatro programas, em ordem crescente. Cada um introduz o que o anterior não tinha.

**Nada nesta parte é normativo.** Ela mostra a forma; a regra mora no §4, e cada exemplo fecha com uma tabela que diz onde cada coisa está especificada. Onde os dois discordarem, o §4 vence.

### 2.1 Um programa completo

```keel
module app.main;

import geom;

extern_c {
    int printf(const char *, ...);
}

priv float perimetro(slice geom.Point pts) {
    float total = 0.0f;
    for (size_t i = 1; i < slice.length(pts); i++)
        total += geom.dist(pts[i-1], pts[i]);
    return total;
}

int main(int argc, char **argv) {
    arena a;
    arena.from_stack(a, 1 << 16);

    buffer geom.Point pts;
    pts = buffer.from(arena.alloc(a, geom.Point, 16), 16);
    if (buffer.capacity(pts) == 0) return 1;

    buffer.push(pts, (geom.Point){0.0f, 0.0f});
    buffer.push(pts, (geom.Point){3.0f, 4.0f});
    buffer.push(pts, (geom.Point){3.0f, 0.0f});

    printf("%f\n", perimetro(slice.of(pts)));
    return 0;
}
```

```c
/* gen/app/main.c */
#include "app/main.h"
#include "geom.h"

int printf(const char *, ...);

static float app_main_perimetro(keel_slice_geom_Point pts) {
    float total = 0.0f;
    for (size_t i = 1; i < keel_slice_geom_Point_length(&pts); i++)
        total += geom_dist(*keel_slice_geom_Point_ptr(&pts, i-1),
                           *keel_slice_geom_Point_ptr(&pts, i));
    return total;
}

int app_main_main(int argc, char **argv) {
    keel_arena a = {0};
    unsigned char keel__st0[1 << 16];
    keel_arena_from_array(&a, keel__st0, sizeof keel__st0);

    keel_buffer_geom_Point pts = {0};
    pts = keel_buffer_geom_Point_as(
              (geom_Point *)keel_arena_alloc_n(&a, 16, sizeof(geom_Point),
                                               alignof(geom_Point)), 16);
    if (keel_buffer_geom_Point_capacity(&pts) == 0) return 1;

    keel_buffer_geom_Point_push1(&pts, (geom_Point){0.0f, 0.0f});
    keel_buffer_geom_Point_push1(&pts, (geom_Point){3.0f, 4.0f});
    keel_buffer_geom_Point_push1(&pts, (geom_Point){3.0f, 0.0f});

    printf("%f\n", app_main_perimetro(keel_buffer_geom_Point_as_slice(&pts)));
    return 0;
}
```

**O `main` do C não está aí, e não é esquecimento.** `main` é função comum do
módulo e vira `app_main_main`; qual delas é o ponto de entrada do programa é
decisão de build, e quem a expressa é a flag:

```sh
cgen -I src -I gen src/app/main.k --main app.main -O2 -Wall -o prog
```

```c
/* gen/main_app_main.c — só existe por causa da flag */
#include "app/main.h"
int main(int argc, char **argv) { return app_main_main(argc, argv); }
```

É o que permite vários módulos com `main` na mesma árvore — programa,
ferramenta, driver de teste — sem que nenhum precise saber qual foi o escolhido.

| No exemplo | Especificado em |
| --- | --- |
| `module app.main` dá o prefixo `app_main_` a tudo que o arquivo declara; nome externo vai qualificado — `geom.Point`, `geom.dist` | §4.1 `module`, `import` |
| `extern_c` traz `printf` sem que keel interprete a declaração | §1.3, §4.1 `import_c` e `extern_c` |
| `main` é função comum do módulo, e a flag `--main` decide qual vira o `main` do C | §4.1 `main` |
| tipo nu — `buffer`, `arena`, `slice` — e verbo qualificado — `buffer.push`, `arena.alloc` | §3.4, §4.11 |
| uma alocação e nenhum `free`: a arena sai da pilha, os pontos saem da arena | §4.4 |

### 2.2 Fronteira com C, e onde vai o cleanup

```keel
module io;

import_c <stdio.h>;
import_c <string.h>;

pub size_t maior_linha(const char *path, arena *scratch) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return 0;
    defer fclose(fp);

    size_t maior = 0;
    char *raw = arena.alloc(*scratch, char, 4096);
    if (!raw) return 0;
    buffer char linha = buffer.from(raw, 4096);

    while (fgets(buffer.ptr(linha), (int)buffer.capacity(linha), fp)) {
        size_t n = strcspn(buffer.ptr(linha), "\n");
        slice char s = slice.from(char, buffer.ptr(linha), n);
        if (slice.length(s) > maior) maior = slice.length(s);
    }
    return maior;
}
```

```c
size_t io_maior_linha(const char *path, keel_arena *scratch) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return 0;

    size_t maior = 0;
    char *raw = (char *)keel_arena_alloc_n(scratch, 4096, sizeof(char), alignof(char));
    if (!raw) { fclose(fp); return 0; }
    keel_buffer_char linha = keel_buffer_char_as(raw, 4096);

    while (fgets(keel_buffer_char_ptr(&linha), (int)keel_buffer_char_capacity(&linha), fp)) {
        size_t n = strcspn(keel_buffer_char_ptr(&linha), "\n");
        keel_slice_char s = keel_slice_char_from(keel_buffer_char_ptr(&linha), n);
        if (keel_slice_char_length(&s) > maior) maior = keel_slice_char_length(&s);
    }
    { size_t keel__rv0 = maior; fclose(fp); return keel__rv0; }
}
```

`FILE`, `fopen`, `fgets` e `strcspn` atravessam sem que keel saiba o que são: é a invariante em uso.

O que keel acrescenta são duas coisas — a **capacidade**, que viaja com o buffer, e o `fclose`, que aparece uma vez ao lado do `fopen` em vez de em cada `return`. E o que ele **não** acrescenta está igualmente à vista: `fgets` escreve por baixo, através do ponteiro cru, e o `length` de `linha` continua zero. Quem sabe o tamanho é o `strcspn` — daí a vista nascer de `slice.from`, com o comprimento escrito por quem o conhece, e não de `slice.of`, que deduziria do `length` e daria uma vista vazia.

| No exemplo | Especificado em |
| --- | --- |
| `import_c`, e o C que atravessa intacto | §1.3, §4.1 `import_c` e `extern_c` |
| `defer fclose(fp)` rodando em cada ponto de saída, e o temporário no `return` | §4.7 `defer` |
| `length` contra `capacity`, `slice.from` como asserção, e por que não existe `set_length` | §4.5 |
| a arena que vem por parâmetro, e a alocação sem `free` | §4.4 |

### 2.3 Muitos agentes, um bloco

```keel
module sim;

pub cofsm ciclo [PROCURA, ATACA, MORRE];

pub typedef struct {
    ciclo  s;
    i32    hp, alvo;
} Agente;

priv i32 procura(buffer Agente *ags, size_t i);

pub void rodada(buffer Agente *ags, arena *a) {
    foreach (Agente *ag, size_t i : ags) {
        cofsm ciclo (ag->s) {
            PROCURA:
                ag->alvo = procura(ags, i);
                if (ag->alvo < 0) cobreak;
                ag->s = ATACA;
            ATACA:
                ag->hp -= 1;
                if (ag->hp <= 0) ag->s = MORRE;
            MORRE:
                ag->hp = 0;
        }
    }
}
```

```c
typedef enum {
    SIM_CICLO_PROCURA,
    SIM_CICLO_ATACA,
    SIM_CICLO_MORRE,
} sim_ciclo;

typedef struct {
    sim_ciclo s;
    i32       hp, alvo;
} sim_Agente;

static i32 sim_procura(keel_buffer_sim_Agente *ags, size_t i);

void sim_rodada(keel_buffer_sim_Agente *ags, keel_arena *a) {
    { keel_buffer_sim_Agente *keel__c0 = ags;
      size_t keel__n0 = keel_buffer_sim_Agente_length(keel__c0);
      for (size_t i = 0; i < keel__n0; i++) {
        sim_Agente *ag = keel_buffer_sim_Agente_ptr(keel__c0, i);
        switch (ag->s) {
        default:
        case SIM_CICLO_PROCURA:
            ag->alvo = sim_procura(ags, i);
            if (ag->alvo < 0) break;
            ag->s = SIM_CICLO_ATACA;
            break;
        case SIM_CICLO_ATACA:
            ag->hp -= 1;
            if (ag->hp <= 0) ag->s = SIM_CICLO_MORRE;
            break;
        case SIM_CICLO_MORRE:
            ag->hp = 0;
            break;
        }
      } 
    }
}
```

Mil agentes num bloco contíguo, uma passagem por rodada. **O estado é dado:** `ciclo` é declarado no topo como qualquer tipo, e é campo do struct do usuário. Não há pilha secundária, não há runtime, não há alocação por agente.

E note o que **não** aparece aqui: `rodada` é `void`, não `corot`, então nenhum verbo de saída de função vale. Quem termina a passagem de um agente é `cobreak`, que sai do `switch` e devolve o controle ao `foreach` — próximo agente. A transição é atribuição comum, porque a variável de estado é do usuário e está à vista.

| No exemplo | Especificado em |
| --- | --- |
| `cofsm ciclo [...]` declarado no topo, e usado como tipo de campo | §4.8 |
| `cobreak`, e quais verbos valem conforme o retorno da função | §4.8 |
| `foreach` com binder por ponteiro sobre `buffer`, e o contêiner avaliado uma vez | §4.7 `foreach` e `apply` |
| a constante de enum escopada no tipo — `PROCURA` nu no fonte, `SIM_CICLO_PROCURA` no gerado | §4.3 |

### 2.4 Travessia paralela

```keel
module fis;

pub typedef struct { f32 x, y, vx, vy, ax, ay; } Part;

pub void passo(buffer Part *ps, f32 dt) {
    parallel integra ALL (size_t w : 0..4; Part *p, size_t i : ps; (dt)) {
        p->vx += dt * p->ax;
        p->vy += dt * p->ay;
        p->x  += dt * p->vx;
        p->y  += dt * p->vy;
    }
}
```

```c
void fis_passo(keel_buffer_fis_Part *ps, f32 dt) {
    keel_buffer_fis_Part *keel__c = ps;
    size_t keel__n = keel_buffer_fis_Part_length(keel__c);
    size_t keel__s = (keel__n + 4 - 1) / 4;

    #pragma omp parallel for schedule(static, 1) num_threads(4) \
                firstprivate(dt) shared(keel__c, keel__n, keel__s)
    for (size_t w = 0; w < 4; w++) {
        size_t keel__lo = w * keel__s;
        size_t keel__hi = keel__lo + keel__s < keel__n ? keel__lo + keel__s : keel__n;
        for (size_t i = keel__lo; i < keel__hi; i++) {
            fis_Part *p = keel_buffer_fis_Part_ptr(keel__c, i);
            p->vx += dt * p->ax;
            p->vy += dt * p->ay;
            p->x  += dt * p->vx;
            p->y  += dt * p->vy;
        }
    }
}
```

**As duas travessias do fonte são os dois laços do gerado**, e o de fora é o que o OpenMP reparte. O laço interno é um `for` sobre índice contíguo, que é a forma que o compilador C auto-vetoriza.

E note o que o `#pragma` compra: sem `-fopenmp` a diretiva é ignorada, este mesmo arquivo compila, e o resultado é o mesmo — as k faixas continuam no gerado, só correm em sequência. **O que torna isso conformante, e não um caminho degradado, é o contrato de `parallel` — que autoriza paralelismo sem prometê-lo — e ele está no §4.7.**

| No exemplo | Especificado em |
| --- | --- |
| as três seções do cabeçalho — workers, travessia, captura —, o nome e a política | §4.7 `parallel` |
| a fórmula das faixas, e a ordem não definida entre elas | §4.7 `parallel` |
| o que acontece sem OpenMP, e o diagnóstico **108** | §4.7 `parallel` |


---

## 3. Gramática (EBNF)

A gramática é de **ilha**. O terminal central é `<opaco>` — *sequência balanceada de tokens que a gramática não analisa como C* —, e tudo que não é construção de keel reduz a ele. Grupo condicional dentro de `<opaco>` continua balanceado quando as alternativas concordam (§1.5).
É por isso que ela cabe em uma página: ela não descreve C, descreve onde o C é interrompido.

**`<opaco>` não é cópia verbatim, e a diferença importa.** Ele é o *placeholder*
de uma região onde keel não sabe — nem quer saber — qual é a estrutura do C.
Dentro dela, o que for construção de keel continua sendo reconhecido pelas outras
produções desta gramática: verbo qualificado (§3.4), açúcar de `[ ]` sobre
símbolo que keel declarou (§4.6), e `..` em região delimitada. O que sobra
depois disso é que atravessa intacto.

```keel
for (size_t i = 1; i < slice.length(pts); i++)          /* cabeçalho opaco…    */
    total += geom.dist(pts[i-1], pts[i]);               /* …com três ilhas     */

while (fgets(buffer.ptr(linha), (int)buffer.capacity(linha), fp))
    ;                                                   /* duas, uma sob cast  */
```

Nenhuma dessas linhas é descrita por produção alguma da gramática, e nas três o
que é de keel é reescrito. Ler `<opaco>` como "texto que sai idêntico" faria os
quatro exemplos do §2 estarem errados; o que sai idêntico é o **resíduo** —
`fgets`, `(int)`, `fp`, `total +=`, `i++`.

Notação: `::=` define, `|` alterna, `{ }` repete zero ou mais vezes, `[ ]` é opcional, `' '` é terminal literal.

### 3.1 Unidade e topo

```ebnf
unidade      ::= decl-modulo { item-topo }

decl-modulo  ::= 'module' nome-modulo [ binder-dim ] [ binder-tipo ] ';'
nome-modulo  ::= IDENT { '.' IDENT }
binder-dim   ::= 'dim' IDENT { ',' IDENT }   /* simétrico com `type`; a forma literal saiu */
binder-tipo  ::= 'type' IDENT { ',' IDENT }

item-topo    ::= import | import-c | extern-c | decl-topo | <opaco>

import       ::= 'import' nome-modulo [ 'as' IDENT ] [ 'types' ] ';'
import-c     ::= 'import_c' ( cabecalho-sistema | STRING ) ';'
extern-c     ::= 'extern_c' '{' <opaco> '}'

decl-topo    ::= [ 'pub' | 'priv' ]
                 ( decl-modificador | decl-instancia | decl-maquina
                 | decl-funcao | decl-keel | <opaco> )

decl-modificador ::= 'modifier' IDENT [ 'byref' ] '{' <opaco> '}'
decl-instancia   ::= 'instance' tipo-keel ';'
decl-maquina     ::= 'cofsm' IDENT lista-estados [ IDENT ] ';'
lista-estados    ::= '[' IDENT { ',' IDENT } ']'

decl-funcao   ::= retorno declarador-fn ( bloco | ';' )
retorno       ::= { spec-c } ( tipo-keel | <opaco-sem-parenteses> )
declarador-fn ::= { '*' { qual-c } } IDENT '(' [ params ] ')'
params        ::= 'void' | param { ',' param } [ ',' '...' ]
param         ::= ( tipo-keel | <opaco-sem-parenteses> )
                  { '*' { qual-c } } [ IDENT ] { sufixo }
spec-c        ::= 'inline' | 'static' | 'extern' | '_Noreturn'
                | '_Thread_local' | 'alignas' '(' <opaco> ')'
                | '[[' <opaco> ']]' | qual-c
```

**A função é a única forma de topo cujo corpo não é opaco**, e por isso precisa de produção própria. É dela que saem o nome a prefixar (§4.1), a separação
cabeçalho/corpo que `pub` e `priv` posicionam (§4.1), a reescrita dos parâmetros
de tipo keel, e o tipo de retorno de que o `return` do `defer` depende (§4.7).
`<opaco-sem-parenteses>` é o `<opaco>` com a restrição de não conter
`(` de topo: é o que sobra do especificador de retorno ou de parâmetro depois de
separado o declarador.

**Como ela se reconhece**, sem lookahead ilimitado e sem consultar tipo nenhum: varre-se até o primeiro `;` ou `{` de topo; é `decl-funcao` se nessa janela há um grupo `( )` de topo, não precedido por `=` de topo, cujo `)` é o último token antes do `{` ou do `;`. O **nome declarado** é o `IDENT` imediatamente anterior a esse `(` — a mesma varredura que o `constexpr` já usa (§4.2). Se o token anterior for `)`, o declarador é aninhado, o objeto é ponteiro para função, e a forma é `decl-keel`.

```keel
int f(void);          /* decl-funcao, sem corpo                 */
int f(void) { ... }   /* decl-funcao, com corpo                 */
int (*f)(void);       /* decl-keel: o `(` vem depois de `)`     */
int x = f(1);         /* decl-keel: há `=` de topo antes do `(` */
```

### 3.2 Tipos e declarações

```ebnf
decl-keel    ::= { spec-c } especificador declarador [ '=' <opaco> ]
                 { ',' declarador [ '=' <opaco> ] } ';'
               | { spec-c } especificador declarador '=' <opaco> cauda-else
               | decl-array | decl-constexpr

cauda-else   ::= 'else' ( bloco | <opaco> ';' )

especificador ::= tipo-keel
tipo-keel     ::= modificador argumento { argumento } | tipo-nulario
tipo-nulario  ::= nome-qualificado
modificador   ::= nome-qualificado [ '(' valor-dim { ',' valor-dim } ')' ]
valor-dim     ::= NUM | IDENT
argumento     ::= { qual-arg } ( tipo-keel | nome-qualificado | tipo-base )
                  { qual-arg }
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

**`tipo-base` existe porque `char` e `bool` são palavra-chave do C**, e o lexer as
emite como TK_CKW (§3.6) — não casariam `nome-qualificado`, que é `IDENT`. Elas
entram porque §4.2 as registra no módulo `keel`: keel guarda o nome, não o
significado. Os outros nomes da camada zero — `size_t`, `ptrdiff_t`, `uintptr_t`
— já são `IDENT` e casam sem ajuda. `buffer char` e `slice const char` são as
formas que a stdlib inteira usa, e é por elas que a produção existe; a proibição
do §4.2 continua valendo para o resto — `buffer int` e `buffer float` são erro,
porque têm grafia keel alternativa e duas grafias dariam duas instâncias.

**`tipo-nulario` é tipo de keel sem argumento**, e a produção existe porque nem
todo tipo da linguagem é instanciado: `arena a;`, `range r;`, e todo nome
injetado por `import … types` — `string s;`, `strbuf sb;` — têm **dois** tokens,
e não a sequência de três em que a regra de contagem do §3.5 se apoia.

Ele se reconhece por **tabela, não por contagem**: `nome-qualificado` casa
`tipo-nulario` quando o nome está registrado na tabela de keel como tipo sem
parâmetro. `arena a;` é declaração de keel porque keel declarou `arena`;
`Foo a;` atravessa opaco porque keel nunca ouviu falar de `Foo`. É a invariante
do §1.3 aplicada ao reconhecimento — keel decide sobre os próprios símbolos e
sobre nenhum outro —, e é a mesma consulta que a regra de redeclaração já faz
sobre o segundo identificador do par.

**`ref` entra em `qual-c`, e é posicional:** aceito só depois de `*` no declarador — `i32 *ref p` —, nunca antes do tipo. Some no lowering.

**A quantidade de `argumento` não é livre:** é a aridade do binder de tipo do módulo, que o `import` registrou antes de o uso ser parseado (§4.9). `map i32 stack f32 m;` são dois argumentos justapostos, e a vírgula só existe na declaração `module`. **`qual-arg` aceita os dois lados** — `buffer char const` e `buffer _Atomic u32` são a mesma instância, porque o nome canônico move o qualificador para antes do tipo em ordem fixa (`backend §2.1`).

**`{ spec-c }` cobre o prefixo do C** — `static`, `alignas(64)`, `_Thread_local` —, e é ele que faz `alignas(64) array u8 memo[N];` ser reconhecido em vez de atravessar opaco: o `alignof` do símbolo é o que o construtor da arena lê (`backend §5.3`).

### 3.3 Statements

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

stmt-keel    ::= decl-keel | decl-maquina
               | defer | foreach | apply | parallel
               | cofsm | coseq | copar | co-status | cobreak

defer        ::= 'defer' [ '[' opcoes-defer ']' ] corpo-defer
opcoes-defer ::= 'later' | 'now' captura-tipada
captura-tipada ::= entrada { ',' entrada }
entrada      ::= <opaco> IDENT
corpo-defer  ::= bloco | <opaco> ';'
captura      ::= '(' IDENT { ',' IDENT } ')'

foreach      ::= 'foreach' '(' binder ',' binder ':' contentor ')' stmt
               | 'foreach' '(' binder ':' contavel ')' stmt
binder       ::= tipo-binder IDENT
tipo-binder  ::= ( tipo-keel | nome-qualificado | 'auto' ) { '*' }
apply        ::= 'apply' '(' tipo-binder ',' contentor ',' IDENT
                 { ',' <opaco> } ')' ';'

parallel     ::= 'parallel' IDENT politica
                 '(' binder ':' contavel ';'
                     binder ',' binder ':' contentor
                     [ ';' captura ] ')' bloco
politica     ::= 'ALL' | 'ANY' | NUM | nome-qualificado

cofsm        ::= 'cofsm' IDENT '(' <opaco> ')' bloco-estados
coseq        ::= 'coseq'  IDENT bloco-chamadas
copar        ::= 'copar'  IDENT politica bloco-chamadas
bloco-estados  ::= '{' { IDENT ':' { stmt } } '}'
bloco-chamadas ::= '{' { <opaco> ';' } '}'

co-status    ::= ( 'cowin' | 'cofail' | 'coagain' ) [ '(' <opaco> ')' ] ';'
cobreak      ::= 'cobreak' ';'
```

**A máquina se declara e se opera com a mesma palavra**, e as duas formas se
separam pelo token depois do nome: `[` ou `;` é `decl-maquina`, `(` é `cofsm`. A
declaração vale nos dois níveis — no topo, quando o estado sobrevive às chamadas;
dentro da função, quando nada escapa dela (§4.8). O `IDENT` final e opcional de
`decl-maquina` é a variável de estado, e escrevê-lo é o que diz que ela é local.

**Não há produção de transição.** A variável de estado tem nome e está à vista nos
dois casos, então mudar de estado é atribuição comum do C, e cai em `<opaco>`.

**`bloco-chamadas` é só uma lista de chamadas**, opacas como qualquer outra
expressão. `coseq` e `copar` não têm rótulo porque não têm salto: nada referencia
um nome de etapa, e o estado delas não escapa (§4.8).

**`stmt-c` não descreve o C, descreve por onde se desce até o C ser
interrompido.** Sem ela, o `<opaco>` balanceado engoliria o bloco de um `if` e o
`foreach` lá dentro nunca seria visto. As formas listadas são exatamente as que
alguma regra de keel precisa enxergar: as de laço e `switch` porque `break` e
`continue` só recebem o cleanup do `defer` quando deixam o escopo, `return`
porque é ponto de saída, `goto` e `rotulo` porque o salto pode sair do escopo
(§4.7). O que está entre parênteses continua opaco em todas elas.

**Onde o `<opaco>` de statement para:** ele consome até o `;` de topo, mas para
diante de `{` — que reduz a `bloco` — e diante de palavra contextual de keel em
posição inicial de statement. É essa parada que faz o `else` da §3.5 se decidir
sem lookahead: um `if` de verdade casa `stmt-c`, e o `else` que sobra só pode ser
cauda de declaração.

Dentro de `bloco-estados`, um `IDENT ':'` de topo é rótulo de estado, não
`rotulo` de C.

### 3.4 Contêiner — a única posição tipada

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

Só a posição do contêiner é analisada **como contêiner**; índices, valores e demais argumentos são `<opaco>`:

```keel
buffer.push(pool[3], f(x, y));         /* pool[3] é contêiner; f(x,y) é opaco */
buffer.length(buffer.get(bs, i + j));  /* buffer.get(bs, …) é contêiner       */
```

`f(x,y)` ser opaco quer dizer que ele não é posição de contêiner — não que keel deixe de olhar dentro dele. Se contivesse `buffer.length(b)`, o verbo seria reescrito ali como em qualquer outra região opaca (§3).

Expressão fora dessa gramática, em posição de contêiner, é recusada: `f(y)[i]` e `(cast)x[i]` exigiriam tipar expressão do C.

### 3.5 As ambiguidades, e como se separam

Todas se resolvem por lookahead fixo, sem consultar tipo nenhum.

| Ambiguidade | Como se separa |
| --- | --- |
| `int f(void);` × `int (*f)(void);` × `int x = f(1);` | token anterior ao `(` de topo, e presença de `=` de topo antes dele (§3.1) |
| `buffer i32 xs;` × `char buffer[256];` | sequência de N identificadores com N ≥ 3 nunca é declaração válida em C |
| `arena a;` × `Foo a;` | tabela: `arena` é tipo que keel declarou, `Foo` não é nome que ele conheça (§3.2) |
| `cofsm mm [A,B] s;` × `cofsm mm (s) {…}` | token depois do nome: `[` ou `;` declara, `(` opera (§3.3) |
| `import buffer;` × `buffer i32 xs;` | token seguinte ao nome nu: `.` ou `;` é qualificação, tipo é modificador |
| `instance coll.stack i32;` × `stack i32 pilha;` | `instance` na primeira posição — e por isso modificador de nome `instance` é error |
| `v[1,2,5]` × `w[1,2,5]` | reescrita só sobre símbolo `array` conhecido; o resto é o operador vírgula do C |
| dois binders × um binder no `foreach` | presença da `,` antes do `:` |
| `else` de saída × `else` de default | primeiro token depois do `else`: `{` ou palavra-chave de salto do C é saída; o resto é default |

**São dois mecanismos, e não um.** Modificador com argumento se separa por
**contagem** — três nomes seguidos não são declaração de C. Tipo sem argumento se
separa por **tabela** — dois nomes seguidos são declaração de C o tempo todo, e o
que decide é keel reconhecer o primeiro como símbolo seu. O segundo é o mais
fiel à invariante dos dois: ele não afirma nada sobre o C, apenas pergunta se o
nome é da casa.

**A linha do `instance` tem uma ressalva própria, e é a única do documento.**
`instance coll.stack i32;` ocupa exatamente a mesma forma que `stack i32 pilha;`
— `IDENT IDENT IDENT ;` nas duas —, e o que as separa é a posição da palavra. Só
que a posição é frágil de um jeito que as outras cinco não são: se alguém
declarar um **modificador chamado `instance`**, as duas leituras viram
`instance(coll.stack(i32))` declarando uma variável, e a declaração de
instanciação — e aí **não se separam por lookahead nenhum**, porque não há token
adiante que decida.

Por isso modificador de nome `instance` é o error **111**
`modificador-chamado-instance`. É a única palavra da linguagem cujo nome é
protegido, e ela é protegida por não haver saída: em toda outra ambiguidade da
tabela existe um token que decide, e aqui não existiria.

E uma cláusula extra em posição de parâmetro: o modificador é reconhecido quando seguido do argumento e de identificador, `*`, `,` ou `)`. Sem ela,
`pub void fill(buffer i32 *);` não casaria.

### 3.6 Léxico

| Elemento | Regra |
| --- | --- |
| Emenda de linha | `\` seguido de newline junta as linhas **antes** da tokenização |
| Comentário | `/* */` e `//`, ignorados |
| Diretiva | `#` até o newline não-emendado; **token único**, repassado verbatim, conteúdo opaco. A **palavra** decide a classe: `#if`/`#ifdef`/`#ifndef` dão `TK_PPC_IF`; `#elif`/`#elifdef`/`#elifndef`/`#else` dão `TK_PPC_ELSE`; `#endif` dá `TK_PPC_END`; todo o resto é `TK_PPC`. Nada além da palavra é lido |
| Literal | `TK_STRING` e `TK_CHAR`, com escapes e com os prefixos de codificação do C (`L`, `u8`, `u`, `U`) por lista fixa; newline não-emendado dentro é o error **43** `literal-com-newline` |
| Pontos | ordem fixa de casamento: `...`, depois `..`, depois `.` |
| Balanceamento | o lexer emparelha `()`, `[]` e `{}` sobre o texto tokenizado; par faltando é o error **40** `delimitador-sem-par`, com a posição da abertura. Grupo condicional conta **por alternativa**, e elas têm de concordar — error **41** (§1.5) |
| Keywords C | conjunto fechado, próprio de keel, emitido como TK_CKW — nunca IDENT |
| Tipos | o lexer não conhece tipo, modificador nem builtin — toda decisão é do parser |

**A ordem dos pontos** existe porque a sintaxe de intervalo esbarra em duas regras do C, e as duas mordem em direções opostas: *pp-number* é guloso, e `2..7` seria um número só, inválido; e `...` é token real, e um `..` guloso quebraria `printf(const char *, ...)`. Nenhuma das duas ordens recusa C válido — `2..7` já não é constante legal.

**`..` entra como pontuação, e nada mais.** Não existe token de intervalo: o lexer não o funde com os operandos, porque `xs[(a+1)..(b*2)]` é legal e um token não contém sequência balanceada. A forma do intervalo é do parser (§3.4).

**Nada disso alcança região opaca**: dentro de literal é `TK_STRING`, dentro de diretiva é o token da diretiva — `TK_PPC` ou uma das três classes de condicional, todas de conteúdo opaco —, e em `extern_c` não pode ocorrer porque `..` não é C. Um `..` nu, fora dessas regiões, só pode ser intervalo — é por isso que ele dispensa delimitador.

**As três classes de condicional existem para o balanceamento, e param aí.** A
linha inteira continua sendo o valor do token e sai verbatim; a condição nunca é
lida. `#elif` cai na mesma classe de `#else` porque, sem avaliar condição, não há
diferença entre os dois — a estrutura que ambos produzem é a mesma: fecha a
alternativa anterior, abre a próxima. E as três são exatamente as três transições
que a contagem por alternativa precisa (§1.5): marcar a profundidade na abertura,
conferir e restaurar a cada alternativa, conferir e sair no fim. Não há classe
para `#define`, `#include` ou `#pragma`: nenhuma delas move delimitador.

**O balanceamento é o que faz uma chave sem par dentro de `extern_c` ser diagnóstico de keel**, apontando a abertura, em vez de erro do compilador C dezenas de linhas adiante.

### 3.7 Palavras do núcleo

O conjunto é **fechado**, e é este. Nenhuma é rígida: toda palavra vale só na posição em que keel a espera, e nenhuma custa um identificador ao usuário.

| Grupo | Palavras |
| --- | --- |
| **Unidade** | `module` `import` `import_c` `extern_c` `as` `types` `pub` `priv` |
| **Tipo composto** | `array` `constexpr` `ref` `type` `dim` `modifier` `instance` `byref` |
| **Fluxo** | `defer` `now` `later` `foreach` `apply` `parallel` `ALL` `ANY` |
| **Cooperativo** | `cofsm` `coseq` `copar` `cobreak` `cowin` `cofail` `coagain` |
| **Resultado** | `else` — cauda de declaração (§4.7). É palavra-chave do C, e a única palavra de keel que não pode sombrear identificador |

E três formas de **açúcar**, que não são palavras: são sintaxe sobre a tabela de símbolos, e por isso não levam qualificação.

| Forma | Significa |
| --- | --- |
| `x[i]` · `v[i,j,k]` | `*ptr(x,i)` — lvalue, não cópia |
| `x[a..b]` · `x[a..]` · `x[..b]` · `x[..]` | vista — **rvalue** |
| `a..b` | intervalo |

**A linha das keywords carrega duas regras do §3.5, e não é cosmética.** Sem
TK_CKW, `unsigned long x;` seria três identificadores seguidos e a regra de
contagem o leria como tipo keel; e `const char *p` em posição de parâmetro
casaria a cláusula do modificador, pela mesma confusão. O parser casa TK_CKW
**por grafia**, e só onde a gramática nomeia uma: `qual-c`, `spec-c`, `stmt-c`,
o `void` de `params`, o `tipo-base` de `argumento`.

Disso saem duas consequências. **O error 16 depende do lexer:** é por `int` e
`float` serem TK_CKW que o parser os vê em posição de `argumento`, não casa
`tipo-base`, e diagnostica em vez de gerar `keel_buffer_int` calado. E **palavra
do C que também é palavra de keel não precisa de reclassificação** — `restrict`,
`auto`, `else`, `inline`, `static` e `constexpr` são TK_CKW, e quem dá o segundo
sentido é a posição, como em todo o resto do §3.5.

O conjunto é **de keel, não do dialeto alvo.** Em C11 `bool` é macro de
`<stdbool.h>`; em C23 é palavra-chave. keel roda antes do `cpp` e não abre
header, então não tem como perguntar: a lista é fechada e é dele.

### 3.8 Palavras do C

O conjunto que o lexer emite como **TK_CKW** é fechado e é **de keel**, não do dialeto alvo: keel roda antes do `cpp` e não abre header, então não tem como perguntar ao compilador quais palavras existem. É o conjunto do C23.

| Grupo | Palavras |
| --- | --- |
| **Tipo e qualificador** | `void` `char` `short` `int` `long` `float` `double` `signed` `unsigned` `bool` `const` `volatile` `restrict` `inline` `auto` `register` `extern` `static` `typedef` `struct` `union` `enum` `constexpr` `typeof` `typeof_unqual` `thread_local` |
| **Fluxo** | `if` `else` `switch` `case` `default` `for` `while` `do` `break` `continue` `goto` `return` |
| **Operador e literal** | `sizeof` `alignof` `alignas` `static_assert` `true` `false` `nullptr` |
| **Formas com sublinhado** | `_Alignas` `_Alignof` `_Atomic` `_BitInt` `_Bool` `_Complex` `_Decimal32` `_Decimal64` `_Decimal128` `_Generic` `_Imaginary` `_Noreturn` `_Static_assert` `_Thread_local` |

Destas, a gramática nomeia dezenove — as de `qual-c`, `spec-c` e `stmt-c` (§3.1, §3.3), o `void` de `params`, o `char` e o `bool` de `tipo-base` (§3.2), o `auto` de `tipo-binder`, o `else` de `cauda-else` e o `constexpr` de `decl-constexpr`. As demais atravessam como parte de `<opaco>`, e estão listadas mesmo assim porque é a classificação como TK_CKW — e não o uso na gramática — que sustenta as regras de reconhecimento do §3.5.

**Palavra de keel não está aqui.** `module`, `buffer`, `defer` e as demais do §3.7 são IDENT, e é por isso que são contextuais e não custam um identificador ao usuário. Só seis tokens são palavra do C **e** de keel ao mesmo tempo — `restrict`, `auto`, `else`, `inline`, `static`, `constexpr` —, e neles quem dá o segundo sentido é a posição.

### 3.9 O que keel conhece

A tabela de símbolos é o que as regras de §3.2 e §3.5 consultam. Ela tem duas origens, e **nada além delas**:

| Origem | O que entra |
| --- | --- |
| **prelúdio** (§1.7) | tipos nus: `i8`…`i64`, `u8`…`u64`, `f16`, `bf16`, `f32`, `f64`, `bool`, `char`, `size_t`, `ptrdiff_t`, `uintptr_t`, `arena`, `buffer`, `slice`, `range`, `corot`, `outcome`; e os qualificadores `keel`, `arena`, `buffer`, `slice`, `range`, `corot`, `outcome` |
| **grafias de `<stdint.h>`** | `int8_t`…`int64_t`, `uint8_t`…`uint64_t`, normalizadas para a grafia keel — não geram instância duplicada (§4.2) |
| `import M [as A]` | o qualificador `M` ou `A` |
| `import M types` | os nomes de tipo que `M` exporta |
| `extern_c { }` | os **nomes**, e só: keel não classifica o que são |
| **declarações do arquivo** | tipos e `typedef`, funções, variáveis de topo, `constexpr`, símbolos `array`, máquinas `cofsm`, nomes de `coseq`/`copar`/`parallel`, e constantes de enum escopadas no tipo |

**Nome vindo de `#include` não está na tabela**, e é essa ausência que a §7.2 escreve como obrigação.

A coleta é em **duas passagens**: a primeira varre o arquivo inteiro e registra tudo — inclusive dentro de corpo de função, por causa do `cofsm` (§4.8) —, e a segunda resolve e gera. Por isso um símbolo pode ser usado antes de aparecer, e por isso a ordem de declaração no fonte não precisa espelhar a ordem no C gerado.

### 3.10 Reconhecimento das palavras contextuais

> **Não há palavra-chave rígida.** Nenhuma palavra de keel custa um identificador ao usuário: `int defer = 5;` e `char buffer[256];` continuam sendo declarações do C, e continuam significando o que sempre significaram.

Cada palavra vale na posição em que a gramática a espera. As ambiguidades com C estão no §3.5; esta seção diz o resto.

#### A rigidez é regional

Dentro de `extern_c` **nenhuma** palavra de keel tem sentido. Uma regra só, sem exceção por palavra, e o lexer a implementa com um estado ligado/desligado sem pilha, porque `extern_c` só ocorre em nível de arquivo — aninhá-lo é o error **7** `extern-c-aninhado`.

```keel
extern_c {
    void *defer(void *ctx);      /* função C chamada `defer`; passa verbatim */
}
```

#### Fora dali, vale a posição

Uma palavra vale desde que não exista declaração **local** de mesmo nome em escopo. Usar palavra sombreada é o warning **44** `sombreamento`. Com `buffer`, que é ao mesmo tempo qualificador do prelúdio e modificador:

| Escrita | Tokens | Leitura |
| --- | --- | --- |
| `char buffer[256];` | `ID ID [` | variável do usuário |
| `buffer[i] = 0;` · `buffer = p;` | `ID [` · `ID =` | variável do usuário |
| `buffer b;` (tipo do usuário) | `ID ID ;` | declaração C |
| `buffer.length(b)` | `ID . ID (` | **keel** — qualificação |
| `buffer i32 xs;` | `ID ID ID ;` | **keel** — modificador |
| `buffer slice i32 buf;` | `ID ID ID ID ;` | **keel**, aninhado |

A última linha mostra o mecanismo: o argumento é lido **recursivamente**, então `buffer slice i32` é `buffer(slice(i32))` (§4.3), sem regra nova para o aninhamento.

#### Como o `.` se resolve

O `.` não é vaga exclusiva de keel — ele é acesso a campo (`p.x`) e designador de inicializador (`{ .x = 1 }`). O identificador à esquerda é **alias de módulo**, **nome de tipo declarado em keel**, ou **expressão de contêiner** (§3.4), decidido por consulta à tabela (§3.9), nessa ordem, sem tipar expressão nenhuma. Qualquer outra coisa é acesso a campo do C.

Declaração local de mesmo nome de um módulo **desliga a qualificação naquele escopo**, com o warning 44:

```keel
void f(void) {
    i32 buffer = 0;           /* warning 44 */
    buffer.length(b);         /* já não é qualificação: é acesso a campo */
}
```

**Alias de módulo seguido de `(` é vaga reservada** — error **126** `alias-com-argumento`. Hoje a forma não significa nada em keel, e a mensagem diz o contorno: renomear o alias no `import`.

```keel
import std.rank as rank;
rank(3,2).axis(v, 0, i);          /* error 126: vaga reservada */
```

A reserva existe porque a forma **é C válido** — chamada devolvendo struct, seguida de acesso a campo — e porque ela é a candidata natural para nomear a instância de um módulo com mais de um `dim` (§4.9). Dar sentido a ela mais tarde, sem reservar agora, faria um programa que hoje atravessa verbatim mudar de sentido em silêncio, que é o princípio 1. **Reservar é seguro nas duas direções:** adotar depois transforma erro em aceito, e desistir transforma erro em passagem verbatim — nenhum programa que funcionava muda em qualquer dos dois casos.

Alias de módulo e nome de tipo com a mesma grafia no mesmo arquivo é o error **86** `alias-e-tipo-colidem` — **exceto quando os dois vêm do mesmo módulo**, que é a forma do §1.7:

```keel
import keel.buffer as buffer types;   /* alias `buffer` e tipo `buffer` — legal */
```

Ali não há ambiguidade a resolver: as duas leituras de `buffer` levam ao mesmo módulo, e é a forma que a base inteira usa. O error existe para o caso em que levam a lugares diferentes.

#### `import` e sombreamento são regras diferentes

**`import` sem `types` não sombreia nada** — não é regra, é consequência: nome externo é qualificado por omissão, então o token nu não se refere a símbolo importado. Acrescentar um `import` desses não muda o sentido de nenhuma linha já escrita, e é a única garantia de estabilidade de edição que este documento dá (§7.6).

**`import M types` é a exceção**, e pode mudar o sentido de uma linha já escrita:

```keel
import_c <foo.h>;      /* foo.h tem um typedef ... Point; */
Point p;               /* token opaco: copiado verbatim   */

import geom types;     /* a partir daqui, Point é geom.Point */
```

Um nome que era token opaco do C passa a ser reescrito, sem que nenhum dos dois lados acuse. Daí a injeção ser **explícita**, dois `types` injetando o mesmo nome ser o error **10b**, o nome injetado ser sombreável por declaração local com o warning **10c**, e o import emitir o `info` **10d** listando o que entrou nu.

#### As palavras dos módulos genéricos

| Escrita | Tokens | Por que não colide |
| --- | --- | --- |
| `module coll type T;` | `module ID type ID ;` | `type` só vale aí; `int type = 5;` continua sendo variável do usuário |
| `pub modifier stack {` | `ID ID {` | `ID ID {` não inicia declaração em C |

`instance` é o terceiro caso, e tem a ressalva do §3.5.

#### Redeclaração que keel não enxerga

keel não classifica declarações do C, então uma redeclaração local de um nome que ele conhece passaria despercebida e faria o despacho errar:

```keel
buffer i32 xs;          /* escopo de arquivo */
void f(void) {
    FILE *xs;           /* keel não sabe que isto é declaração */
    xs[3];              /* reescreveria como buffer — errado   */
}
```

> Ao encontrar os padrões `IDENT IDENT` ou `IDENT * IDENT` em início de statement, onde o **segundo** identificador é nome de símbolo keel conhecido, keel emite o error **19** `redeclaracao-de-simbolo`.

Ele não classifica tipo nenhum — apenas recusa o caso em que erraria (princípio 5). É o único resíduo da invariante que aparece como restrição ao usuário.

> Razão: `rationale §3.10`.

---

## 4. As construções, em uso

Uma seção por construção, da forma à regra por extenso. O que cai fora daqui são as razões, que estão em `keel-rationale.md`.

### 4.1 Unidade

O módulo é a unidade de keel, e é o que o C não tem. Ele resolve três coisas com o mesmo nome: identidade, namespace e prefixo de símbolo.

#### `module` — o módulo é o arquivo

```keel
module net.http;          /* símbolos saem net_http_*, arquivo em net/http.k */
pub i32 get(const char *url);
```

```c
i32 net_http_get(const char *url);
```

A declaração é **obrigatória** e vem antes de tudo, exceto comentário e espaço em branco — nem diretiva de pré-processador a precede. Por ser puramente posicional, `module` é palavra só nesse ponto. Arquivo sem ela é o error **1** `sem-module`.

O nome qualificado contém a estrutura de pastas relativa à raiz de busca, e **o caminho faz parte da identidade**: `src/http/security.k` declara `module http.security;` e `src/security.k` declara `module security;` — módulos distintos ainda que o stem coincida. Divergência é o error **2** `module-fora-do-caminho`.

O nome vira prefixo de todo símbolo exportado, e portanto vira **ABI**. Daí as regras de nome de arquivo:

- o stem tem que ser **identificador C válido** — hífen é proibido, não normalizado: error **3** `stem-invalido`;
- nomes que diferem só por caixa são o error **4** `stem-ambiguo-por-caixa`, porque coexistem no Linux e colidem no macOS e no Windows.

**Colisão de símbolos** é verificada antes de gerar qualquer coisa, sobre o **conjunto de símbolos exportados** dos módulos que a tradução alcança — não sobre os prefixos, que não bastariam:

```plain
módulo net       com função http_get   → net_http_get
módulo net.http  com função get        → net_http_get   ← colisão
```

Nenhum dos dois prefixos é igual ao outro, e a colisão só apareceria no link. É o error **5** `simbolo-colidido`, com os dois caminhos na mensagem. O conjunto inclui tipos, tags e **constantes de enum**.

**O alcance é o grafo de `import`, e não o programa.** keel traduz um módulo por vez e não vê o programa inteiro (`cgen-tool-spec.md §4.3`), então o conjunto verificado é o do módulo em tradução mais o fecho transitivo dos seus imports. A garantia é exatamente esta:

> **Dois módulos ligados por `import`, direta ou transitivamente, nunca colidem em silêncio.** Dois módulos que não se importam podem colidir, e quem acusa é o linker.

O limite é onde ele tem que estar. Módulos que se importam **têm** de compor, e a colisão entre eles é erro de projeto que só o keel enxerga, porque só ele conhece a regra de mangling. Módulos que não se conhecem não compõem por definição, e a colisão entre eles é a mesma que dois `.c` definindo o mesmo global — falha conhecida, mensagem conhecida, do linker.

Fechar isso exigiria uma etapa global sobre todos os fontes antes de gerar qualquer um, que é a coisa que o modelo por arquivo existe para não ter: ela derrubaria a compilação separada da §1.4 e faria o gerado depender de quais outros módulos estavam presentes na invocação.

#### `import`

```ebnf
import ::= 'import' nome-modulo [ 'as' IDENT ] [ 'types' ] ';'
```

| Forma | Verbo | Tipo |
| --- | --- | --- |
| `import M;` | `M.verb()` | `M.Tipo` |
| `import M as m;` | `m.verb()` | `m.Tipo` |
| `import M types;` | `M.verb()` | `Tipo` |
| `import M as m types;` | `m.verb()` | `Tipo` |

Faz três coisas, nesta ordem: garante que o módulo esteja disponível, parseando-o inteiro se o gerado não existe ou está desatualizado; carrega a **interface pública** na tabela de símbolos — tipos, protótipos, variáveis e campos de struct com tipo keel, com `priv` lida e não registrada; e emite a inclusão no C gerado.

`as` renomeia o qualificador; `types` decide o que entra nu. São independentes e **puramente de fonte**: o símbolo gerado é sempre o do módulo de origem, e nem a checagem de colisão, nem a ABI, nem o depfile enxergam a diferença.

- **A ordem é fixa** — o `as` renomeia o *módulo*, então fica colado no caminho. `import M types as m;` é o error **10a** `import-ordem-trocada`.
- **O alias substitui, não acrescenta.** `import M as m;` dá `m.Tipo` e tira `M.Tipo`.
- **Um alias por import.** O qualificador é identidade, não coordenada.

**Todo nome externo é escrito qualificado**, e por omissão nome curto não existe. Por isso **a visibilidade é transitiva sem risco**: se `A` importa `B` e `B` importa `C`, `A` enxerga tudo que é público em `C`, e não há vazamento de namespace a evitar. Usar símbolo de módulo não importado diretamente é o `info` **49** `import-indireto` — higiene de dependência, não erro.

**`types` injeta apenas o espaço de tipos do C** — `typedef` e tags de `struct`, `union` e `enum`. Função e variável continuam qualificadas pelo módulo; constante de enum, pelo **tipo** (§4.3). A injeção é **aditiva**: `M.Tipo` continua válido ao lado de `Tipo`.

Dois imports com `types` injetando o mesmo nome é o error **10b** `types-duplicado`; nome injetado sombreado por declaração local é o warning **10c** `types-sombreado`; e o import emite o `info` **10d** `types-injetados`, listando o que entrou nu.

> **Visibilidade é transitiva; injeção não é.** `types` muda como os tokens **daquele arquivo** são lidos e não deixa vestígio na interface: se `b` escreve `Vec` nu, no header de `b` sai `c_Vec`, exatamente como sairia se ele tivesse escrito `c.Vec`. Não é que a propagação seja proibida — não existe canal por onde ela aconteceria. Quem escrever `Vec` num terceiro módulo recebe o diagnóstico do compilador C, com o `#line` na linha certa, porque para keel aquilo é token que ele nunca viu.

#### Import circular é error

Não por limitação de análise: **o lowering não fecha.** Em C, ciclo de valor é impossível, e ciclo por ponteiro exige declaração adiantada em vez de inclusão mútua — gerar isso obrigaria a quebrar cada `.h` em dois, destruindo a propriedade de um `.h` e um `.c` por módulo.

A tabela de módulos tem três estados — `não carregado`, `em carga`, `carregado` — e pedir um que está `em carga` é o error **6** `import-circular`, com a cadeia inteira na mensagem (`a → b → c → a`). A saída idiomática é a mesma de um programador C: o tipo compartilhado sobe para um **módulo de definições**, cuja implementação sai sem nenhuma definição — só tipos, protótipos e declarações. É o análogo keel do header.

#### As três fases, e o orçamento de análise

O import é resolvido **recursivamente e sob demanda**; módulo já carregado na mesma invocação não é reprocessado, e a partir do segundo import só sai a inclusão. Cada módulo é processado em três fases:

| Fase | O que faz |
| --- | --- |
| **1 — coleta** | varredura do arquivo inteiro: tipos, funções, variáveis, campos, marcas de visibilidade, e nome e conjunto de estados de cada `cofsm`, em qualquer profundidade. Nenhuma referência externa é resolvida |
| **2 — análise, por função** | procedência de arena e escape (§4.4), uso de filha depois de `reset`, aritmética sobre `ref`, pontos de saída para `defer` (§4.7), estados alcançados (§4.8) |
| **3 — emissão** | resolve nomes, despacha verbos, gera o conteúdo |

A fronteira entre 2 e 3 cabe numa linha: **a fase 2 é tudo que precisa enxergar mais da função do que o ponto de emissão.** O custo é uma varredura mais duas caminhadas por função, sobre a faixa de tokens dela — a fase 2 não relê o fonte.

E as fases são o **teto que a linguagem se impõe**:

> Construção nova pode exigir do parser, no máximo, coleta de declaração na fase 1 e análise dentro de um escopo na fase 2. Construção que exija análise entre funções, entre módulos, ou ponto fixo sobre o programa está fora.

É esse orçamento que põe função genérica livre e sobrecarga fora (§5), e que deixa `cofsm` e `constexpr` dentro.

#### `import_c` e `extern_c` — a fronteira com o C

```keel
import_c <stdio.h>;             /* sai na interface: dependência C pública */

extern_c {                      /* sai na implementação: privado           */
    void   *malloc(size_t);
    ssize_t read(int fd, void *buffer, size_t n);
}
```

`import_c` é passagem pura — keel não abre o header e não aprende nada. Sai na **interface** porque é a construção que declara dependência C **pública**: é ela que sustenta um `FILE *` no protótipo de uma função `pub`.

`extern_c` sai **verbatim na implementação**, de modo que o compilador C confronte a declaração com a do header real e reclame de divergência. keel não verifica; o C verifica de graça. Dele, keel registra apenas os **nomes** declarados literalmente ali, sem classificar o que são — basta para o sombreamento e para não reclamar de símbolo desconhecido. Tipo incompleto não é caso à parte: `typedef struct FILE FILE;` registra um nome como qualquer outra declaração.

1. **Só em nível de arquivo** — error **7** `extern-c-aninhado`. Um `return` dentro de bloco aninhado seria invisível ao parser e pularia o cleanup dos `defer` pendentes, silenciosamente.
2. **Nenhuma palavra de keel vale ali dentro**, `defer` inclusive (§3.10).
3. **Passa verbatim, inteiro**, diretivas incluídas, sem reindentação.
4. Uma inclusão dentro do bloco é **texto que viaja, não conhecimento**.
5. **Chaves têm que fechar** — error **40**.
6. **Não existe tipo de modificador na assinatura ali dentro**: `buffer char` é sintaxe de keel, e ali não há keel.

A regra 6 não é falta, porque a travessia acontece no **ponto de chamada**, que é código keel:

```keel
pub void dump(int fd, slice u8 dados) {
    write(fd, slice.ptr(dados), slice.length(dados));
}
```

```c
void io_dump(int fd, keel_slice_u8 dados) {
    write(fd, keel_slice_u8_ptr(&dados), keel_slice_u8_length(&dados));
}
```

**Exportar para o C não é papel de `extern_c`** — o bloco resolve uma direção só. Quando um símbolo keel precisa de **nome fixo** (`dlsym`, assembly, tabela de vetores de interrupção, API de biblioteca compartilhada), o mecanismo é um adaptador escrito em C, compilado como qualquer `.c` do projeto. Callback não precisa disto: o que se passa a uma função C é endereço, não nome, e `qsort(base, n, sz, mod_cmp)` funciona com o símbolo manglado.

#### `main` — ponto de entrada

**`main` é função comum do módulo:** recebe prefixo, vai para a interface, é pública como qualquer outra. Cada módulo pode ter a sua, e várias podem ser linkadas no mesmo programa sem colidir — o módulo carrega o próprio driver de teste. **Qual delas vira o `main` do C é decisão de build, nunca do fonte**, e é a flag `--main <módulo>` que a expressa. **O argumento é nome de módulo**, não caminho nem nome de símbolo. A flag gera uma unidade à parte, contendo só o `main` do C que chama a do módulo; a unidade do próprio módulo sai idêntica com ou sem ela, e sem a flag o conjunto compila como biblioteca. Exemplo no §2.1.

`main` definida dentro de `extern_c` é o error **8** `main-em-extern-c`; `priv main` é o **9** `main-privada`; e `main` assinada fora das duas formas do C é o **10** `main-assinatura`.

#### `pub` e `priv` — visibilidade

Controlam **posicionamento**: se a declaração cai na interface ou na implementação. É conceito de keel, porque "arquivo" não existe no C. A ausência da palavra significa `pub`. `static` mantém integralmente o sentido C onde quer que apareça, e é repassada.

> **`static` é ligação; `pub`/`priv` é posicionamento.** Eles se sobrepõem para objetos e funções apenas porque o C junta as duas coisas por convenção.

```keel
pub  typedef struct {...} Point;       /* definição do tipo na interface      */
priv typedef struct {...} Point;       /* definição do tipo na implementação  */

pub  static inline i32 fn(void) {...}  /* static inline + corpo, na interface */
priv static inline i32 fn(void) {...}  /* idem, na implementação              */

priv static f32 helper(void) {...}     /* static sem inline: só implementação */
```

- **Tipo não tem ligação.** `static typedef struct {...} Point;` não é C; esconder um tipo é posicionamento puro, e só a marca o expressa. `static` aplicada a tipo é o error **13** `static-em-tipo`.
- **`static inline` com corpo é genuinamente ambíguo** — idioma de inline em header, ou helper de ligação interna? Os dois são usos reais e o C não os distingue. Sem a marca é o error **12** `inline-sem-visibilidade`. **É o caso que motiva a construção existir.**
- **`static` sem `inline`, com corpo, não é ambíguo:** num header produziria uma cópia por unidade de tradução, sem ninguém para chamá-la de fora. Implica `priv`, e `pub static` é o error **11** `pub-static`.

Variável pública tem ligação externa: qualquer módulo que a enxergue pode ler **e escrever**. Para exportar somente leitura, `pub const`; para estado mutável encapsulado, `priv` mais funções de acesso `pub`.

> Razão: `rationale §4.1`.

### 4.2 Tipos e marcadores

#### Tipos de tamanho fixo

Definidos **pela linguagem**, por largura e formato, não por referência a um tipo do C. Vivem no módulo `keel` e entram nus pelo prelúdio (§1.7).

| keel | Definição | O backend emite |
| --- | --- | --- |
| `i8` `i16` `i32` `i64` | inteiro com sinal, complemento de dois, exatamente N bits | `typedef` + `static_assert` de largura |
| `u8` `u16` `u32` `u64` | inteiro sem sinal, exatamente N bits | idem |
| `f32` `f64` | IEEE 754 binary32 / binary64 | idem |
| `f16` | IEEE 754 binary16 | idem, sob guarda de disponibilidade |
| `bf16` | bfloat16 — 1 de sinal, 8 de expoente, 7 de mantissa | idem, sob guarda |
| `bool` `char` | booleano; unidade de caractere estreita | **nada** — são do C |
| `size_t` `ptrdiff_t` `uintptr_t` | idem C | **nada** — são do C |

As duas últimas linhas são de espécie diferente: **keel só registra o nome.** Não emite `typedef`, não fixa largura, não confere nada. Estão na camada zero porque um nome só serve como argumento de modificador se estiver ali, e `buffer size_t` precisa nomear a mesma instância em todo módulo do build.

**A grafia é a identidade do tipo.** `buffer int` e `buffer i32` gerariam dois tipos distintos, e layout idêntico não cria compatibilidade em C:

```c
keel_buffer_i32 a;
keel_buffer_int b = a;      /* error: incompatible types, ainda que i32 seja int */
```

> Os tipos aritméticos por palavra-chave do C — `int`, `unsigned`, `short`, `long`, `long long`, `float`, `double`, `long double` e suas combinações — são o error **16** `tipo-c-como-argumento` no argumento de modificador. `void` é o error **17** `void-como-argumento`, por não ter `sizeof`. `_Float16` e `__bf16` caem na mesma regra: a grafia keel é `f16` e `bf16`.

```keel
buffer int  xs;          /* error 16: use i32 */
buffer i32  xs;          /* ok */
buffer char msg;         /* ok — char é byte de texto, sem alias na família */
buffer geom.Point pts;   /* ok — tipo de usuário não tem grafia alternativa */
```

A restrição vale **só** dentro do argumento de um modificador. Todo o resto continua C: `int x = 5;`, parâmetros, retornos, campos de struct. Para casar com API C que usa `long` de verdade, o escape é `typedef long clong;` e então `buffer clong`.

Os nomes de `<stdint.h>` são aceitos e **normalizam para a grafia keel**, então `buffer int32_t` e `buffer i32` são a mesma instância. **`size_t` e `char` são permitidos e `int` não** porque a diferença é a grafia alternativa: os dois primeiros têm uma só, `int` tem três. Normalizar `int` → `i32` seria pior que recusar — numa plataforma de `int` com 16 bits, `buffer int` passaria a ter elementos de 32 bits em silêncio.

**Os formatos estreitos** `f16` e `bf16` são **formatos de armazenamento**:

> O formato estreito descreve o dado na memória; o acumulador continua sendo `f32`.

Num alvo sem unidade binary16 a aritmética é emulada pelo compilador C promovendo a `float` — mais lenta que `f32`, e correta. Eles não trazem operador, conversão implícita nem saturação: `f16` soma com `+` porque o C soma, e converte com cast porque o C converte. Disponibilidade é do alvo, e a guarda vai **dentro** do C gerado, porque keel não testa macro de plataforma (§1.4) — error **103** `formato-estreito-indisponivel`.

**O que não está aqui:**

| Fora | Por quê |
| --- | --- |
| `f128`, `i128`, `u128` | dependem de recurso opcional do C23 e de decisão de backend |
| `i4`, `u4` e larguras sub-byte | não há tipo C para meia palavra, e um elemento sem endereço é categoria nova — o empacotamento é biblioteca |
| conversão saturante | qualquer módulo keel a escreve em três linhas; entra na base, não no núcleo |

> Razão: `rationale §4.2`.

#### `array`

**Não é modificador e não gera tipo:** é um marcador que registra o símbolo na tabela de keel e some no lowering.

```keel
array i32  v[100];
array char msg[] = "keel";
array i32  m[2,3,4];
```

```c
i32  v[100];
char msg[] = "keel";
i32  m[2][3][4];
```

A tabela **não guarda o tamanho** no caso unidimensional — guarda o bit "é vetor de verdade". Por isso `array i32 v[] = {1,2,3};` e `array i32 v[] = {[9]=1};` funcionam sem keel contar nada.

O marcador habilita quatro coisas: `buffer.of(v)` de um argumento só, com a capacidade vindo do `sizeof` do compilador C; a reescrita de `v[i,j,k]`; bounds check em debug para vetor C comum; e `keel.length(v)`/`keel.capacity(v)` como constantes de compilação.

```keel
array i32 v[100];
i32      *p = buffer.ptr(xs);

buffer i32 a = buffer.of(v);         /* ok    */
buffer i32 b = buffer.of(p);         /* error 24 — a capacidade tem que ser dita */
buffer i32 c = buffer.from(p, 100);  /* ok    */
```

`buffer.of` de um argumento sobre símbolo que não é `array` é o error **24** `buffer-of-tamanho`; `keel.ptr`, `buffer.of` e `slice.of` sobre `array` multidimensional são o error **28** `view-sobre-array-nd`.

**A notação com vírgula** baixa para vetor multidimensional C **de verdade**, não bloco plano com linearização: o tipo continua sendo `i32[2][3][4]`, e código C existente que faz `v[i][j][k]` continua funcionando.

A reescrita acontece **apenas sobre símbolo `array` conhecido**. No declarador, `[1,2,5]` não é C válido — a gramática do C aceita `[ assignment-expression ]`, e expressão-vírgula não é uma. Em posição de expressão, porém, `w[1,2,5]` é o operador vírgula do C e significa `w[5]`:

```keel
int   w[10][20][30];      /* C puro: keel não sabe o que é */
array i32 v[2,3,4];

w[1,2,5];                 /* copiado verbatim: continua o operador vírgula */
v[1,2,5];                 /* reescrito para v[1][2][5]                     */
```

- **Todos os índices ou nenhum.** `v[i]` sobre vetor de 3 dimensões é o error **27** `array-indexacao-parcial`.
- `keel.length(v)` e `keel.capacity(v)` são o **total** de elementos.
- `keel.dim(v,k)` é o tamanho da dimensão `k`, base zero, casando com a posição do índice. **`k` é literal inteiro** — error **29** `dim-k-nao-constante`. É o único ponto do documento em que keel lê o *valor* de uma constante, e por isso o único em que um `constexpr` não serve. Não há forma `keel.dim(v)`; em vetor 1D, `keel.dim(v,0)` é igual a `keel.length(v)`.
- **A forma C é sinônimo aceito** — `array i32 v[2][3][4];`. A da vírgula é o estilo da casa; a de colchetes existe para anotar código C já escrito, e é a única em que a primeira dimensão é dedutível: `array i32 v[][3][4] = {...}`.

**Em parâmetro:**

| Parâmetro | Decisão |
| --- | --- |
| `array T v[N]` (1D) | error **25** `array-1d-em-parametro` — use `slice T` |
| `array T v[]` | error **26** `array-sem-dimensao-em-parametro` |
| `array T v[d0,d1,…]` (nD) | **permitido** — sai `void f(i32 v[static 2][3][4]);`, e `v[i,j,k]` continua funcionando |

> Razão: `rationale §4.2`.

#### `constexpr`

É palavra de keel **e** do C23, e **não quer dizer a mesma coisa nas duas**. O que keel registra é mais estreito, e a diferença cabe numa linha:

> **`constexpr` de keel é constante nomeada e tipada, e não é objeto.** O símbolo tem o tipo escrito, vale onde o C exige expressão constante, e o inicializador é conferido contra o tipo. **Não tem endereço:** `&K`, ou qualquer uso que exija lvalue, é o error **124** `constexpr-endereco`.

**A recusa vale nos dois perfis**, e é de propósito — não é o C11 pesando sobre o C23. Ela é a definição, então orienta o uso certo mesmo onde o alvo aceitaria mais: quem precisa do endereço não quer uma constante, quer um objeto, e a forma dele é `static const T k = K;`, uma linha, à vista.

**A obrigação é esta; a forma é do backend.** Como ela se materializa é do perfil de geração (`backend §9`): sob C23 a declaração sai **verbatim** — a ferramenta certa é usada, e o compilador C confere representabilidade; sob C11 sai uma macro de nome gerado, com os usos reescritos, mais a conferência do inicializador. **A linguagem não muda com o perfil**, e é a recusa do endereço que sustenta isso: o C11 não tem construção nenhuma que dê constante de tradução **e** endereço — `static const` dá o segundo e não o primeiro, `enum` dá o primeiro e não o segundo, e não há terceira. Estreitado o contrato ao que os dois entregam, cada perfil emite a sua melhor ferramenta e os dois aceitam o mesmo conjunto. Sobra **um** diagnóstico de diferença, nomeado no §7.3.

```keel
pub  constexpr size_t MAX_LEN = 4096;
priv constexpr int    K       = 1 << 4;

array char linha[MAX_LEN];
```

- **O nome declarado é o `IDENT` imediatamente anterior ao `=`.** Todo o resto da declaração, o tipo inclusive, é `<opaco>`: keel não precisa saber o que `size_t` significa para registrar `MAX_LEN`.
- **O declarador tem de ser simples** (§4.3) — `x` ou `*x`. Nome enterrado, como em `constexpr int (*fp)(void) = f;`, é o error **120** `declarador-enterrado`, com `note` mandando usar `typedef`. A razão é que keel precisa **reconstruir** a declaração para conferi-la, e reconstruir exige saber onde o nome entra no declarador — que é a gramática de declaradores do C, e ela está fora (§1.3).
- **Constante agregada não é `constexpr`.** Declarador com `[`, ou inicializador que abre `{`, é o error **121** `constexpr-agregado`. Agregado não serve onde o C exige expressão constante, nem em C23; a forma dele é `static const`, que keel não conhece e repassa intacta.
- **keel registra que o símbolo é constante; quase nunca lê o valor.** As exceções são **duas**, e ambas pedem um numeral onde não cabe expressão: o `k` de `keel.dim(v,k)`, e o argumento de `dim` (§4.9). Nas duas, o que se lê é **um literal decimal** — na chamada, ou no inicializador desta declaração. keel não dobra constante em lugar nenhum.
- **Sem `=`, nada é registrado** e a declaração atravessa: `constexpr` sem inicializador não é C válido, e quem diagnostica é o compilador C.
- Em escopo de arquivo o nome recebe o prefixo do módulo e `pub`/`priv` decidem o posicionamento — regras gerais, sem emenda. **Em escopo de bloco não há prefixo no fonte**; sob C11 o backend gera um nome próprio e reescreve os usos (`backend §9.2`), o que é grafia e não linguagem.
- O tipo é escrito pelo usuário e verificado pelo compilador C: keel não deduz tipo de literal em lugar nenhum.
- **É o que serve o módulo de rank fixo** (§4.9): o binder `dim` existe para o rank que varia por instância, e onde ele não varia a constante já resolve.

**É o que torna verificáveis os diagnósticos que exigem constância** — 29 (`keel.dim`), 33 (`arena.from_stack`), 92 (`parallel`) e 97 (política de `copar`). `#define` e `enum` continuam invisíveis, o que é o §1.4 em vigor e não uma omissão; os quatro diagnósticos nomeiam a saída:

```plain
app.k:12:26: error: `arena.from_stack` com tamanho não constante [arena-stack-nao-constante]
note: keel não enxerga `#define`; declare `constexpr size_t MAX = 4096;`
```

> Razão: `rationale §4.2`.

#### `ref`, e por que `restrict` não é par dele

`ref` não gera tipo, não entra na identidade do tipo, e some no lowering. **`restrict` parecia o par dele e não é** — é palavra do C que keel não reinterpreta.

**`ref`** fica na mesma posição sintática de `const` e significa **aponta para exatamente um elemento**. `NULL` é valor legítimo — é o que permite receber diretamente o retorno dos verbos que podem falhar. O que ele proíbe é **aritmética**:

```keel
i32 *ref p = arena.alloc(a, i32, 1);
if (!p) return -1;
*p = 42;                         /* ok       */
p->campo;                        /* ok       */
p + 1;   p++;   p += 2;   p[0];  /* error 31 */
slice.from(i32, p, n);           /* error 32 */
```

> **É a única análise de keel que olha para dentro de expressão do C.** No escopo da declaração, varre-se um conjunto **fechado** de formas aplicadas ao símbolo declarado `ref`: aditivo binário (`p + 1`, `q - p`), aditivo com atribuição (`p += 2`), incremento e decremento, e deslocamento por índice (`p[i]`). Qualquer uma é o error **31** `ref-aritmetica`.

A análise é de **forma**, não de tipo. As duas perguntas são *este token é o símbolo que eu declarei `ref`?* e *o operador vizinho está no conjunto?* — e nenhuma consulta tipo. A invariante do §1.3 fica de pé: `q - p` é recusado sem que keel saiba o que `q` é.

O preço é que a varredura **não segue o valor**. Um ponteiro comum que recebe o `ref` é declarador C, atravessa opaco, e dali em diante a aritmética é legítima:

```keel
i32 *ref p = arena.alloc(a, i32, 1);
i32 *q = p;                      /* saída explícita, sem diagnóstico */
q + 1;                           /* ok: `q` não é ref                */
```

Não é buraco: `ref` marca uma declaração, não persegue um endereço. Quem escreve a segunda linha está dizendo por escrito que quer o ponteiro para muitos.

- **Exige inicializador** — `i32 *ref p;` é o error **30** `ref-sem-inicializador`.
- `ptr(p)` é a saída explícita de `ref` para ponteiro comum. A distinção existe na fronteira em que importa: `ptr(x)` devolve `T *`, ponteiro para **muitos**, com aritmética liberada; `ptr(x,i)` devolve `T *ref`, o endereço de **um**.

**`restrict` não é qualificador de contêiner em keel.** Ele é palavra do C, entra em `qual-c` (§3.2) e **atravessa verbatim em declarador C comum** — que é onde ele funciona:

```keel
priv void gemm_kernel(i8 *restrict a, i8 *restrict b, i8 *restrict c,
                      size_t m, size_t n) { … }

pub void gemm(slice i8 a, slice i8 b, buffer i8 *c, size_t m, size_t n) {
    gemm_kernel(slice.ptr(a), slice.ptr(b), buffer.ptr(c), m, n);
}
```

O kernel toma ponteiros e promete o que `restrict` promete; a fronteira de contêiner fica na função de fora, onde `length` e `capacity` ainda existem. **É a forma que o C já expressa bem, e por isso keel não a duplica** (princípio 9).

**Escrever `restrict` antes de um modificador é o error 101** `restrict-em-conteiner`, com a `note` dando o par acima.

> Razão: `rationale §4.2`.

### 4.3 Forma dos tipos compostos

#### Identidade nominal e nome canônico

O nome canônico de um tipo é fixado no **ponto de declaração** e não depende de quem o usa.

| Origem | Nome canônico |
| --- | --- |
| Tipo do C ou da camada zero | a grafia normalizada — `i32`, `char`, `const char` |
| Nome declarado pelo usuário no módulo `M` | `M.nome` |
| Instância do modificador `mod` de `M`, sobre `A` | `M.mod` aplicado ao nome canônico de `A` |

O argumento **carrega a própria qualificação**: `buffer geom.Point` é a instância de `keel.buffer` sobre `geom.Point`, e é a mesma instância em todo módulo que a escreva. É isso que torna a atribuição entre módulos possível.

Corolário deliberado: se `A` e `B` declararem cada um seu `Point`, ainda que idênticos, são tipos **distintos** e não intercambiáveis (princípio 4). Para compartilhar, importa-se a mesma declaração (§4.1).

"Nome" cobre os **três espaços de identificador do C** — `typedef`, tag e constante de enum —, porque os três aparecem na interface e os três colidem entre módulos. Vale só em escopo de arquivo; `enum` em escopo de bloco não é tocado; e dentro de `extern_c` nada disso vale.

**Constante de enum: o escopo é o tipo.**

> O nome canônico de uma constante de enum é o **nome canônico do seu enum**, mais a constante. Enum sem nome não tem escopo próprio, e suas constantes ficam no do módulo.

```keel
module sim;
typedef enum { PARADO, ANDANDO } Estado;
enum { MAX = 64 };
```

| Nome canônico | Símbolo C | Escrita, de fora |
| --- | --- | --- |
| `sim.Estado.PARADO` | `sim_Estado_PARADO` | `sim.Estado.PARADO`, ou `Estado.PARADO` com `types` |
| `sim.MAX` | `sim_MAX` | `sim.MAX` |

A escrita é o nome canônico **encurtado pela esquerda** na medida em que o `import` deixou algo nu. Quatro consequências, e nenhuma pede regra nova:

- **Não se pula o nível do meio.** `sim.PARADO` é o error **85** `enum-sem-o-tipo`. Dentro do módulo o nome nu continua valendo, como para todo o resto.
- **A repetição deixa de ser colisão.** `Estado.PARADO` e `Tarefa.PARADO` convivem no mesmo módulo, e os símbolos gerados não se encontram porque o escopo entrou no nome.
- **A constante continua sendo um inteiro do C.** A reescrita troca identificador e mais nada: no gerado ela serve de `case`, de índice, de operando de `|`. O escopo é notação do fonte e não sobrevive à travessia.
- **O mangling não é caso novo:** cai na mesma regra recursiva que dá `keel_buffer_geom_Point`.

#### Forma das declarações

```plain
<modificador> <argumento> <declarador> [= <inicializador>]
                        { ',' <declarador> [= <inicializador>] } ;
```

Vários declaradores separados por vírgula são aceitos, como em C, e cada um registra o próprio símbolo:

```keel
array  f32 x[10], y[3,4];
buffer i32 b1, *pb;
```

```c
f32 x[10], y[3][4];
keel_buffer_i32 b1, *pb;
```

A tabela guarda de cada símbolo **se é valor ou ponteiro** — o mesmo bit que o despacho exige para emitir `&x` ou `x` (§4.11).

> **`*` e `[N]` pertencem ao declarador, nunca ao argumento.**

```keel
buffer i32 x;            /* buffer de i32                */
buffer i32 *x;           /* ponteiro para buffer de i32  */
buffer i32 x[10];        /* vetor de 10 buffers          */
slice const char s;      /* vista sobre chars const      */
buffer slice i32 grid;   /* aninhado: buffer(slice(i32)) */
```

É a mesma lógica de `const int *p`. **Buffer de ponteiros exige `typedef`:**

```keel
typedef i32 *pint;
buffer pint x;
```

`<modificador> <argumento>` é um **especificador de tipo** e vale onde quer que um especificador valha, inclusive dentro de parênteses de ponteiro para função:

```keel
buffer i32 (*f)(void);
typedef slice u8 (*Leitor)(i32);
```

keel nunca precisa da gramática de declaradores do C para isso — basta reconhecer o par modificador-argumento. O que a forma decide é **quais declarações registram um símbolo de contêiner**:

| | Declarador simples (`x`, `*x`, `x[N]`) | Declarador composto |
| --- | --- | --- |
| Nome do tipo substituído · instância gerada | sim | sim |
| Símbolo entra na tabela | **sim** | **não** |

`f` acima é ponteiro para função, não contêiner: `buffer.length(f())` é o error **15** `fora-da-gramatica-de-conteiner`.

#### Aninhamento

O argumento de um modificador **pode** ser outra instância de modificador. `buffer slice char` é um vetor de vistas — a estrutura *jagged* clássica:

```c
typedef struct keel_buffer_slice_char {
    size_t           cap;
    size_t           len;
    keel_slice_char *ptr;
} keel_buffer_slice_char;
```

A representação interna de tipo é uma **árvore**, e o nome canônico cai dela. Não há ciclo possível: o tipo é escrito por extenso, logo a profundidade é finita e a ordem é topológica por construção.

> **Aninhamento não é multidimensionalidade.** `buffer slice i32` é *jagged*: cada linha tem seu tamanho e sua origem. Um contêiner retangular de rank 2 é um bloco só, e é outro tipo. Não são substitutos.

A profundidade é limitada pelo **teto de identificador do alvo** — error **14** `nome-acima-do-teto` —, não por regra própria.

#### Tipos keel dentro de struct

Campos com tipo keel são suportados, e o mecanismo não viola a invariante: keel aprende apenas os nomes de tipo que **ele mesmo** definiu.

```keel
pub typedef struct {
    buffer Particle ps;      /* colhido: keel sabe o tipo */
    array  f32      grid[16,16];
    FILE           *log;     /* opaco: atravessa          */
} World;

pub void step(World *w, f32 dt) {
    foreach (Particle *p, size_t i : w->ps) p->x += p->vx * dt;
    w->grid[3,4] = 1.0f;
}
```

```c
typedef struct {
    keel_buffer_sim_Particle ps;
    f32                      grid[16][16];
    FILE                    *log;
} sim_World;
```

O mecanismo, em quatro passos:

1. Ao parsear o corpo do `struct`, keel reconhece `buffer Particle ps;` — é o mesmo padrão de declaração de sempre — e registra *`World` tem campo `ps: buffer Particle`*.
2. Só então `World` entra na tabela de tipos, com os campos **de tipo keel**. Os demais não são colhidos: nada em keel precisa da lista completa de um agregado.
3. `World w;`, `World *w` e `struct World w;` passam a casar como declaração e registram o símbolo.
4. `w.ps` e `w->ps` são nós da gramática de contêiner (§3.4).

O conjunto de nomes conhecidos continua sendo exatamente o que keel declarou — `FILE` nunca entra. Struct pública com campo keel exporta esse conhecimento pela interface.

O `f32` do **campo** é obrigatório pela regra de grafia (§4.2); o `f32 dt` do parâmetro é só estilo, porque a restrição vale no argumento de modificador e não em C comum.

#### Capacidade fixa

> **Tamanho é fixo, mas pode ser determinado em tempo de execução no ponto de criação. Uma vez fixado, não muda.**

`slice` não cresce, por definição. `buffer` cresce apenas até `capacity`, que é dinâmica na criação e imutável depois. **Não há realocação e não há crescimento automático.**

Consequência: vista derivada de contêiner **nunca fica pendurada por crescimento**, porque não há crescimento.

O modelo de dados é uniformemente de **pilha**: buffer empilha elementos até a capacidade, arena empilha bytes até a capacidade.

> Razão: `rationale §4.3`.

### 4.4 Memória

#### O modelo

**Arena é pilha, não grafo de objetos.** É a disciplina do frame de chamada — empilha, e o topo volta de uma vez —, com uma região maior e um tempo de vida que o programador escolhe. Nada aqui tem dono, contagem de referência ou destrutor.

> **Não existe desalocar um ponteiro.** Escreve-se `p = arena.alloc(a, T, n)`, usa-se `p`, e no fim do escopo `p` some — sem `free`, porque nada foi individualmente adquirido. No fim do escopo da arena, a arena some.

Três regras, e não há uma quarta:

1. **Arena vive o escopo em que foi declarada.** A variável é objeto automático: some no fim do bloco, como qualquer struct local. Para descer na hierarquia, passa-se por referência.
2. **Arenas se usam hierarquicamente, e a hierarquia é do programador.** keel não rastreia em tempo de execução quem recortou de quem.
3. **Memória vinda do heap precisa de `defer`.** É o único caso em que alguém tem que devolver algo, e quem devolve é o C.

Da regra 1 decorre a propriedade que dispensa toda contabilidade: **filha não sobrevive ao pai** — não por regra da linguagem, por escopo do C.

#### Escape

O que decide se um `return` é seguro é a **procedência da arena**, não o fato de retornar `buffer` ou `slice`. Cada símbolo de contêiner guarda, na tabela, de qual símbolo de arena nasceu — informação presente na própria declaração, sem análise de fluxo.

| Procedência da arena | Retornar contêiner dela |
| --- | --- |
| parâmetro (`arena *a`) | seguro |
| variável de módulo, ou `from_array` sobre `array` de escopo de arquivo | seguro |
| `from_parent` de arena segura | seguro (herda) |
| `from_stack` | **inseguro** |
| `from_array` sobre `array` local | **inseguro** |
| `from_memory` sobre ponteiro local | **inseguro** — o `free` da regra 3 roda na saída |

```keel
pub buffer Person carrega(arena *a, size_t n) {      /* seguro: a arena é do chamador */
    buffer Person b = buffer.from(arena.alloc(a, Person, n), n);
    return b;                                        /* cópia do descritor, não do dado */
}

priv slice i32 tmp(void) {
    arena s;  arena.from_stack(s, 4096);
    buffer i32 b = buffer.from(arena.alloc(s, i32, 16), 16);
    return slice.of(b);                              /* error 35 */
}
```

```plain
sim.k:9:12: error: retorno de `slice i32` cuja arena tem armazenamento local [arena-escape]
sim.k:7:15: note: `s` nasce de `arena.from_stack`, que morre com o frame
sim.k:9:12: note: a arena precisa vir de parâmetro, de escopo de arquivo, ou de um pai seguro
```

É o error **35** `arena-escape`. Regras da análise:

- **Símbolo reatribuído de outra origem** é marcado como desconhecido e não diagnosticado — conservador, sem falso positivo.
- **Limite conhecido:** `*out = slice.of(b);` escapa igual e não é pego. Sair por parâmetro de saída exigiria análise de fluxo, que esta camada não faz.
- Para atravessar a morte do frame existe `clone`, que aloca no destino e copia — e devolve `outcome`, porque alocar pode falhar e o que ele devolve é descritor (§4.11):

```keel
outcome buffer Point saida = buffer.clone(destino, temp) else return -1;
```

#### Fronteira com o modelo individual

Arena é um modelo de memória **de grupo**. Boa parte da biblioteca C vive no modelo oposto, **individual**: cada objeto tem dono, e o dono libera ou realoca.

> **Os dois modelos não se misturam.** Memória devolvida por `arena.alloc` nunca é entregue a função que possa `free` ou `realloc` — ela é um pedaço no meio de uma região maior, e a operação é comportamento indefinido (§7.4).

`arena.from_memory` não é exceção, é o contrário: ali o que se libera é a **região inteira**, com o ponteiro original, e a arena nunca foi dona dela.

A forma correta é deixar o C ser dono, limpar com `defer`, e atravessar por vista:

```keel
pub void processa(FILE *f) {
    char  *linha = NULL;        /* memória é do getline */
    size_t cap   = 0;
    defer free(linha);          /* [later]: vê o valor final de linha */

    for (;;) {
        ssize_t n = getline(&linha, &cap, f);
        if (n < 0) break;
        consome(slice.from(char, linha, (size_t)n));
    }
}
```

**O `defer` sem `[now]` é obrigatório aqui:** `linha` muda a cada `getline`, e o que precisa ser liberado é o valor **final** (§4.7).

#### `arena` — tipo, não modificador

A arena é **não tipada**: uma região linear de bytes com alinhamento de plataforma. Como não há argumento de tipo, ela não é instanciada e não gera header por instância. No fonte, `arena a;` e `arena *a` são declarações C comuns; keel reconhece o padrão para registrar o símbolo — necessário para `defer`, para o despacho e para o escape — e registra junto **se ele é valor ou ponteiro** (§4.11).

```keel
arena.alloc(a, Particle, 100)
```

```c
(Particle *)keel_arena_alloc_n(&a, 100, sizeof(Particle), alignof(Particle))
```

**O `T` fica na chamada**, e é ele que dá `alignof(T)` e o cast. **A contagem e o tamanho do elemento atravessam separados**, e o produto é feito dentro da biblioteca — é a forma do `calloc`, e existe pela mesma razão:

> **`arena.alloc(a, T, n)` devolve `NULL` quando `n * sizeof(T)` não cabe em `size_t`.** A multiplicação não é escrita no ponto de chamada, e o transbordamento não produz uma região pequena que o programa acredita ser grande.

Este é o **único** ponto em que keel emite `n * sizeof(T)`: `clone` multiplica um comprimento que já coube na memória uma vez, e `buffer.from`, `slice.from` e `from_parent` recebem contagens ou bytes sem multiplicar. Uma verificação, num lugar.

**O `T` nunca chega à biblioteca.** Quem carrega o tipo é o verbo, que não é função e sim reescrita — por isso a arena não precisa de instância por tipo, ao contrário de `buffer` e `slice`.

**Arena é `byref`:** por valor em parâmetro é o error **58** `byref-param`. Vale a recíproca, e ela é informação de leitura: **função com parâmetro `arena *` é função que vai alocar.**

#### Construtores

Nomeados pela **origem da memória**, porque é a origem que determina tempo de vida e modo de falha — a única coisa que o leitor do ponto de chamada precisa saber.

```keel
array u8 memo[65536];
arena a;  arena.from_array(a, memo);        /* vetor do usuário: .bss, .data ou pilha */
arena s;  arena.from_parent(s, a, 4096);    /* sub-arena: recorta do pai              */
arena t;  arena.from_stack(t, 4096);        /* vetor no frame atual; N constante      */
arena h;  arena.from_memory(h, mem, cap);   /* região crua: malloc, mmap, linker      */
```

Todos devolvem `bool`. A struct nasce zerada, então arena não inicializada tem `arena.capacity(a) == 0` e todo `arena.alloc` nela falha limpo.

**`from_array` exige símbolo `array u8`** — error **34** `arena-from-array-nao-u8` — e não tem forma com ponteiro e tamanho. `unsigned char` não é grafia aceita (§4.2); `char` continua permitido, para texto.

O elemento tem que ser `u8` por duas razões, e **nenhuma delas é uma permissão do padrão**: a declaração passa a dizer que aquilo é armazenamento bruto e nada mais, e nada no programa tem motivo para ler aquela memória com outro tipo. Sobre um `array f32 x[100]`, o vetor plausivelmente ainda está vivo como float — há um segundo caminho de acesso, com outro tipo, e é aí que o otimizador tem licença.

**O que o padrão diz, e o que keel supõe.** Objeto com tipo declarado tem esse tipo como **tipo efetivo**. A regra do tipo-caractere vale no outro sentido: ela deixa ler a representação de um objeto **por** bytes, e não tratar um vetor de bytes declarado como objeto arbitrário. Só armazenamento **alocado** — sem tipo declarado — recebe tipo efetivo pela escrita. Daí:

> **keel supõe que armazenamento de tipo-caractere, suficientemente alinhado, serve de respaldo para os objetos que a arena entrega.** É a suposição de todo alocador escrito em C, e é a segunda guarda nomeada do §7.1.

Ela vale para `from_array`, `from_stack` e, por herança, `from_parent`. **`from_memory` é a porta estritamente conforme**: sobre `malloc` ou `mmap` o armazenamento não tem tipo declarado, e a suposição não é usada.

**Como a suposição é honrada é do lowering, não da linguagem.** Este documento diz que a arena entrega objetos sobre armazenamento que ela não alocou; qual armazenamento sai, com que alinhamento e sob que remédio de build, é de `keel-c-backend.md §5.4.1`. Um backend cujo alvo ofereça uma rota conforme deve tomá-la, e nada aqui muda com isso — é a mesma separação que já vale para `f32` ser binary32 e `typedef float f32;` ser como *este* backend entrega binary32.

**`from_parent` é, por definição, memória do pai** — equivale a tomar `n` bytes com `arena.alloc` e usá-los como armazenamento. Duas consequências:

**A invalidação.** `reset` ou `restore` do pai invalida a filha e tudo que nasceu dela. Isso é **semântica, não proteção**: pela regra 2, keel não rastreia nada em runtime. O que keel recusa é o caso léxico, e o alvo é preciso — **não é o `reset`, é o uso da filha depois dele**:

```keel
arena s;  arena.from_parent(s, a, 4096);
processa(s);
arena.reset(a);
arena.alloc(s, i32, 10);     /* error 59: `s` usada depois de `arena.reset(a)` */
```

A distinção importa, porque resetar o pai depois de a filha ter servido é exatamente o idioma que devolve o recorte. A verificação é **varredura léxica em ordem de fonte**, sem fluxo de dados; fora do escopo — pai recebido por referência e resetado num callee — não há diagnóstico.

**Recortar do pai não volta.** Carvar é `arena.alloc` no pai, e `arena.alloc` não desfaz quando a filha sai de escopo:

```keel
/* errado: -4096 do pai a cada volta, até esgotar */
for (size_t i = 0; i < n; i++) {
    arena s;  arena.from_parent(s, a, 4096);
    processa(s, i);
}
/* certo: a filha nasce uma vez, e o reset reaproveita */
arena s;  arena.from_parent(s, a, 4096);
for (size_t i = 0; i < n; i++) { processa(s, i); arena.reset(s); }
```

O que é automático é o **descritor**, não o armazenamento.

**`from_stack` exige tamanho constante de compilação** — error **33** `arena-stack-nao-constante`. Um `constexpr` satisfaz; um `#define` não, e a mensagem diz isso.

```c
keel_arena t = {0};
unsigned char keel__st0[4096];
keel_arena_from_array(&t, keel__st0, sizeof keel__st0);
```

**Não há VLA nem `alloca` em lugar nenhum:** `keel__st0` é vetor automático de tamanho fixo, e compila onde `__STDC_NO_VLA__` está definido.

**`from_memory` toma ponteiro e tamanho crus** — `malloc`, `mmap`, ou região de linker script. O tamanho é asserção de quem chama, e é aqui que vive a regra 3: o `defer free(mem)` vem na linha seguinte, antes de qualquer uso. A arena é só uma vista sobre a região — não adquiriu nada e não devolve nada.

**Escolher entre `from_stack` e `from_parent`** não é escolher origem de memória; é decidir o que acontece com o resultado:

| Scratch | O resultado sobrevive ao escopo? | Serve para |
| --- | --- | --- |
| `from_stack` | não — o vetor morre com o frame | scratch **consumido**: computa, decide, descarta |
| `from_parent` | **sim** — a memória é do pai, e o dado já está lá | scratch **produtor**: o resultado fica, sem cópia |

#### Alinhamento, falha e verbos de topo

O alinhamento tem dois níveis, e os dois se pedem em C. A **base** é resolvida dentro do construtor; como `array u8` tem alinhamento 1, isso custa até `alignof(max_align_t) - 1` bytes, e quem quiser mais escreve `alignas(64) array u8 memo[N];`, que keel copia verbatim. A **alocação** herda o alinhamento do tipo, porque `arena.alloc` emite `alignof(T)` — então sobre-alinhamento se pede no **tipo**, não na chamada. Não há quarta posição em `arena.alloc`.

`arena.alloc` falha por três razões, num canal só:

| Razão | Quando | Diagnóstico em debug |
| --- | --- | --- |
| capacidade | o que resta na arena não chega | — |
| alinhamento | `alignof(T)` excede o alinhamento da base | 82 `alloc-alinhamento` |
| transbordamento | `n * sizeof(T)` não cabe em `size_t` | 109 `alloc-overflow` |

Todas devolvem `NULL`, e o `[[nodiscard]]` obriga a olhar.

`arena.mark(a)` devolve o topo corrente, `arena.restore(a,m)` volta a ele, `arena.reset(a)` zera; `arena.length(a)` é o ocupado e `arena.capacity(a)` o total, mantendo a simetria com `buffer`.

**`reset` é reúso, não liberação.** Nenhuma memória volta ao sistema, porque nenhuma foi adquirida do sistema. Arena com tempo de vida da aplicação — sobre `array u8` de escopo de arquivo — **nunca precisa de reset**. O que precisa ser devolvido é o que veio do heap, e para isso o verbo é `free`, num `defer`.

**Arena não é thread-safe.** `arena.alloc` lê e escreve `top` sem sincronização. O idioma suportado é **uma arena por thread**, recortada do pai enquanto o pai está quieto — é o contrato do `parallel` (§4.7): faixa disjunta é de graça, ponteiro compartilhado é do programa.

#### O que põe a arena fora da biblioteca comum

| Verificação | Diagnóstico |
| --- | --- |
| procedência da arena no `return` | 35 `arena-escape` |
| `from_array` exige `array u8` | 34 `arena-from-array-nao-u8` |
| `from_stack` exige constante | 33 `arena-stack-nao-constante` |
| uso de filha depois de `reset` do pai | 59 `arena-filha-apos-reset` |

Nenhuma é expressável pelo protocolo de despacho (§4.9): duas exigem saber qual **construtor** produziu o símbolo, uma exige varredura em ordem de fonte, e uma exige a fase que varre o arquivo inteiro antes de resolver. É essa lista, e não a grafia, que define a categoria.

> Razão: `rationale §4.4`.

### 4.5 Contêineres

Os três módulos da base que dão o linear: um dono, uma vista e um intervalo. São **modificadores comuns** pela regra do §4.9 — `keel.buffer` declara `modifier buffer byref`, `keel.slice` declara `modifier slice`, e `keel.range` declara um tipo sem parâmetro. Nada aqui é privilegiado; o que a base garante é que todo módulo do build concorda sobre eles.

| | rank 1 | rank ≥ 2 |
| --- | --- | --- |
| **dimensões em compilação, armazenamento embutido** | `array f32 x[10]` | `array f32 v[3,10]` |
| **dimensões em execução, com dono** | `buffer f32 b` | stdlib — `tensor(2) f32 t` |
| **vista não-proprietária** | `slice f32 s` | stdlib — `view(2) f32 v` |

A linha do `array` é da linguagem em qualquer rank, porque o C já tem vetor multidimensional: keel não acrescenta representação, só passa a conhecer a que existe (princípio 1). As outras duas precisam de descritor, e descritor com dimensões de execução em rank N é um modificador (§4.10).

#### `buffer T`

Sequência de comprimento variável, limitada por capacidade fixa, que **possui** seu armazenamento. A criação é sempre sobre memória que já existe.

```keel
buffer i32 xs = buffer.from(arena.alloc(a, i32, 16), 16);   /* sobre arena  */
array  i32 v[100];
buffer i32 ys = buffer.of(v);                               /* sobre vetor C: len == cap */
```

```c
keel_buffer_i32 xs = keel_buffer_i32_as(
    (i32 *)keel_arena_alloc_n(&a, 16, sizeof(i32), alignof(i32)), 16);
keel_buffer_i32 ys = keel_buffer_i32_of(v, sizeof v / sizeof(i32));
```

**O OOM se resolve sem tipo de erro:** se `arena.alloc` devolveu `NULL`, o buffer nasce com `cap == 0`, `push` falha limpo, `buffer.capacity(xs) == 0` é o teste, e nada fica indefinido.

**Buffer sobre vetor C `const` não existe** — `clear` seguido de `push` escreveria em memória const. É o error **21** `buffer-sobre-const`; vetor const entra como `slice const char`.

**A capacidade é fixa depois de criada** (§4.3).

**Semântica de valor.** `buffer` é struct, e struct copia. As duas regras que fecham a armadilha são o `byref` do §4.9 em vigor:

```keel
priv void enche(buffer i32  xs) { buffer.push(xs, 1); }    /* error 58 */
priv void enche(buffer i32 *xs) { buffer.push(xs, 1); }    /* ok       */
```

```plain
sim.k:4:25: error: `buffer i32` por valor em parâmetro [byref-param]
sim.k:4:25: note: `slice i32` para ler, `buffer i32 *` para escrever
```

Atribuição entre buffers é o warning **46** `byref-atribuido`, nomeando o aliasing — é local e visível, e às vezes é o que se quer. **Retorno por valor continua permitido** — não há aliasing, a cópia do callee morre —, sujeito à análise de procedência do §4.4.

#### `slice T`

Vista não-proprietária sobre memória contígua. **Não é `byref`:** vista por valor é o uso normal, e é essa a diferença de desenho entre os dois módulos.

```keel
slice i32 all  = slice.of(xs);
slice i32 part = slice.of(xs, 2, 7);   /* semiaberto: [2,7)      */
slice i32 mesm = slice.of(xs, r);      /* r é range              */
slice char l   = slice.from(char, linha, (size_t)n);   /* ponteiro cru */
```

A terceira forma é **aridade 2**, e não custa regra nova: o despacho conta argumentos, então `r` atravessa opaco e quem confere o tipo é o compilador C. Construível a partir de `buffer T` do mesmo `T`, de `array T` e de outro slice.

Em `slice.from`, o tipo é escrito na chamada e não inferido, porque **keel não conhece o tipo de `linha`** — a mesma razão pela qual `arena.alloc(a, T, n)` carrega o `T`. Sobre um `ref` é o error **32** `slice-from-sobre-ref`.

Duas consequências deliberadas:

- **Não existe `set_length`.** `length` de um buffer significa *quantos elementos foram empilhados*; quando o C escreve por baixo, o buffer genuinamente não sabe.
- Com `slice.from`, a passagem por fora fica **visível no ponto de chamada**:

```keel
buffer u8 buf = buffer.from(arena.alloc(a, u8, 4096), 4096);
ssize_t n = read(fd, buffer.ptr(buf), buffer.capacity(buf));
slice u8 dados = slice.from(u8, buffer.ptr(buf), (size_t)n);
```

O custo é não poder `push` depois de o C ter preenchido um prefixo.

#### `range`

Intervalo semiaberto de `size_t` — dois campos e quatro verbos. É o que a forma `a..b` produz quando é guardada em vez de consumida na hora.

```keel
range r = 2..7;
slice i32 s = slice.of(xs, r);
foreach (auto i : r) { ... }
```

| Verbo | Devolve | Consumido por |
| --- | --- | --- |
| `range.first(r)` | o início | forma de intervalo do `foreach` |
| `range.limit(r)` | o fim, **exclusivo** | idem |
| `range.length(r)` | `limit - first` | forma percorrível |
| `range.get(r,i)` | `first + i` | idem |

- **É tipo, não modificador** — como `arena`, e pela mesma razão: não tem parâmetro. Escreve-se `range r;`, nunca `range T r;`.
- **Não há verbo de construção.** `range r = a..b;` é a forma da linguagem, e `keel_range_make` é o símbolo que ela emite, não um nome que se escreva.
- **Não há intervalo aberto fora do índice** — `range r = 2..;` é o error **83** `recorte-aberto`: o lado vazio é preenchido pelo `length` de um contêiner, e aqui não há contêiner.
- **Não declara `ptr`**, porque um intervalo não tem armazenamento. Daí `r[i]` não existir e o binder por ponteiro não se aplicar, sem caso especial em nenhum dos dois.
- **Não é privilegiado**, e é aqui que isso se prova: qualquer tipo do usuário que declare `first` e `limit` ganha a forma de um binder do `foreach`, sem keel jamais saber que ele existe.

#### O que vale sobre quê

| Verbo | `buffer T` | `slice T` | `array T` |
| --- | --- | --- | --- |
| `length(x)` | ✔ | ✔ | ✔ constante |
| `capacity(x)` | ✔ | — | ✔ constante |
| `keel.dim(x,k)` | — | — | ✔ |
| `get(x,i)` · `set(x,i,v)` | ✔ | ✔ | ✔ |
| `at(x,i)` | ✔ | ✔ | ✔ |
| `x[i]` · `x[a..b]` | ✔ | ✔ | ✔ |
| `ptr(x)` · `ptr(x,i)` | ✔ | ✔ | ✔ |
| `push(x)` · `push(x,v)` · `pop(x)` · `clear(x)` | ✔ | — | — |
| `slice.of(x)` · `slice.of(x,r)` · `slice.of(x,i,j)` | ✔ | ✔ | ✔ |
| `buffer.of(x)` | — | — | ✔ |
| `buffer.clone(a,x)` · `slice.clone(a,x)` | ✔ | ✔ | ✔ |

- `push(x)` sem valor reserva o slot e devolve o ponteiro, permitindo inicializar no lugar sem cópia extra. `push(x,v)` escreve o valor.
- `pop(x)` devolve o slot removido — memória ainda válida, acima de `len` — ou `NULL` se vazio. Faz par exato com `push`.
- `clear(x)` zera `len`; a capacidade permanece. É o que permite reutilizar um buffer vindo de vetor C, que nasce cheio.
- `set(x,i,v)` exige `i < length(x)`: escrever em slot nunca empilhado é bug, não extensão — diagnóstico **23** `set-fora-de-length`, em debug. É o que torna `length` uma invariante e não um comentário.
- `slice.of(x,a,b)` exige `a <= b` **e** `b <= length(x)` — sobre os dois limites, não sobre a diferença, porque `size_t` é sem sinal e limite invertido dá *wrap*. Vazio é legal.
- `clone(a,x)` aloca em `a` o mesmo comprimento e copia, e é o que faz um resultado atravessar a morte do escopo que o produziu. **Devolve `outcome` do tipo que o qualificador nomeia**, com `code != OK` quando a alocação falha. É o único verbo da base que produz um tipo falível, e por isso o único que casa com a cláusula `else`.
- Verificação de limites em `get`, `set` e `ptr(x,i)` existe em debug e não em release.

**`get` e `set` são error quando o elemento é instância de modificador**, porque `get` devolve **cópia** e para elemento que é contêiner isso é perda silenciosa de dado:

```plain
sim.k:8:18: error: `get` sobre elemento que é instância de modificador [get-copia-conteiner]
sim.k:8:18: note: use `buffer.ptr(grid, 3)` ou `grid[3]`, que aliasam o elemento real
```

É o error **22** `get-copia-conteiner` — a mesma falha do parâmetro por valor, uma indireção adiante, e igualmente invisível ao compilador C.

#### `at(x,i)` — a porta do índice de execução

Há dois tipos de índice num programa, e a base os separa em vez de tratá-los igual.

**O índice derivado** é o que o próprio programa produziu: contador de `foreach`, worker de `parallel`, resultado de `string.find`, `i` de um `for` limitado por `length(x)`. Ele é verificável **onde é escrito**, e para ele existem `get`, `set`, `ptr` e o açúcar `x[i]`, todos sem custo em release.

**O índice de fora** é o que veio de dado: campo de arquivo, argumento de linha de comando, byte de protocolo. Nada no programa o limita, e nenhuma análise estática o alcança — a verificação tem de acontecer, e em release.

```keel
outcome i32 v = buffer.at(xs, idx_do_arquivo) else return -1;
```

```c
keel_outcome_i32 v = keel_buffer_i32_at(&xs, idx_do_arquivo);
if (keel_outcome_i32_failed(v)) return -1;
```

- **`at` devolve `outcome T`**, pela regra do modo de falha (§4.11): o que ele devolve é valor, não ponteiro, e não há elemento fora da faixa que sirva de sentinela.
- **Fora de faixa é `NONE`, não um código de erro.** Nada deu errado — a posição simplesmente não tem valor. Um índice mau não é uma falha do contêiner.
- **A verificação está em toda build.** É a única da base que não é `debug`, e é o que faz de `at` uma garantia em vez de conveniência de desenvolvimento.
- **Ele é do protocolo, não um caso especial:** quem declara `at(<mod>, size_t)` ganha o verbo.

Não há açúcar para `at`, e isso é deliberado: `x[i]` é a forma barata e tem de continuar parecendo barata. Quem paga a verificação escreve o nome dela.

> **A doutrina, numa linha:** todo índice ou é derivado — e então a verificação é estática, ou de `debug` — ou vem de fora, e então passa por `at`. **A base tem uma porta só para índice não verificável, e ela está escrita no fonte.**

#### A linearização de um módulo é interna

Vale para `buffer` e `slice`, e para todo modificador que possua uma linearização.

> Um modificador que possui uma linearização **não expõe o próprio armazenamento**: ele expõe vistas por verbo, e reserva `ptr(m)` para entregar tudo a uma função C.

É a mesma opacidade que o warning 48 cobra dos campos gerados, aplicada ao layout que o módulo escolheu. E uma assimetria que não aparece na sintaxe: **em row-major denso a linha é contígua e a coluna não é.** Uma linha pode devolver `slice T`; uma coluna precisa de vista com passo.

> Razão: `rationale §4.5`.

### 4.6 O açúcar ao operador [ ]

Três formas de sintaxe do núcleo. **Não levam qualificação**, porque não são chamadas: são sintaxe sobre a tabela de símbolos, e o módulo do `ptr` é o do tipo de `x`, que o parser já tem.

```keel
x[i]                /* elemento: *ptr(x,i) — lvalue, não cópia  */
v[i,j,k]            /* rank cheio                               */
x[a..b]  x[a..]  x[..b]  x[..]     /* recorte: vista, rvalue    */
a..b                /* intervalo, fim exclusivo                 */
```

O açúcar cai sobre **qualquer** tipo que declare o verbo correspondente, inclusive modificador do usuário: quem declara `ptr(G *, size_t)` ganha `x[i]` de graça.

#### `x[i]` — elemento

Definido como `*ptr(x,i)` — **lvalue com aliasing correto**, não cópia. Por isso ele é a forma *segura* de alcançar elemento-contêiner, e `get` é a insegura.

```keel
buffer.push(grid[3], 42);
grid[3][7] = 5;
```

```c
keel_buffer_i32_push1(keel_buffer_buffer_i32_ptr(&grid, 3), 42);
*keel_buffer_i32_ptr(keel_buffer_buffer_i32_ptr(&grid, 3), 7) = 5;
```

**A forma com vírgula generaliza:** `x[i, j, …]` ≡ `*ptr(x, i, j, …)`, despachando para a função `ptr` da aridade correspondente. Ela tem uma segunda entrada quando o modificador declara `dim N` (§4.9). Para `array`, a mesma escrita é reescrita direta para `v[i][j]`, sem função no meio: ali keel conhece as dimensões e o C tem o tipo. Sobre modificador, quem conhece o layout é o `ptr` que o módulo declarou — **é ali que row-major ou column-major é decidido**, e a linguagem não opina.

O lowering é **por função, nunca por macro**, o que garante que cada argumento seja avaliado exatamente uma vez: `x[i++]` incrementa `i` uma vez só.

Forma dentro da gramática mas sem `ptr` correspondente é o error **68** `aridade-sem-ptr`, com a mensagem listando as aridades que existem. A gramática do §3.4 aceita qualquer número de vírgulas; quem restringe é o conjunto de funções que o modificador declarou.

#### `x[a..b]` — recorte

> `x[a..b]` ≡ `slice.of(x, a, b)` — **rvalue**, semiaberto `[a,b)`.

O asterisco de `*ptr(x,i)` não aparece, e não poderia: `slice.of` devolve descritor por valor. Daí a diferença de categoria — **`x[i]` é lvalue e `x[a..b]` é rvalue**, mesma sintaxe. `xs[2..7] = s;` é erro do compilador C sobre um rvalue, no ponto certo.

Os dois lados do `..` são `<opaco>`, então `xs[i..i+n]` e `xs[(a+1)..(b*2)]` atravessam sem que nada seja tipado.

| Escrita | Equivale a |
| --- | --- |
| `x[a..]` | `slice.of(x, a, length(x))` |
| `x[..b]` | `slice.of(x, 0, b)` |
| `x[..]` | `slice.of(x)` |

O lado ausente é decidível por varredura — o `..` está à vista e o operando não —, e o que entra no lugar são dois verbos que já existiam.

**`x[a..]` é o único açúcar em que o contêiner aparece duas vezes**, e isso cobra uma restrição: `x` tem que ser **caminho sem índice** — `IDENT`, `.`, `->`, `*`, `&` e parênteses, sem `[ ]` e sem verbo no meio. `xs[2..]` e `w->ps[2..]` passam; `grid[3][2..]` é o error **84** `recorte-aberto-com-indice`, e a mensagem dá as duas saídas: nomear a linha, ou escrever o limite.

| Regra | Diagnóstico |
| --- | --- |
| tipo sem o `of` da aridade que a forma exige | error **77** `recorte-sem-of` |
| `a <= b` e `b <= length(x)` | **79** `recorte-fora-de-faixa`, em debug |
| os dois limites literais e invertidos — `xs[7..2]` | error **78** `recorte-invertido`, em compilação |

Note o que **não** está aqui: `x[i, a..b]`. A forma com vírgula despacha **contando** argumentos, nunca examinando a forma de um deles. Recorte em mais de uma dimensão é verbo de biblioteca (§4.10).

> Razão: `rationale §4.11`.

### 4.7 Fluxo

#### `defer`

Registra um statement para executar na **saída do escopo léxico**, em ordem inversa de registro. Sem pilha de runtime, sem closure, sem alocação: o cleanup é emitido estaticamente em cada ponto de saída.

```keel
defer fn(x);                     /* statement              */
defer { ... }                    /* bloco                  */
defer [later] free(linha);       /* explícito; é o default */
defer [now int fd] { close(fd); } /* captura no registro    */
```

**Pontos de saída:** fim natural do bloco, `return`, `break` e `continue` que deixam o escopo, e `goto` para rótulo fora do escopo. `return` anterior ao registro não recebe cleanup.

Em `return expr;`, **`expr` é avaliada antes do cleanup**, e o valor retornado é o dela — daí o temporário. O temporário é declarado com o tipo de retorno **escrito na função**, copiado como sequência de token; função cujo declarador de retorno enterra o nome — `int (*f(void))[10]` — é o error **120**, pelo mesmo motivo do `constexpr` e da captura. A forma se decide pelo tipo de retorno da **função**, nunca pela expressão:

| Retorno da função | `return expr;` baixa para |
| --- | --- |
| não-`void` | `{ ⟨tipo de retorno⟩ keel__rvN = expr; ⟨cleanup⟩ return keel__rvN; }` |
| `void` | `{ expr; ⟨cleanup⟩ return; }` — `expr` continua sendo avaliada |

`defer` em corpo de laço executa ao fim de **cada iteração**, por consequência direta da regra léxica.

**Captura.** Sem lista, o corpo referencia as variáveis diretamente, com o valor que tiverem **na saída**. Com `[now]`, cada entrada é copiada **no registro**, e o corpo referencia as cópias, que sombreiam os originais.

**A entrada se escreve como um parâmetro, e o tipo é obrigatório:**

```keel
defer [now int fd] { close(fd); }
defer [now int fd, FILE *saida] { relata(saida, fd); }
```

keel não deduz tipo que não conhece, e é a mesma razão pela qual `slice.from(char, p, n)` leva o tipo escrito e a cláusula `else` só existe em posição de declaração. **O nome capturado é o `IDENT` imediatamente anterior à vírgula ou ao `]`**; todo o resto da entrada é `<opaco>` e sai copiado. Declarador com o nome enterrado é o error **120** `declarador-enterrado` — o contorno é `typedef`.

**A lista mora dentro do colchete, não depois dele**, e é o que dá ao `defer` uma forma só: `defer <opções> <corpo>`, com o corpo sempre logo após o `]`. Fora do colchete ela seria ambígua com o próprio statement adiado — `defer [now] (void)fn();` teria duas leituras, e `(void)` é lista de parâmetros legítima. Dentro do colchete, `later` seguido de qualquer coisa que não seja `]` é o error **39** `later-com-captura`.

| Situação | Diagnóstico |
| --- | --- |
| corpo de statement de controle sem chaves — `if (p) defer free(p);` | error **36** `defer-sem-bloco` |
| com chaves, em corpo de `if`, `else` ou `switch` | warning **47** `defer-em-bloco-de-controle`, nomeando onde roda. Corpo de laço não avisa |
| salto que entra em escopo por cima de um registro de `defer` — `goto`, `case` ou `default` | error **38** `salto-sobre-defer` |
| `defer` em escopo de arquivo | error **37** `defer-em-escopo-de-arquivo` |

**Escopo com `defer` pendente tem uma entrada só.**

> Um salto entra **por cima** de um registro quando o rótulo está num escopo `S`, textualmente depois de um `defer` de `S`, **e a origem do salto não está dentro de `S`**. É o error **38** `salto-sobre-defer`, e o C tem exatamente duas formas de produzi-lo:
>
> - **`goto`** para rótulo nessa posição, escrito fora do escopo do rótulo;
> - **`case` ou `default`** textualmente posterior a um `defer` do mesmo corpo de `switch` — aqui a origem é o próprio `switch`, que está sempre fora do corpo.

A enumeração é exaustiva porque a lista do C é fechada — não há terceira forma —, então a propriedade é total e não é heurística. É a mesma regra que o próprio C aplica a tipo variavelmente modificado, e pela mesma razão.

**A origem é metade da regra, e é a metade que libera o idioma útil.** Salto que já passou pelo registro não é salto por cima: `goto` **de dentro** de `S` para rótulo **de `S`** é legal, porque toda execução que o alcança já registrou. É essa metade que dá a saída do error **123** adiante.

```keel
switch (op) {
case 0:
    defer cleanup();
case 1:                      /* error 38: entra por cima do registro */
    break;
}

switch (op) {
case 0: { defer cleanup(); … }   /* ok: o escopo do `defer` são as chaves */
case 1: break;
}
```

```keel
void f(void) {
    defer cleanup();
    { … goto fim; }          /* ok: origem dentro do escopo do rótulo */
fim:
    return;                  /* aqui o cleanup sai */
}
```

A `note` dá a saída de cada forma: no `goto`, reestruturar o salto; no `case`, **pôr chaves no corpo do rótulo**, que é uma linha e é o idioma correto — as chaves fazem do corpo do `case` um escopo com uma entrada só, e o `defer` passa a rodar na saída dele.

#### `defer` implícito e sombreamento

A forma sem `[now]` **não copia nada**: o corpo referencia os símbolos pelo nome, e é resolvido onde o cleanup sai. Se um escopo mais interno redeclara um desses nomes e tem ponto de saída, o cleanup ligaria ao símbolo errado, sem diagnóstico:

```keel
int x = 1;
defer usa(x);
{
    int x = 2;
    return;              /* error 123: `usa(x)` ligaria ao `x` interno */
}
```

> **`defer` sem `[now]` cujo corpo nomeia um símbolo redeclarado em escopo mais interno que contenha ponto de saída do escopo do `defer` é o error 123** `defer-later-sombreado`.

**As duas saídas estão na mensagem, e as duas são idioma:**

```keel
defer [now int x] usa(x);        /* 1. copiar no registro: a cópia não sombreia */

int x = 1;                       /* 2. tirar a saída de dentro do sombreamento */
defer usa(x);
{ int x = 2; goto fim; }
fim:
return;
```

A segunda é a que o `goto` legal acima existe para permitir, e é a leitura certa do problema: **se o sombreamento é necessário, o ponto de saída não pertence ao escopo que sombreia.** Levá-lo para o escopo do `defer` põe a ligação onde ela sempre esteve certa, e o cleanup volta a ser automático.

**`[now]` não precisa da regra**, e é o que separa as duas formas: a cópia é feita no registro, o corpo referencia a cópia, e nenhum símbolo do usuário é reencontrado na saída.

**A recusa é uniforme, e é de propósito.** Dá para classificar saltos caso a caso — para trás, para a frente, entre irmãos —, mas cada classificação vira regra que o usuário teria de aprender para entender por que o salto dele passou e o do colega não. Salto sobre escopo com `defer` é raro o bastante para que a recusa uniforme custe pouco.

**Dois caminhos escapam do cleanup**, ambos fora do alcance de keel: `longjmp` a partir de escopo com `defer` pendente, e `return` vindo de expansão de macro (§1.4).

> Razão: `rationale §4.7`.

#### `foreach` e `apply`

A construção que faz keel **saber que um laço percorre um contêiner** — e o único lugar onde ele sabe disso. Duas formas, separadas pela **vírgula antes do `:`**, por varredura, sem consultar tipo.

```keel
foreach (i32 v, size_t i : xs)          { ... }   /* cópia: usa get   */
foreach (Particle *p, size_t i : ps)    { ... }   /* alias: usa ptr   */
foreach (auto i : 0..n)                 { ... }   /* intervalo        */
```

```c
{ keel_buffer_i32 *keel__c0 = &xs;
  size_t keel__n0 = keel_buffer_i32_length(keel__c0);
  for (size_t i = 0; i < keel__n0; i++) { i32 v = keel_buffer_i32_get(keel__c0, i);
    ...
} }
```

O cabeçalho ocupa **uma linha de saída**, então o corpo mapeia 1:1 e nenhuma diretiva `#line` é necessária.

**`foreach` não conhece `buffer`: exige verbos**, resolvidos pelo despacho geral (§4.11).

> **Percorrível** é o tipo que declara `length` e, conforme o binder, `get` ou `ptr`. **Contável** é o que declara `first` e `limit`.

Dois binders pedem percorrível; um binder pede contável. Verbo faltando é o error **69** `nao-percorrivel` ou o **73** `nao-contavel`, nomeando qual falta e sobre qual tipo. Os dois conjuntos não recebem nome e não são declaráveis: a verificação é no ponto de uso.

| Regra | Diagnóstico |
| --- | --- |
| binder por valor sobre elemento que é instância de modificador | error **70** `foreach-copia-conteiner` — use `T *` |
| binder de índice que não é `size_t` | error **72** `indice-nao-size-t` |
| `push`/`pop`/`clear` sobre o contêiner percorrido, no corpo | error **71** `mutacao-na-travessia` |

**Os dois binders são obrigatórios** na forma de contêiner, mesmo quando o corpo não usa o índice: `i` é a variável do laço, e o binder impede o `for` gerado de inventar um nome. O tipo escrito também desfaz a leitura errada — `i32 v, i` seria lista de declaradores do C; `i32 v, size_t i` não é lista de declaradores em C nenhum.

**Contêiner e comprimento são avaliados uma vez.** É semântica, não otimização: percorre-se a sequência que existia na entrada, e um `push` no corpo não estende a travessia em silêncio.

**`apply`** é `foreach` de corpo fixo: `apply(i32, xs, dobra)` equivale a `foreach (i32 v, size_t i : xs) { dobra(v, i); }`. `fn` recebe **elemento e índice**, nessa ordem. O primeiro argumento é o binder sem o nome — por isso admite `*` e tipo composto —, e ele não é redundante com o tipo do elemento: escolhe entre **valor e ponteiro**, que é a única informação que keel não tem por outro caminho.

**O contêiner é sempre linear.** Não há forma que percorra um agregado multidimensional de uma vez: percorre-se um `foreach` por eixo, com acesso de rank cheio no corpo (§4.10). Recorte em mais de uma dimensão é verbo, não sintaxe.

**A forma de intervalo** tem um binder, e o fim é **exclusivo**: `1..1001` conta mil vezes. O limite é avaliado uma vez.

```c
{ for (size_t i = 1; i < 1001; i++) { ... } }
```

- **O literal aberto não vale aqui** — `..7` e `2..` são o error **83** `recorte-aberto`: o lado que falta vem do `length` de um contêiner, e no binder o contêiner é o próprio intervalo.
- **Não há binder por ponteiro:** `range` não declara `ptr`, então a forma cai no error 69, e um `*` no binder é o error **81** `binder-ponteiro-em-intervalo`.
- **O tipo do binder é copiado, não deduzido.** keel emite a palavra verbatim e quem deduz `auto` é o compilador C. A única substituição é `foreach (auto i : 1..1001)`, que sai `size_t`, porque `auto i = 1` deduziria `int`.
- **Contável não é `range`:** qualquer tipo que declare `first` e `limit` ganha a forma. E `range` declara os quatro verbos, então serve também à forma de dois binders — `foreach (size_t v, size_t i : r)` dá valor e posição. Sobre o **literal** isso é o error **80** `foreach-dois-binders-em-literal`.

> Razão: `rationale §4.7`.

#### `parallel`

Percorre um contêiner linear em **k faixas disjuntas, uma por worker**. É o produto de duas travessias: a externa, sobre os workers, **sem ordem definida entre eles**; a interna, sobre a faixa, em série pelo índice.

**A construção autoriza paralelismo, não o promete.** O que ela afirma é que as faixas são independentes — quem as executa, e se as executa ao mesmo tempo, é da plataforma. É a mesma categoria do `restrict` do C: afirmação do programador que o backend *pode* explorar. Um programa cuja correção dependa de duas faixas correrem **simultaneamente** está fora do contrato, e keel não lhe dá o vocabulário para tentar: faixas são disjuntas, captura escalar é cópia, e não há `lock`.

```plain
parallel <nome> <política> ( <workers> ; <travessia> [ ; <captura> ] ) { corpo }
```

```keel
parallel busca ANY (size_t w : 0..4; i32 x, size_t i : xs; (alvo, achados)) {
    interrupted;
    if (x == alvo) { achados[w] = i; win; }
}
if (ok(busca)) { ... }
```

| Seção | Forma | De onde vem |
| --- | --- | --- |
| `size_t w : 0..4` | um binder sobre intervalo | a forma de intervalo do `foreach`, sem diferença |
| `i32 x, size_t i : xs` | dois binders sobre contêiner | idem |
| `(alvo, achados)` | lista de identificadores | própria — **sem tipo**, porque quem declara a cópia é o `firstprivate` do OpenMP, não keel |

As três seções já existiam. O que `parallel` acrescenta é a decisão de que a primeira roda em paralelo. **Quem materializa é o OpenMP**; o conjunto de cláusulas é do backend.

**`k` e a política são escritos e constantes**, nunca deduzidos — error **92** `parallel-nao-constante`. Um `constexpr` satisfaz; um `#define` não. Por isso a produção de `politica` (§3.3) aceita `nome-qualificado` ao lado do numeral: o que ela **não** aceita é expressão. **`parallel` dentro de `parallel` é o error 90** `parallel-aninhado`: dividir de novo dentro da faixa é outro `k`.

**O nome é obrigatório e único no módulo.** Ele dá nome estável, sob edição, aos símbolos gerados — bandeira, contador, status por worker — e é o símbolo que se interroga depois do bloco. Sem nome é o error **87** `parallel-sem-nome`; colidir com qualquer símbolo que o módulo declare é o **91** `parallel-nome-repetido`. Não há mangling por função nem por escopo: os dois moveriam os símbolos gerados quando a função fosse renomeada ou o bloco envolvido por chaves.

| Predicado | Verdadeiro quando |
| --- | --- |
| `ok(nome)` | toda faixa terminou sem `fail` e sem interrupção |
| `failed(nome)` | algum worker executou `fail` |
| `interrupted(nome)` | a bandeira foi ligada, e ao menos um worker parou por causa dela |

Os três ficam **nus** (§4.11): o argumento é símbolo do núcleo, não contêiner, e não há módulo a nomear.

**A promessa é de execução permitida, e não de resultado único.**

> **A execução em série, faixa por faixa em ordem crescente de `w`, é uma das execuções que o bloco permite.** Ela não é a única, e duas execuções permitidas da mesma entrada **podem** discordar.

Isso é o que a construção afirma, e o teto do que ela pode afirmar. Sob `ALL` não há bandeira, ninguém interrompe ninguém, toda faixa roda inteira, e as três respostas são funções da entrada: **sob `ALL` o resultado é único**. Fora de `ALL` a bandeira existe, e é ela que traz a diferença:

| | Depende do escalonamento? |
| --- | --- |
| o que o corpo escreveu, sob `ALL` | não |
| o que o corpo escreveu, fora de `ALL` | **sim** — faixa interrompida para de escrever |
| `ok`, `failed` | sob `ALL` não; fora de `ALL` **sim** |
| `interrupted` | **sim**, sempre — a série, no limite, quase sempre lê verdadeiro |

**A razão é uma só, e é a bandeira.** Quem para por causa dela não executa o resto do corpo — nem as escritas, nem um `fail` que viria depois. Duas faixas que discordariam sobre `failed` numa ordem concordam noutra, sem que haja corrida sobre dado nenhum: o que muda é **quanto de cada faixa chegou a rodar**. É a semântica de busca, e é o que a política pede.

O que **não** varia, e é o que torna a construção utilizável: as faixas são as da fórmula normativa em qualquer implementação, a partição não é do OpenMP, e a interrupção nunca produz escrita a mais — só escrita a menos. Um programa correto sob esta construção é um que **não depende de quantas faixas terminaram**: escreve por `w` em contêiner disjunto, e reduz em série depois do bloco.

**A política é quantas vitórias o bloco espera antes de interromper o resto:** `ALL` vale `0` e é map — ninguém interrompe ninguém; `ANY` vale `1` e é busca; um número é busca limitada. Elas não chegam ao C gerado — o que sai é o número —, então um `#define ALL` vindo de header não colide com nada. **Não há `join` a escrever, e não há como não juntar:** o bloco espera os k workers sempre, e a política muda o que ele reporta, nunca quem ele espera.

**As faixas.** `length(c)` é lido uma vez, antes do fork. Com `n` elementos e `k` workers o passo é `(n + k - 1) / k`, e a faixa de `w` é `[w·passo, min((w+1)·passo, n))`. **A partição é escrita por keel, não escolhida pelo OpenMP** — é por isso que a fórmula é normativa e não depende de `schedule`. Faixa vazia é legal. A ordem entre faixas não é definida; dentro de uma faixa é a do índice, crescente.

**A captura.** Escalar entra por **cópia, uma por worker**; atribuir a um escalar capturado dentro do corpo é o error **114** `captura-escrita`. Instância `byref` entra por **ponteiro**. Só local precisa entrar — símbolo de escopo de arquivo o corpo enxerga naturalmente. **A lista é escrita, nunca deduzida:** descobrir quais identificadores o corpo toca é entender o corpo. Esquecer um nome vira erro do compilador C, nomeando a variável, na linha do `.k`.

**Os três verbos do corpo:**

| Escrita | Encerra a faixa com | Conta para a política |
| --- | --- | --- |
| `win;` | `SUCCESS` | **sim** — incrementa o contador, e liga a bandeira ao alcançar o alvo |
| `fail;` | `FAILURE` | não |
| `interrupted;` | `INTERRUPTED`, **se** a bandeira estiver ligada; senão não faz nada | não |

Fim natural da faixa é `SUCCESS`, sem verbo. **Nenhum deles é `break`:** os três baixam para um salto ao fim do corpo do worker, dentro do bloco estruturado que o OpenMP exige, e saem de qualquer profundidade de aninhamento. `break` e `continue` do usuário continuam significando o que sempre significaram. **`return` no corpo é o error 122** `return-em-parallel`: o corpo é bloco estruturado de OpenMP, e sair dele por salto é proibido — sem o diagnóstico, quem recusa é o compilador C, com mensagem sobre um construto que o programador não escreveu. Para encerrar a faixa existem os três verbos; para sair da função, interroga-se o bloco depois dele. Os três são **pontos de saída de escopo**, então `defer` registrado no corpo executa neles — e em `interrupted`, **só quando ele de fato interrompe**: sem a bandeira ligada ele não é saída, e não há o que limpar. Fora de corpo de `parallel` são o error **89** `verbo-fora-de-parallel`.

**`interrupted` é statement, não predicado** — é o ponto de interrupção declarado, sem `if` para esquecer. *Onde* aceitar interrupção é do programa, então keel não o injeta: com política diferente de `ALL`, corpo sem `interrupted` é o error **88** `interrupted-ausente`; sob `ALL` não há bandeira, e escrevê-lo é o mesmo error pelo motivo oposto. **`fail` nunca conta como vitória e nunca liga a bandeira.**

O detalhe por worker — quem falhou, o que cada um achou — não sai da construção: escreve-se num contêiner indexado por `w`, e reduz-se em série depois do bloco. É por isso que `w` **tem** que ser binder.

> Faixas são **disjuntas** e capturas escalares são **cópias**. Sobre isso não há corrida possível, e não foi preciso analisar nada para saber.

O que atravessa por ponteiro é compartilhamento de verdade, e é do programa (princípio 6). **Não há `monitor`, `lock` nem `executive`** — `<threads.h>` e `<stdatomic.h>` atravessam por `import_c`.

**Custo em plataforma.** É a única construção cujo gerado depende de algo que não é C. **Sem OpenMP o programa compila e roda em série, e a série é uma execução permitida:** diretiva desconhecida é ignorada, e o que sobra são as duas travessias, aninhadas, sobre o contêiner inteiro — a partição não é do OpenMP, e sobrevive. Não é degradação de semântica, é o contrato sendo cumprido: a construção nunca prometeu paralelismo, e sequência é uma das ordens que ela já permitia. Sob `ALL` o resultado é literalmente o mesmo; fora de `ALL` é um dos permitidos, pela razão da bandeira dita acima. Por isso o diagnóstico **108** `openmp-indisponivel` é `warning`, e não `error`; quem exige paralelismo o promove pela flag de build. **O runtime é o do OpenMP, não de keel.**

> Razão: `rationale §4.7`.

#### Cláusula `else`

Cauda de declaração: testa o valor recém-inicializado e, se for falha, **sai ou repara**.

```keel
outcome Cfg c       = cfg.le(path) else return -1;              /* saída  */
outcome string name = login()      else string.from("(anon)");  /* default */
```

```c
keel_outcome_cfg_Cfg c = cfg_le(path);
if (keel_outcome_cfg_Cfg_failed(c)) return -1;

keel_outcome_keel_string name = app_login();
if (keel_outcome_keel_string_failed(name))
    name = keel_outcome_keel_string_win(keel_string_from("(anon)"));
```

**A forma é declaração, e é dela que sai todo o resto.** O teste precisa saber se o valor é falha, e isso vem do tipo; keel não tipa expressão, logo o tipo tem de estar **escrito** — e o único lugar onde um tipo é escrito é uma declaração. Daí a cláusula ser cauda de `decl-keel` e não operador em posição de expressão.

**`failed` é o protocolo, e é a única porta.** Um tipo é falível quando declara o verbo `failed`; qualquer outra coisa é o error **116** `else-tipo-nao-falivel`. A forma de default pede um verbo a mais, `win`, o construtor de resultado bom do mesmo módulo — tipo falível sem `win` só aceita a forma de saída, error **119** `else-default-sem-win`. É o mesmo mecanismo de *percorrível* e *contável*: keel não consulta lista de tipos privilegiados, consulta a tabela e despacha. `outcome` é a implementação de referência, e um modificador de terceiro entra do mesmo jeito.

**Escalar nunca é falível**, porque zero é sucesso em `open` e falha em `fopen`, e escolher uma convenção seria adivinhar. **Ponteiro também não**, e a razão é a mesma vista do outro lado:

| | Zero significa | Não-zero significa |
| --- | --- | --- |
| `outcome T` — o campo `code` | **sucesso** | falha |
| `T *` — o ponteiro | **falha** | sucesso |

Uma cláusula que cobrisse os dois carregaria as duas convenções sob a mesma palavra, e o leitor teria de saber o tipo para saber o que `else` testou. Para ponteiro o `if` manual continua sendo a forma — é uma linha, é a mesma que se escreveria em C, e ela diz qual é o teste.

**As duas formas se separam pelo primeiro token depois do `else`:** `{` ou palavra-chave de salto do C (`return`, `break`, `continue`, `goto`) é **saída**, e o statement é copiado verbatim; qualquer outra coisa é **default**, e a expressão vai para `M_win(…)`. É lookahead fixo de um token sobre um TK_CKW que o lexer já emite, sem consultar tipo. Olhar o primeiro token **não é classificar o operando** — por isso a forma de saída não verifica o corpo.

**Das quatro coisas que uma propagação de erro exige, a cláusula faz o teste:**

| | Quem faz |
| --- | --- |
| teste — este valor é falha? | **a cláusula**, pelo verbo `failed` |
| extração — o valor bom | o programa |
| conversão — erro do callee para erro do caller | o programa, no statement de saída |
| saída com cleanup | `defer`, que já é dono de todos os pontos de saída |

A conversão exigiria o tipo de uma **chamada**, e por isso fica de fora (§5). A expressão e o statement são `<opaco>` nas duas formas.

| Forma | O que garante depois da linha |
| --- | --- |
| saída | que **o teste aconteceu** e que o statement rodou. Nada sobre o símbolo |
| default | que **o símbolo tem valor válido** — a única garantia forte da linguagem sobre um resultado, e ela existe porque ali é keel quem escreve a atribuição |

**Nenhum temporário é criado:** o símbolo declarado é o que a cláusula lê e o que ela repara.

| Onde não vale | Diagnóstico |
| --- | --- |
| declaração sem inicializador | error **115** `else-sem-inicializador` |
| mais de um declarador | error **118** `else-multiplos-declaradores` |
| atribuição — `x = f() else …` | error **117** `else-em-atribuicao` |

A última recusa não é técnica: a garantia da construção é sobre um **ponto do programa** — daqui para a frente —, e atribuição não cria ponto nenhum.

> Razão: `rationale §4.7`.

### 4.8 Cooperativo

O modelo cabe em duas frases, e a segunda é a que decide tudo o mais:

> **`cofsm` é um switch sobre uma variável de estado do usuário.** A **função que o
> contém** é a máquina; se ela devolve `corot`, ela é uma corrotina, e uma máquina
> de nível acima a chama como chamaria qualquer outra função que fosse corrotina.

Não há frame preservado, não há pilha secundária e não há suspensão: os verbos de
saída baixam para um `return` de verdade, e a chamada seguinte entra pelo topo da
função. O que sobrevive entre chamadas é o dado que o usuário declarou.

**Corolário, e é dele que sai o resto:** o laço é do programa, não da construção.
Quem quer uma passagem por estado não escreve laço; quem quer correr até bloquear
escreve `while (1)`. keel não tem opinião sobre isso.

#### O que sai de quê

A confusão que essa seção existe para evitar é entre três níveis. São dois:

| Sai de | Como |
| --- | --- |
| **o bloco de estado** — isto é, o `switch` | fim do bloco, ou `cobreak` |
| **a função** | `cowin(v)`, `cofail(n)`, `coagain(v)` |

Não há terceiro. **A `cofsm` não tem valor de retorno**, porque bloco não retorna:
valor pertence a função. Chegar ao código depois do `cofsm` significa, e só pode
significar, que nenhum verbo terminal disparou.

Os três verbos valem **só em função cujo retorno seja `corot`**. Numa função
`void` não há o que devolver, e a condição de parada é uma variável do programa —
que é o que ela sempre foi.

#### `cofsm`

Duas formas com a mesma palavra, separadas pelo token depois do nome: `[` ou `;`
**declara**, `(` **opera** (§3.5).

**A declaração diz onde o estado vive**, e o critério é escape — o mesmo do §4.4.
Se nada escapa da função, o tipo e a variável nascem juntos, ali:

```keel
void f(void) {
    cofsm mm [ISSO, AQUILO, AQUILOUTRO] state;   /* tipo e variável, locais */
    bool fim = false;
    while (!fim) {
        cofsm mm (state) { ... }
        if (...) fim = true;
    }
}
```

Se o estado precisa sobreviver às chamadas — isto é, se a máquina é ela mesma uma
corrotina —, só o tipo é declarado, no topo, e `pub`/`priv` decide se o `enum` sai
no `.h`:

```keel
pub cofsm mm [ISSO, AQUILO];
```

```keel
pub typedef struct { mm s; i32 hp; } Agente;    /* mil agentes, um bloco */
```

**A lista de estados é obrigatória e mora na declaração.** Daí vêm três coisas de
graça: os valores ficam estáveis sob edição, o `.h` fecha sem ver o corpo, e a
checagem é exaustiva nos dois sentidos — rótulo fora da lista e entrada da lista
sem rótulo são erro.

**Não há verbo de transição.** A variável tem nome e está à vista, então mudar de
estado é atribuição comum, e o nome nu da constante vale dentro do módulo (§4.3):

```keel
pub corot i16 myfunc(Agente *ag) {
    while (1) {
        cofsm mm (ag->s) {
            ISSO:   corot r = etapa(ag);
                    if (corot.failed(r))  cofail(1);      /* sai de myfunc  */
                    if (corot.ongoing(r)) coagain(0);     /* sai de myfunc  */
                    ag->s = AQUILO; cobreak;              /* sai do switch  */
            AQUILO: cowin(ag->alvo);                      /* sai de myfunc  */
        }
    }
}
```

Nesse desenho a transição é de graça — `cobreak` cai no `while`, que redespacha na
mesma chamada — e o bloqueio custa uma chamada, que é o `coagain` devolvendo o
controle a quem dirige a máquina.

#### `coseq` e `copar`

São **máquinas completas**: funções inteiras fornecidas pela linguagem. Entram,
rodam, saem — **nunca devolvem ONGOING**, porque "parou no meio da cadeia" e
"parou no meio da rodada" não querem dizer nada. Logo nada delas escapa além do
resultado, e nada precisa ser declarado fora.

```keel
coseq passo { ola(a, ag); autentica(a, ag); pronto(a, ag); }
copar coleta ANY { parser(a, ag); tempo(a, ag); leitor(a, ag); }

if (outcome.failed(coleta)) return -1;
```

**O nome nomeia o que escapa** — e essa é a regra que unifica as quatro
construções da linguagem que levam nome. No `cofsm` escapa o estado, e o nome é o
tipo dele. No `coseq`, no `copar` e no `parallel` (§4.7) escapa só o status, e o
nome é o status: o símbolo que se interroga depois do bloco.

O resultado é `outcome` (§4.10), não `corot` — dois estados, porque o terceiro não
pode ocorrer —, e com ele vem a cláusula `else` de graça. O `code` diz **qual**
participante quebrou, pelo ordinal na lista escrita.

`coseq` avança na etapa seguinte a cada SUCCESS, repete a mesma em ONGOING, e
encerra no primeiro FAILURE. `copar` chama todos os que ainda não terminaram, a
cada rodada, e a política — `ALL`, `ANY` ou um número — diz quantos sucessos
encerram.

**Quem sobrou simplesmente deixa de ser chamado.** Não há passagem de
encerramento, não há bandeira e **não há nada análogo ao `interrupted` do
`parallel`** (§4.7): uma participante não tem como perguntar se a máquina já
encerrou, e não precisa. As duas construções são cooperativas — a participante
**retorna** entre passagens —, então não existe execução a interromper no meio;
o que existe é uma chamada que não acontece mais.

O que a passagem cedida deixou pendurado já foi liberado: `coagain` é `return` de
verdade, e o `defer` daquele frame rodou nele (§4.7). E o que atravessa passagens
mora no estado que **o chamador passou** — no exemplo acima, `a` e `ag` são dele —,
então limpar depois do bloco é sempre possível, e é onde se limpa:

```keel
copar coleta ANY { parser(a, ag); tempo(a, ag); leitor(a, ag); }
libera_pendencias(ag);          /* o bloco acima detém o estado de todas */
```

> **Por que aqui não, se no `parallel` sim.** No `parallel` as faixas correm de
> fato ao mesmo tempo, e o corpo é escrito dentro do bloco — keel enxerga onde pôr
> o ponto de interrupção. Aqui a participante é função opaca, compilada à parte, e
> nada do corpo dela é de keel. Ver `rationale §4.8`.

> **Elas existem porque o C não as expressa.** Um vetor de ponteiro para função
> não serve: os argumentos são ligados no ponto de escrita, mas as chamadas
> acontecem depois e repetidamente, dentro do corpo da máquina. Guardar isso é
> closure, e closure está na tabela do §5. Sem closure, a única saída é a
> linguagem emitir as chamadas textualmente.

#### `corot`

O tipo de quem **cede o controle** — `outcome` é o de quem **termina**. Os dois
têm o mesmo layout, `{ i32 code; T v; }`, e **não** a mesma leitura do zero:

| `code` | `corot` | `outcome` |
| --- | --- | --- |
| `< 0` | SUCCESS | erro |
| `== 0` | **ONGOING** | **OK** |
| `> 0` | FAILURE, e qual | erro |

O zero do `corot` é o estado neutro de propósito: uma struct zerada diz "ainda não
terminou", que é a única leitura segura para algo que ainda não rodou. No
`outcome` o zero é sucesso, porque lá não há estado neutro a proteger. E o zero
sendo ONGOING dá a regra que falta: **`cofail(0)` é erro**, porque diria "ainda
estou rodando" com a palavra que anuncia falha — diagnosticado na compilação
sempre que o argumento é literal.

**Não há constantes de status.** FAILURE é uma região, não um valor, e expor
`corot.FAILURE` seria mentira. O acesso é sempre por verbo:

```keel
corot.ok(r)   corot.ongoing(r)   corot.failed(r)   corot.value(r)   corot.code(r)
```

Os predicados são **passado ou estado, nunca imperativo** — `failed`, e não
`fail`, porque `cofail` já é a ordem e as duas não podem ter a mesma palavra.

**A regra do vocabulário** cabe numa linha: **palavra `co*` sai da função; verbo
`corot.*` lê um valor.**

#### Diagnósticos

| # | Nome | Quando |
| --- | --- | --- |
| 60 | `cofsm-sem-nome` | máquina sem nome |
| 61 | `estado-de-outro-tipo` | variável de estado cujo tipo não é o da máquina |
| 62 | `estado-repetido` | rótulo de estado repetido no mesmo `cofsm` |
| 63 | `cobreak-fora-de-cofsm` | `cobreak` fora de bloco de estado |
| 64 | `cofail-zero` | `cofail(0)` com argumento literal — zero é `ONGOING` |
| 65 | `cofsm-nome-repetido` | duas máquinas com o mesmo nome no módulo |
| 66 | `estado-fora-de-faixa` | estado fora de faixa na variável — `debug`, alcançado pelo `default:` do gerado |
| 67 | `corot-sem-simbolo` | verbo de `corot` sobre expressão que não é símbolo |
| 93 | `co-verbo-fora-de-corot` | `cowin`, `cofail` ou `coagain` em função cujo retorno não é `corot` |
| 94 | `cowin-sem-valor` | `cowin` sem valor, sobre `corot T` |
| 95 | `estagio-invalido` | statement em bloco de `coseq` ou `copar` que não é chamada |
| 97 | `copar-politica-nao-constante` | política que não é constante de compilação |
| 98 | `estado-fora-da-lista` | rótulo que não está na lista declarada |
| 99 | `estado-sem-rotulo` | entrada da lista sem rótulo correspondente no corpo |

Os dois últimos são a **exaustividade**, nos dois sentidos, e é por isso que a lista de estados mora na declaração e é obrigatória.

#### A composição fecha pela função

Uma etapa de `coseq` é uma chamada, e o corpo dela pode ser um `cofsm`, um `copar`,
outro `coseq` ou uma função comum — sem que nenhum dos dois lados precise saber
qual. Porque estado é dado, um orquestrador inteiro é um struct com um campo por
nível.

### 4.9 Módulos genéricos

Um módulo pode ser parametrizado por tipo e por dimensão. O que ele declara não é um tipo: é um **modificador**, da mesma família de `buffer` e `slice` — que por esta regra deixam de ser caso especial, porque *são* os modificadores dos módulos `keel.buffer` e `keel.slice`.

```keel
module coll type T;

pub modifier stack byref { size_t cap, len;       T *ptr; }
pub modifier queue byref { size_t cap, head, len; T *ptr; }

pub inline size_t length(stack *s)    { return s->len; }
pub inline T     *push(stack *s, T v) { ... }
```

```keel
module sim;
import coll types;

stack i32        pilha;
queue geom.Point fila;
stack slice i32  aninhado;
```

São três formas novas e nada mais — o binder na linha `module`, `modifier` e `instance` —, e a substituição é **textual** sobre nome de tipo e sobre numeral. **Tudo do §4.3 vale sem emenda:** `*` e `[N]` pertencem ao declarador, o argumento normaliza pela regra de grafia, o aninhamento funciona, e o teto de identificador continua sendo o único limite de profundidade.

#### O parâmetro é do módulo

O binder vive na declaração `module`, e é o único lugar onde `dim` e `type` são palavras.

```keel
module coll type T;             /* um parâmetro de tipo         */
module map  type K, V;          /* dois                         */
module tens dim N type T;       /* um de dimensão, um de tipo   */
```

**A ordem é rígida — `dim` antes de `type`.** É ela que dá ao parser a assinatura completa do modificador, a partir da linha `module`, antes de parsear qualquer uso. O uso a espelha: `tensor(3) f32` põe o numeral entre parênteses e o tipo justaposto, na ordem em que foram declarados.

**`dim` tem uma forma só — `dim N`, identificador.** A forma literal `dim 2` saiu: o binder existe para dar ao corpo um nome que ele substitui, e um numeral não liga símbolo nenhum. Quem quer o rank fixo escreve o módulo sem `dim` e o numeral na mão (`rationale §4.9`).

Dentro do módulo, `T` é nome de tipo como qualquer outro — campo, parâmetro, retorno, `sizeof`, `alignof`. Declarar símbolo local de mesmo nome é o error **51** `nome-de-parametro`. **Todos os modificadores de um módulo compartilham a lista de parâmetros:** um `map` sobre `K,V` é outro módulo.

#### `modifier`

Declara um agregado parametrizado **e** registra o nome como modificador — é o que separa as duas coisas que o `struct` sozinho confundiria:

```keel
pub  modifier stack { size_t cap, len; T *ptr; }   /* uso: `stack i32 s;`  */
priv struct   No    { T v; struct No *prox; }      /* struct comum         */
```

- `modifier` fora de módulo genérico é o error **50** `modifier-fora-de-generico`: não há o que parametrizar, e agregado sem parâmetro é `struct`.
- Modificador de nome `instance` é o error **111** (§3.5).
- Um módulo pode declarar quantos modificadores quiser; o nome do módulo não precisa coincidir com o de nenhum.

> **Declaração que menciona um parâmetro ou um modificador do módulo pertence à instância** — tipo, função e variável são instanciados por argumento. **A que não menciona nenhum dos dois é emitida uma vez, no módulo**, e não pertence a instância nenhuma.

A verificação é **léxica**: o nome aparece, ou não aparece, nos tokens da declaração. Não há análise de dependência e não há "usa indiretamente". Ela paga em dois lugares:

- `outcome.OK` e `outcome.NONE` não mencionam `T` nem `outcome`, então existem como **nome nu** — uma cópia por instância não teria de qual sair;
- `size_t length(stack *s)` não escreve `T`, mas escreve `stack`, e continua **na instância** — que é o que a tabela de identidade logo abaixo já dizia por outro caminho.

**`byref` é o único bit que não vem da assinatura**, e diz que o descritor não deve ser duplicado. Dele saem duas regras: passagem por valor em parâmetro é o error **58** `byref-param`, e atribuição entre duas instâncias é o warning **46** `byref-atribuido`. A segunda é fácil de esquecer — `a = b;` é atribuição de struct, C válido, e copia o ponteiro junto. É a propriedade que separa `buffer` de `slice`, e é deliberadamente **um bit, e não um vocabulário**.

#### Identidade da instância

Com `M` o módulo genérico e `A` o argumento canônico:

| Declaração em `M` | Nome canônico |
| --- | --- |
| modificador `G` | `M.G` sobre `A` |
| função ou variável cujo primeiro parâmetro ou tipo de retorno é `G` | **pertence** a `M.G` sobre `A` |
| qualquer outra | `M.<nome>` sobre `A` |

A segunda linha é o que faz uma função **pertencer** a um modificador, e é ela que o despacho consulta.

**Um modificador pode declarar o mesmo nome em aridades diferentes** — é o que permite `ptr(m)`, a base, conviver com `ptr(m,i,j)`, o elemento. Os símbolos se separam pelo sufixo de aridade. Isto **não é sobrecarga**: a aridade está escrita no ponto de chamada, contar argumentos é sintático, e nenhum tipo de argumento é examinado.

A regra reproduz os nomes da base **sem uma letra de diferença**, e a regra de encurtamento (§1.7) devolve `keel_buffer_i32` e `keel_slice_char`.

#### Despacho

> Numa chamada `m.f(x, …)` em que `x` é expressão de contêiner (§3.4) cujo tipo é instância do modificador `G` de `M`, `f` é procurado entre as funções que pertencem a `G`. Achando, a chamada é reescrita para a função da instância, com o endereço do contêiner no lugar do primeiro argumento.

Nada aí é sistema de tipos: não há inferência, unificação nem subtipagem. É a mesma tabela fixa de sempre, lida da assinatura em vez de embutida. O restante — qualificador, par valor/ponteiro, o açúcar que cai junto — está no §4.11.

#### Instanciação

**Quem instancia é quem usa:** escrever `stack i32` em qualquer posição de especificador gera a instância.

O módulo genérico é carregado como **template** — parseado uma vez, validado com `T` opaco, e sem artefato próprio além do par que todo módulo exige. Validar com `T` opaco é possível porque tudo que keel verifica trata `T` como nome de tipo desconhecido, exatamente como já trata todo tipo vindo do C. **A instância é gerada às cegas**, e quem reclama do resultado é o compilador C.

**A instanciação é recursiva:** se o corpo de `stack` declara `buffer T`, então `stack i32` obriga a gerar também `buffer i32`, e a coleta roda de novo sobre o resultado.

**O grafo de genéricos é acíclico.** Módulo cujo corpo se instancie, direta ou transitivamente, regride ao infinito — error **53** `generico-circular`, com a cadeia na mensagem. Isso proíbe recursão na **definição**, não no **uso**: `stack stack i32` continua legal, e é finito porque está escrito por extenso.

#### `instance`

Por omissão tudo num módulo genérico é `pub inline` e a instância é header-only — é o que preserva a invariante de build, nenhum alvo de compilação sem `.k` correspondente. Função `pub` **sem** `inline` sai `extern` na instância, e o corpo precisa de dono:

```keel
module instances;
import coll;

instance coll.stack i32;
instance coll.stack geom.Point;
```

- **Coloca, não gera.** A geração já acontece pelo uso; `instance` decide onde moram os corpos.
- **Não é uso e não declara nome** — o `import` continua obrigatório.
- **Só em escopo de arquivo** — error **55** `instance-fora-de-arquivo` —, e o argumento tem que ser modificador de módulo genérico — error **56** `instance-nao-modificador`.
- Sobre genérico inteiramente `pub inline` é o warning **57** `instance-inutil`.

#### `dim` — parâmetro de dimensão

Parametriza por um **número**, substituído como numeral do mesmo jeito que `type` é substituído como nome de tipo.

```keel
module tens dim N type T;
pub modifier tensor byref { size_t dims[N]; size_t passos[N]; T *ptr; }
```

```c
typedef struct tens_tensor_3_f32 {      /* de tensor(3) f32 */
    size_t dims[3]; size_t passos[3]; f32 *ptr;
} tens_tensor_3_f32;
```

> `N` substitui como **numeral decimal** em limite de vetor, limite de laço, `sizeof`, `alignof` e expressão constante. Em lugar nenhum mais.
>
> **`dim` nunca gera declaração** — nem parâmetro, nem campo, nem função, nem laço. Usá-lo onde geraria uma é o error **107** `dim-gera-declaracao`.

A segunda linha é o fechamento: é o que separa `dim` de metaprogramação de tempo de compilação.

**Um módulo pode ter mais de um `dim`**, e a lista é simétrica com a de `type`:

```keel
module std.rank dim N, M type T;      /* dois ranks, independentes */
```

**Cada argumento é o valor, escrito de duas formas:**

> **literal decimal**, ou **identificador registrado como `constexpr` cujo inicializador é um literal decimal**. Qualquer outra coisa é o error **106** `dim-nao-constante`.
>
> **O valor tem de ser ≥ 1** — error **125** `dim-abaixo-de-um`, com a cadeia de instanciação na mensagem.

O piso é da linguagem e não da biblioteca, porque abaixo dele o gerado não é C: um modificador com `dim N` quase sempre declara `size_t dims[N]`, e vetor de tamanho zero não existe. É também o **caso-base** que uma família de ranks precisaria escrever à mão — `rank(1,0)` para de instanciar aqui, com a cadeia, em vez de gerar struct inválida.

**Não há aritmética no argumento.** `view(N-1)` não é escrevível, e é deliberado: `dim` substitui um valor, não avalia expressão. Relação entre ranks se escreve como **parâmetro a mais**, e a consistência é do autor da biblioteca, em C:

```keel
module std.rank dim N, M type T;
static_assert(M == N - 1, "std.rank: destino é origem menos um");
```

Depois da substituição isso é `static_assert(1 == 2 - 1, …)` — expressão constante comum, conferida pelo compilador C com a mensagem que o autor escolheu. É o princípio 3: keel não avalia nada, e a instância errada é recusada por quem sabe recusá-la.

```keel
tensor(3) f16 t1;

constexpr i8 DIM = 3;
tensor(DIM) f16 t2;            /* mesma instância que t1 */
```

**As duas formas produzem o mesmo tipo, e isso é normativo:**

> **`M(K) T` com `K` valendo `k` é o mesmo tipo que `M(k) T`** — mesmo nome canônico, mesma struct, mesmas funções. Nominal e estruturalmente idênticos, sem conversão entre eles porque não há dois.

```c
typedef struct tens_tensor_3_f16 { … } tens_tensor_3_f16;   /* uma vez, para os dois */
```

**Quem substitui é o valor, nunca o símbolo**, e as duas razões são de espécie diferente. A primeira é de identidade: com o símbolo no nome, `tensor(3) f32` e `tensor(DIM) f32` com `DIM == 3` seriam instâncias distintas de layout idêntico, que não se convertem — o princípio 4 fragmentado por grafia. A segunda é de build, e é a que fecha:

```keel
/* a.k */  constexpr i8 DIM = 3;   tensor(DIM) f16 t;
/* b.k */  constexpr i8 DIM = 4;   tensor(DIM) f16 u;
```

Com o símbolo, os dois pedem `tens_tensor_DIM_f16` com **layouts diferentes**, e o header de instância — que é escrito por quem usa — deixa de ser função das entradas. É o determinismo de `backend §7.1` quebrado, e a falha aparece como duas invocações se sobrescrevendo em laço.

**Macro e constante de enum continuam fora**, e agora por uma razão só: keel não as enxerga (§1.4), e não há declaração dele de onde ler o valor. **Inicializador que não seja literal também fica fora** — `constexpr i8 DIM = 2 + 1;` é o mesmo error 106. A fronteira é essa: keel **lê um token** de uma declaração que ele mesmo registrou; não dobra constante, porque dobrar constante é avaliar expressão.

Dentro de um módulo genérico `tensor(N) T` é legal, porque depois da substituição `N` já é o valor.

**O acessor de rank cheio recebe um vetor**, e um acessor serve todo rank:

```keel
pub inline T *ptr(tensor *t, size_t idx[static N]) {
    size_t off = 0;
    for (size_t d = 0; d < N; d++) off += idx[d] * t->passos[d];
    return t->ptr + off;
}
```

O `for` tem limite constante, então o compilador C o desenrola e o vetor de índices some por SROA: sai `t->ptr + idx0*p0 + idx1*p1 + idx2*p2`. **Em `-O0` o vetor é materializado e o laço roda** — é o preço, e ele existe.

**Um acessor, e só ele.** A indexação sobre modificador com `dim` é **total ou nenhuma**:

> Num modificador com `dim N`, para `x[i₁, …, i_k]`:
>
> | `k` | |
> | --- | --- |
> | `k == N` | `*ptr(x, (size_t[N]){i₁, …, i_k})` — o acessor de rank cheio |
> | `k ≠ N` | **error 104** `indices-fora-do-rank` |

**Não existe indexação parcial**, e a razão é que ela não tem dono possível. Escrevê-la caberia ao autor da biblioteca, que é quem sabe o que o layout significa — mas ele não conhece `N`, que só existe no ponto de uso. Se ele escrever a família até uma aridade arbitrária, digamos cinco, um uso com `N = 3` deixa as de aridade 4 e 5 **declaradas e chamáveis**, indexando além do rank. E se keel a gerasse, o resultado seria código que ninguém escreveu, num lugar onde um bug pareceria do programador — que é o princípio 2 na direção em que ele mais custa.

**Nada se perde na operação principal**, porque o acessor por vetor já serve todo rank. O que se perde é o descritor de rank reduzido, e ele volta como **verbo de biblioteca** sobre um rank concreto, escrito por extenso — nunca como família gerada.

**O sufixo conta índices, não argumentos do C.** O acessor cheio de um `tensor(2)` recebe **um** argumento depois do contêiner — o vetor de índices — e mesmo assim é `_ptr2`, porque o ponto de chamada escreveu dois índices. É o que faz o nome dizer o rank, e o que alinha o caso `dim` com o rank fixo, em que `mat_matrix_f32_ptr2` sai de dois índices escritos por extenso. Modificador com `dim` mas sem `ptr` de rank cheio faz `x[i,…]` ser o error **105** `dim-sem-ptr-cheio`.

**O nome canônico carrega o valor, e não a grafia:**

| Declaração | Uso | Nome canônico |
| --- | --- | --- |
| `module tens dim N type T;` | `tensor(3) f32` | `tens_tensor_3_f32` |
| idem, com `constexpr i8 DIM = 3;` | `tensor(DIM) f32` | `tens_tensor_3_f32` — **o mesmo** |
| `module mat type T;` (rank na mão) | `matrix f32` | `mat_matrix_f32` |

`tensor(1) f64` é **estruturalmente** um `slice f64` e **não é** um `slice f64`, pelo princípio 4. A conversão é verbo de biblioteca, verificada, nunca implícita.

#### Regra de fechamento

Não é lista de trabalho futuro: é o **limite do mecanismo**, escrito junto com ele.

| Fica de fora | Motivo |
| --- | --- |
| Constraints, concepts, interfaces | O único predicado sobre `T` é `byref`, e ele é sobre o modificador, não sobre o parâmetro. Genérico que precise comparar ou copiar `T` recebe ponteiro de função em runtime |
| Especialização, total ou parcial | Exigiria casar padrão sobre argumento de tipo — é o que transforma substituição em avaliação |
| Valor padrão de parâmetro | Faria a aridade deixar de ser conhecida antes do uso, que é a propriedade que dispensa os chevrons |
| Função genérica livre | Exigiria deduzir `T` do argumento no ponto de chamada. O escape é definir a função dentro do módulo genérico |
| Sobrecarga escrita pelo usuário | Muda o mangling de toda função do módulo |
| Aridade variável de parâmetros | Não há caso, e abriria recursão sobre lista de tipos |
| Genérico que se instancia | Regressão infinita; error 53 |
| Família de funções cuja **quantidade** depende de `dim` | Não tem dono: a biblioteca não conhece `N`, e keel gerá-la produziria código que ninguém escreveu (§4.9, indexação parcial) |
| Aritmética no argumento de `dim` — `view(N-1)` | `dim` substitui um valor, não avalia expressão. Rank reduzido é verbo sobre rank concreto |

A tabela é um **caso particular do orçamento de análise do §4.1**: quando aparecer a próxima ideia para os genéricos, o teste é aquele, não esta lista.

Decorre dela uma restrição de projeto: **função da stdlib grande demais para inline não pode ser genérica.** Ela vira função sobre `slice u8` ou sobre ponteiro cru, com a parte genérica sendo um invólucro fino.

> Razão: `rationale §4.9`.

### 4.10 Stdlib

Módulos escritos em keel, importados, sem privilégio nenhum. Estão neste degrau, e não na biblioteca de usuário, pelo critério do §1.6 — **é o que as bibliotecas precisam concordar.**

#### `string` e `strbuf`

São **dois módulos**, como `keel.buffer` e `keel.slice` são dois, e pela mesma razão: o alias é o qualificador do verbo.

```keel
import std.string as string types;
import std.strbuf as strbuf types;
```

> **`string` é `slice const char`. `strbuf` é `buffer char`.** São nomes para instâncias da base, não tipos com struct própria.

```keel
module std.string;   pub typedef slice const char string;
module std.strbuf;   pub typedef buffer char       strbuf;
```

O `typedef` do C não cria tipo, então `string` e `slice const char` são **o mesmo tipo**: não há conversão a escrever em fronteira nenhuma, e uma biblioteca que receba `slice const char` aceita a `string` de outra sem saber que ela existe. Três consequências:

- **Os verbos de medida e de acesso são os da base** — `slice.length(s)`, `s[i]`, `s[a..b]`, `buffer.push(sb, c)`. Não existe `string.length`, e escrevê-la é o error 112 `qualificador-errado`, que nomeia `slice`. Os dois módulos declaram **só o que é de texto**.
- **Os módulos não são genéricos**, então os verbos são funções de módulo — passo 2 do §4.11 —, e o `&` vem do parâmetro declarado. Na escrita não há diferença.
- **`strbuf` é `byref`**, porque `buffer` é: por valor em parâmetro é o error 58, e a mensagem indica `string` para ler e `strbuf *` para escrever.

**O elemento é `char`, não `u8`**, porque `char` é a unidade de texto do C e atravessa para `printf` e `strcspn` sem cast. Quem quer bytes escreve `slice u8`.

| | |
| --- | --- |
| **Codificação** | UTF-8 por convenção. Os verbos indexam **bytes**, não pontos de código |
| **Terminação** | **Não é terminada em NUL** — o comprimento vem do descritor. É o que faz `s[a..b]` custar zero |
| **Propriedade** | nenhuma; é uma vista, e quem morre é a origem |

**A ponte para o C é explícita, e tem uma direção em cada módulo:**

| Verbo | O que faz |
| --- | --- |
| `string.of(x)` | de literal — `sizeof - 1`, constante — ou de `const char *` — mede com `strlen` |
| `string.from(p, n)` | de ponteiro cru mais comprimento; `n` **asseverado** |
| `strbuf.cstr(sb)` | escreve `'\0'` acima de `len`, sem contá-lo, e devolve `const char *` — ou `NULL` se não couber |

As duas fontes de `of` se separam por token — literal é `TK_STRING` —, sem tipar nada.

```keel
strbuf sb = buffer.from(arena.alloc(a, char, 256), 256);
strbuf.append(sb, nome);
strbuf.append(sb, string.of(".txt"));
FILE *f = fopen(strbuf.cstr(sb), "rb");
```

**Os verbos** — todos `pub inline`, e nenhum aloca exceto os que recebem `arena *`:

| `std.string` — leitura | Devolve |
| --- | --- |
| `string.of(x)` · `string.from(p, n)` | `string` |
| `string.text(sb)` | `string` — o conteúdo corrente de um `strbuf` |
| `string.eq(a, b)` · `string.cmp(a, b)` | `bool` · `int` |
| `string.find(h, n)` · `string.rfind(h, n)` | `size_t`, ou `string.npos` |
| `string.starts_with(s, p)` · `string.ends_with(s, p)` | `bool` |
| `string.trim(s)` · `trim_left` · `trim_right` | `string` |
| `string.split(s, sep, rest)` | `string` — o primeiro campo; `rest` é `string *ref` |
| `string.clone(a, s)` | `outcome string` — alocada em `a`; falha pela regra do §4.11 |

| `std.strbuf` — escrita | Devolve |
| --- | --- |
| `strbuf.append(sb, s)` · `strbuf.append_char(sb, c)` | `bool` — falha limpo se não couber |
| `strbuf.cstr(sb)` | `const char *` ou `NULL` |

Empilhar, esvaziar, medir e criar são da base, e `std.strbuf` não os redeclara — é por isso que a tabela dele tem três linhas.

**`string.text(sb)` é o caminho de volta**, e existe porque `slice.of(sb)` daria `slice char` e não `slice const char` — duas instâncias distintas pela regra de grafia. A conversão de qualificação é verbo porque implícita ela não é.

**`string.split` é a única forma da stdlib com saída por parâmetro**, e o `ref` é o que diz que `rest` é um elemento, não um vetor.

**O que não está aqui:** formatação. `printf` e companhia são C, atravessam por `import_c`, e escrevem em `buffer.ptr(sb)` com `buffer.capacity(sb)` — o par que `snprintf` pede.

> Razão: `rationale §4.10`.

#### `tensor` e `view`

O segundo motivo da categoria existir: **rank ≥ 2 é onde as bibliotecas divergem em silêncio** — duas que escolham linearizações diferentes não compõem.

```keel
module std.tensor dim N type T;
pub modifier tensor byref { size_t dims[N]; buffer T dados; }      /* denso, row-major */

module std.view dim N type T;
pub modifier view { size_t dims[N]; size_t passos[N]; T *ptr; }    /* com passo */
```

`tensor` é `byref` — possui armazenamento, e copiar o descritor duplicaria o dono. `view` não é: vista por valor é o uso normal, como em `slice`. Só `std.tensor` importa `std.view`, porque é dele que saem as vistas, e não há ciclo.

| | `tensor(N) T` | `view(N) T` |
| --- | --- | --- |
| Layout | denso, row-major | passo por dimensão |
| Possui | sim | não |
| Descritor | `N` dimensões + `buffer` | `N` dimensões + `N` passos + ponteiro |
| `byref` | sim | não |

`tensor` é o caso comum e cabe menor; `view` é o geral, e é o que dá coluna, submatriz e transposta — a transposta sai trocando dois pares de campos, sem mover um byte:

| Recorte de uma matriz `r × c` | `dims` | `passos` |
| --- | --- | --- |
| linha | `[c]` | `[1]` |
| coluna | `[r]` | `[c]` |
| submatriz | `[r', c']` | `[c, 1]` |
| transposta | `[c, r]` | `[1, c]` |

**Conversão para `slice` é verificada, nunca implícita:** `slice.of(v)` sobre um `view` exige `N == 1` e passo `1`; fora disso não há região contígua a descrever, e o verbo devolve comprimento zero. É o mesmo estatuto de `tensor(1) f64`, que é estruturalmente um `slice f64` e não é um `slice f64`.

**Medida e acesso** — o protocolo do §4.11, que o açúcar consome:

| Verbo | Devolve |
| --- | --- |
| `tensor.length(t)` · `view.length(v)` | total de elementos |
| `tensor.dim(t, k)` · `view.dim(v, k)` | tamanho da dimensão `k` |
| `tensor.ptr(t, idx[static N])` | `T *ref` — acessor de rank cheio, e o único |
| `tensor.ptr(t)` | `T *` — a base, para entregar a uma função C |

```keel
tensor(2) f32 m;
f32 x = m[i,j];                /* k = N: o único caso */
f32 y = m[i];                  /* error 104: índices fora do rank */
```

É a tabela do §4.9 em uso, e é ela que faz `m[i,j,k]` e `m[i]` sobre um `tensor(2)` serem erro de compilação (104) em vez de `assert` de execução.

**Construção e recorte:**

| Verbo | Devolve |
| --- | --- |
| `tensor.alloc(a, d0, …, dN-1)` | `outcome tensor(N) T` — aloca na arena |
| `tensor.of(b, d0, …, dN-1)` | `tensor(N) T` — sobre `buffer T` que já existe |
| `view.of(t)` | `view(N) T` — a vista densa do tensor inteiro |
| `view.sub(v, r0, …, rN-1)` | `view(N) T` — recorte por `range` em cada eixo |
| `view.swap(v, k1, k2)` | `view(N) T` — troca dois eixos; transposta é `swap(v,0,1)` |
| `tensor.clone(a, v)` | `outcome tensor(N) T` — materializa uma vista com passo em denso |

**Os dois que alocam devolvem `outcome`**, pela regra de modo de falha do §4.11: o que eles produzem é descritor, e descritor mal construído não tem valor fora da faixa que o denuncie. Os que só recortam não alocam e não falham.

`view.of` e `tensor.clone` são os dois verbos de conversão, e caem na regra do §4.11 sem emenda: o qualificador nomeia o **resultado**, e o despacho é pelo primeiro argumento — por isso `view.of` é declarado em `std.tensor`. `tensor.alloc` é o único que recebe `arena *`, e diz isso na assinatura; os demais não alocam, porque recorte é aritmética sobre o descritor.

**`view.sub` é o recorte multidimensional**, e é verbo em vez de sintaxe pela razão do §4.6:

```keel
view(2) f32 bloco = view.sub(view.of(m), 2..8, 0..4);
```

**A travessia é sempre por dimensão.** `foreach` exige contêiner linear, e nem `tensor` nem `view` o são acima de rank 1 — então se aninha um `foreach` por eixo, e o acesso é sempre de rank cheio:

```keel
foreach (auto i : 0..tensor.dim(m,0))
    foreach (auto j : 0..tensor.dim(m,1))
        m[i,j] *= 2.0f;
```

Sem descritor intermediário: o acessor por vetor serve todo rank, e o compilador C desenrola o laço de índices por SROA (§4.9).

**O que falta aqui, e falta de propósito: reduzir rank.** Tomar a linha `i` de uma matriz como um `view(1)` relaciona **dois** ranks, e nenhuma das formas de escrever isso está fechada — a chamada teria de nomear a instância de um módulo com dois `dim`, e essa vaga está **reservada** (§4.3, error 126) em vez de decidida. Enquanto não estiver, a saída é o construtor geral:

```keel
view(1) f32 lin = view.from(tensor.ptr(m, (size_t[2]){i, 0}),
                            (size_t[1]){ tensor.dim(m, 1) },
                            (size_t[1]){ 1 });
```

Explícito e sem instância escondida — e com o passo escrito à mão, que é o custo de a decisão estar aberta.

**O teto de rank.** `N` é literal, então cada rank é um tipo, e o nome canônico carrega o numeral. Não há teto na linguagem; há na prática, e é o descritor: `view(N)` custa `2N` campos `size_t` mais um ponteiro, copiado a cada passagem por valor. Quatro cobre matriz, volume e lote de volumes.

**Rank de execução** — um tensor lido de arquivo, cujo rank vem do modelo — é outro tipo, com `rank` em campo e teto de dimensões fixo, e é biblioteca de usuário: ele paga em `assert` de execução o que `dim` paga em erro de compilação. A pergunta que decide entre os dois é **se o rank é conhecido onde o tipo é escrito**.

> Razão: `rationale §4.10`.

#### `outcome`

Tipo-resultado de **dois** estados — tem valor, ou não tem —, para a função que termina e pode não ter o que devolver. **Ele é `optional` e `result` no mesmo tipo.** É o par de `corot` no outro eixo: `corot` tem três status porque existe uma passagem que não terminou; `outcome` tem dois porque não existe, e os dois particionam o tipo inteiro.

```keel
pub outcome u32 porta(const char *path);
```

```c
typedef struct keel_outcome_u32 { i32 code; u32 v; } keel_outcome_u32;
keel_outcome_u32 cfg_porta(const char *path);
```

`outcome` é **modificador comum** do módulo `keel.outcome`: nome canônico, instanciação por uso, header próprio, mangling recursivo. `outcome slice u8` e `buffer outcome i32` funcionam sem regra nova.

**O protocolo é `failed`; `outcome` é uma implementação dele.**

> O que a cláusula `else` conhece é o verbo **`failed`**. Todo tipo que o declara é falível, e `outcome` é apenas a implementação de referência.

- **Modificador do usuário entra pela mesma porta:** um `modifier` que declare `pub inline bool failed(<mod> e)` funciona com `else` sem que keel saiba o que ele é.
- **`outcome` não é obrigatório.** Biblioteca com tipo de resultado próprio declara `failed` e acabou.
- **O protocolo tem um nome exigido e um proibido:** o tipo declara `failed` e **não** declara `ongoing`. O segundo é o que põe `corot` de fora, e de propósito — ele declara os três (§4.8), e três status independentes não cabem num predicado binário: `else` teria de decidir em silêncio o que fazer com ONGOING, e qualquer decisão estaria errada em metade dos usos. Continua sendo consulta à tabela por nome, sem lista de tipos privilegiados: o que a tabela responde é *este módulo declara `ongoing`?*

| Verbo | Aridade | Resultado |
| --- | --- | --- |
| `outcome.value(e)` · `outcome.code(e)` | 1 | o valor, do tipo `T` · o código, `i32` |
| `outcome.failed(e)` · `outcome.ok(e)` | 1 | `bool` |
| `outcome.win(v)` | 1 | resultado com valor e `code == OK` |
| `outcome.fail(c)` | 1 | falha com código `c` |
| `outcome.none()` | 0 | falha com o código reservado `NONE` |

Os três produtores tomam a instância **do alvo**, como `buffer.from` — linha 4 da tabela do §4.11: o tipo vem do declarador que os inicializa, do símbolo à esquerda da atribuição, ou do retorno da função que os envolve. Fora dessas posições é a mesma recusa do error **113**.

```keel
pub outcome u32 porta(const char *path) {
    outcome slice char s = cfg.campo(path, "port") else return outcome.none();
    u32 v;
    if (!parse_u32(outcome.value(s), &v)) return outcome.fail(EINVAL);
    return outcome.win(v);
}
```

**O código é `i32`, e não é parâmetro do módulo.** Dois parâmetros dariam duas instâncias com o mesmo `T` que não se convertem — a conversão de erro que a cláusula `else` recusou, voltando pela porta dos fundos. Com o código fixo, `outcome i32` de um módulo é `outcome i32` de outro, e a composição entre bibliotecas não pede acordo. O catálogo de códigos é do programa, que é onde ele sempre esteve em C.

Duas constantes são escopadas no tipo:

| Constante | Valor | O que é |
| --- | --- | --- |
| `outcome.OK` | `0` | o único código de sucesso |
| `outcome.NONE` | `INT32_MIN` | um código de falha, reservado, para "não há valor" |

**`OK` vale zero, e isso é normativo:** `code` vem primeiro no layout, e é o que faz `{0}` e `memset` deixarem um `outcome T` válido com valor presente. **`failed` é comparação com zero, não com uma lista** — acrescentar código de erro novo não toca a função.

| `code` | Significa | `failed` |
| --- | --- | --- |
| `OK` | há valor | falso |
| `NONE` | **não há valor**, e não houve erro | verdadeiro |
| qualquer outro | houve erro, e o código diz qual | verdadeiro |

As três linhas são **dois estados, não três**: `failed` particiona o tipo em *tem valor* e *não tem*, e `NONE` mora do lado de lá junto com os erros porque não ter valor e ter valor errado são o mesmo estado formal. O que `NONE` distingue é o *motivo*, para quem lê o `code`. É por isso que a forma de default da cláusula cobre os dois sem saber qual é.

O valor é reservado e não derivado de `OK`: `!0` é `1`, o primeiro código que todo catálogo usa; `~0` é `-1`, o segundo. `INT32_MIN` está fora de qualquer catálogo real.

**A polaridade é oposta à do ponteiro**, e é por isso que a cláusula `else` não cobre os dois: aqui zero é sucesso; num `T *`, zero é a falha.

**O que ele não é.** Não é `Result`, e não tem `map`, `and_then` nem `?`. Encadeamento sem escrever o `return` exige o operador de propagação, que continua na tabela das sete recusas (§5). O que existe é a linha do §4.7 — teste da construção, tipo da base, `return` do programa —, e ela é a fronteira, não um degrau para outra coisa.

> Razão: `rationale §4.10`.

### 4.11 Despacho e verbos da base

#### Como uma chamada qualificada resolve

Toda chamada qualificada tem a forma `m.f(x, …)`, com `m` nome ou alias de módulo importado. Resolve em três passos, nesta ordem:

1. **Verbo** — se `f` pertence ao tipo de `x`, com `x` expressão de contêiner (§3.4) cujo tipo é instância de modificador ou `array`. A chamada é reescrita para a função da instância.
2. **Função de módulo** — senão, se `m` declara uma função `f` da aridade escrita. Sai qualificada como qualquer outra: `geom.dist(a, b)` vira `geom_dist(a, b)`.
3. **Erro** — senão. Se `f` pertence ao tipo de outro módulo, é o error **112** `qualificador-errado`, que nomeia qual; se não pertence a nenhum, a chamada sai qualificada e quem acusa é o compilador C.

**Não há quarta categoria**, e a ordem importa: é ela que deixa um módulo comum declarar função de primeiro parâmetro contêiner sem competir com o despacho.

Expressão fora da gramática de contêiner, em posição de contêiner, é o error **15** `fora-da-gramatica-de-conteiner`.

**Achar a função e achar a instância são duas perguntas.** No caso geral as duas se respondem no primeiro argumento, e ninguém precisa distingui-las. Onde não se respondem no mesmo lugar, o inventário é fechado e está adiante, em *De onde vem a instância*.

#### O qualificador é leitura; a função vem do primeiro argumento

O qualificador não escolhe a função — quem escolhe é o tipo do primeiro argumento. (De onde vem a **instância** é outra pergunta, e tem tabela própria adiante.) Ele existe porque o leitor precisa saber de quem o verbo é, e é **conferido** contra o despacho.

> **Verbo que produz um contêiner — `of`, `from`, `clone` — é qualificado pelo tipo produzido. Todo o resto é qualificado pelo módulo do tipo do primeiro argumento.**

```keel
buffer.length(b)          /* mede um buffer   → qualificador do primeiro argumento */
slice.of(b)               /* produz um slice  → qualificador do resultado          */
buffer.clone(a, s)        /* produz um buffer a partir de um slice                 */
```

```c
keel_buffer_i32_length(&b)
keel_buffer_i32_as_slice(&b)
keel_buffer_i32_clone(&a, &s)        /* devolve keel_outcome_keel_buffer_i32 */
```

Nos dois casos o qualificador nomeia o **tipo que se vai ter na mão depois da linha**. **O `outcome` não entra nele**: `buffer.clone` devolve `outcome buffer i32` e continua se chamando `buffer.clone` — o que a regra nomeia é o produto, e `outcome` é o envelope de falha.

Sobre `array`, que é marcador do núcleo e não instância de módulo, o qualificador é `keel`: `keel.length(v)`, `keel.ptr(v, i)`, `keel.dim(v, k)`.

#### De onde vem a instância

> **A instância vem de um de quatro lugares, e a lista é fechada.** Fora dela, é o
> contêiner do primeiro argumento — o caso geral, e a esmagadora maioria.

| De onde | Verbos | Como se reconhece |
| --- | --- | --- |
| **1. contêiner do 1º argumento** | todo o resto | o caso geral |
| **2. tipo escrito na chamada** | `slice.from(T, p, n)`, `arena.alloc(a, T, n)` | o `T` é argumento, e está à vista |
| **3. contêiner de outro argumento** | `buffer.clone(a, x)`, `slice.clone(a, x)`, `tensor.clone(a, v)`, `string.clone(a, s)`, `view.of(t)` | o 1º argumento é `arena *`, e a instância vem do **seguinte** |
| **4. alvo** | `buffer.from(p, cap)`, `outcome.win(v)`, `outcome.fail(c)`, `outcome.none()`, `tensor.alloc(a, d₀…)` | nenhum argumento carrega o tipo; ele vem do declarador que inicializa, do símbolo à esquerda da atribuição, ou do tipo de retorno da função que envolve |

Nada aí é inferência: **cada linha é um nome numa tabela**, lida antes de parsear a chamada, e a posição de onde ler é sintática nas quatro. A linha 3 não contraria a regra do primeiro argumento — ela a aplica, e a arena é o primeiro argumento porque é ela quem recebe a alocação; o que a linha registra é que a **instância** está no argumento seguinte, que é o único do qual ela poderia vir.

**A linha 4 é a única que olha para fora da chamada**, e cobra por isso: fora de inicialização, de atribuição a símbolo do tipo, ou de `return`, é o error **113** `from-sem-alvo`.

**Um caso que não é de instância e vale registrar:** `string.of(x)` tem duas fontes — literal e `const char *` — e se separam **por token**, `TK_STRING` ou não. Não é sobrecarga por tipo, é a mesma varredura léxica de sempre; a instância de `string` é única e não está em disputa.

#### Valor ou ponteiro

> O símbolo guarda se foi declarado **como valor ou como ponteiro** (§4.3). Verbo sobre símbolo-valor emite `&x`; sobre símbolo-ponteiro, emite `x`.

```keel
arena  a;   arena.alloc(a, i32, 4);      /* valor    */
arena *p;   arena.alloc(p, i32, 4);      /* ponteiro */
```

```c
keel_arena_alloc_n(&a, 4, sizeof(i32), alignof(i32));
keel_arena_alloc_n(p,  4, sizeof(i32), alignof(i32));
```

Vale igual para `buffer T *`, `slice T *` e para os nós `*`, `&`, `.` e `->` da gramática de contêiner — é o que faz `buffer.length(b)` e `buffer.length(w->ps)` saírem certos sem o parser tipar expressão nenhuma. É informação da **declaração**, não da expressão.

**O `&` vem do parâmetro declarado, não de a chamada ser verbo.** Numa chamada qualificada cujo parâmetro é ponteiro para instância de modificador, e cujo argumento é símbolo-valor desse tipo, keel emite `&x` — indiferentemente pelo passo 1 ou pelo passo 2. Fora desse caso, chamada de função passa os argumentos como escritos.

**Acesso direto a campo de instância** — `b.len`, `s.ptr` — é o warning **48** `campo-de-instancia`: os campos são do backend, e o verbo existe para que o layout possa mudar.

#### Os verbos

Todo verbo é escrito **qualificado pelo módulo que o declara** (§1.7).

**`keel`** — os primitivos, e os verbos do núcleo sobre `array`

| Verbo | Aridade | Nota |
| --- | --- | --- |
| `keel.length(v)` · `keel.capacity(v)` | 1 | constantes de compilação |
| `keel.dim(v, k)` | 2 | `k` literal inteiro |
| `keel.get(v, i)` · `keel.set(v, i, x)` | 2 · 3 | |
| `keel.at(v, i)` | 2 | `outcome T` — verificado em toda build |
| `keel.ptr(v)` · `keel.ptr(v, i)` | 1 · 2 | `T *` · `T *ref` |

**`arena`** — nomeada pela **origem da memória**, que é o que determina tempo de vida e modo de falha

| Verbo | Aridade | Nota |
| --- | --- | --- |
| `arena.from_array(a, memo)` | 2 | `memo` é `array u8` |
| `arena.from_stack(a, N)` | 2 | `N` constante |
| `arena.from_parent(s, pai, n)` | 3 | recorta do pai |
| `arena.from_memory(h, p, n)` | 3 | região crua vinda do C |
| `arena.alloc(a, T, n)` | 3 | `T *` ou `NULL` |
| `arena.mark(a)` · `arena.reset(a)` | 1 | |
| `arena.restore(a, m)` | 2 | |
| `arena.length(a)` · `arena.capacity(a)` | 1 | ocupado · total |

**`buffer`**

| Verbo | Aridade | Nota |
| --- | --- | --- |
| `buffer.of(v)` | 1 | só sobre `array` — tamanho **deduzido** |
| `buffer.from(p, cap)` | 2 | ponteiro cru — `cap` **asseverado** |
| `buffer.length(b)` · `buffer.capacity(b)` | 1 | |
| `buffer.get(b, i)` · `buffer.set(b, i, x)` | 2 · 3 | |
| `buffer.at(b, i)` | 2 | `outcome T`; `NONE` fora de faixa |
| `buffer.ptr(b)` · `buffer.ptr(b, i)` | 1 · 2 | |
| `buffer.push(b)` · `buffer.push(b, v)` | 1 · 2 | `T *ref` ou `NULL` |
| `buffer.pop(b)` · `buffer.clear(b)` | 1 | |
| `buffer.clone(a, x)` | 2 | `x` é `buffer` ou `array` — devolve `outcome buffer T` |

**`slice`**

| Verbo | Aridade | Nota |
| --- | --- | --- |
| `slice.of(x)` · `slice.of(x, r)` · `slice.of(x, i, j)` | 1 · 2 · 3 | vista — tamanho **deduzido** |
| `slice.from(T, p, n)` | 3 | ponteiro cru — `n` **asseverado** |
| `slice.length(s)` · `slice.get(s, i)` | 1 · 2 | |
| `slice.at(s, i)` | 2 | `outcome T`; `NONE` fora de faixa |
| `slice.ptr(s)` · `slice.ptr(s, i)` | 1 · 2 | |
| `slice.clone(a, s)` | 2 | devolve `outcome slice T` |

**`corot`** — `corot.ok(r)`, `corot.ongoing(r)`, `corot.failed(r)`, `corot.value(r)`, `corot.code(r)`. Só leitura: quem produz são as palavras `co*` (§4.8).

**`outcome`** — `outcome.value(e)`, `outcome.code(e)`, `outcome.failed(e)`, `outcome.ok(e)`, e os produtores `outcome.win(v)`, `outcome.fail(c)`, `outcome.none()`.

**`range`** — `range.first(r)`, `range.limit(r)`, `range.length(r)`, `range.get(r, i)`.

#### As três regras que os nomes carregam

**`of` é vista, `from` é asserção.** `of` recebe símbolo que keel conhece, e o tamanho vem do `sizeof` do compilador C ou do comprimento da origem. `from` recebe ponteiro do C, e o tamanho é promessa de quem chama. São dois regimes de segurança, e por isso têm nomes diferentes em vez de aridades diferentes do mesmo nome.

**A arena nomeia pelo outro eixo.** Três dos quatro construtores dela também deduzem o tamanho, e ainda assim todos se chamam `from_<origem>`: ali o que decide o erro não é como o tamanho foi obtido, é **quem morre quando**.

**O modo de falha segue a categoria do retorno.**

| O verbo devolve | Falha como | Porque |
| --- | --- | --- |
| ponteiro — `T *`, `T *ref` | `NULL` | o C já dá um valor fora da faixa útil |
| valor — contêiner, descritor, struct | `outcome` do mesmo tipo | não há valor fora da faixa: um `buffer` mal construído é um `buffer` |

`arena.alloc` e `buffer.push` caem na primeira linha; `clone` e `at`, na segunda. É a mesma razão que tira ponteiro da cláusula `else` (§4.7), lida do lado de quem produz. A stdlib, quando for escrita, segue esta regra.

#### O que fica nu

Duas famílias, por razões diferentes: o **açúcar** (§3.7), porque é sintaxe e não chamada; e os predicados sobre bloco `parallel` — `ok(nome)`, `failed(nome)`, `interrupted(nome)` —, porque o argumento é símbolo do núcleo e não contêiner.

`fail` aparece em dois papéis e eles não se cruzam: `cofail(n)` **produz** uma falha e sai da função (§4.8); `fail` nu **encerra** uma faixa de `parallel` (§4.7). `failed` é sempre interrogação — `failed(nome)` sem qualificador lê um bloco `parallel`; `outcome.failed(e)` e `corot.failed(r)` leem um valor. Quem leva qualificador é verbo de módulo; quem não leva é do núcleo.

**Declarar `failed` não basta para ser falível.** O protocolo do §4.7 exige também **não** declarar `ongoing`, e é só por isso que `corot` fica fora da cláusula `else` sem que a exclusão precise ser escrita como exceção.

---

## 5. O que keel não tem

A lista é curta e tem uma razão só — todas batem no princípio 7, a invariante do §1.3. **É a lista do que keel não pode**, não a do que ele dispensou: recusa por valor, e não por capacidade — `match`, `load_text` —, é o princípio 9, e não entra aqui.

| Recurso | O que ele exigiria |
| --- | --- |
| Operador de propagação de erro | achar o operador dentro de expressão arbitrária |
| Tupla anônima e desestruturação de retorno | o tipo de uma **chamada** |
| Função genérica livre | deduzir o parâmetro de tipo do argumento na chamada |
| Corrotina com retomada de posição | entender o corpo, para içar locais através da suspensão |
| Lambdas e closures | idem, mais captura implícita |
| Fatiamento multidimensional | despacho pela **forma** dos argumentos, não pela contagem |
| Inversão de layout — `soa` | conhecer o tipo dos campos, para saber o que é invertível |

Nenhuma foi cortada por escopo, prazo ou dificuldade. **Uma linguagem cujas recusas têm sete razões diferentes está inacabada; uma cujas recusas têm uma razão só está terminada.**

A cláusula `else` (§4.7) **não sai da primeira linha da tabela**, e vale dizer por
quê. Ela não é uma versão parcial do operador: é outra construção, que resolve o
caso doloroso — testar e sair na mesma linha — sem pedir nada do que a camada não
tem. Ela faz o **teste**, que vem de um tipo escrito numa declaração; não faz a
extração nem a conversão, que viriam do tipo de uma **chamada**. O operador
continua recusado pela razão de sempre, e por mais duas que só aparecem quando se
tenta desenhá-lo: `x?` só se separa de `x ? y : z` procurando o `:` de mesma
profundidade, e içar o operando muda o sentido — `a && f()?` avaliaria `f`
incondicionalmente, porque o temporário sai antes do statement. A última é a que
fecha a discussão: é semântica, não sintática, e continua valendo com um parser
completo.

Sete recusas, um motivo só — ainda.

Lida do outro lado, a tabela é o roteiro do `cdod` — a camada de cima, que tipa expressão e produz keel.

---

## 6. Diagnósticos

Tabela única, numeração estável, um nome kebab-case por diagnóstico. Cada seção
cita apenas os seus; esta é a lista completa.

`error` interrompe a geração — nada é escrito. `warning` e `info` não. `debug` não
é diagnóstico de tradução: é verificação em tempo de execução, presente só em
build de debug (§7.1).

A coluna **Origem** diz de qual documento a regra é. `núcleo`, `privilegiada` e
`biblioteca` são deste; `backend` é de `keel-c-backend.md`, e esses casos
aparecem aqui só para que a numeração seja única no projeto.

| # | Nome | Diagnóstico | Sev. | Origem | § |
| --- | --- | --- | --- | --- | --- |
| 1 | `sem-module` | Arquivo sem `module` como primeiro token significativo | `error` | núcleo | §4.1 |
| 2 | `module-fora-do-caminho` | Nome declarado em `module` divergente do caminho relativo à raiz | `error` | núcleo | §4.1 |
| 3 | `stem-invalido` | Stem de arquivo que não é identificador C válido | `error` | núcleo | §4.1 |
| 4 | `stem-ambiguo-por-caixa` | Nomes de arquivo diferindo apenas por caixa | `error` | núcleo | §4.1 |
| 5 | `simbolo-colidido` | Colisão entre símbolos exportados de módulos do build | `error` | núcleo | §4.1 |
| 6 | `import-circular` | Import circular, com a cadeia completa na mensagem | `error` | núcleo | §4.1 |
| 7 | `extern-c-aninhado` | `extern_c` em posição aninhada, isto é, fora do nível de arquivo | `error` | núcleo | §4.1 |
| 8 | `main-em-extern-c` | `main` definida dentro de `extern_c` | `error` | núcleo | §4.1 |
| 9 | `main-privada` | `priv` aplicado a `main` | `error` | núcleo | §4.1 |
| 10 | `main-assinatura` | `main` assinada fora das duas formas do C | `error` | núcleo | §4.1 |
| 10a | `import-ordem-trocada` | `import M types as m;` — a mensagem dá a forma correta | `error` | núcleo | §4.1 |
| 10b | `types-duplicado` | Dois imports com `types` injetando o mesmo nome nu | `error` | núcleo | §4.1 |
| 10c | `types-sombreado` | Nome injetado por `types` sombreado por declaração local | `warning` | núcleo | §4.1 |
| 10d | `types-injetados` | Lista dos nomes que `types` injetou, no ponto do import | `info` | núcleo | §4.1 |
| 11 | `pub-static` | `pub static` em escopo de arquivo | `error` | núcleo | §4.1 |
| 12 | `inline-sem-visibilidade` | `static inline` sem `pub`/`priv` em nível de módulo | `error` | núcleo | §4.1 |
| 13 | `static-em-tipo` | `static` aplicada a tipo | `error` | núcleo | §4.1 |
| 14 | `nome-acima-do-teto` | Nome gerado acima do teto de comprimento do alvo | `error` | backend | §4.3 |
| 15 | `fora-da-gramatica-de-conteiner` | Expressão fora da gramática de contêiner em posição de contêiner | `error` | núcleo | §4.3 |
| 16 | `tipo-c-como-argumento` | Tipo aritmético por palavra-chave do C como argumento de modificador | `error` | núcleo | §4.2 |
| 17 | `void-como-argumento` | `void` como argumento de modificador | `error` | núcleo | §4.2 |
| 18 | `nome-reservado` | Identificador do usuário no espaço reservado `keel_` | `error` | backend | §1.5 |
| 19 | `redeclaracao-de-simbolo` | Redeclaração local de nome de símbolo keel conhecido | `error` | núcleo | §3.10 |
| 21 | `buffer-sobre-const` | `buffer` sobre vetor C `const` — a mensagem indica `slice const T` | `error` | biblioteca | §4.5 |
| 22 | `get-copia-conteiner` | `get` ou `set` sobre elemento que é instância de modificador | `error` | biblioteca | §4.11 |
| 23 | `set-fora-de-length` | `set` com índice fora de `length` | `debug` | backend | §4.11 |
| 24 | `buffer-of-tamanho` | `buffer.of` de um argumento sobre símbolo que não é `array` | `error` | biblioteca | §4.2 |
| 25 | `array-1d-em-parametro` | `array` unidimensional em parâmetro de função | `error` | núcleo | §4.2 |
| 26 | `array-sem-dimensao-em-parametro` | `array T v[]` sem dimensão em parâmetro | `error` | núcleo | §4.2 |
| 27 | `array-indexacao-parcial` | Indexação parcial de `array` multidimensional | `error` | núcleo | §4.2 |
| 28 | `view-sobre-array-nd` | `keel.ptr`, `buffer.of` ou `slice.of` sobre `array` multidimensional | `error` | biblioteca | §4.2 |
| 29 | `dim-k-nao-constante` | `keel.dim(v,k)` com `k` não constante | `error` | núcleo | §4.2 |
| 30 | `ref-sem-inicializador` | `ref` sem inicializador | `error` | núcleo | §4.2 |
| 31 | `ref-aritmetica` | Aritmética ou indexação sobre `ref` | `error` | núcleo | §4.2 |
| 32 | `slice-from-sobre-ref` | `slice.from` sobre `ref` | `error` | núcleo | §4.2 |
| 33 | `arena-stack-nao-constante` | `arena.from_stack` com tamanho não constante; a mensagem indica `arena.from_parent` | `error` | privilegiada | §4.4 |
| 34 | `arena-from-array-nao-u8` | `arena.from_array` sobre símbolo que não é `array u8` | `error` | privilegiada | §4.4 |
| 35 | `arena-escape` | `return` de contêiner cuja arena tem armazenamento local | `error` | privilegiada | §4.4 |
| 36 | `defer-sem-bloco` | `defer` como corpo de statement de controle sem chaves | `error` | núcleo | §4.7 |
| 37 | `defer-em-escopo-de-arquivo` | `defer` em escopo de arquivo | `error` | núcleo | §4.7 |
| 38 | `salto-sobre-defer` | Salto que entra em escopo por cima de um registro de `defer`: `goto` para rótulo interno, ou `case`/`default` posterior a um `defer` do mesmo `switch` | `error` | núcleo | §4.7 |
| 39 | `later-com-captura` | `later` seguido de lista de captura, dentro do colchete do `defer` | `error` | núcleo | §4.7 |
| 40 | `delimitador-sem-par` | Chave, parêntese ou colchete sem par — inclusive dentro de `extern_c` | `error` | núcleo | §3.6 |
| 41 | `chaves-em-ramos` | Alternativas de um grupo condicional que discordam na contagem de delimitadores | `error` | núcleo | §1.5 |
| 42 | `define-sobre-keel` | `#define` ou `#undef` de palavra contextual de keel ou de nome `keel_` | `error` | núcleo | §1.5 |
| 43 | `literal-com-newline` | Literal de string ou char com newline não-emendado | `error` | núcleo | §3.6 |
| 44 | `sombreamento` | Sombreamento de palavra contextual, de verbo ou de nome de módulo | `warning` | núcleo | §3.10 |
| 46 | `byref-atribuido` | Atribuição entre instâncias de modificador `byref`, nomeando o aliasing | `warning` | biblioteca | §4.9 |
| 47 | `defer-em-bloco-de-controle` | `defer` registrado em corpo de `if`, `else` ou `switch` | `warning` | núcleo | §4.7 |
| 48 | `campo-de-instancia` | Acesso direto a campo de instância de modificador | `warning` | backend | §4.11 |
| 49 | `import-indireto` | Uso de símbolo de módulo não importado diretamente | `info` | núcleo | §4.1 |
| 50 | `modifier-fora-de-generico` | `modifier` fora de módulo genérico | `error` | núcleo | §4.9 |
| 51 | `nome-de-parametro` | Declaração de símbolo com o nome de um parâmetro do módulo | `error` | núcleo | §4.9 |
| 53 | `generico-circular` | Módulo genérico que se instancia, com a cadeia na mensagem | `error` | núcleo | §4.9 |
| 55 | `instance-fora-de-arquivo` | `instance` fora de escopo de arquivo | `error` | núcleo | §4.9 |
| 56 | `instance-nao-modificador` | Argumento de `instance` que não é modificador de módulo genérico | `error` | núcleo | §4.9 |
| 57 | `instance-inutil` | `instance` sobre genérico inteiramente `pub inline` | `warning` | núcleo | §4.9 |
| 58 | `byref-param` | Instância `byref` por valor em parâmetro — `arena`, `buffer` e todo modificador marcado | `error` | biblioteca | §4.9, §4.5, §4.4 |
| 59 | `arena-filha-apos-reset` | Uso de arena filha depois de `reset`/`restore` do pai, no mesmo escopo | `error` | privilegiada | §4.4 |
| 60 | `cofsm-sem-nome` | `cofsm`, `coseq` ou `copar` sem nome | `error` | núcleo | §4.8 |
| 61 | `estado-de-outro-tipo` | Variável de estado cujo tipo não é o da fsm ou o da cadeia | `error` | núcleo | §4.8 |
| 62 | `estado-repetido` | Rótulo de estado repetido no mesmo `cofsm` | `error` | núcleo | §4.8 |
| 63 | `cobreak-fora-de-cofsm` | `cobreak` fora de bloco de estado | `error` | núcleo | §4.8 |
| 64 | `cofail-zero` | `cofail(0)` com argumento literal — zero é `ONGOING` | `error` | núcleo | §4.8 |
| 65 | `cofsm-nome-repetido` | Duas máquinas com o mesmo nome no módulo | `error` | núcleo | §4.8 |
| 66 | `estado-fora-de-faixa` | Estado fora de faixa na variável de estado | `debug` | backend | §4.8 |
| 67 | `corot-sem-simbolo` | Verbo de `corot` sobre expressão que não é símbolo | `error` | biblioteca | §4.8 |
| 68 | `aridade-sem-ptr` | Índice de aridade N sobre modificador sem `ptr` dessa aridade — a mensagem lista as que existem | `error` | núcleo | §4.6 |
| 69 | `nao-percorrivel` | `foreach` sobre tipo que não declara `length`, ou `get`/`ptr` conforme o binder | `error` | núcleo | §4.7 |
| 70 | `foreach-copia-conteiner` | `foreach` por valor sobre elemento que é instância de modificador — a mensagem indica `T *` | `error` | núcleo | §4.7 |
| 71 | `mutacao-na-travessia` | `push`, `pop` ou `clear` sobre o contêiner percorrido, no corpo do `foreach` ou do `parallel` | `error` | núcleo | §4.7, §4.7 |
| 72 | `indice-nao-size-t` | Binder de índice cujo tipo não é `size_t` | `error` | núcleo | §4.7 |
| 73 | `nao-contavel` | `foreach` de um binder sobre tipo que não declara `first` e `limit` | `error` | núcleo | §4.7 |
| 77 | `recorte-sem-of` | Índice por intervalo sobre tipo que não declara o `of` da aridade que a forma exige | `error` | núcleo | §4.6 |
| 78 | `recorte-invertido` | Intervalo com os dois limites literais e início maior que fim | `error` | núcleo | §4.6 |
| 79 | `recorte-fora-de-faixa` | Intervalo cujos limites violam `a <= b <= length(x)` | `debug` | backend | §4.6 |
| 80 | `foreach-dois-binders-em-literal` | `foreach` de dois binders sobre literal de intervalo — a mensagem indica nomear o intervalo | `error` | núcleo | §4.7 |
| 81 | `binder-ponteiro-em-intervalo` | Binder por ponteiro na forma de intervalo | `error` | núcleo | §4.7 |
| 82 | `alloc-alinhamento` | `arena.alloc` cujo `alignof(T)` excede o alinhamento da base da arena | `debug` | backend | §4.4 |
| 83 | `recorte-aberto` | Recorte com ponta aberta fora de índice | `error` | núcleo | §3.4 |
| 84 | `recorte-aberto-com-indice` | `x[a..]` sobre caminho que contém índice ou verbo | `error` | núcleo | §4.6 |
| 85 | `enum-sem-o-tipo` | Constante de enum escrita sem o nível do tipo | `error` | núcleo | §4.3 |
| 86 | `alias-e-tipo-colidem` | Alias de módulo e nome de tipo com a mesma grafia no mesmo arquivo | `error` | núcleo | §4.3 |
| 87 | `parallel-sem-nome` | `parallel` sem nome | `error` | núcleo | §4.7 |
| 88 | `interrupted-ausente` | `interrupted` ausente no corpo quando a política não é `ALL`, ou presente sob `ALL` | `error` | núcleo | §4.7 |
| 89 | `verbo-fora-de-parallel` | `win`, `fail` ou `interrupted` fora de corpo de `parallel` | `error` | núcleo | §4.7 |
| 90 | `parallel-aninhado` | `parallel` aninhado em corpo de `parallel` | `error` | núcleo | §4.7 |
| 91 | `parallel-nome-repetido` | Dois `parallel` com o mesmo nome no módulo | `error` | núcleo | §4.7 |
| 92 | `parallel-nao-constante` | `k` ou política de `parallel` que não é constante de compilação | `error` | núcleo | §4.7, §4.7 |
| 93 | `co-verbo-fora-de-corot` | `cowin`, `cofail` ou `coagain` em função cujo retorno não é `corot T` | `error` | núcleo | §4.8 |
| 94 | `cowin-sem-valor` | `cowin` sem valor | `error` | núcleo | §4.8 |
| 95 | `estagio-invalido` | Statement em bloco de `coseq` ou `copar` que não é chamada | `error` | núcleo | §4.8 |
| 97 | `copar-politica-nao-constante` | Política de `copar` que não é constante de compilação | `error` | núcleo | §4.8 |
| 98 | `estado-fora-da-lista` | Rótulo de estado que não está na lista declarada do `cofsm` | `error` | núcleo | §4.8 |
| 99 | `estado-sem-rotulo` | Estado na lista declarada do `cofsm` sem rótulo correspondente no corpo | `error` | núcleo | §4.8 |
| 101 | `restrict-em-conteiner` | `restrict` escrito antes de um modificador — a `note` dá a forma com ponteiro | `error` | núcleo | §4.2 |
| 103 | `formato-estreito-indisponivel` | Módulo usa `f16` ou `bf16` e o alvo não oferece o formato | `error` | backend | §4.2 |
| 104 | `indices-fora-do-rank` | `x[i, …]` com número de índices diferente do `dim N` do modificador — indexação é total ou nenhuma | `error` | núcleo | §4.9, §4.6 |
| 105 | `dim-sem-ptr-cheio` | Modificador com `dim` sem `ptr` de rank cheio, usado com `k == N` índices — a mensagem dá a assinatura que falta | `error` | núcleo | §4.9 |
| 106 | `dim-nao-constante` | Argumento de `dim` que não é literal decimal nem `constexpr` de inicializador literal | `error` | núcleo | §4.9 |
| 107 | `dim-gera-declaracao` | `dim` usado onde geraria declaração — parâmetro, campo ou função | `error` | núcleo | §4.9 |
| 108 | `openmp-indisponivel` | Módulo usa `parallel` e o alvo não oferece OpenMP — a travessia sai em série | `warning` | backend | §4.7 |
| 109 | `alloc-overflow` | `arena.alloc` cujo `n * sizeof(T)` não cabe em `size_t` | `debug` | backend | §4.4 |
| 110 | `nome-canonico-colidido` | Duas declarações do mesmo módulo produzindo o mesmo nome canônico | `error` | núcleo | §1.3 |
| 111 | `modificador-chamado-instance` | Modificador declarado com o nome `instance` | `error` | núcleo | §3.5 |
| 112 | `qualificador-errado` | Verbo qualificado por módulo que não é o do primeiro argumento nem o do resultado | `error` | núcleo | §4.11 |
| 113 | `from-sem-alvo` | Verbo que toma a instância do alvo — `buffer.from`, `outcome.win/fail/none`, `tensor.alloc` (§4.11) — fora de inicialização, de atribuição a símbolo do tipo, ou de `return` | `error` | biblioteca | §4.11 |
| 114 | `captura-escrita` | Atribuição a escalar capturado, no corpo de um `parallel` | `error` | núcleo | §4.7 |
| 115 | `else-sem-inicializador` | Cláusula `else` em declaração sem inicializador | `error` | núcleo | §4.7 |
| 116 | `else-tipo-nao-falivel` | Cláusula `else` sobre declaração cujo tipo não declara `failed`, ou declara também `ongoing` — ponteiro e escalar incluídos | `error` | núcleo | §4.7 |
| 117 | `else-em-atribuicao` | Cláusula `else` sobre atribuição a símbolo existente | `error` | núcleo | §4.7 |
| 118 | `else-multiplos-declaradores` | Cláusula `else` em declaração com mais de um declarador | `error` | núcleo | §4.7 |
| 119 | `else-default-sem-win` | Cláusula `else` na forma de default sobre tipo falível que não declara `win` | `error` | núcleo | §4.7 |
| 120 | `declarador-enterrado` | Declarador cujo nome não é o último token, onde keel precisa reconstruir a declaração — `constexpr`, captura de `[now]`, tipo de retorno sob `defer`. A `note` manda usar `typedef` | `error` | núcleo | §4.2, §4.7 |
| 121 | `constexpr-agregado` | `constexpr` com declarador de vetor ou inicializador entre chaves | `error` | núcleo | §4.2 |
| 122 | `return-em-parallel` | `return` no corpo de um `parallel` — bloco estruturado de OpenMP não admite salto para fora | `error` | núcleo | §4.7 |
| 123 | `defer-later-sombreado` | `defer` sem `[now]` cujo corpo nomeia símbolo redeclarado em escopo mais interno com ponto de saída — a `note` dá as duas saídas, `[now]` ou `goto` | `error` | núcleo | §4.7 |
| 124 | `constexpr-endereco` | `&` sobre símbolo `constexpr`, ou uso que exija lvalue — a `note` dá a saída, `static const T k = K;` | `error` | núcleo | §4.2 |
| 125 | `dim-abaixo-de-um` | Argumento de `dim` que resolve para valor menor que 1 — a mensagem dá a cadeia de instanciação | `error` | núcleo | §4.9 |
| 126 | `alias-com-argumento` | Alias de módulo seguido de `(` — vaga reservada; a `note` manda renomear o alias | `error` | núcleo | §4.3 |

**Os buracos são deliberados**, e a numeração é preservada para que referências
externas não quebrem:

| # | O que aconteceu |
| --- | --- |
| 20, 54 | absorvidos pelo **58**: `byref` (§4.9) é um bit só, e `buffer`, `arena` e modificador do usuário caem na mesma regra |
| 45 | removido: módulo cujo nome é palavra contextual é caso legítimo e determinístico (§3.10) |
| 52 | absorvido pelo **111** |
| 74, 75, 76 | eram de `soa`, que não entrou em keel (`rationale §1.3`) |
| 96 | removido: `coseq` e `copar` produzem um valor e a execução segue — não são terminais (§4.8) |
| 100, 102 | removidos com `restrict` sobre contêiner (§4.2): o lowering por hoisting não era honrado pelo compilador C e o caminho misto era UB. O **101** sobrevive com outro sentido, recusando a grafia |

---

## 7. Conformidade

Três documentos descrevem keel, e cada um obriga sobre uma coisa:

| Documento | Do que é dono |
| --- | --- |
| **este** | o que depende só do fonte: o que é aceito, o que é recusado, e o que o gerado significa |
| `keel-c-backend.md` | o que depende do alvo: a grafia dos símbolos, o layout dos tipos gerados, a forma exata da saída |
| `cgen-tool-spec.md` | o que depende do build: resolução de caminho, depfiles, flags, formato das mensagens |

### 7.1 Implementação conforme

> Uma implementação é **conforme** quando, para todo programa keel:
>
> 1. aceita exatamente o que este documento aceita e recusa exatamente o que ele
>    recusa, emitindo os diagnósticos de severidade `error` da §6
>    nos casos listados, e nenhum `error` fora deles;
> 2. o C que ela produz tem o sentido especificado pelo par `keel`/`c` de cada
>    seção — não necessariamente a grafia, que é do backend;
> 3. o C que ela produz é estritamente conforme ao **perfil de geração** em vigor —
>    C11 ou C23 (`backend §9`) —, sem extensão de compilador, salvo onde uma seção
>    diz o contrário e nomeia a guarda. **São dois casos**, e os dois estão
>    escritos:
>    o `#pragma omp` do `parallel` (§4.7), em que diretiva desconhecida é ignorada
>    e o programa sem OpenMP continua correto; e o **respaldo de tipo-caractere da
>    arena** (§4.4), em que `from_array`, `from_stack` e `from_parent` entregam
>    objetos sobre armazenamento de tipo declarado. O segundo não tem alternativa
>    conforme em C que não seja `malloc` — que é `from_memory` —, e **honrá-lo é do
>    backend** (`backend §5.4.1`), não desta camada.

**O perfil não muda o que a linguagem aceita.** Os dois recusam o mesmo conjunto e
aceitam o mesmo conjunto; o que varia é a grafia do gerado e **uma** obrigação de
diagnóstico, listada no §7.3. Uma implementação que aceitasse mais sob um perfil
do que sob o outro não é conforme em nenhum dos dois.

A cláusula 1 tem duas metades, e a segunda costuma faltar: **recusar mais do que a
spec recusa também é não-conformidade.**

| Severidade | Obrigação |
| --- | --- |
| `error` | detectar e recusar; nada é escrito |
| `warning`, `info` | detectar e ter como reportar. Se sai por omissão, sob flag ou agrupado, é da ferramenta |
| `debug` | obrigatório em build de debug, **proibido** em release — não é diagnóstico de tradução, é verificação em execução |

### 7.2 O que a implementação não pode fazer

As duas proibições são a invariante do §1.3 escrita como obrigação:

- **Não abrir header C.** Nenhum `typedef`, macro ou declaração vinda de
  `import_c`, de `#include` ou de `extern_c` participa de decisão da tradução.
- **Não tipar expressão do C.** Fora da posição de contêiner (§3.4), toda
  expressão é `<opaco>` — reconhecem-se as ilhas de keel dentro dela, e nada mais.

### 7.3 Comportamento definido pela implementação

Varia entre implementações conformes; programa que dependa disso não é portável
entre elas. **A lista é fechada:**

| | Onde |
| --- | --- |
| a grafia dos símbolos gerados e o layout dos tipos gerados | `backend §2`, `backend §4.5` |
| o teto de comprimento de nome do alvo, e o error 14 | `backend §2.4` |
| a disponibilidade de `f16` e `bf16`, e o error 103 | §4.2, `backend §3.2` |
| a disponibilidade do OpenMP, e o diagnóstico 108 | §4.7 |
| o alinhamento de plataforma da base de uma arena | §4.4 |
| a ordem entre faixas de um `parallel` | §4.7 |
| a numeração dos temporários gerados (`keel__c0`, `keel__st0`, …) | `backend §2.3` |
| o perfil de geração, e a **única** obrigação que ele muda: sob C11 um `constexpr` cujo inicializador **não é representável no tipo** deixa de ser recusado. A incompatibilidade de tipo continua sendo, e a constância do inicializador também (`backend §9.2`) | `backend §9` |

### 7.4 Comportamento indefinido

> **keel não acrescenta nenhum comportamento indefinido ao C.**

Todo comportamento indefinido de um programa keel é do C gerado. As três
superfícies que as construções abrem estão listadas porque cada uma é uma
promessa do programador, e nenhuma é verificável em release:

| Superfície | A promessa | Onde |
| --- | --- | --- |
| ponteiro de `arena.alloc` entregue a `free`/`realloc` | não acontece | §4.4 |
| uso de arena filha depois de `reset` do pai, fora do escopo | não acontece | §4.4 |

As duas têm diagnóstico onde são decidíveis por varredura — 59 — e não têm
onde não são. **Fora delas, toda regra de keel é decidida na tradução.**

### 7.5 Extensões

> **Uma implementação conforme não define palavra contextual nova, verbo novo nem
> construção nova.**

Toda palavra de keel vale por posição (§3.5), e acrescentar uma muda o sentido de
programas já escritos que usem aquele identificador naquela posição.

O que ela **pode** acrescentar:

- diagnósticos `warning` e `info` além dos da §6, desde que nenhum programa
  aceito passe a ser recusado;
- flags, formatos de saída e artefatos de build, que são da ferramenta;
- módulos de biblioteca, que não pedem permissão nenhuma.

**A extensão certa de keel é um módulo.**

### 7.6 Programa conforme

> Um programa é **conforme** quando não recebe nenhum diagnóstico de severidade
> `error` deste documento, e quando o C gerado a partir dele é aceito por um
> compilador C23 conforme.

A segunda metade não é redundante: é o princípio 3 na letra. keel não
reimplementa o sistema de tipos do C, então há programa que keel aceita e o
compilador C recusa — com os nomes que o usuário escreveu e o `#line` apontando
para o `.k`.

**Um programa conforme continua conforme se um `import` for acrescentado sem
`types`** (§4.1). É a única garantia de estabilidade que este documento dá sobre
edição de fonte.
