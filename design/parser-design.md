# parser — desenho do reconhecimento e da tabela de símbolos

> © 2026 Faculdade de Engenharia Elétrica e de Computação, Universidade
> Federal do Amazonas (FEEC/UFAM). Licenciado sob
> [CC BY-SA 4.0](../LICENSE-DOCS.md) ([tradução](../LICENSE-DOCS.pt.md)) —
> plano de implementação, não contrato normativo; o `cgen` e a Base keel têm
> licença própria, ver [`LICENSE.md`](../LICENSE.md). Escrita e revisão
> tiveram auxílio de Claude Opus (Anthropic), sob direção humana.

**Documento de implementação.** Cobre `engine/parser.c` e `engine/symtab.c` da
[`cgen-tool.md`](cgen-tool.md) §3.1 — o que fica entre o lexer
([`lexer-design.md`](lexer-design.md)) e a emissão
([`codegen-design.md`](codegen-design.md)). O contrato é
[`keel-spec.md`](../keel-spec.md) §§2 e 4. Este documento não acrescenta regra:
fixa a estrutura de dados, a ordem das passagens e onde cada verificação da
linguagem cai.

Onde **decide** algo que os normativos deixam aberto, a decisão vem marcada
**[P*n*]** e listada na §9.

---

## 1. O problema, numa frase

> Achar as ilhas sem entender o mar.

O parser percorre tokens procurando **posições** onde uma palavra contextual
tem função keel, e **símbolos** que ele mesmo registrou. Tudo o mais é região
opaca: atravessada com balanceamento de delimitadores, sem interpretação, e
copiada para a saída.

Duas consequências governam o desenho inteiro:

**Não há árvore de sintaxe do C.** O que o parser produz é uma lista de
declarações reconhecidas, cada uma com as fatias de texto opaco que a
acompanham. Uma função vira `{ assinatura reconhecida, corpo opaco }`, e o
corpo é revisitado depois para achar as ilhas de dentro.

**O reconhecimento depende do que já foi registrado.** `buffer i32 x;` só é uma
declaração keel porque `buffer` está na tabela como modificador de aridade 1.
Antes do import que o traz, a mesma linha é C opaco. É por isso que a coleta
vem antes da resolução (§3).

---

## 2. As estruturas

```c
typedef struct {
    KParser          lex;        /* lexer + lookahead (lexer-design §7)  */
    keel_arena      *arena;      /* tudo vive aqui; nada é liberado      */
    KSymbolTable    *syms;
    KDiagnosticSink *diag;
    KLoader         *loader;     /* a volta para tool.c, em import       */
    keel_buffer_Open opens;      /* pilha de delimitadores               */
} KParserCtx;
```

**[P1] Tudo é arena, nada é liberado.** Uma invocação do cgen tem vida curta e
um pico de memória previsível; `free` por nó só acrescentaria caminhos de erro.
`keel_arena` é o tipo da própria base, e é o que o desenho do cgen §3.2 já
estabelece para os tipos de apoio.

**Toda posição é uma fatia do fonte**, não uma cópia. `keel_slice_char` sobre o
buffer do arquivo, que vive enquanto o módulo estiver carregado. Linha e coluna
são calculadas sob demanda, contando quebras até o ponteiro — só o diagnóstico
precisa delas, e diagnóstico é raro.

### 2.1 A tabela de símbolos

```c
typedef enum {
    K_SYM_TYPE,        /* typedef, struct/union/enum nomeado, tipo de módulo */
    K_SYM_MODIFIER,    /* aridade e espécie de cada parâmetro                */
    K_SYM_FUNCTION,    /* retorno declarado e aridades                       */
    K_SYM_VARIABLE,
    K_SYM_CONSTANT,    /* constexpr, constante de enum                       */
    K_SYM_TAGS,        /* conjunto fechado e seus valores                    */
    K_SYM_MODULE,      /* alias ativo de import                              */
    K_SYM_EXTERN_C     /* nome de extern_c: só a distinção variável/função   */
} KSymKind;
```

O que cada espécie guarda é exatamente o que a spec §2.3 autoriza consultar — e
**nada além**. Em particular, `K_SYM_EXTERN_C` guarda a distinção entre
variável e função e mais nada: não há tipo ali, porque a linguagem não permite
conhecê-lo.

