# keel

**Um pré-processador de C com sintaxe própria.** keel reconhece suas construções
no meio do seu código C, traduz o que reconhece e copia o resto intacto. O que
sai é C legível, que o seu compilador compila.

```keel
module reading;
import_c <stdio.h>;

pub int first(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return EOF;
    defer fclose(file);
    return fgetc(file);
}
```

```c
int reading_first(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return EOF;
    int result = fgetc(file);
    fclose(file);
    return result;
}
```

`FILE`, `fopen`, `fgetc` e `EOF` keel não conhece, e não precisa: ela reconheceu
`module`, `pub` e `defer`, e o resto atravessou. É essa a ideia inteira.

> **Estado: a linguagem está fechada, o compilador não existe.** Os quatro
> documentos normativos estão completos e sem questão em aberto, a biblioteca
> base está escrita em keel, e há 21 casos de teste com a saída C esperada nos
> dois perfis. O `cgen` — a ferramenta que faria essa tradução — está em fase de
> desenho. Hoje o repositório é uma especificação com um oráculo executável, não
> um compilador que você possa rodar.

---

## Por que existir

C não tem como expressar certas coisas sem macro. keel dá sintaxe a elas, com
uma regra que não abre exceção: **toda construção tem uma tradução para C
escrita e legível**, e o que a tradução custa está documentado. Você nunca
precisa adivinhar o que foi gerado.

O critério de admissão é estreito de propósito. Economizar tokens não basta —
uma construção só entra se expressar uma operação ou uma garantia que C não
oferece, ou oferece de um jeito ruim.

**O que keel não faz:** não analisa suas expressões C, não resolve tipos de
headers, não deduz o tipo de uma chamada que não conhece. Quem valida tipo é o
compilador C, sempre. keel verifica apenas as regras das próprias construções —
e quando não consegue decidir, ela recusa em vez de adivinhar.

## O modelo: ilhas num mar de C

Seu arquivo `.k` é C. Dentro dele há ilhas que keel reconhece — pelas palavras
que ocupam certas posições, e pelos símbolos que ela mesma registrou. O mar em
volta é copiado byte a byte.

Isso tem consequências que vale saber desde o começo:

- **O dialeto do seu C é assunto seu.** Use `nullptr`, `typeof`, o que quiser:
  keel não olha. Se você gerar sob o perfil C11 um módulo que usa palavra do
  C23, quem reclama é o compilador C, com a mensagem dele.
- **Macros não participam.** keel roda antes do pré-processador C. O que ela
  precisa reconhecer tem que estar escrito, não vir de expansão.
- **Comentários e literais são opacos.** `/* grade[1,2] */` é comentário, não
  indexação.

## O que a linguagem tem

### Módulos

Um arquivo, um módulo, e o nome do módulo é o caminho do arquivo. Os símbolos
saem prefixados — `pub int first` vira `reading_first`. `import` traz
outro módulo keel; `import_c` entrega um header ao compilador C; `extern_c`
delimita C que atravessa sem nenhum tratamento.

### Modificadores

Um modificador atua sobre o tipo que o segue e define sua representação e suas
operações:

```keel
buffer i32 data;              // sequência que cresce, com cap e len
slice const char text;        // vista de extensão fixa, não possui nada
outcome i32 r;                // um i32 mais um código de resultado
tagged Cycle void state;      // uma etiqueta de um conjunto declarado
```

`buffer T` e `slice T` são o par memória/visão: um lado possui o armazenamento
e pode crescer, o outro descreve um trecho e não possui nada. A conversão só
faz sentido num sentido, e a linguagem impõe essa direção.

### Cleanup léxico

`defer` registra trabalho para a saída do escopo, em ordem inversa. Não há
runtime: o backend varre os pontos de saída e injeta o corpo em cada um.

### Travessia

```keel
foreach (i32 x, size_t i : xs) { ... }           // índice
walk    (i32 *p, buffer.cursor c : xs) { ... }   // cursor
apply   (i32, xs, doubler);                      // função sobre cada elemento
```

### Execução particionada

