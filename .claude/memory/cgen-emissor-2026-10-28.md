---
name: cgen-emissor-2026-10-28
description: estado do emissor do cgen (M3 a M6) até 2026-10-31, o fluxo com Haiku em worktree e onde estão os abertos
metadata:
  type: project
---

O emissor fecha, byte a byte e nos dois perfis, os casos 005, 008, 014 e 017 do
golden e os oráculos à mão de `test/cgen/gen/`. Construções feitas: nível de arquivo
(typedef struct, `constexpr`, variável, `tags`), `array`, `foreach` sobre intervalo
literal, `constexpr` de bloco sob C11, índice verificado, e `defer` completo (fim do
bloco, `return` em escada ou inline, `break`/`continue`).

**Why:** o fluxo "oráculo à mão pelo Opus/Sonnet, uma construção por Haiku em worktree"
funcionou e achou muitos defeitos do próprio golden (alinhamento à mão, `#line` fora do
invariante). **How to apply:** repita o fluxo no M5; confira o golden antes de culpar o
agente; leia `tools/agents/passagem-2026-10-28.md` (estado, pendências e o próximo passo).

**Atualização (fim de 2026-10-28):** o M5 fecha 001, 002, 003, 006 e 007 (instâncias de
genérico do usuário, `instance`, usos de outro módulo pelas ilhas); acessor de vetor com sufixo
de rank `_N` (decisão do André, backend §2.1.1); falta emitir a `/base` (vale o `.k`) (oráculo `golden/<p>/keel/`) e o `--instance`. Ver a
seção "Atualização" de `tools/agents/passagem-2026-10-28.md`.

Decisões do André nesta sessão: `tags` segue a escrita do autor; `constexpr` C11 com
parênteses por contagem de tokens, `(void)`/`_Static_assert` no objeto de conferência e
contador por função; cleanup do `defer` em linhas próprias com `#line`; testes em `test/`
na raiz, `engine/{lexer,parser,emit}/` com `.h` ao lado do `.c`.

**Atualização (2026-10-29):** a base sai do cgen. Modo de instância no parser
(`k_collect_instance_islands`) e no emissor (`e->inst`, o `.h` da instância pelo `emit.c`);
`--instance` por módulo sintético em memória; oráculo `test/cgen/base.sh` (92 de `golden/<p>/keel`,
16 wip: `keel.array` sem modificador e `slice_of_*`, função sobre protocolo). O golden da base é
corrigido conferindo cada linha contra o `.k`, nunca copiado da ferramenta. Ver a última seção de
`tools/agents/passagem-2026-10-28.md`.

**Atualização (2026-10-30):** M5 fechado (`base.sh` 108 ok: a base inteira sai do cgen, `--instance`
inclusive para `keel.array T` e para função sobre protocolo `keel.slice.of <concreto>`); modo `debug`
do runner (R2, `DEBUG`, caso 026); M6 com `gen_golden` 23 ok, 0 wip. Em 2026-10-31 fecharam 010 (`routine`, unidade = módulo instanciado), 009/029 (`match`), 004/012 (`parallel`), 019 (`walk`), 030/031 (exemplos do README): 25 dos 31 casos pelo cgen. Abertos e plano na última seção de
`tools/agents/passagem-2026-10-28.md`. Lição: o golden às vezes tem alinhamento/ordem feitos à mão;
corrige-se conferindo contra o `.k` e a regra, nunca copiando a saída da ferramenta.

**Atualização (2026-11-01):** **28 dos 32 casos do golden saem do cgen byte a byte**
(`gen_golden` 37 ok, 1 wip; `base.sh` 124 ok; `golden/run.sh` 60 ok, 3 wip). Fecharam 020
(`extent`, `emit/extent.c`), 025 (`keel.length` do núcleo, `emit/core.c`), 016 (função sem
corpo) e o `.state = {0}` do inicializador (`emit/routine.c`). O renome passou a ter regra
única: **o emissor renomeia onde o parser deixou uma ilha, e em nenhum outro lugar** — o parser
é quem conhece os escopos, e passou a registrar local e parâmetro declarados com tipo C, e a ler
como local o que tem o nome de um tipo do módulo. O `symbol-redeclaration` da spec §2.5 passou a
disparar quando o tipo é palavra-chave do C. Caso novo **032-defer-jump** (golden à mão, emissão
pendente): `goto` saindo de escopo e `defer` de bloco interno.

Dois goldens estavam errados e foram corrigidos à mão contra o `.k` e as regras: 010/031 (sem o
`.state = {0}`) e 016 (escada onde a regra pede inline, e sem o `#line` do cleanup).