A busca é por grafia, num mapa por nome. **[P2] O mapa é ordenado e a iteração
é por ordem de inserção**, nunca por hash — a ordem de emissão é a ordem de
declaração (codegen §6), e uma tabela que só soubesse responder "existe?"
obrigaria a guardar a ordem duas vezes.

### 2.2 O escopo

Escopo léxico com encadeamento simples: cada bloco empilha, cada `}` desempilha.
O nível de arquivo é a raiz, e abaixo dela ficam os símbolos importados —
separados, porque `shadowed-injected-name` precisa distinguir "declarou por
cima de um nome injetado" de "redeclarou o próprio".

### 2.3 A árvore

**[P6] Não há árvore de sintaxe de C** (§1) — o que existe é uma lista
ordenada de declarações reconhecidas, cada uma com as fatias de C opaco que a
acompanham, mais uma lista ordenada de ilhas por corpo de função. É essa
estrutura que dá tipo ao `KModuleDecl *decl` que o codegen já assume
([`codegen-design.md`](codegen-design.md) §3), e aos campos que o despejo de
`--stop-after=parse` imprime ([`cgen-tool.md`](cgen-tool.md) §5.2) — o despejo
é uma projeção desta árvore, não um mecanismo à parte.

```c
/* Uma declaração de topo, na ordem do fonte — decl-module + top-item da
 * spec §2.2. Índice em KModuleDecl.decls. */
typedef enum {
    K_TOP_IMPORT, K_TOP_IMPORT_C, K_TOP_EXTERN_C,
    K_TOP_MODIFIER, K_TOP_INSTANCE, K_TOP_TAGS, K_TOP_EXTENT,
    K_TOP_TYPEDEF, K_TOP_STRUCT, K_TOP_FUNCTION, K_TOP_KEEL_DECL,
    K_TOP_OPAQUE               /* C que o passo 6 da §4 não classificou */
} KTopKind;

typedef enum { K_VIS_DEFAULT, K_VIS_PUB, K_VIS_PRIV } KVisibility;

typedef struct KTopDecl KTopDecl;
struct KTopDecl {
    KTopKind     kind;
    KVisibility  vis;
    bool         is_inline;          /* só K_TOP_FUNCTION */
    keel_slice_char name;            /* nome keel; vazio em K_TOP_OPAQUE */
    keel_slice_char pos;             /* token que ancora o diagnóstico (§5.2) */
    union {
        struct { keel_slice_char alias, module_path; bool types; } import_;
        struct { keel_slice_char header; } import_c;
        struct { bool type_h; keel_slice_char body; } extern_c;
        struct { bool byref; keel_buffer_KToken body; } modifier;   /* retém tokens (§5) */
        KKnownType instance;                          /* known-type já resolvido */
        struct { KTagItem *items; size_t count; } tags;
        struct { KExtentDim *dims; size_t dim_count;
                 KExtentField *fields; size_t field_count; } extent;
        struct { keel_slice_char underlying; } typedef_;          /* spelling C, opaco */
        struct { KField *fields; size_t count; } struct_;
        KFunctionDecl function;
        struct { keel_slice_char declarator, init; keel_slice_char else_tail; } keel_decl;
        struct { keel_slice_char span; } opaque;
    } as;
};

/* decl-function da spec §2.2. body_opaque é a fatia entre '{' e '}' (passo 2,
 * "coleta"); islands só existe depois da passagem 3, "resolução" (§3). */
typedef struct {
    keel_slice_char return_type;    /* spelling, opaco: o backend não interpreta */
    KParam         *params;
    size_t          param_count;
    bool            has_body;
    keel_slice_char body_opaque;
    KIsland        *islands;
    size_t          island_count;
} KFunctionDecl;

/* Uma ilha dentro de um corpo — a unidade que --stop-after=parse imprime
 * (cgen-tool.md §5.2). O detalhe de cada espécie é o que a tabela do §5.2 já
 * fixa; a struct só nomeia os campos em vez de compor a string na hora. */
typedef enum {
    K_ISLAND_TYPE, K_ISLAND_NAME, K_ISLAND_CALL, K_ISLAND_FROM_STACK,
    K_ISLAND_REF, K_ISLAND_DEFER, K_ISLAND_IMPLICIT_INIT,
    K_ISLAND_FOREACH, K_ISLAND_MATCH, K_ISLAND_INDEX,
    K_ISLAND_RANGE_INDEX, K_ISLAND_ARRAY, K_ISLAND_ARRAY_INDEX
} KIslandKind;

typedef struct {
    KIslandKind kind;
    keel_slice_char pos;             /* âncora da espécie, ver tabela §5.2 */
    union { /* um membro por espécie, campos = colunas do §5.2 */
        struct { KKnownType written; keel_slice_char c_symbol; } type_;
        struct { keel_slice_char name, symbol; } name_;
        struct { keel_slice_char callee, symbol; int arity;
                 KAdaptMark *marks; size_t mark_count; } call_;
        /* … from_stack (mesma forma de call_), ref, defer, implicit_init,
             foreach, match, index_, range_index_, array_, array_index_ */
    } as;
} KIsland;

/* Uma instância pedida — modificador + argumentos, fechada pelo parser
 * (§5). Uma por identidade canônica; ordem = primeiro uso. */
typedef struct {
    KKnownType      shape;           /* modificador + argumentos, canônico */
    keel_slice_char symbol;          /* produzido por mangle.c — ver §10 */
    keel_slice_char first_use;       /* posição, para a nota de instanciação (backend §6.1) */
} KInstanceUse;

/* A raiz: o que um .k inteiro produz para o emissor. */
typedef struct {
    keel_slice_char module_name, pos;
    KTopDecl       *decls;           /* ordem do fonte — codegen §3 exige isso */
    size_t          decl_count;
    KInstanceUse   *instances;       /* fecho completo, ordem de primeiro uso */
    size_t          instance_count;
} KModuleDecl;
```

