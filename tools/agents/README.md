# agents — tarefas delegadas a subagentes, verificadas

A partir de 2026-10-22, o fluxo do cgen é este: **o Opus orquestra** (planeja,
escreve o cartão e o oráculo, revisa o diff, integra, commita) e **os Haiku
implementam** (um cartão por agente, com a verificação feita pelo próprio
agente antes de reportar). O harness anterior, com modelo local
(`tools/harness/`, qwen via `loop.py`), foi aposentado nesta data. O que se
aprendeu com ele e continua valendo está na skill
[`haiku-task`](../../.claude/skills/haiku-task/SKILL.md).

O Haiku é bem mais capaz que o modelo local. Por isso o cartão pode delegar
tarefas maiores (um arquivo inteiro, um refactor por partes, uma auditoria),
e o agente executa mais verificações por conta própria. O que não se delega
continua igual: decisões de normativo, e o oráculo de uma tarefa de
implementação, que é escrito à mão por quem orquestra e nunca derivado da
ferramenta.

## Layout

| Caminho | O quê |
| --- | --- |
| `verify.sh` | a aceitação comum: build (sem aviso novo além de `warnings.baseline`), `make -C tools/cgen check`, `golden/run.sh`; e o snapshot de comportamento para refactor (`snapshot DIR` antes, `compare DIR` depois) |
| `warnings.baseline` | quantos avisos o build limpo tem hoje; uma tarefa que remove aviso baixa o número, nenhuma sobe |
| `tasks/<id>.md` | um cartão por tarefa: objetivo, escopo de arquivos, contrato, aceitação, formato do relatório |
| `reports/<id>.md` | o relatório que o agente escreve ao terminar |
| `plano-<data>.md` | o plano da sessão que distribuiu as tarefas |

## Regras que todo cartão herda

1. O agente só toca nos arquivos que o cartão lista. Não edita normativos
   (`keel-spec.md`, `keel-rationale.md`, `keel-c-backend.md`,
   `cgen-tool-spec.md`), não edita `golden/*/expected` nem `golden/c11|c23`,
   e não commita: quem commita é o orquestrador, depois de revisar.
2. Código, comentários e mensagens em inglês; prosa de documento e de
   relatório em pt-BR.
3. Achou incoerência entre o cartão, o código e os normativos? Para e
   registra no relatório. Não decide sozinho.
4. A tarefa só termina com `sh tools/agents/verify.sh` (ou `compare`, em
   refactor) imprimindo `ALL OK`, e com a saída colada no relatório.
