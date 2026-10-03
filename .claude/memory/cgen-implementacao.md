---
name: cgen-implementacao
description: cgen — os 5 designs fechados em 2026-09-20; M0 e M1 fechados, parser (M2) completo em 2026-10-03; falta a emissão (M3+)
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

**Estado em 2026-10-03:** M0 (driver) e M1 (lexer) fechados; o M2 (parser)
está completo: toda a gramática da §2.2 (inclusive `instance`), as ilhas das
etapas 4a–4i, o fecho de instâncias como dado (`KAst.closure`, semeado também
pelas chamadas de `keel.array` e de função sobre protocolo), a superfície
degenerada, a análise de papéis (§4.12) e os diagnósticos das três passagens
— 136 dos 148 do catálogo com caso de falha (43 casos em
`tools/cgen/test/diag/`). `info` sai por padrão, só do módulo traduzido.
Ainda não há emissão de C. O detalhe está em `tools/cgen/src/engine/README.md`.

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

1. **O M3 em diante** (`cgen-tool.md` §9): emissão sem ilhas, depois com elas,
   usando `KAst.closure` e as ilhas.
2. **`dim-generates-declaration`**: o André precisa dizer o que ele recusa (a
   leitura de `#if N` exigiria olhar dentro de diretiva, o que ele descartou).
3. **Revisão do André** dos formatos novos do dump: espécies `parallel`,
   `worker-exit`, `else`, `extent`, `column`, e as linhas `closure`,
   `unavailable`, `decl instance`.
4. **Os casos 022–024 do golden** (o `list.h` do Linux) continuam `WIP`.
5. Fora do parser: os sete de molde (v1), `instance-depth`,
   `specific-format-unavailable` (backend) e `indirect-import`, que pede
   resolução por import transitivo.

Decisões ainda abertas: `buffer.of(v)` como nome que o núcleo conhece, `byref`
em `typedef` e o `{0}` da arena, e o prefixo das instâncias aninhadas.

## Pendências registradas nos designs

- `codegen` C1: a ordem das cláusulas do `#pragma` não está sob teste.
- `codegen` C2 resolvida (2026-10-02): o fecho calculado bate com os headers
  de instância do golden; `clone` copia por `memcpy((void *)data, …)`.
- `parser` Q1, Q2.
- `diag` R1 resolvida (2026-10-02): `info` sai por padrão, só do módulo traduzido.
- `diag` R2: o runner não tem modo para caso `debug` (compilar, rodar sob
  `--checks=on`, afirmar o `abort`). Os 7 ficam sem teste.

Ver [[base-real-keel-em-progresso]], [[golden-perfis-autorais]],
[[discutir-antes-de-editar-spec]].
