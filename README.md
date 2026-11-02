# keel

**Um pré-processador de C com sintaxe própria.** keel reconhece suas construções
no meio do seu código C, traduz o que reconhece e copia o resto intacto. O que
sai é C legível, que o seu compilador compila.

<!-- golden: cases/027-readme/reading.k:1-9 -->
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

<!-- golden: cases/027-readme/expected/c23/reading.c:7-15 -->
```c
int reading_first(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return EOF;

    { int keel__rv0 = fgetc(file);
#line 7 "reading.k"
    fclose(file);
    return keel__rv0; }
}
```

`FILE`, `fopen`, `fgetc` e `EOF` keel não conhece, e não precisa: ela reconheceu
`module`, `pub` e `defer`, e o resto atravessou. É essa a ideia inteira.

> **Estado: a linguagem está fechada, e o compilador emite C para quase toda
> ela.** Os quatro documentos normativos estão completos, a biblioteca base está
> escrita em keel, e há 57 casos de teste com a saída C esperada nos dois perfis
> (três em construção). O `cgen` lê um `.k`, resolve os imports, reconhece toda a
> gramática, verifica as regras de keel, emite C (`--stop-after=gen`) e chama o
> compilador C sobre ele: **54 dos 57 casos saem byte a byte e o C compila**, nos dois perfis, e a
> base inteira sai dele. Ficam fora os três casos em construção.

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
```

### Execução particionada

```keel
parallel search ANY (size_t w : 0..4; slice i32 part : xs; (target, where)) {
    foreach (i32 x, size_t i : part) {
        if (parallel.interrupted(search)) break;
        if (x == target) { buffer.set(where, w, i + 1); win; }
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
| `foreach` de dois binders, `x[i]` | `length`, e `get` ou `ptr` |
| `foreach` de um binder | `first`, `limit` |
| `walk` | `begin`, `has_next`, `next` |
| `parallel` | `partition` |
| `x[a..b]` | `length` e `as_slice(x, a, b)` |
| `match` | `tag` |
| `else` | `failed`, e `win` para a forma com valor padrão |

Não há registro a fazer, marcação a escrever nem permissão a pedir. keel não
distingue um módulo da base de um módulo seu ao resolver.

Os contratos têm nome, no prelúdio `keel`: `IndexPtr`, `IndexGet`, `Countable`,
`Traversable`, `Partitionable`, `Sliceable`, `Taggable`, `Failable` e
`Winnable`. Sem import nenhum, um nome desses serve de tipo de parâmetro, como
uma interface: `size_t count(IndexPtr xs)` ganha uma instância por tipo
concreto, resolvida na tradução.

## A base

Dez módulos, escritos em keel, com uma exceção de licença para que o C gerado
do seu programa não seja arrastado ao copyleft (veja abaixo). Todos exigem
import explícito — o único implícito é `import keel types;`, que traz os nomes
de tipo primitivos e os protocolos.

| Módulo | O que traz |
| --- | --- |
| `keel.arena` | armazenamento por região, com tempo de vida explícito |
| `keel.buffer`, `keel.slice`, `keel.range` | as sequências, a vista e o intervalo |
| `keel.array` | o acesso por chamada a um `array`, e o seu `x[a..b]` |
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
compilador C. `-o` tem o significado do gcc: nomeia o arquivo da última fase. É
do compilador C quando ele roda, e do cgen quando a invocação para antes, como
em `cgen -o app.parse --stop-after=parse app.k`.

## keel por exemplos

Os pares mostram a operação essencial da tradução. Um bloco precedido de `<!-- golden: caminho:linhas -->` é um trecho literal de um arquivo de [`golden/`](golden/), conferido por `test/docs.sh`: o que está ali é o que o cgen gera, sem o cabeçalho e os `#include` do arquivo. Os `#line` que aparecem dentro de um trecho são os do arquivo gerado: eles ligam o C à linha do `.k`, e a regra é do backend (§6). Nos exemplos cooperativos, o código `1` representa uma falha sem estabelecer a codificação detalhada de diagnóstico por participante.

Os exemplos 1 a 4 e 7 são módulos independentes. Os exemplos 5 e 6 compõem o mesmo módulo `coop`: as funções auxiliares são apresentadas uma vez no exemplo 5. Nenhum import da base, além de `import keel types;`, é implícito.

### 1. Módulo completo e chamada C

`import_c` disponibiliza um header ao compilador C. O marcador `array` registra o vetor; a chamada `puts` permanece C.

<!-- golden: cases/028-readme-main/ola.k:1-8 -->
```keel
//keel
module ola;
import_c <stdio.h>;

pub int main(int argc, char **argv) {
    array char message[] = "Ola, keel!";
    puts(message);
    return 0;
}
```

<!-- golden: cases/028-readme-main/expected/c23/ola.c:7-11 -->
```c
//C gerado
int ola_main(int argc, char **argv) {
    char message[] = "Ola, keel!";
    puts(message);
    return 0;
}
```

<!-- golden: cases/028-readme-main/expected/c23/main_ola.c:3-3 -->
```c
//C gerado
int main(int argc, char **argv) { return ola_main(argc, argv); }
```

`ola_main` é a função do módulo; o wrapper `main`, numa unidade à parte (`main_ola.c`, gerada por `--main ola`), fornece a entrada do programa; ele repassa `argc` e `argv`, por isso `main` de módulo os recebe. O header é processado pelo compilador C; keel não precisa conhecer a assinatura de `puts` para preservar a chamada.

Contratos na [spec](keel-spec.md): §§1.2, 4.1 e 4.2. [Justificativa: fronteira com C](keel-rationale.md#fronteira-com-c-e-conflitos-léxicos).

### 2. Retorno e cleanup

A função devolve o primeiro caractere lido e fecha o arquivo antes de retornar. Se a abertura falhar, o registro de `defer` não é alcançado.

<!-- golden: cases/027-readme/reading.k:1-9 -->
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

<!-- golden: cases/027-readme/expected/c23/reading.c:7-15 -->
```c
//C gerado
int reading_first(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return EOF;

    { int keel__rv0 = fgetc(file);
#line 7 "reading.k"
    fclose(file);
    return keel__rv0; }
}
```

A expressão de retorno é avaliada antes da limpeza. `FILE`, `fopen`, `fgetc`, `fclose` e `EOF` são tratados pelo compilador C. O `defer` sai inline, e a expressão vai para o temporário `keel__rv0`; o `#line` leva a linha do `defer` no `.k`.

Contratos na [spec](keel-spec.md): §§4.1 e 4.6. [Justificativa: limite de análise](keel-rationale.md#fronteira-com-c-e-conflitos-léxicos).

### 3. Trecho fixo, elementos mutáveis e intervalo

Um `slice` de dois elementos aponta para parte de um vetor. Alterar seus elementos por índice ou por ponteiro modifica o mesmo armazenamento. O trecho não cresce.

<!-- golden: cases/027-readme/span.k:1-18 -->
```keel
//keel
module span;
import keel.slice as slice types;
import keel.range as range types;
import keel.array as array;

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

<!-- golden: cases/027-readme/expected/c23/span.c:12-24 -->
```c
//C gerado
i16 span_sum(void) {
    i16 dados[3] = {1, 2, 3};
    keel_slice_i16 part = keel_slice_of2_keel_array_i16(dados, 3, 1, 3);
    *keel_slice_i16_ptr1(part, 0) = 20;
    *(keel_slice_i16_ptr(part) + 1) = 30;

    keel_range indices = keel_range_of(0, 2);
    i16 total = 0;
    { size_t keel__f0 = keel_range_first(indices); size_t keel__l0 = keel_range_limit(indices); for (size_t i = keel__f0; i < keel__l0; i++) {
        total += *keel_slice_i16_ptr1(part, i);
    } }
    return total;
}
```

Quatro coisas aparecem no C. `slice.of(dados, 1, 3)` vira a função da instância, `keel_slice_of2_keel_array_i16`, que recebe o vetor e o comprimento (`3`, vindo da dimensão declarada); `part[0]` é `*keel_slice_i16_ptr1(part, 0)`; o literal `0..2` fora de uma travessia é `keel_range_of(0, 2)`; e o `foreach` sobre o `range` conta de `first` a `limit`, com um bloco em volta que fecha no `}` do corpo.

O resultado é `50`; `dados` termina com `{1, 20, 30}`. O intervalo é semiaberto: inclui zero e um, exclui dois. O armazenamento permanece válido durante toda a utilização do trecho.

Contratos na [spec](keel-spec.md): §§5.2 e 5.3, e §§4.5 e 4.7. [Justificativa: memória por região](keel-rationale.md#memória-por-região).

### 4. Resultado com default e extração explícita

A função aceita um valor não negativo e ajusta um resultado explicitamente declarado. Um código negativo representa falha em `outcome`; o default ajusta o próprio resultado para sucesso com valor zero.

<!-- golden: cases/027-readme/result.k:1-13 -->
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

<!-- golden: cases/027-readme/expected/c23/result.c:8-17 -->
```c
//C gerado
keel_outcome_i16 result_accept(i16 entry) {
    keel_outcome_i16 r = {0};
    if (entry < 0) return keel_outcome_i16_fail(&r, entry);
    return keel_outcome_i16_win1(&r, entry);
}

i16 result_get_value(i16 entry) {
    keel_outcome_i16 res = result_accept(entry); if (keel_outcome_i16_failed(res)) keel_outcome_i16_win1(&res, 0);
    return keel_outcome_i16_value(res);
}
```

A instância `keel_outcome_i16` tem o layout `{ i32 code; i16 value; }` e mora em `keel/keel_outcome_i16.type.h`; os `#include` do arquivo gerado ficam fora do trecho.

`get_value(7)` devolve `7`; `get_value(-7)` devolve `0`. A declaração de `res` continua tendo tipo `outcome i16`; a extração para `i16` só ocorre na chamada explícita a `outcome.value`.

Contratos na [spec](keel-spec.md): §§4.10 e 5.5. [Justificativa: resultados finais](keel-rationale.md#resultados-finais-e-estados-cooperativos).

### 5. `match`: o laço pertence à função

A função `note`, declarada abaixo, é usada também no exemplo seguinte. `count` mantém seu progresso no contador passado pelo programa. Cada chamada retorna normalmente: `corot.again` entrega `ONGOING`; `corot.win` entrega `SUCCESS`; `corot.fault(r, 1)` entrega `FAILED`. O retorno é `corot`, que carrega o estado e o código, e nenhum valor: o que a passagem produz fica no contador que o programa passou. `note` é uma função comum de retorno `void`.

<!-- golden: cases/031-readme-coop/coop.k:1-36 -->
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

<!-- golden: cases/031-readme-coop/expected/c23/coop.c:14-54 -->
```c
//C gerado
static keel_corot coop_count(i16 *counter, i16 limit, bool failure) {
    keel_corot r = {0};
    ++*counter;
    if (*counter < limit) return keel_corot_again(&r);
    if (failure)              return keel_corot_fault(&r, 1);
    return keel_corot_win(&r);
}

static void coop_note(i16 *counter) {
    ++*counter;
}



i16 coop_manual(void) {
    keel_tagged_coop_Cycle_void state = {0};
    keel_tagged_coop_Cycle_void_mark(&state, coop_Cycle_WAIT);
    i16 calls = 0;
    bool finished = false;

    while (!finished) {
        switch (state.tag) {
        case coop_Cycle_WAIT: goto keel__m0_WAIT;
        case coop_Cycle_END:  goto keel__m0_END;
        default:              KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
        }
#line 28 "coop.k"
        keel__m0_WAIT: {
            keel_corot r = coop_count(&calls, 2, false);
            if (keel_corot_faulted(r) || keel_corot_ok(r)) keel_tagged_coop_Cycle_void_mark(&state, coop_Cycle_END);
        }
        goto keel__m0_end;
#line 31 "coop.k"
        keel__m0_END: {
            finished = true;
        }
        keel__m0_end: ;
#line 34 "coop.k"
    }
    return calls;
}
```

A mudança para `END` só é observada no próximo despacho do laço escrito pelo usuário; não há passagem automática de um braço ao seguinte. O valor que a função devolve é definido no contexto, pelo laço e pelos contadores do programa, e a máquina de estados é gerada pelos verbos de `corot`. O teste `corot.faulted(r) || corot.ok(r)` significa que a chamada terminou, com falha ou sucesso; não é o predicado de falha de `outcome`.

O C usa `switch` somente para escolher um rótulo; os corpos dos braços ficam fora dele. Isso preserva a associação de controles escritos pelo usuário aos seus próprios laços. `match` não produz o valor retornado por `manual`: esse retorno é um statement da função.

A variável de estado é um `tagged Cycle void`: um valor etiquetado sem valor associado. É a mesma construção usada para despachar um `corot`, que é um valor etiquetado com três tags e valor associado.

Contratos na [spec](keel-spec.md): §§4.9 e 5.4. [Justificativa: controle e máquina completa](keel-rationale.md#estrutura-de-controle-e-máquina-completa).

### 6. `routine.par`: tabela de slots e política

Esta função continua o módulo `coop` do exemplo 5, acrescentando os imports de
`keel.routine`, `keel.slice` e `keel.array` (`slice.of` pede os verbos de
`keel.array`). As participantes são descritas em um vetor de
`routine.slot`, montado pelo programa. `routine.par` percorre essa fatia em
ciclos e devolve um `outcome u32` cujo valor associado é a quantidade de
sucessos. O estado de cada slot fica no próprio slot e é lido depois.

<!-- golden: cases/031-readme-coop/coop.k:38-61 -->
```keel
//keel
import keel.routine as routine types;
import keel.slice   as slice   types;
import keel.array   as array;

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

<!-- golden: cases/031-readme-coop/expected/c23/coop.c:60-79 -->
```c
//C gerado
static keel_corot coop_stage(i16 *counter) {
    keel_corot r = {0};
    ++*counter;
    if (*counter < 2) return keel_corot_again(&r);
    return keel_corot_win(&r);
}

keel_outcome_u32 coop_set_of(i16 *a, i16 *b) {
    keel_routine_slot_i16 steps[2] = {
        { .f = coop_stage, .ctx = a, .state = {0} },
        { .f = coop_stage, .ctx = b, .state = {0} },
    };

    keel_outcome_u32 r = keel_routine_i16_par(keel_slice_of_keel_array_keel_routine_slot_i16(steps, 2), 1);

    { for (size_t i = 0; i < 2; i++) { keel_routine_slot_i16 *sl = &steps[i];
        if (keel_corot_ok(keel_routine_slot_i16_state(sl))) coop_note(a);
    } }
    return r;
}
```

A instância `routine i16` é a unidade `keel_routine_i16`: o `typedef` da participante e o `slot` saem no `.type.h` dela, e os verbos, no `.h`.

<!-- golden: cases/031-readme-coop/expected/c23/keel/keel_routine_i16.type.h:8-11 -->
```c
//C gerado
#line 18 "keel/routine.k"
typedef keel_corot (*keel_routine_i16)(i16 *ctx);
#line 25 "keel/routine.k"
typedef struct keel_routine_slot_i16 { keel_routine_i16 f; i16 *ctx; keel_corot state; } keel_routine_slot_i16;
```

<!-- golden: cases/031-readme-coop/expected/c23/keel/keel_routine_i16.h:44-61 -->
```c
//C gerado
static inline keel_outcome_u32 keel_routine_i16_par(keel_slice_keel_routine_slot_i16 s, u32 target) {
    keel_outcome_u32 r = {0};
    size_t n = keel_slice_keel_routine_slot_i16_length(s);
    KEEL_CHECK(target <= n, "par-target-above-total");
    u32 m = (u32)n, S = 0, F = 0, q = target ? target : m;
    for (size_t i = 0; i < n; ++i) keel_slice_keel_routine_slot_i16_ptr1(s, i)->state = (keel_corot){0};
    for (;;) {
        for (size_t i = 0; i < n; ++i) {
            keel_routine_slot_i16 *p = keel_slice_keel_routine_slot_i16_ptr1(s, i);
            if (!keel_corot_ongoing(p->state)) continue;
            p->state = p->f(p->ctx);
            if (keel_corot_ok(p->state))           ++S;
            else if (keel_corot_faulted(p->state)) ++F;
        }
        if (S >= q)    return keel_outcome_u32_win1(&r, S);
        if (m - F < q) { keel_outcome_u32_value1(&r, S); return keel_outcome_u32_fail(&r, 1); }
    }
}
```

O alvo `1` é alcançado no **fim** de um ciclo, e não na primeira vitória: o ciclo
chama todas as entradas ainda em `ONGOING`, e a política é julgada depois (spec
§5.6). O valor associado do resultado é a quantidade de entradas que terminaram
em `SUCCESS`. A travessia seguinte, um `foreach` de binder por ponteiro sobre o
`array`, lê o estado de cada slot sem copiar o slot.

O estado é lido pelos verbos de `corot`, e não pelo campo: `routine.state(steps[0])`
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

<!-- golden: cases/030-readme-scan/scan.k:1-14 -->
```keel
//keel
module scan;
import keel.buffer   as buffer   types;
import keel.slice    as slice;
import keel.parallel as parallel;

pub void sum(buffer i32 *xs, buffer u32 *totals) {
    parallel sum ALL (size_t w : 0..4; slice.slice i32 part : xs; (totals)) {
        u32 t = 0;
        walk (i32 *p, slice.cursor c : part) {
            t += (u32)*p;
        }
        totals[w] = t;
    }
}
```

<!-- golden: cases/030-readme-scan/expected/c23/scan.c:14-32 -->
```c
//C gerado
void scan_sum(keel_buffer_i32 *xs, keel_buffer_u32 *totals) {
    keel_parallel_control sum = { .workers = 4, .target = 0 };
    {   keel_buffer_i32 *keel__c0 = xs;

        #pragma omp parallel for num_threads(4) default(none) \
                shared(keel__c0, sum, totals)
        for (size_t w = 0; w < 4; w++) {
            keel_slice_i32 part = keel_buffer_i32_partition(keel__c0, 4, w);
#line 8 "scan.k"
            u32 t = 0;
            {   keel_slice_i32 keel__c1 = part; keel_slice_cursor c = keel_slice_i32_begin(keel__c1); while (keel_slice_i32_has_next(keel__c1, &c)) { i32 *p = keel_slice_i32_next(keel__c1, &c);
                t += (u32)*p;
            } }
            *keel_buffer_u32_ptr1(totals, w) = t;
            keel__end0: ;
        }
    }
#line 14 "scan.k"
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
depois do bloco, `parallel.ok(sum)` é verdadeiro porque nenhum worker escreveu
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
| [`base/`](base/) | os onze módulos da base, em keel: o prelúdio `keel` e os dez de `keel/` |
| [`golden/`](golden/) | 57 casos com a saída C esperada, nos dois perfis; três ainda em construção (`WIP`) |
| [`tools/cgen/`](tools/cgen/) | o transpilador: lê, verifica e emite C (`--stop-after=gen`) |
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