`KKnownType`, `KParam`, `KField`, `KTagItem`, `KExtentDim`/`KExtentField` e
`KAdaptMark` são os tipos auxiliares menores — a forma de `known-type` da
gramática, um parâmetro, um campo de struct, um item de `tags`, uma
dimensão/coluna de `extent`, uma marca `&k`/`type:k` do §5.2. Ficam à parte,
não embutidos no corpo de `KTopDecl`, porque cada um é reusado em mais de um
lugar: `KParam` em função e em `modifier`; `KKnownType` em `specifier`, em
`K_TOP_INSTANCE` e em `KInstanceUse.shape`.

**`body_opaque` e `islands` coexistem de propósito.** A passagem 2 preenche só
`body_opaque`; a passagem 3 preenche `islands` a partir dele. É o que permite
que a etapa `decl` do despejo (§3.2) funcione antes de a etapa `ilha` existir
— o campo já está lá, só vazio.

**`modifier.body` é `buffer KToken`, não `keel_slice_char`**, pela mesma razão
da §5: um módulo genérico retém o fluxo de tokens depois de processado, porque
o corpo é revisitado por cada instância que o fecho descobre — sobreviver como
fatia de texto não bastaria.

**Nenhum campo novo em `KSymbolTable`.** A árvore referencia símbolos por
grafia (`keel_slice_char name`) e a tabela resolve; não duplica o que a §2.1
já guarda.

---

## 3. As passagens

> **[P3] Três passagens sobre o mesmo buffer de tokens, não uma.**

Uma passagem só não serve: `pub i32 f(void) { … }` pode chamar `g`, declarada
depois, e `buffer i32 x;` precisa que `buffer` já esteja na tabela. A
alternativa — resolver por adiamento, com lista de pendências — dá o mesmo
resultado com mais estado.

| | Passagem | O que faz | O que ainda não faz |
| --- | --- | --- | --- |
| 1 | **cabeçalho** | `module`, `import`, `import_c`, `extern_c`; carrega o fecho | não olha declaração |
| 2 | **coleta** | registra tipos, modificadores, funções, variáveis, constantes, tags do arquivo | não entra em corpo de função |
| 3 | **resolução** | percorre corpos e inicializadores; reconhece construções, usos e verbos | — |

A passagem 1 é a única que chama de volta a ferramenta. Ela termina com o fecho
de imports resolvido, e é nela que `circular-import` aparece — a pilha de carga
vive em `tool.c`, e o parser só reporta na posição do `import`.

A passagem 2 não entra em corpo porque o corpo pode usar o que vem depois dele.
Ela atravessa `{ … }` como região balanceada e guarda a fatia.

A passagem 3 é a única que pode criar instância, e é onde o fecho (§5) cresce.

### 3.2 Etapas de construção, com saída

