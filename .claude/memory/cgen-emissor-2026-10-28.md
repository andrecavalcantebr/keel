---
name: cgen-emissor-2026-10-28
description: estado do emissor do cgen (M3 e parte do M6) em 2026-10-28, o fluxo com Haiku em worktree e o próximo passo (M5, a base)
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
do runner (R2, `DEBUG`, caso 026); M6 com `gen_golden` 23 ok, 0 wip. Próximo: o 010 (`routine`,
instância cuja unidade é o módulo instanciado) — plano na última seção de
`tools/agents/passagem-2026-10-28.md`. Lição: o golden às vezes tem alinhamento/ordem feitos à mão;
corrige-se conferindo contra o `.k` e a regra, nunca copiando a saída da ferramenta.