Decisões do André nesta sessão: inicializador previsto no backend é implementação, não decisão;
texto de normativo é **só especificação** (justificativa ao rationale, caso específico em regra
separada e a evitar); o emissor para de renomear local e parâmetro, com diagnóstico no
sombreamento. Em aberto: a regra do `defer` de bloco interno (a escada por bloco não existe sem
marca em runtime; o texto proposto é "sai inline em cada saída do bloco"). O estado detalhado
está na última seção de `tools/agents/passagem-2026-10-28.md`.

**Atualização (2026-11-01, à noite):** **29 dos 32 casos do golden saem do cgen
byte a byte** (`gen_golden` 38 ok, 0 wip). Fechou o 032: `defer` com `goto`
saindo de escopo, e `defer` de bloco interno numa função cuja escada é do
escopo da função. `emit/defer.c` passou a decidir o modo (`RET_NONE`/
`RET_INLINE`/`RET_LADDER`) pelos `defer` do **próprio escopo da função**, e não
mais da função inteira; um `defer` de bloco interno é sempre inline, em toda
saída desse bloco (backend §5.5.2 r6, decisão do André: a escada por bloco não
existe sem marca em runtime). `goto` deixou de ser recusa geral.

Lição registrada: ao tocar `defer.c`, a varredura de saídas corria em toda
função mesmo sem `defer` — e tropeçou no `break` que `match` e `parallel` já
tratam do jeito dele, derrubando três casos que fechavam (009, 012, 029). A
correção foi guardar a varredura com "há `defer` nesta função" antes de entrar.
Rodar `gen_golden` inteiro ao tocar um arquivo de construção, não só o caso
novo: a cobertura de regressão são os outros casos da tabela.

**Atualização (2026-11-02):** **32 dos 35 casos** saem do cgen (só 022–024 fora); os 9 `debug`
do catálogo têm teste (casos 009, 026 e 033); entraram `#line` da coluna nos `KEEL_CHECK` do
`extent`, `priv tags` no `.c` (034), `match` sobre coluna de `extent` (035),
`--parallel-lowering=serie` (oráculo em `test/cgen/gen/serie/`) e a fase do `cc` com depfile
(`tool/run_cc.c`, `cli/cc_phase.sh`). Abertos, com proposta e sem "sim": `priv tags`/`extent priv`
contra D29, `-c` com `--main` e `--instance` sem `--stop-after` (um `cc` por unidade),
`constexpr` multilinha, e `defer { }`/multilinha em `defer.c`. Detalhe na última seção da
passagem.

**Atualização (2026-11-02, noite):** **41 dos 44 casos** saem do cgen (só 022–024 fora). O levantamento de
28 formas recusadas (`tools/agents/survey/`) fechou inteiro, em sete cartões ao Haiku (T11–T17), com
oráculos à mão (casos 038–044). M4 feito (`cc`, depfile, `--instance`, `-f` grava sempre). Normativo sem
pendência. Próximo: `linux-list` (022–024). Detalhe: última seção da passagem e `levantamento-2026-11-02.md`.

**Atualização (2026-11-02, noite):** `emit.c` repartido em `emit/{lines,files,decls,generic,supported}.c` (T36, snapshot e suíte iguais); falta o redesenho do `supported()` e o corte do `uses.c`/`defer.c`. **54 dos 57 casos** saem do cgen (só 022–024 fora), e o `gen_golden` agora também
compila o C escrito. Levantamentos 2 e 3 fechados; oráculos 054–058 escritos por Haiku (piloto, skill haiku-task);
cartões T18–T35. Pendente do André: instância de módulo genérico (`g01`) e `priv inline` em genérico (`g02`).
Próximo: refatoração do `emit.c` por partes. Detalhe: última seção da passagem.

**Próximos passos (2026-11-02):** ver a seção "Próximos passos" no fim de `tools/agents/passagem-2026-10-28.md` (decisões do
André sobre genérico/`priv inline`, `linux-list`, `supported()` por construção e o corte de `uses.c`/`defer.c`, itens
pequenos, piloto de oráculo por Haiku no `AGENTS.md`, rodar de novo o levantamento 3 antes dos testes de estresse).

**Atualização (2026-11-04):** genérico que cita só parâmetro (`g01`→059) e `priv inline` em
genérico (`g02`→060) fechados: backend §4.3 r1 (a unidade de header é a instância do **módulo**,
não do modificador — resolve a colisão com o golden 010) e r6 (`priv inline` sai `static inline`
no `.h` da instância). **022 e 023 do `linux-list` fechados** (`import_c` e `extern_c [type_h]`):
achado e corrigido um bug real — o conteúdo de uma diretiva de topo fora de `extern_c` ia para o
`.type.h`, e a spec (§4.1 r13) manda para o `.h`; e o conteúdo de `extern_c` estava sendo
branqueado nos comentários, quando devia atravessar intacto. `k_em_emit_opaque_range` (lines.c)
cobre as duas formas, e `COPIED_SPAN` no `layout-check.py` para de tentar normalizar linha em
branco de texto copiado de verdade.

