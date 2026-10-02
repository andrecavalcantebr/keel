---
name: cgen-implementacao
description: cgen — os 5 designs fechados em 2026-09-20; M0 e M1 fechados, parser (M2) até as ilhas 4a–4h; tool/ e engine/ no fonte
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

**Estado em 2026-10-02:** o M0 (driver) e o M1 (lexer) estão fechados. O M2
(parser) reconhece módulos, imports, declarações (inclusive `decl-protocol`,
papéis e `typedef … byref`) e as ilhas das etapas 4a–4h
(`--stop-after=parse`, com `-o` no sentido do gcc). Há 20 casos de falha em
`tools/cgen/test/diag/`, e o catálogo de diagnósticos é gerado da spec §6.2
(`gen-diags.py`), com as mensagens escritas em `diag.c` à medida que o motor
passa a emitir cada id. Ainda não há emissão de C. O detalhe por etapa está em
`tools/cgen/src/engine/README.md`; o alinhamento com a revisão de 2026-10-02,
em [[protocolos-estruturais-2026-10-02]].

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

1. **Etapa 4i do parser** (`parser-design.md` §3.2): o fecho de instâncias.
   As 4e, 4g e 4h saíram em 2026-10-02, com as espécies `parallel`,
   `worker-exit`, `else`, `extent` e `column` (formato em `cgen-tool.md` §5.2,
   a revisar com o André).
2. **Análise de papéis** (spec §4.12: `region-escape`,
   `child-region-after-invalidation`) e o parâmetro de protocolo fora da
   primeira posição.
3. **O M3 em diante** (`cgen-tool.md` §9): emissão sem ilhas e depois com elas.
4. **Os casos 022–024 do golden** (o `list.h` do Linux, GPLv2-only) continuam
   `WIP`: falta `expected/` e `VERIFY`. A P16, de que 023 e 024 dependiam, foi
   resolvida em 2026-09-21 (`cgen-tool.md` §13).

Decisões ainda abertas: `buffer.of(v)` como nome que o núcleo conhece, `byref`
em `typedef` e o `{0}` da arena, e o prefixo das instâncias aninhadas.

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