```keel
parallel search ANY (size_t w : 0..4; slice i32 part : xs; (target, where)) {
    foreach (i32 x, size_t i : part) {
        if (parallel.interrupted(search)) break;
        if (x == target) { set(where, w, i + 1); win; }
    }
}
return parallel.ok(search);
```

O bloco divide o contêiner entre workers e declara um símbolo de controle que
você consulta depois. Quem divide é o verbo `partition` do próprio contêiner,
não uma fórmula embutida — então funciona sobre qualquer tipo que o declare. O
mecanismo de execução (série ou OpenMP) é escolha da invocação, e não muda a
semântica.

### Resultado e estado cooperativo

`outcome T` é resultado final, de dois estados. `corot` é estado cooperativo, de
três — sucesso, andamento, falha. A cauda `else` trata a falha no ponto da
declaração:

```keel
outcome slice i32 copy = slice.clone(a, source) else return -1;
```

### Conjuntos de tags

`tags` declara um conjunto fechado; `match` despacha sobre ele com
exaustividade verificada:

```keel
tags Status [SUCCESS = -1, ONGOING = 0, FAILED = 1];
```

## Protocolos: como o seu tipo entra nas construções

Isto é o que mais distingue keel de um gerador de código com tipos
privilegiados. Uma construção do núcleo não conhece uma lista de tipos
abençoados — ela procura **verbos, por nome e aridade**. Declare os verbos e o
seu módulo participa.

| Para usar | Declare |
| --- | --- |
| `foreach` de dois binders, `apply`, `x[i]` | `length`, e `get` ou `ptr` |
| `foreach` de um binder | `first`, `limit` |
| `walk` | `begin`, `has_next`, `next` |
| `parallel` | `partition` |
| `x[a..b]` | o verbo de `range-index` do seu módulo, na aridade escrita — o nome é seu |
| `match` | `tag` |
| `else` | `failed`, e `win` para a forma com valor padrão |

Não há registro a fazer, marcação a escrever nem permissão a pedir. keel não
distingue um módulo da base de um módulo seu ao resolver.

## A base

Nove módulos, escritos em keel, com uma exceção de licença para que o C gerado
do seu programa não seja arrastado ao copyleft (veja abaixo). Todos exigem
import explícito — o único implícito é `import keel types;`, que traz só os
nomes de tipo primitivos.

| Módulo | O que traz |
| --- | --- |
| `keel.arena` | armazenamento por região, com tempo de vida explícito |
| `keel.buffer`, `keel.slice`, `keel.range` | as sequências, a vista e o intervalo |
| `keel.outcome`, `keel.corot` | resultado final e estado cooperativo |
| `keel.tagged` | valor com etiqueta |
| `keel.routine` | composição cooperativa, `seq` e `par` |
| `keel.parallel` | o símbolo de controle de um bloco `parallel` |

Os tipos primitivos são `i8`…`i64`, `u8`…`u64`, `f16`, `f32`, `f64` e `bf16`,
com largura e representação fixadas — não aliases de `int`.

## Como se compilaria

`cgen` é um driver no molde do `gcc`: transpila e chama o compilador C sobre o
que gerou. Trocar `gcc` por `cgen` num Makefile existente é a forma pretendida
de uso.

```sh
cgen -I src --profile=c23 src/app/main.k -o app
```

As opções próprias são poucas e fechadas; tudo o mais atravessa verbatim para o
compilador C. `-o` é dele, não do cgen.

## keel por exemplos

Os pares mostram a operação essencial da tradução. O C usa representações locais dos tipos necessários para tornar os exemplos legíveis; a organização em headers, os nomes internos exatos, o mapeamento de linhas e as verificações de debug pertencem ao backend. Nos exemplos cooperativos, o código `1` representa uma falha sem estabelecer a codificação detalhada de diagnóstico por participante.

Os exemplos 1 a 4 e 7 são módulos independentes. Os exemplos 5 e 6 compõem o mesmo módulo `coop`: as funções auxiliares são apresentadas uma vez no exemplo 5. Nenhum import da base, além de `import keel types;`, é implícito.