**024 (linux-list por módulo), em andamento — achado bloqueador:** um campo de struct que
referencia um tipo de **outro** módulo (`typedef struct { ... struct list_head link; ... } task;`,
com `list_head` de `linux.list`) não é renomeado — fica `struct list_head`, e o C não compila.
Confirmado que o mecanismo de renome cross-módulo **funciona** em geral (`struct Foo f;` como
variável local vira `struct a_m_Foo f;` certinho) — só falha dentro do corpo de uma struct. Causa:
`isl_declaration` (que resolve o tipo e emite a ilha `K_ISLAND_TYPE`) só é chamada em
`parser_islands_flow.c`/`parser_islands_index.c`, para declaração de variável/parâmetro/retorno;
nenhum lugar percorre `K_AST_TYPE.body_first..body_end` para aplicar o mesmo tratamento ao
especificador de um **campo**. Contraria a spec (§4.2: "`tagged-type` faz da declaração uma
declaração keel quando o nome foi declarado em keel — struct, union, enum ou extent do módulo
**ou de um import**"). Nenhum caso do golden testava `struct` cruzando módulo antes do 024.
André pediu para guardar e retomar depois, sem consertar agora — é lacuna de reconhecimento
(mais código, mais superfície), diferente dos dois fixes de roteamento acima. `golden/cases/024-linux-list-module/{linux/list.k,tasks.k}`
têm o resto do caso já encaminhado (26 `static inline`→`pub inline`, os ramos `CONFIG_DEBUG_LIST`
retirados — colidiam o nome canônico —, `import ... types` para a struct), commit `f15ebe2`; falta
o `expected/` dos dois perfis, que depende do fix. Segue `|wip`, fora do oráculo.

**Atualização (2026-11-05):** **024 fechado, os 59 casos saem do cgen** (golden 120 ok, 0 WIP).
Corrigido o achado bloqueador: `isl_struct_fields` agora lê `struct/union/enum T f;` pelo `T`
(antes só um especificador de uma palavra). `expected/` do 024 nos dois perfis, `VERIFY` gnu11/gnu2x.
Achado em aberto, de normativo: um `pub inline` que só usa macro do módulo importado não leva o
`#include` do `.h` do import (o cgen não vê chamada); no caso, `next` ficou fora de linha. Falta
`supported()` por construção e o corte de `uses.c`/`defer.c`.

**Atualização (2026-11-05, tarde):** diretiva de topo volta ao `.type.h` (linguagem §4.1 r13, backend §4.1 e
§4.3.2 r8/r11, rationale), por decisão do André: portar C exige só `module nome;`, a diretiva precede o tipo
que a usa, e é a única camada que a instância de um genérico alcança (era o bug do `string.h` do `keel.buffer`,
as 8 falhas de `base.sh` que vinham do `5fc7a6d`). Macros são globais e invisíveis ao keel (spec §4.1 r16).
`test/run.sh` verde por inteiro pela primeira vez desde o `5fc7a6d` (gen_golden 69, base 124, golden 120, docs 45).
024 sem `import_c` a mais; `layout-check.py` ganhou o corpo Allman. **Pendência registrada (backend §4.1 r8):**
`#if/#ifdef` de topo que abrange tipo e função fica dividido entre as camadas; replicar o esqueleto nos dois
headers é a saída, e foi adiada. Falta `supported()` por construção e o corte de `uses.c`/`defer.c`.

**Atualização (2026-11-05, noite):** grupos condicionais (spec §2.4) garantidos na implementação. Achados: os
scanners de extensão (`k_scan_braced_opaque`, `k_scan_opaque_until`, `balanced` de `ast.c`) somavam as chaves de
todas as alternativas em linha reta, e `#  else` com espaços não era `#else` no `k_check_delimiters`; limite de
grupos 32→64. Agora `KGroupDepth` (`engine/lexer/group_depth.c`) conta por alternativa. Testes: `diag/delims2`
(aninhado, cadeia, adjacente, 63 níveis) e o golden 061 (60 casos). **Ainda em linha reta** (pendente): os
pareadores da análise de ilhas (`isl_close_paren`, `block_end`, `isl_enclosing_brace`) e um `}` que fecha o corpo
dentro de um grupo. `test/run.sh` verde (gen_golden 71, base 124, golden 122, docs 45).
