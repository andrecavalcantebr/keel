---
name: cgen-implementacao
description: cgen — os 5 designs fechados em 2026-09-20; a implementação não começou; tool/ e engine/ no fonte
metadata: 
  node_type: memory
  type: project
  originSessionId: eaa22aed-ef9c-4f55-828b-7c4fe65d59ac
  modified: 2026-09-21T02:23:43.500Z
---

Estado em **2026-09-20** (tag `v0`, commit `6c9ead8`). O histórico de como se
chegou aqui está no git; isto é onde as coisas param.

## O que existe

Os cinco designs em `design/`, um por peça de `cgen-tool.md` §3.1:

| | Cobre |
| --- | --- |
| `cgen-tool.md` | o de cima: CLI, arquitetura, fases, bootstrap. Decisões D1–D8 |
| `lexer-design.md` | `engine/lexer.c` |
| `parser-design.md` | `engine/parser.c`, `symtab.c`. Decisões P1–P5 |
| `codegen-design.md` | `engine/emit/`. Decisões E1–E5 |
| `diag-design.md` | `engine/diag.c`, `tool/report.c`. Decisões G1–G4 |

**A implementação não começou.** `tools/cgen/src/tool/main.c` tem 23 linhas e
imprime uma string.

## As duas decisões de arquitetura de 2026-09-20

**[D8] O fonte se parte em `tool/` e `engine/`, num executável só.** Nada em
`engine/` abre arquivo, escreve ou termina o processo. **O `make boundary`
verifica isso** e roda junto com `all` — recusa `<stdio.h>`, `<stdlib.h>`,
`<unistd.h>`, `<fcntl.h>` e `<sys/*.h>` em `engine/`. Testado nos dois
sentidos. O que compra: o motor é testável sem sistema de arquivos.

**[E5] Um `.c` por funcionalidade em `engine/emit/`**, mais cinco de mecânica
comum (`writer`, `mangle`, `layer`, `instance`, e o orquestrador). A fronteira
que dá sentido ao corte é o `writer.c`: nenhum arquivo de construção escreve
texto direto no buffer, todos passam pelo `KWriter`, que é o único que conhece
os contadores de linha (decisão E4).

## O que falta, na ordem

1. **Os casos de falha do golden.** Hoje são **zero** — o runner conta `xfail`
   e nunca viu um. O molde é `diag-design.md` §7 [G3]: um caso agrupa vários
   diagnósticos, e o `VERIFY` compara só o **conjunto de ids** lidos dos
   marcadores `/* DIAG: id */` do fonte contra os `[id]` da saída.
2. **Casos de C simples e complexo**, pedido do André: código que ninguém
   escreveu pensando em keel, para testar a premissa do parser de ilhas.
   Primeiro complexo em andamento (2026-09-21): o `list.h` do Linux em três
   casos, `golden/cases/022-024-linux-list-*` (import_c, extern_c, módulo),
   mesmo `main`, listagem por `walk` e por `list_for_each_entry`. GPLv2-only
   (linha própria no LICENSE.md). Marcados `WIP` (marcador novo do runner):
   `expected/` e `VERIFY` (gnu11/gnu2x sem -pedantic) por fazer; 023 e 024
   esperam a **P16** (visibilidade do C no módulo, cgen-tool.md §13 — André
   inclina a "em C tudo é público").
3. **O M0 do `cgen-tool.md` §9** — o driver.
4. **As mensagens de diagnóstico** (133 ids, nenhuma escrita) — escritas junto
   com o `diag.c`, não antes. **Não travam os casos de falha** (correção do
   André, 2026-09-21): a mensagem é molde que depende do que o programa
   escreveu (nomes, aridade, outro participante), linha/coluna vêm do formato
   de impressão, e o id kebab-case já diz do que se trata; o G3 nunca compara
   texto. Guia: `diag-design.md` §4 [G2] e as três famílias do §5.

## Pendências registradas nos designs

- `codegen` C1: a ordem das cláusulas do `#pragma` não está sob teste.
- `codegen` C2: o fecho de instâncias da base foi calculado à mão; o emissor
  tem que dar exatamente os mesmos 7 arquivos.
- `parser` Q1, Q2.
- `diag` R1: a severidade `info` sai por padrão? Candidato é `-W<nome>`.
- `diag` R2: o runner não tem modo para caso `debug` (compilar, rodar sob
  `--checks=on`, afirmar o `abort`). Os 7 ficam sem teste.

Ver [[base-real-keel-em-progresso]], [[golden-perfis-autorais]],
[[discutir-antes-de-editar-spec]].
