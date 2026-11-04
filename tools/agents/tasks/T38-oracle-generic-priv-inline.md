# T38 — oráculo: `priv inline` em módulo genérico

## Objetivo

Você é **autor de oráculo** (piloto, `.claude/skills/haiku-task/SKILL.md`, seção "Oracle-author cards"). Escreva **à mão** o caso `golden/cases/060-generic-priv-inline/`: `.k`, `proof.c`, `NOTES`, `expected/c23/` e `expected/c11/`. **Não rode `tools/cgen/cgen` neste caso e não leia as fontes do emissor** (`tools/cgen/src`). Leia a spec e os goldens análogos.

## Regra a cobrir (backend §4.3, regra 6, recém-escrita — leia-a em `keel-c-backend.md`)

A função `priv inline` de módulo genérico sai `static inline` no `.h` da instância, com o **protótipo na seção 2 e o corpo na seção 4** (§4.3.2, regra 3). O módulo genérico não tem `.c`. O nome manglado leva o prefixo do módulo; a privacidade é por nome. Hoje o cgen recusa a forma.

## A forma

```keel
module app.peek type T;
pub modifier box byref { T v; }
priv inline T peek(box *b) { return b->v; }
pub inline T get(box *b) { return peek(b); }
```

Ajuste se a sintaxe real pedir (veja `golden/cases/007-instance` para módulo genérico do usuário e seu cliente; `golden/cases/010-routine` e `/base` para o nome do header `<módulo>_<args>` e o mangling). Um cliente `app.peek_use` instancia `box i32`, chama `get`, e expõe por `pub` o que o `proof.c` observa. O `proof.c` toca só a interface pública (não chama `peek`). Cubra também `box` de um segundo argumento (por exemplo `box i64`) para mostrar que `peek` sai uma vez por instância, com nome distinto. Pergunta que você deve decidir pela spec e registrar: o nome C de `peek` (`app_peek_i32_peek`? — siga o mangling do §2.1 do backend, e o dos verbos `pub` vizinhos).

## Arquivos

Crie só `golden/cases/060-generic-priv-inline/**`, mais a linha `060-generic-priv-inline|<fonte do módulo cliente>|-|wip` em `test/cgen/gen/golden-cases.txt` (formato das linhas vizinhas), e `golden/line-check.allow` só se for código injetado. Não toque normativos, outros casos, nem `tools/cgen/src`.

## Aceitação

`golden/run.sh` — o caso novo `ok` nos dois perfis e o resumo sem `fail`.

## Relatório

`tools/agents/reports/T38-oracle-generic-priv-inline.md`, pt-BR: decisões, nomes gerados, saída do `golden/run.sh`, e toda dúvida em que a spec não decide. Entregue UMA vez e termine. Não faça commit nem `git add`.
