# T8a — auditoria dos designs, metade A

Só leitura. Nenhum arquivo do repositório foi editado; este é o único arquivo criado.

Normativos consultados: `keel-spec.md`, `keel-c-backend.md`, `cgen-tool-spec.md`.
`keel-rationale.md` não foi consultado diretamente.

## 1. Uma linha por design

| Design | Linhas lidas | Achados |
| --- | --- | --- |
| `design/codegen-design.md` | 511 (todas) | 5 (A1 a A5) |
| `design/parser-design.md` | 504 (todas) | 1 (P1) |
| `design/lexer-design.md` | 247 (todas) | 2 (L1 e L2) |

Dúvidas (sem correção proposta fechada): D1 a D5, na seção 3.

## 2. Achados

### A1 — conceito superado: `arena.c` reescreve `alloc(a,T,n)`

- **onde:** `design/codegen-design.md:66` (tabela por construção) e `:452` (tabela "o que cada construção pede").
- **o design diz:** `arena.c | §5.4 | a reescrita de alloc(a,T,n)`; e `arena | §5.4 | reescrita de alloc(a,T,n) materializando sizeof/alignof/cast`.
- **o normativo diz:** `keel-spec.md:2046`: "`alloc(a, T, n)` é `alloc(a, n, sizeof(T), alignof(T))`, com `type T` apagado (§4.4)". `keel-spec.md:972-973` (§4.4, regras 13 e 14): o parâmetro `type` apagado é uma regra geral, emitida como dois `size_t`. `keel-c-backend.md:1020` (§5.4, regra 2): "`arena.alloc(a, T, n)` tem o `type T` apagado (§5.16)". O que a arena tem de específico no núcleo está na tabela de `keel-spec.md:1964`: `= {0}` implícito e recusa de parâmetro por valor. O restante segue os papéis gerais (§4.12).
- **tipo:** conceito superado.
- **correção proposta:**
  - Em `codegen-design.md:66`, a linha passa a ser:
    ```
    | `arena.c` | §5.4 | definição de `arena` sem inicializador recebe `= {0}` (regra 8); construtores e a forma crua `alloc(a, n, sz, al)` |
    ```
  - Em `codegen-design.md:452`, a linha da arena passa a ser:
    ```
    | `arena` | §5.4 | `= {0}` implícito na definição; construtores e forma crua de `alloc` |
    | parâmetro `type` apagado | §5.16 | `sizeof`/`alignof` na chamada, pares `size_t` na assinatura, cast do retorno `X *` |
    ```
- **certeza:** alta. A linha do parâmetro `type` aponta para uma seção sem arquivo próprio; ver A4.

### A2 — contradição: `extern_c { … }` vai para o `.c`

- **onde:** `design/codegen-design.md:141`.
- **o design diz:** `| extern_c { … } | — | — | conteúdo intacto |`.
- **o normativo diz:** `keel-c-backend.md:354`: "`extern_c { … }` / `pub extern_c { … }` | — | o conteúdo, intacto | —", ou seja, vai para o `.h`. A mesma tabela manda `extern_c [type_h]` ao `.type.h` (`:355`) e só `priv extern_c` ao `.c` (`:356`). A regra 15 de `keel-c-backend.md` §4.1 diz: "sem qualificador ou com `pub`, vai para o `.h`; com `[type_h]`, para o `.type.h`; `priv`, para o `.c`".
- **tipo:** contradição.
- **correção proposta:** substituir a linha de `codegen-design.md:141` por estas três:
  ```
  | `extern_c { … }` / `pub extern_c { … }` | — | conteúdo intacto | — |
  | `extern_c [type_h] { … }` / `pub extern_c [type_h] { … }` | conteúdo intacto | — | — |
  | `priv extern_c { … }` | — | — | conteúdo intacto |
  ```
- **certeza:** alta.

### A3 — omissão: a tabela "quem vai onde" não tem as linhas de diretiva e de C de topo

- **onde:** `design/codegen-design.md:131-141`, tabela "Escrita no `.k` | `.type.h` | `.h` | `.c`".
- **o design diz:** a tabela termina em `extern_c { … }`. Não há linha para diretiva de topo, para C de topo nem para `extern_c [type_h]` e `pub extern_c`.
- **o normativo diz:** `keel-c-backend.md:357`: "diretiva de topo | — | preservada | —". `keel-c-backend.md:358`: "C de topo que não é diretiva nem construção keel (`static_assert`) | o conteúdo, intacto | — | —". Ambas estão na tabela de §4.1, que a própria §4 do design diz ser a fonte.
- **tipo:** omissão em tabela que o design apresenta como a regra de posição.
- **correção proposta:** acrescentar, depois da linha de `extern_c`, estas linhas:
  ```
  | diretiva de topo | — | preservada | — |
  | C de topo que não é diretiva nem construção keel (`static_assert`) | conteúdo intacto | — | — |
  ```
  (as três linhas de `extern_c` estão em A2.)
