# keel — contexto para agentes

keel é um parser de ilhas: ilhas de keel num mar de C. O `cgen` transpila `.k`
para C11/C23. O `README.md` apresenta o projeto.

## Antes de começar

- **A memória do projeto está em [`.claude/memory/`](.claude/memory/MEMORY.md).**
  Leia o `MEMORY.md` primeiro: é o índice, e cada linha aponta para um fato.
  A cópia foi feita em 2026-09-21, a partir da memória local do Claude Code; o
  que diverge do repositório vale menos que o repositório.
- **Nenhuma pendência em aberto nos normativos.** P16 (visibilidade do C que
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
