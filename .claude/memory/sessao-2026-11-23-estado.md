---
name: sessao-2026-11-23-estado
description: "Estado exato ao suspender a sessão de 2026-11-23 (0.4.0): o que está commitado, o que NÃO está, e a fila de pendências para retomar"
metadata:
  node_type: memory
  type: project
  originSessionId: a30bc17f-45e1-4b2d-8fa8-c5040b5a7d64
  modified: 2026-11-23T22:48:18.253Z
---

Branch `0.4.0` (cgen `0.4.0-dev`). Decisões de design em [[decisoes-v1-mais-2026-11-23]]. Sessão suspensa a pedido do André, antes de commitar e fazer push.

## Teto de nome 215: commitado (ba9e195, 2026-11-23)
Suíte completa isolada: 164 ok, 0 fail; docs 45 ok. Push da 0.4.0 feito em seguida (ver git log). A cópia de memória em `.claude/memory/` recebeu este arquivo e `decisoes-v1-mais-2026-11-23.md`.

## Já commitado nesta sessão (0.4.0, ainda sem push)
`85a387d` `x[i, j]` sobre modificador com `dim` (golden 082, diag `index-rank`); `e27223d` LINE_CAP no cgen-tool-spec §7.1; `20278a0` tabela de limites (manual §7.8, llm-context); `f69921e`/`174570c` estudos E01 e E02 + registro; `c11a693` medição de limites + E03; `0bd6f3c` `extent-limit-exceeded` (>4 grupos, antes saía sem diagnóstico); `506216d` A1-A3 do `keel_libs/keel_achados.md` registrados. Relatórios dos Haiku em `tools/agents/reports/limites-{auditoria,medicao}.md`.

## Fila de pendências (todas na 0.4.0, por pedido do André)
1. (feito) Commit + push do 215.
2. **Intervalos** `..<` / `..=` / `keel_inclusive`: o André aprovou o desenho, mas só disse "sim" ao 215; propor de novo o texto (spec §4.5, backend §5.3/§5.17, rationale) e esperar o "sim"; depois golden (esperado à mão, `|wip` em `test/cgen/gen/golden-cases.txt`) e implementação no parser/emissor.
3. **`pub import`** (por módulo, alias de fachada; busca relativa à fachada, `-I` só da fachada) e depois **handles** (índice geracional sobre `pool`). O André quer discutir os dois logo depois do commit.
4. **Bugs de `keel_libs/keel_achados.md`**: A1 (`outcome T` com `T` typedef de outro módulo: o `.type.h` não inclui o módulo de `T`; repro `/tmp/ka2/r1`, pode não existir mais: está inteiro no `keel_achados.md`) e A3 (verbo de módulo genérico com 2 modificadores sai `two_run`); fazer caso no golden **antes** do conserto. A2 (`typedef buffer u8 text` passa como C cru) é pergunta de spec: recusar com diagnóstico?
5. (E02 e a parte de mensagens do E03 feitos em 2026-11-23, teste cli/limit_messages; restam tempo quadrático, arena, cobertura de `pub struct S {..} s0;` e protótipo sem corpo) **E02** (mensagem única do `implementation-limit`, trocar por `implementation limit: %s` com frase por ponto) e **E03 restante** (mensagens enganosas de match/protocol/defer/parallel; tempo quadrático por linhas: 1000 funções 0,5 s, 4000 7 s, 10 000 42 s, fazer `perf` antes de supor a causa; `pub struct S {..} s0;` e protótipo sem corpo recusados como "não coberto": lacuna da spec ou do emissor?). Decidir se entram na 0.4.0.
6. **Arena 64 MiB**: subir a arena ou baixar os tetos (vários nunca disparam: ~400 módulos, ~10 000 funções). O André acha que vai ser preciso subir.
7. **Comparador como eixo (B3/B6)**: só depois dos ajustes.
8. Aviso `docs: could not read the version of keel-manual.md or of the cgen` persiste (manual sem versão legível); resolve-se no corte da 0.4.0.
9. Cortar a 0.4.0 quando fechar: números nos cabeçalhos dos 4 normativos, manual, `main.c`, `test/cgen/cli/args_partition.sh`; CHANGELOG deixa de dizer "em andamento"; merge no `master`.

**Why:** o André suspendeu o notebook no meio do ciclo; esta lista é para retomar em outro chat sem perda.
**How to apply:** ler primeiro este arquivo e [[decisoes-v1-mais-2026-11-23]]; conferir `git status -sb` e `git log --oneline -12` antes de agir.