- **certeza:** alta.

### A4 — omissão: a tabela por construção para na §5.14

- **onde:** `design/codegen-design.md:61-72`, tabela "Arquivo | Backend | Construção". Os parágrafos das linhas 74-96 explicam por que §5.10 e §5.11 não têm arquivo, mas não tratam das seções seguintes.
- **o design diz:** a tabela cobre §5.1 a §5.14. §5.17 aparece só de passagem, em `index.c`, como "o `assert` sob `--checks`" (`:65`).
- **o normativo diz:** `keel-c-backend.md:1644` (§5.15 `extent`), `:1694` (§5.16 parâmetro `type`), `:1729` (§5.17 verificações de debug, com a lista de `:1962`: `index-out-of-length`, `set-out-of-length`, `array-index-out-of-bounds`, `range-index-out-of-bounds`, `extent-index-out-of-bounds`, `tag-out-of-range`, `alloc-overflow`), `:1787` (§5.18 binder de dimensão) e `:1832` (§5.19 função sobre protocolo).
- **tipo:** omissão em lista que se diz "cada seção" (a correspondência com as seções do backend).
- **correção proposta:** acrescentar à tabela:
  ```
  | `extent.c` (nome a decidir) | §5.15 | `extent`: struct, colunas e acesso verificado |
  | (a decidir: `decl.c` ou arquivo próprio) | §5.16 | parâmetro `type` apagado: pares `size_t`, `sizeof`/`alignof` na chamada, cast do retorno |
  | `check.c` (nome a decidir) | §5.17 | verificações `debug`: `KEEL_CHECK` injetado em `as_slice` e as demais de §5.17; o `assert` de `index.c` (§5.3) continua em `index.c` |
  | `binder.c` (nome a decidir) | §5.18 | binder de dimensão `keel__N` |
  | `protocol.c` (nome a decidir) | §5.19 | função sobre protocolo |
  ```
- **certeza:** alta quanto à omissão. Os nomes de arquivo são sugestão, não decisão.

### A5 — omissão: a tabela "o que cada construção pede" para na §5.14

