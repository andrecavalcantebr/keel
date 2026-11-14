# keel — contexto para agentes

keel é um parser de ilhas: ilhas de keel num mar de C. O `cgen` transpila `.k`
para C11/C23. O `README.md` apresenta o projeto.

Este arquivo é o contexto canônico de qualquer agente. O `CLAUDE.md` só aponta
para cá.

## Antes de começar

- **A memória do projeto está em [`.claude/memory/`](.claude/memory/MEMORY.md).**
  Leia o `MEMORY.md` primeiro: é o índice, e cada linha aponta para um fato.
  A cópia foi feita em 2026-09-21, a partir da memória local do Claude Code, e
  atualizada em 2026-11-07 (os 67 casos do golden saem do cgen: os dois
  novos, 067-068, vêm de dois bugs achados avaliando um uso real em
  `../keel_libs`). Desde então o golden chegou a 69 casos (070 é o último) e a
  cópia ficou defasada: a memória local do Claude Code tem entradas mais
  novas. O que diverge do repositório vale menos que o repositório.
- **Estado (2026-11-14): a 0.2.0 está publicada** (tag `v0.2.0`, `master`). Não
  há pendência de normativo em aberto. O que mudou nos normativos, por versão,
  está no [`CHANGELOG.md`](CHANGELOG.md), e o que fica depois está no
  [`ROADMAP.md`](ROADMAP.md). Os achados das bibliotecas entram por
  [`achados/README.md`](achados/README.md). O histórico das pendências do cgen
  (T40-T44 e outras) está em `.claude/memory/cgen-pendencias.md`; P16
  (visibilidade do C que atravessa um módulo) foi resolvida em 2026-09-21, com o
  registro em `design/cgen-tool.md` §13 e o briefing em
  [`.claude/p16-visibilidade-do-c.md`](.claude/p16-visibilidade-do-c.md).
- **Branches.** `master` é o que a última release prometeu: só recebe
  documentação não normativa que valha para a versão corrente e o merge da
  próxima release. O trabalho rotineiro vai na branch de longa duração da
  versão seguinte, hoje a `0.3.0`. Antes de commitar, `git status -sb`.

## Regras de trabalho

- **Discuta antes de editar os normativos** (`keel-spec.md`,
  `keel-rationale.md`, `keel-c-backend.md`, `cgen-tool-spec.md`): proponha o
  texto e o argumento, e só escreva depois do "sim" do André.
- **Os normativos são versionados (desde 2026-11-14).** Cada um traz a versão
  que descreve no cabeçalho. Toda alteração normativa leva a marca
  *(desde X.Y.Z)* na regra, no diagnóstico ou na opção que entrou ou mudou, com
  X.Y.Z a versão em que a mudança sai, e uma linha no `CHANGELOG.md`. O que
  existia na 0.1.2 não leva marca. Quando a versão sobe, o número muda no
  cabeçalho dos quatro documentos, no manual, no `main.c` e em
  `test/cgen/cli/args_partition.sh`.
- **Os achados das bibliotecas entram como relatório de bug**, e não como
  pedido de módulo (desde 2026-11-14): só valem se reproduzirem como bug do
  cgen ou lacuna da spec, com repro mínimo. O fluxo e o registro estão em
  [`achados/README.md`](achados/README.md). A linguagem está fechada, salvo
  bug ou erro de spec; a "stdlib" da v1 é só um conjunto de casos para depurá-la.
- **A prosa dos documentos é em pt-BR, e o código é em inglês**: nomes,
  comentários, ids de diagnóstico e mensagens do cgen, inclusive o código dentro
  dos documentos.
- **Os perfis c11 e c23 do golden são escritos à mão**, nunca derivados um do
  outro. "A emissão segue o `.k` sempre."
- `golden/run.sh` é o oráculo, e tem de ficar verde. Caso em construção leva
  `WIP` (ver `golden/README.md`).
- Trabalho local vai na branch da versão seguinte (hoje `0.3.0`, ver
  "Branches" acima), com um commit local depois de cada alteração. Os dois oráculos (`make -C tools/cgen check` e `golden/run.sh`,
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