As três passagens acima não precisam nascer completas para dar sinal de vida.
O despejo `--stop-after=parse` já distingue quatro níveis cumulativos
([`cgen-tool.md`](cgen-tool.md) §5.2), e o oráculo
(`tools/cgen/test/parse_dump.sh`) já compara por `diff -u` em cada um —
**cada etapa abaixo é aditiva**: uma vez acesa, faz aparecer linhas novas sem
quebrar as anteriores.

| Etapa | Nível do despejo | Cobre (spec §2.2) | Passa a existir | Oráculo |
| --- | --- | --- | --- | --- |
| 1 | `header` | `unit`, `decl-module`, `import`, `import-c`, `extern-c` (casca, corpo `<opaque>`) | `KModuleDecl` com nome e imports; `paths.c`/`tool.c`/`KLoader` mínimos | `parse_dump.sh header` |
| 2 | `decl` | `top-decl` completo, corpo de função e de `struct`/`modifier` ainda `<opaque>` (passagem 2) | `KTopDecl` por declaração; `KSymbolTable` populada | `parse_dump.sh decl` |
| 3 | `inst` | aplicação direta de modificador em `specifier`, sem entrar em corpo | `KInstanceUse` das instâncias escritas literalmente | `parse_dump.sh inst` |
| 4a | `ilha` (parcial) | `decl-keel`, `container`, `call` | primeiras `KIsland` de espécie `type`, `name`, `call`, `ref` | golden 001 |
| 4b | `ilha` | `decl-array`, `index`/`range-index` | espécies `array`, `array-index`, `index`, `range-index` | golden 002, 003, 006, 015, 025 |
| 4c | `ilha` | `defer` | espécie `defer` | golden 001, 005, 014, 017 |
| 4d | `ilha` | `foreach`, `walk`, `apply` | espécie `foreach` (e o que `walk`/`apply` precisarem) | golden 001, 019 |
| 4e | `ilha` | `parallel`, `worker-exit` | manager e `win`/`fail` | golden 004, 012 |
| 4f | `ilha` | `match`, `tags-block` | espécie `match` | golden 009 |
| 4g | `ilha` | `assign-else`, `else-tail` | tratamento de resultado (via `call` com marca) | golden 011, 018, 021 |
| 4h | `ilha` | `decl-extent`, `extent-column` | espécie(s) de coluna de `extent` | golden 020 |
| 4i | `ilha` (fecho) | resolução completa de instância por fecho de genérico (§5) | `KInstanceUse` fechado, não só direto | golden 007, base inteira |