- **onde:** `design/codegen-design.md:447-462`.
- **o design diz:** as linhas vão de "declaração com modificador" (§5.1) a "`outcome`, `corot`" (§5.14).
- **o normativo diz:** os mesmos `keel-c-backend.md:1644`, `:1694`, `:1729`, `:1787` e `:1832` de A4.
- **tipo:** omissão.
- **correção proposta:** acrescentar, depois de `outcome`, `corot`:
  ```
  | `extent` | §5.15 | struct de colunas e acesso verificado (`extent-index-out-of-bounds`, §5.17) |
  | parâmetro `type` apagado | §5.16 | pares `size_t` na assinatura, `sizeof`/`alignof` na chamada e no corpo |
  | verificações `debug` | §5.17 | `KEEL_CHECK` injetado no início do verbo, e a linha de `#line` depois dele (backend §6, linha injetada) |
  | binder de dimensão | §5.18 | `keel__N` como parâmetro e como expressão |
  | função sobre protocolo | §5.19 | verbo de instância que chama o verbo do protocolo |
  ```
- **certeza:** alta.

### L1 — omissão: a lista de palavras contextuais não tem `protocol`

- **onde:** `design/lexer-design.md:112-118` (`k_contextual_words`), com a afirmação de `:126-127`: "Essa é a lista de palavras-chave contextuais do núcleo keel".
- **o design diz:** a lista não contém `protocol`.
- **o normativo diz:** `keel-spec.md:218`: "`protocol` | declaração de arquivo, seguido do nome e de `type` ou `[` (§5.1)". `keel-spec.md:580` exclui de `define-over-keel-word` só as palavras de papel (`parent`, `child`, `invalidates`, `consumes`) e os verbos de módulo. `protocol` não está entre essas exclusões, então entra na lista.
- **tipo:** omissão em lista que se diz completa.
- **correção proposta:** em `design/lexer-design.md:116`, inserir `"protocol"` entre `"priv"` e `"pub"`:
  ```
      "parallel", "priv", "protocol", "pub", "ref", "tags", "type", "type_h", "types",
  ```
  `keel_code` (v1, `keel-spec.md:220`) não entra agora: começa por `keel_` e cai em `define-over-keel-name`, o que é outra questão (D2).
- **certeza:** alta.
- **verificado sem achado:** a lista de palavras C (`k_c_words`, `lexer-design.md:150-161`) tem as mesmas 59 grafias de `keel-spec.md:199-208`, conferidas por comparação. A lista de papéis excluídos da lista contextual confere com `keel-spec.md:580`.

### L2 — referência errada: "backend §4.2" onde é §2.3

- **onde:** `design/lexer-design.md:121`.
- **o design diz:** "A reserva vale para o fonte do usuário (backend §4.2)."
- **o normativo diz:** `keel-c-backend.md:212` (§2.3, regra 3): "Identificadores começando com `keel_` ou `KEEL_` são reservados: declará-los ou defini-los no fonte do usuário é erro". `keel-c-backend.md:383` é o início de §4.2, "O prelúdio: `keel.k`", que não trata de reserva.
- **tipo:** referência errada.
- **correção proposta:** trocar "(backend §4.2)" por "(backend §2.3, regra 3)".
- **certeza:** alta.

### P1 — omissão: `KTopKind` não tem a declaração `protocol`

- **onde:** `design/parser-design.md:120-125` (enum `KTopKind`). O próprio design diz, em `:118-119` e `:109-115`, que a árvore segue "decl-module + top-item da spec §2.2".
- **o design diz:** `K_TOP_IMPORT, K_TOP_IMPORT_C, K_TOP_EXTERN_C, K_TOP_MODIFIER, K_TOP_INSTANCE, K_TOP_TAGS, K_TOP_EXTENT, K_TOP_TYPEDEF, K_TOP_STRUCT, K_TOP_FUNCTION, K_TOP_KEEL_DECL, K_TOP_OPAQUE`. Não há espécie para `protocol`. A espécie de símbolo `K_SYM_PROTOCOL` existe (`:86`), mas a declaração de topo não tem onde ser guardada.
- **o normativo diz:** `keel-spec.md:306-307`: `top-decl` inclui `decl-protocol`; `keel-spec.md:312-313`: `decl-protocol ::= 'protocol' IDENT 'type' IDENT '{' { proto-item } '}' | 'protocol' IDENT '[' qualified-name … ']' ';'`.
- **tipo:** omissão em enum que mapeia o top-item da gramática.
- **correção proposta:** em `design/parser-design.md:122`, acrescentar `K_TOP_PROTOCOL` depois de `K_TOP_EXTENT`, e um membro correspondente no `union as` (`:136-150`). A forma exata do membro depende de `proto-item` (spec §5.1), e fica a decidir:
  ```c
  K_TOP_IMPORT, K_TOP_IMPORT_C, K_TOP_EXTERN_C,
  K_TOP_MODIFIER, K_TOP_INSTANCE, K_TOP_TAGS, K_TOP_EXTENT, K_TOP_PROTOCOL,
  K_TOP_TYPEDEF, K_TOP_STRUCT, K_TOP_FUNCTION, K_TOP_KEEL_DECL,
  K_TOP_OPAQUE
  ```
  e, no `union as`, `struct { keel_slice_char name; /* proto-item a definir, spec §5.1 */ } protocol;`.
- **certeza:** alta quanto à omissão; a forma do membro é proposta.

### Correção à própria tarefa: a citação `linguagem §4.5` está certa

- **onde:** `design/lexer-design.md:10`: "(linguagem §4.5)" para "a visão sai da memória, nunca o contrário".
- **verificação:** `keel-spec.md:1037` está dentro de §4.5 (item 5 de "Regras") e diz: "é o lado **memória** que importa o lado visão para devolvê-lo, e nunca o contrário". A citação está correta. O cartão supôs que §4.5 era só indexação; o trecho de `keel-spec.md:1037-1038` mostra que não é o caso.
- **resultado:** nenhum achado.

## 3. Dúvidas

- **D1 — fórmula de mangling (`codegen-design.md:229-233`).** A fórmula `símbolo = <módulo com . → _> [ _ <argumento canônico> ]* [ _<sufixo de aridade> ]` não diz se o nome do modificador entra. O backend diz `M.mod` aplicado a `A` vira `M_mod_<A>` (`keel-c-backend.md:103`, tabela de §2.1), e dá `coll.stack i32` → `coll_stack_i32` (exemplos de §2.1), `keel.buffer.buffer` → `keel_buffer_i32` pela regra 2 de §2.1 (`:123`, encurtamento), e `stack i32` de `stack` → `stack_stack_i32` na tabela de §4.1 (`:316-320`). A fórmula do design só fecha esses casos se "módulo" for lido como `M.mod`; não está explícito. Decisão do André sobre o texto.

- **D2 — `keel_code` (v1).** `keel-spec.md:220` lista `keel_code` como palavra contextual. Pelo `keel-c-backend.md:212`, identificadores que começam por `keel_` são reservados, e `keel-spec.md:580` manda `define-over-keel-name` como erro. Se `keel_code` for palavra da linguagem, ela colide com a regra de reserva. O normativo não resolve isso. Preciso de decisão antes de L1 ser completada com a palavra.

- **D3 — exceção dos módulos da base em `lexer-design.md:121-124`.** O design diz que os módulos da base "são donos dos nomes `KEEL_`", e que "é o prelúdio que define `KEEL_CHECKS` e `KEEL_CHECK`". Os normativos não confirmam isso sem ambiguidade:
  - `keel-c-backend.md:212` restringe a reserva a "fonte do usuário", e a `keel-spec.md:580` não tem exceção para a base.
  - `keel-c-backend.md:212` também diz que "`KEEL_CHECKS` chega pela linha de comando do compilador C, e não por `#define` no fonte".
  - `keel-c-backend.md:418` (§4.2, regra 5) diz "o único `#if` do prelúdio é o de `KEEL_CHECKS`", e `:1733-1737` (§5.17) mostra `#ifndef KEEL_CHECKS` / `#define KEEL_CHECKS 1` no `keel.type.h` emitido.
  Não sei dizer se o prelúdio-fonte contém esse `#define` ou se ele só aparece no texto emitido. O design depende da primeira leitura. Decisão para o André.

- **D4 — aridade de `as_slice` no lexer (`lexer-design.md:9`).** O design escreve `buffer.as_slice(fonte)`, com um argumento. `keel-spec.md:1038` diz que "o protocolo pede só a forma de três parâmetros", e a forma keel é `as_slice(x,a,b)`. Provavelmente o trecho fala de uma função C do próprio cgen, e não de uma chamada keel; confirmar.

- **D5 — espécies de símbolo (`parser-design.md:77-87`).** `keel-spec.md:513-516` (§2.3, "Informação de símbolos") lista como origens: "parâmetros genéricos" e "símbolos das construções cooperativas e paralelas". `KSymKind` não tem espécie para os binders de módulo (`dim`, `tags`, `type`), e o símbolo de controle de `parallel` (`parallel.control`, `keel-spec.md`, tabela de "O que o núcleo conhece pelo nome", em torno da linha 1958-1968) não tem nome próprio. Pode caber em `K_SYM_TYPE` e `K_SYM_CONSTANT`, mas o design não diz. Confirmar.

- **Fora do critério normativo (não é achado):** `parser-design.md:292-298` e `:499-500` ("Dependência a decidir com André" e a pendência Q3 sobre `mangle.c`) pedem decisão sobre um marco. O estado registrado em memória do projeto diz que `writer` e `mangle` já estão prontos (M3). A pendência pode estar desatualizada. Não é incoerência com normativo, só com o próprio estado.

## 4. Verificado sem achado

- `codegen-design.md` §7.4 (instanciação degenerada) confere com `keel-spec.md:815-818` (regras 11 a 14) e `keel-c-backend.md:869` (§5.2.1).
- `codegen-design.md` §8 (`#line`) confere com `keel-c-backend.md` §6, regras 1 a 6 e a tabela de regiões.
- `codegen-design.md` §5 (nomes): tamanhos 255, 31 e 63 conferem com `keel-c-backend.md:230-232` (§2.4), e `__` / `_Atomic` com `:125` (§2.1, regra 4).
- `parser-design.md` §4 (reconhecimento, passo 5) e §6 (ordem de resolução) conferem com `keel-spec.md:527-540` e `:928-932`.
- `parser-design.md` §7: todos os 26 identificadores de diagnóstico conferidos existem no catálogo (`keel-spec.md`, §6.2).
- `lexer-design.md` §3 a §8: splice, pontuação `..`/`...`, `literal-with-newline`, `define-over-keel-name` / `define-over-keel-word` conferem com `keel-spec.md` §2.1 e §2.5.

## 5. Resumo para o orquestrador

8 achados (A1 a A5, L1, L2 e P1) e 5 dúvidas (D1 a D5). Os três mais graves:
1. A2: `codegen-design.md:141` manda `extern_c` para o `.c`, e o backend manda para o `.h` (`keel-c-backend.md:354`). Contradição direta que afeta a visibilidade.
2. P1: `parser-design.md:120-125` não tem `K_TOP_PROTOCOL`, embora o design diga que segue o top-item da spec (`keel-spec.md:306-312`).
3. L1: `lexer-design.md:112-118` não tem `protocol`, numa lista que o design declara completa (`keel-spec.md:218`).
