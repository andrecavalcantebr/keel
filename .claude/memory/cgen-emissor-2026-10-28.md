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