4a–4i seguem a ordem dos casos golden, no espírito do M6 do
[`cgen-tool.md`](cgen-tool.md) §9 ("uma construção por vez, na ordem dos casos
golden") — esta tabela só nomeia essa ordem por espécie de ilha.

**Dependência a decidir com André:** a etapa 3 (`inst`) já precisa do
**símbolo C** de cada instância (`inst <modificador> <argumentos> <símbolo>
<pos>`), que é produzido pela função de nomeação canônica que
[`codegen-design.md`](codegen-design.md) §2 atribui a `mangle.c`. Isso empurra
`mangle.c` — só a função de nomeação, não `writer.c` — para antes do fim do
M2, e não a partir do M3 como a tabela de marcos sugere hoje (§10, pendência
nova).

### 3.3 Recuperação de erro

**[P4] O parser não tenta reparar; ele ressincroniza e continua.** Diante de um
diagnóstico de `error`, abandona a construção corrente e avança até o próximo
ponto seguro — `;` no nível externo, `}` que fecha o escopo corrente, ou
início de declaração de topo. Continuar é o que permite relatar vários erros na
mesma invocação, que é o que um build espera.

Nada é escrito quando houve `error` (desenho do cgen §3.3, passo 8), então a
recuperação não precisa produzir árvore correta — só precisa não travar nem
inventar símbolo.

---

## 4. Reconhecer uma declaração

É o ponto mais delicado do parser, porque é onde C e keel se parecem mais. A
spec §2.3 dá a tabela das condições; a implementação natural é um teste em
ordem, sobre a lookahead, no início de cada declaração:

```plain
1. palavra do núcleo em posição de topo?        → construção keel
2. IDENT registrado como modificador?           → consome a aridade declarada,
                                                  recursivamente, e segue para
                                                  o declarador
3. IDENT registrado como tipo?                  → declarador
4. 'array' | 'constexpr'?                       → forma própria
5. casa a regra de função da spec §2.3?         → assinatura reconhecida,
                                                  corpo opaco
6. senão                                        → região opaca até o ';' externo
```

**O passo 5 é o que mais erra.** A regra da spec não é "tem parênteses": o
grupo final de parâmetros não pode ser precedido de `=` externo, e o `(` tem
que ser imediatamente precedido do `IDENT` do nome. `int (*f)(void);` falha
porque o token anterior ao grupo é `)`; `int x = f(1);` falha pelo `=`. As duas
continuam C opaco, e **isso não classifica o tipo delas** — o parser não
aprendeu nada sobre `f` ou `x`.

**O passo 2 consome por aridade, não por gramática.** `buffer slice char lines`
é `buffer` aplicado a `slice char`, e o parser sabe disso porque `buffer` tem
aridade 1 e `slice` tem aridade 1. Sem os símbolos, a mesma sequência é três
identificadores.

### 4.1 A ressalva de `symbol-redeclaration`

Os padrões `IDENT IDENT` e `IDENT '*' IDENT` em início de statement, com o
segundo identificador sendo símbolo keel conhecido, são recusados **sem tentar
resolver o primeiro como tipo C**. É a spec §2.5, e a implementação é literal:
dois tokens de lookahead e uma consulta. O parser não precisa saber se o
primeiro é um tipo.

---

## 5. As instâncias

O parser é quem descobre instância, e quem **fecha** o conjunto antes de
entregar ao emissor (codegen §3). A estrutura é `KInstanceUse` (§2.3); esta
seção descreve como o conjunto se fecha, não a forma do dado.

```plain
uso reconhecido  →  identidade canônica  →  já na tabela?  →  não: registra,
                                                                e fecha sobre
                                                                os verbos do
                                                                genérico
```

**O fecho exige o corpo do genérico.** `slice i32` puxa `outcome i32` porque
`at` devolve `outcome T` — e saber isso exige ler a assinatura de `at` em
`slice.k`. Por isso um módulo genérico **retém o fluxo de tokens** depois de
processado (spec da ferramenta §3), em vez de descartá-lo.

**A superfície degenerada é calculada aqui** (codegen §7.4, decisão E3). É
função pura de (genérico, argumentos), e o parser precisa dela por dois
motivos: para fechar corretamente — um verbo que sumiu não puxa a instância que
ele devolveria — e para emitir `verb-not-in-instance` quando o programa chama
um verbo ausente.

A ordem importa: **calcular a superfície antes de fechar**. Fechar primeiro
geraria `outcome const char` para um `at` que não existe, que foi exatamente o
erro cometido à mão ao derivar o golden.

### 5.1 A posição de criação

Cada instância guarda a posição do **primeiro uso** que a criou. É o que
alimenta a nota de instanciação do backend §6.1:

```plain
coll.k:14:12: error: invalid operands to binary +
sim.k:7:1: note: in the instantiation of coll.stack at geom.Point
```

"Primeiro" é na ordem de resolução, que é a ordem do fonte — determinística,
como tudo o mais.

---

## 6. Resolver uma operação

A spec §4.4 dá a ordem, e ela é curta:

```plain
1. verbo do tipo conhecido do contêiner, na aridade escrita
2. função do módulo qualificador, na aridade escrita
3. verbo pertence a outro módulo         → wrong-qualifier
   verbo existe no genérico, não na
   instância escrita                     → verb-not-in-instance
   senão                                 → emite a chamada para o C validar
```

A **posição de contêiner** é a produção `container` da spec §2.2, e é onde o
parser tem que resistir à tentação de ser esperto: uma chamada C desconhecida,
um cast, uma expressão arbitrária **não** fornecem identidade de contêiner.
O diagnóstico é `not-a-container-expression`, e ele é uma recusa, não um
fallback.

A adaptação de argumento — o `&` que o parser insere — vem do **parâmetro
declarado no callee**, nunca do call site (spec §4.4). Isso significa que
resolver uma chamada exige a interface do módulo chamado, e é a segunda metade
do backend §7.2 que é fácil de perder.

---

## 7. Onde cai cada verificação

O parser é dono de quase todo o catálogo. A tabela agrupa por passagem, que é o
que a implementação precisa saber:

| Passagem | Diagnósticos |
| --- | --- |
| lexer | `literal-with-newline`, `unmatched-delimiter`, `delimiter-mismatch-across-branches`, `define-over-keel-name` |
| 1 cabeçalho | `missing-module`, `module-path-mismatch`, `invalid-stem`, `circular-import`, `symbol-collision`, `duplicate-alias`, `import-clause-order`, `duplicate-injected-name`, `nested-extern-c`, `alias-type-collision` |
| 2 coleta | `modifier-outside-generic`, `parameter-name-reuse`, `modifier-named-instance`, `pub-static`, `inline-without-visibility`, `static-on-type`, `canonical-name-collision`, `undeclared-tags`, `duplicate-tag`, `unnamed-tags`, `nonconstant-dim`, `dim-below-one` |
| 3 resolução | todo o resto: contêiner, aridade, protocolo, `else`, `match`, `parallel`, `array`, `ref`, `constexpr`, `verb-not-in-instance` |
| emissor | `name-too-long`, `reserved-name` |
| backend, em execução | os 7 `debug` |

**`symbol-collision` é da passagem 1 e não da 2** porque compara o módulo
contra o fecho de imports, e o fecho só existe depois que todos carregaram.

---

## 8. Testes

O motor não abre arquivo (desenho do cgen §3.1), e é isso que torna o teste do
parser barato:

```c
KModule *mods[] = { mock("keel.slice", SLICE_SRC), … };
k_process(src, "app.cfg", true, mock_loader(mods), &diag, &out);
```

Três famílias, na ordem em que valem a pena:

1. **Reconhecimento.** Pares (fonte, lista de declarações reconhecidas). É onde
   a tabela da §4 é exercitada, e onde `int (*f)(void)` tem que continuar
   opaco.
2. **Diagnóstico.** Um fonte por identificador do catálogo, afirmando o
   identificador emitido e a posição. São os casos de falha que o golden ainda
   não tem — e a §7 do [`diag-design.md`](diag-design.md) diz como agrupá-los
   sem escrever 133 casos.
3. **Fecho de instâncias.** Dado um `.k`, o conjunto fechado tem que ser
   exatamente o esperado. O caso de regressão já existe: a base fecha em 7
   instâncias além das que os módulos declaram, e isso foi calculado à mão
   (codegen §13, C2).

---

## 9. Decisões deste documento

| | Decisão | Por quê |
| --- | --- | --- |
| P1 | Tudo em arena, nada é liberado | invocação de vida curta e pico previsível; `free` por nó só acrescentaria caminho de erro |
| P2 | Tabela de símbolos ordenada, iteração por inserção | a ordem de emissão é a de declaração; guardar a ordem duas vezes é onde ela diverge |
| P3 | Três passagens, não uma com adiamento | o adiamento dá o mesmo resultado com mais estado, e o estado é onde o determinismo escapa |
| P4 | Recuperação por ressincronização, sem reparo | nada é escrito quando há `error`, então a árvore não precisa ficar correta — só não travar nem inventar símbolo |
| P5 | A superfície degenerada é calculada antes do fecho | fechar primeiro geraria instância para verbo que não existe; foi o erro cometido à mão ao derivar o golden |
| P6 | A saída do parser é `KModuleDecl` + lista de `KTopDecl`/`KIsland` (§2.3), não uma árvore de C | é a estrutura que dá tipo ao `KModuleDecl *decl` que o codegen já assume, e ao que o despejo do §3.2 imprime — sem inventar um segundo mecanismo só para depurar |

---

## 10. Pendências

| | Onde | Divergência |
| --- | --- | --- |
| Q2 | spec §4.4 × este documento | a ordem de resolução tem três passos e o `verb-not-in-instance` entrou como caso do passo 3; se um dia houver um quarto receptor de verbo, a ordem precisa ser reenunciada em vez de emendada |
| Q3 | §3.2 × `cgen-tool.md` §9 | a etapa `inst` do despejo já precisa do símbolo produzido por `mangle.c` (codegen §2), o que antecipa essa função — só a nomeação, não `writer.c` — para dentro do M2; a tabela de marcos do §9 ainda lista `mangle.c` a partir do M3 |

Resolvida em 2026-09-22: Q1. A spec §2.3 agora diz o que fazer no EOF sem
delimitador aberto: `unexpected-eof`, distinto de `unmatched-delimiter`
(que continua cobrindo o caso com delimitador aberto pendente).
