# T37 — oráculo: constante e tipo de módulo genérico que citam só parâmetro

## Objetivo

Você é **autor de oráculo** (piloto, `.claude/skills/haiku-task/SKILL.md`, seção "Oracle-author cards"). Escreva **à mão** o caso `golden/cases/059-generic-param-only/`: `.k`, `proof.c`, `NOTES`, `expected/c23/` e `expected/c11/`. **Não rode `tools/cgen/cgen` neste caso e não leia as fontes do emissor** (`tools/cgen/src`): isso capturaria a saída da própria ferramenta. Leia a spec e os goldens análogos.

## Regra a cobrir (backend §4.3, regra 1, recém-escrita — leia-a em `keel-c-backend.md`)

A unidade de header é a **instância do módulo** (módulo + argumentos), não a do modificador. Uma declaração que cita só parâmetro (sem modificador) — `constexpr` ou `typedef` — sai no **mesmo par `.type.h`/`.h`** que o modificador, com o nome manglado pelos argumentos. Hoje o cgen recusa isso (`implementation-limit`).

## A forma

Um módulo genérico `app.pbox` com `dim N type T`:

```keel
module app.pbox dim N type T;
pub constexpr size_t CAP = N;                 // cita só N
pub typedef struct { T first; T last; } Pair; // cita só T
pub modifier box byref { T v[N]; }
pub inline size_t cap(box *b) { return CAP; }
pub inline Pair ends(box *b) { Pair p = { b->v[0], b->v[N - 1] }; return p; }
```

Ajuste os detalhes se a sintaxe real pedir (veja `golden/cases/006-dim`, `007-instance`, `010-routine` para `dim`, uso do módulo e o nome do header de instância; o header se chama `<módulo>_<args>`, como `keel_routine_ag2_Ag.type.h`; no §2.1 do backend está o mangling de `dim`). Um módulo cliente `app.pbox_use` (ou outro nome — o caminho do arquivo segue o `module`) instancia `box(3) i32`, usa `cap` e `ends` e expõe por `pub` o que o `proof.c` observa (`cap == 3`, extremos de um `box` preenchido). O módulo do caso é o cliente; o genérico é importado, então tem só `.type.h` e `.h` da instância (mais o par do módulo, se o backend §4.4 r5 o mandar, mesmo vazio).

## Arquivos

Crie só `golden/cases/059-generic-param-only/**`, mais a linha `059-generic-param-only|<fonte do módulo cliente>|-|wip` em `test/cgen/gen/golden-cases.txt` (copie o formato das linhas vizinhas), e linhas em `golden/line-check.allow` só se for código injetado. Não toque normativos, outros casos, nem `tools/cgen/src`.

## Aceitação

`golden/run.sh` — o caso novo aparece `ok` nos dois perfis (compila, roda o `proof.c`, passa layout-check e line-check) e o resumo não tem `fail`. Se o runner exigir que o caso exista na lista de WIP, siga `golden/README.md`. O `expected` deve seguir a spec, nunca "o que parece mais fácil". Use `#line` como os goldens vizinhos.

## Relatório

`tools/agents/reports/T37-oracle-generic-param-only.md`, pt-BR: decisões de sintaxe, nome dos arquivos gerados, a saída do `golden/run.sh`, e **toda dúvida** em que a spec não decide (não adivinhe: escreva a pergunta). Entregue UMA vez e termine. Não faça commit nem `git add`.