### 1. Módulo completo e chamada C

`import_c` disponibiliza um header ao compilador C. O marcador `array` registra o vetor; a chamada `puts` permanece C.

```keel
//keel
module ola;
import_c <stdio.h>;

int main(void) {
    array char message[] = "Ola, keel!";
    puts(message);
    return 0;
}
```

```c
//C gerado
#include <stdio.h>

int ola_main(void) {
    char message[] = "Ola, keel!";
    puts(message);
    return 0;
}

/* Entry wrapper, emitted by the build for the selected module. */
int main(void) { return ola_main(); }
```

`ola_main` é a função do módulo; o wrapper `main` fornece a entrada do programa. O header é processado pelo compilador C; keel não precisa conhecer a assinatura de `puts` para preservar a chamada.

Contratos na [spec](keel-spec.md): §§1.2, 4.1 e 4.2. [Justificativa: fronteira com C](keel-rationale.md#fronteira-com-c-e-conflitos-léxicos).

### 2. Retorno e cleanup

A função devolve o primeiro caractere lido e fecha o arquivo antes de retornar. Se a abertura falhar, o registro de `defer` não é alcançado.

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

```c
//C gerado
#include <stdio.h>

int reading_first(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return EOF;
    int result = fgetc(file);
    fclose(file);
    return result;
}
```

A expressão de retorno é avaliada antes da limpeza. `FILE`, `fopen`, `fgetc`, `fclose` e `EOF` são tratados pelo compilador C.

Contratos na [spec](keel-spec.md): §§4.1 e 4.6. [Justificativa: limite de análise](keel-rationale.md#fronteira-com-c-e-conflitos-léxicos).

### 3. Trecho fixo, elementos mutáveis e intervalo

Um `slice` de dois elementos aponta para parte de um vetor. Alterar seus elementos por índice ou por ponteiro modifica o mesmo armazenamento. O trecho não cresce.

```keel
//keel
module span;
import keel.slice as slice types;
import keel.range as range types;

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

```c
//C gerado
#include <stddef.h>
#include <stdint.h>

typedef int16_t i16;
typedef struct { size_t len; i16 *ptr; } keel_slice_i16;
typedef struct { size_t first, limit; } keel_range;

i16 span_sum(void) {
    i16 dados[3] = {1, 2, 3};
    keel_slice_i16 part = {2, dados + 1};
    part.ptr[0] = 20;
    *(part.ptr + 1) = 30;

    keel_range indices = {0, 2};
    i16 total = 0;
    for (size_t i = indices.first; i < indices.limit; ++i) {
        total += part.ptr[i];
    }
    return total;
}
```

O resultado é `50`; `dados` termina com `{1, 20, 30}`. O intervalo é semiaberto: inclui zero e um, exclui dois. O armazenamento permanece válido durante toda a utilização do trecho.

Contratos na [spec](keel-spec.md): §§5.2 e 5.3, e §§4.5 e 4.7. [Justificativa: memória por região](keel-rationale.md#memória-por-região).

### 4. Resultado com default e extração explícita

A função aceita um valor não negativo e ajusta um resultado explicitamente declarado. Um código negativo representa falha em `outcome`; o default ajusta o próprio resultado para sucesso com valor zero.

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

```c
//C gerado
#include <stdint.h>

typedef int16_t i16;
typedef struct { int32_t code; i16 value; } keel_outcome_i16;

keel_outcome_i16 result_accept(i16 entry) {
    keel_outcome_i16 r = {0};
    if (entry < 0) { r.code = entry; return r; }
    r.code = 0;
    r.value = entry;
    return r;
}

i16 result_get_value(i16 entry) {
    keel_outcome_i16 res = result_accept(entry);
    if (res.code != 0) {
        /* outcome.win(res, 0): adjusts res itself. */
        res.code = 0;
        res.value = 0;
    }
    return res.value;
}
```

`get_value(7)` devolve `7`; `get_value(-7)` devolve `0`. A declaração de `res` continua tendo tipo `outcome i16`; a extração para `i16` só ocorre na chamada explícita a `outcome.value`.

Contratos na [spec](keel-spec.md): §§4.10 e 5.5. [Justificativa: resultados finais](keel-rationale.md#resultados-finais-e-estados-cooperativos).

### 5. `match`: o laço pertence à função

A função `note`, declarada abaixo, é usada também no exemplo seguinte. `count` mantém seu progresso no contador passado pelo programa. Cada chamada retorna normalmente: `corot.again` entrega `ONGOING`; `corot.win` entrega `SUCCESS`; `corot.fault(r, 1)` entrega `FAILED`. O retorno é `corot`, que carrega o estado e o código, e nenhum valor: o que a passagem produz fica no contador que o programa passou. `note` é uma função comum de retorno `void`.

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

```c
//C gerado
#include <stdbool.h>
#include <stdint.h>

typedef int16_t i16;
typedef struct { int32_t code; } keel_corot;
typedef struct { int32_t code; i16 value; } keel_outcome_i16;

typedef enum { coop_WAIT, coop_END } coop_Cycle;
typedef struct { int32_t tag; } keel_tagged_coop_Cycle_void;

static keel_corot coop_count(i16 *counter,
                                         i16 limit, bool failure) {
    keel_corot r = {0};
    ++*counter;
    if (*counter < limit) { r.code =  0; return r; }
    if (failure)              { r.code =  1; return r; }
    r.code = -1; return r;
}

static void coop_note(i16 *counter) {
    ++*counter;
}

i16 coop_manual(void) {
    keel_tagged_coop_Cycle_void state = {0};
    state.tag = coop_WAIT;
    i16 calls = 0;
    bool finished = false;

    while (!finished) {
        switch (state.tag) {
        case coop_WAIT: goto keel__m0_WAIT;
        case coop_END:    goto keel__m0_END;
        default:                goto keel__m0_end;
        }
        keel__m0_WAIT: {
            keel_corot r = coop_count(&calls, 2, false);
            if (r.code > 0 || r.code < 0) state.tag = coop_END;
        }
        goto keel__m0_end;
        keel__m0_END: {
            finished = true;
        }
        keel__m0_end: ;
    }
    return calls;
}
```

`manual()` devolve `2`. A mudança para `END` só é observada no próximo despacho do laço escrito pelo usuário; não há passagem automática de um braço ao seguinte. O teste `corot.faulted(r) || corot.ok(r)` significa que a chamada terminou, com falha ou sucesso; não é o predicado de falha de `outcome`.

O C usa `switch` somente para escolher um rótulo; os corpos dos braços ficam fora dele. Isso preserva a associação de controles escritos pelo usuário aos seus próprios laços. `match` não produz o valor retornado por `manual`: esse retorno é um statement da função.

A variável de estado é um `tagged Cycle void`: um valor etiquetado sem valor associado. É a mesma construção usada para despachar um `corot`, que é um valor etiquetado com três tags e valor associado.

Contratos na [spec](keel-spec.md): §§4.9 e 5.4. [Justificativa: controle e máquina completa](keel-rationale.md#estrutura-de-controle-e-máquina-completa).

### 6. `routine.par`: tabela de slots e política

Esta função continua o módulo `coop` do exemplo 5, acrescentando os imports de
`keel.routine` e `keel.slice`. As participantes são descritas em um vetor de
`routine.slot`, montado pelo programa. `routine.par` percorre essa fatia em
ciclos e devolve um `outcome u32` cujo valor associado é a quantidade de
sucessos. O estado de cada slot fica no próprio slot e é lido depois.

```keel
//keel
import keel.routine as routine types;
import keel.slice   as slice   types;

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

```c
//C gerado
typedef keel_corot (*keel_routine_i16)(i16 *);

typedef struct {
    keel_routine_i16 f;
    i16             *ctx;
    keel_corot  state;
} keel_routine_slot_i16;

typedef struct {
    size_t len;
    keel_routine_slot_i16 *ptr;
} keel_slice_keel_routine_slot_i16;

typedef struct { int32_t code; uint32_t value; } keel_outcome_u32;

static keel_corot coop_stage(i16 *counter) {
    keel_corot r = {0};
    ++*counter;
    if (*counter < 2) { r.code = 0; return r; }
    r.code = -1; return r;
}

static inline keel_outcome_u32
keel_routine_par_i16(keel_slice_keel_routine_slot_i16 s, uint32_t target) {
    uint32_t m = (uint32_t)s.len, S = 0, F = 0;
    uint32_t q = target ? target : m;
    for (size_t i = 0; i < s.len; ++i) s.ptr[i].state.code = 0;
    for (;;) {
        for (size_t i = 0; i < s.len; ++i) {
            if (s.ptr[i].state.code != 0) continue;
            keel_corot r = s.ptr[i].f(s.ptr[i].ctx);
            s.ptr[i].state = r;
            if (r.code < 0) ++S; else if (r.code > 0) ++F;
        }
        if (S >= q)     return (keel_outcome_u32){ 0, S };
        if (m - F < q)  return (keel_outcome_u32){ 1, S };
    }
}

keel_outcome_u32 coop_set_of(i16 *a, i16 *b) {
    keel_routine_slot_i16 steps[2] = {
        { coop_stage, a, {0} },
        { coop_stage, b, {0} },
    };
    keel_slice_keel_routine_slot_i16 s = { 2, steps };
    keel_outcome_u32 r = keel_routine_par_i16(s, 1);

    for (size_t i = 0; i < 2u; ++i) {
        keel_routine_slot_i16 *sl = &steps[i];
        if (sl->state.code < 0) coop_note(a);
    }
    return r;
}
```

Com `*a` e `*b` em zero, o alvo `1` é alcançado no **fim** do segundo ciclo, e
não na primeira vitória: as duas entradas são chamadas nesse ciclo e as duas
terminam em `SUCCESS`. O resultado é válido, com valor `2`; a travessia
seguinte chama `note` duas vezes, e os contadores terminam `*a == 4` e
`*b == 2`.

O estado é lido pelos verbos de `corot`, e não pelo campo: `routine.state(s)`
devolve um `corot`, sobre o qual valem `corot.ok`, `corot.ongoing`,
`corot.faulted`, `corot.code` e também `match`, pelo contrato da spec §4.9. O
inicializador sem `state` deixa o campo zerado, que em `corot` é `ONGOING` — o
estado inicial correto, sem escrita.

A composição é uma chamada de função da base, não uma construção do núcleo.
Não há região de finalização com fronteira léxica: o que antes seria escrito
depois de um rótulo de saída é simplesmente o statement seguinte à declaração
de `r`, sujeito às regras comuns de `return`, `goto` e `defer`.

A tabela é escrita pelo programa e pode ser estática ou montada em runtime;
`slice.of` fornece a fatia em qualquer dos casos, e exige o marcador `array`
sobre o vetor. Como `routine.par` escreve o estado de cada entrada, a tabela não
pode ser `const` e um mesmo vetor não deve alimentar duas execuções concorrentes
da composição.

Contrato na [spec](keel-spec.md): §5.6.

### 7. Partição e travessia por cursor

Cada worker recebe uma parte do contêiner e a percorre por cursor, escrevendo
seu resultado numa posição própria. Nada é compartilhado entre workers além do
contêiner de saída, e cada um escreve num índice diferente.

```keel
//keel
module scan;
import keel.buffer   as buffer   types;
import keel.slice    as slice    types;
import keel.parallel as parallel;

pub void sum(buffer i32 *xs, buffer u32 *totals) {
    parallel sum ALL (size_t w : 0..4; slice i32 part : xs; (totals)) {
        u32 t = 0;
        walk (i32 *p, slice.cursor c : part) {
            t += (u32)*p;
        }
        totals[w] = t;
    }
}
```

```c
//C gerado
void scan_sum(keel_buffer_i32 *xs, keel_buffer_u32 *totals) {
    keel_parallel_control sum = { .workers = 4, .target = 0 };
    {   keel_buffer_i32 *keel__c0 = xs;

        #pragma omp parallel for num_threads(4) default(none) \
                shared(keel__c0, sum, totals)
        for (size_t w = 0; w < 4; w++) {
            keel_slice_i32 part = keel_buffer_i32_partition(keel__c0, 4, w);
            u32 t = 0;
            keel_slice_cursor c = keel_slice_i32_begin(&part);
            while (keel_slice_i32_has_next(&part, &c)) {
                i32 *p = keel_slice_i32_next(&part, &c);
                t += (u32)*p;
            }
            *keel_buffer_u32_ptr(totals, w) = t;
            atomic_fetch_add_explicit(&sum.wins, 1, memory_order_relaxed);
            keel__end0: ;
        }
    }
}
```

Três coisas que o par mostra, e que os contratos detalham:

- **`parallel` não percorre.** Ele chama `partition` uma vez por worker e liga a
  parte ao binder; percorrer é escolha do corpo, que aqui usa `walk` e poderia
  usar `foreach` ou um `for` escrito à mão (spec §4.8).
- **`walk` não pede comprimento.** Pede `begin`, `has_next` e `next`, e o cursor
  é escrito com seu tipo — `slice.cursor`, do módulo e não da instância, porque
  guarda uma posição e não depende de `T` (spec §§4.7 e 5.3).
- **A saída por worker vai num contêiner indexado por `w`.** Não vai numa
  captura: captura escalar é cópia por worker, e escrever nela é o error
  `captured-write`. `totals` entra por ponteiro porque é instância `byref`, e
  cada worker escreve num índice diferente — a disjunção é do programa, não da
  construção.

Sob `ALL` não há interrupção, e o fim natural de um worker não emite veredito:
depois do bloco, `parallel.ok(soma)` é verdadeiro porque nenhum worker escreveu
`fail`.
A busca que para cedo é a outra política, e usa `win` mais
`parallel.interrupted` (spec §§4.8 e 5.7).

Contratos na [spec](keel-spec.md): §§4.7, 4.8, 5.3 e 5.7. [Justificativa: particionável e percorrível](keel-rationale.md#particionável-e-percorrível).

Os pares são ilustração; os contratos estão na [spec](keel-spec.md).

## O repositório

| Caminho | O que é |
| --- | --- |
| [`keel-spec.md`](keel-spec.md) | a linguagem: o que depende só do fonte |
| [`keel-rationale.md`](keel-rationale.md) | as razões; nada ali é regra |
| [`keel-c-backend.md`](keel-c-backend.md) | o que depende do alvo: nomes, artefatos, perfis |
| [`cgen-tool-spec.md`](cgen-tool-spec.md) | o que depende da invocação |
| [`base/`](base/) | os dez módulos da base, em keel |
| [`golden/`](golden/) | 22 casos com a saída C esperada, nos dois perfis |
| [`design/`](design/) | desenho da implementação, e as ideias que não são v0 |

A divisão entre os quatro documentos é normativa e usada como critério: quando
uma decisão trava, a primeira pergunta é em qual deles a regra mora.

Começando a ler: [keel por exemplos](#keel-por-exemplos), acima,
mostra a linguagem em pares keel/C, e é o caminho mais curto para ver se ela faz
sentido para você.

## Licenças

São duas, e a separação é deliberada.

- **A base e o `cgen`**: GPLv3 **com uma exceção** ([`LICENSE.md`](LICENSE.md),
  [nota em português](LICENSE.pt.md)). O `cgen` copia trechos de fonte da base
  para dentro do C gerado de todo projeto — não é vínculo de biblioteca, é fonte
  colado. A exceção isenta esse texto gerado, para que o seu programa não seja
  arrastado ao copyleft por transitividade de transpilação.
- **A documentação**: CC BY-SA 4.0 ([`LICENSE-DOCS.md`](LICENSE-DOCS.md),
  [nota](LICENSE-DOCS.pt.md)).

© 2026 Faculdade de Engenharia Elétrica e de Computação, Universidade Federal do
Amazonas (FEEC/UFAM). Escrita e revisão tiveram auxílio de Claude (Anthropic),
sob direção humana.
