# keel — contexto para agentes

keel é um parser de ilhas: ilhas de keel num mar de C. O `cgen` transpila `.k`
para C11/C23. O `README.md` apresenta o projeto.

Este arquivo é o contexto canônico de qualquer agente. O `CLAUDE.md` só aponta
para cá.

## Antes de começar

- **A memória do projeto está em [`.claude/memory/`](.claude/memory/MEMORY.md).**
  Leia o `MEMORY.md` primeiro: é o índice, e cada linha aponta para um fato.
  A cópia foi feita em 2026-09-21, a partir da memória local do Claude Code, e
  atualizada em 2026-11-05 (os 59 casos do golden saem do cgen). O
  que diverge do repositório vale menos que o repositório.
- **Nenhuma pendência de normativo em aberto** desde 2026-11-06 (a última
  leva, a regra 20 da §4.4 e o diagnóstico `type-arg-foreign` — argumento
  de tipo apagado que precisa de renome, achado investigando `../keel_maths`
  — foi escrita com o "sim" do André; T40-T44 em
  `.claude/memory/cgen-pendencias.md`). P16 (visibilidade do C que
  atravessa um módulo) foi resolvida em 2026-09-21; o registro está em
  `design/cgen-tool.md` §13, e o briefing que a levantou, em
  [`.claude/p16-visibilidade-do-c.md`](.claude/p16-visibilidade-do-c.md), fica
  como histórico.

## Regras de trabalho

- **Discuta antes de editar os normativos** (`keel-spec.md`,
  `keel-rationale.md`, `keel-c-backend.md`, `cgen-tool-spec.md`): proponha o
  texto e o argumento, e só escreva depois do "sim" do André.
- **A prosa dos documentos é em pt-BR, e o código é em inglês**: nomes,
  comentários, ids de diagnóstico e mensagens do cgen, inclusive o código dentro
  dos documentos.
- **Os perfis c11 e c23 do golden são escritos à mão**, nunca derivados um do
  outro. "A emissão segue o `.k` sempre."
- `golden/run.sh` é o oráculo, e tem de ficar verde. Caso em construção leva
  `WIP` (ver `golden/README.md`).
- Trabalho local vai direto no `master`, com um commit local depois de cada
  alteração. Os dois oráculos (`make -C tools/cgen check` e `golden/run.sh`,
  juntos em `test/run.sh`) rodam antes de cada commit. Os testes do cgen
  moram em `test/cgen/`, ao lado do `golden/`.

## Fluxo de trabalho (desde 2026-10-22)

- **O Opus 5.5 orquestra:** lê, planeja, escreve os cartões de tarefa e os
  oráculos, revisa o diff de cada agente, integra e commita.
- **Os Haiku 5.5 implementam:** cada um recebe um cartão em
  `tools/agents/tasks/` e só reporta depois de passar a própria verificação
  (`tools/agents/verify.sh`). Como o Haiku é mais capaz que o modelo local de
  antes, o cartão pode delegar tarefas maiores (um arquivo inteiro, um
  refactor por partes, uma auditoria) e pedir mais verificações ao próprio
  agente.
- O modelo local (qwen, `tools/harness/`) não é mais usado: o harness foi
  retirado em 2026-10-22, e o que ele ensinou e continua valendo está na skill
  [`haiku-task`](.claude/skills/haiku-task/SKILL.md).
- Não se delega a decisão sobre normativo, nem o oráculo de uma tarefa de
  implementação: o oráculo é escrito à mão, nunca capturado da ferramenta.
- O detalhe está em [`tools/agents/README.md`](tools/agents/README.md).
