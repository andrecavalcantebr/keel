# T3 — partir o `parser_islands.c` pelas seções que ele já tem

## Objetivo

`tools/cgen/src/engine/parser/parser_islands.c` tem 4.850 linhas e 194 funções
`static`, uns 60% do motor. O próprio arquivo já está dividido em seções
comentadas (`/* ---- <nome> ---- */`). Esta tarefa o parte em seis arquivos
seguindo essas seções, **sem mudar uma linha de código**: é mover, não
reescrever.

Pré-requisito: a T2 está commitada. Use o commit corrente (`HEAD`) como
origem.

## Snapshot (antes de qualquer edição)

```sh
sh tools/agents/verify.sh snapshot /tmp/keel-snap-T3
```

## A partição

Os números de linha são os do `HEAD` no início da tarefa. Confira com
`grep -n '^/\* ----' tools/cgen/src/engine/parser/parser_islands.c`: se não baterem
com os marcos abaixo, siga os **marcos**, não os números.

| Arquivo | Seções (marco de início → até o próximo da tabela) | ~linhas |
| --- | --- | --- |
| `parser_islands.c` (fica) | comentário de topo e includes; `the detail column`; `locals`; `the module a call lands in`; `protocols`; `calls`; `seeds from calls` (com os quatro `static` de estado, que o `k_collect_islands` zera); e, do fim, `extern_c and main (spec §4.1)` com o `k_collect_islands` | ~1040 |
| `parser_islands_types.c` | `container types`; `names`; `declarations` (a primeira, linha ~1765) | ~1220 |
| `parser_islands_index.c` | `stage 4b: array, array-index, index, range-index` | ~310 |
| `parser_islands_flow.c` | `defer, foreach, walk, match`; `parallel, win, fail`; `extent`; `the else of a result` | ~1180 |
| `parser_islands_closure.c` | `the closure of the instances`; `provenance and invalidation`; `instance (spec §4.3)` | ~460 |
| `parser_islands_checks.c` | `names across modules`; `declarations (passage 2 …)`; `protocol declarations`; `parameters type and over a protocol`; `layout` | ~650 |

E um header novo, `parser_islands_internal.h`, interno à passagem. Ele
recebe:
- os `#define`, `typedef` e `struct` que hoje estão no topo do arquivo, antes
  da primeira seção, e quaisquer outros tipos usados por mais de um dos seis
  arquivos;
- os protótipos das funções que passam a ser chamadas de outro arquivo.

Guarda: `#ifndef KEEL_ENGINE_PARSER_ISLANDS_INTERNAL_H`. Comentário de topo,
em inglês: interno à passagem 3, não é API do motor. Ele inclui o que os
tipos pedem (`islands.h`, `instances.h`, `names.h`, `parser.h`, `<stdint.h>`,
`<string.h>`, como o topo de hoje). **Nenhum include de `<stdio.h>`,
`<stdlib.h>` ou `<sys/*.h>`** (`make boundary`).

## Regras do corte

1. **Código movido, não mudado.** Cada linha de código sai do arquivo antigo
   e entra num dos novos, igual. As únicas mudanças permitidas:
   - tirar o `static` de uma função que outro arquivo passa a chamar, e
     declará-la no header interno. O nome **não muda**. Se o link acusar
     colisão com um símbolo externo de outro arquivo do motor, aí sim
     renomeie com o prefixo `isl_` e registre no relatório;
   - funções que só o próprio arquivo novo usa continuam `static`;
   - os quatro `static` de estado global da seção `seeds from calls`
     (`seed_nodes`, `seed_node_count`, `seeds`, `seed_count`) ficam
     `static` no arquivo que os usa. Se mais de um arquivo os usar, pare e
     registre no relatório, sem transformá-los em `extern`;
   - protótipos adiantados de função `static` que hoje existam no arquivo
     vão para onde a função for.
2. Quantos nomes cada arquivo exporta é dado do relatório, não limite: não
   junte seções por causa disso. (Revisão de 2026-10-22: a primeira tentativa
   parou porque `seed_count`/`seed_node_count` são zerados em
   `k_collect_islands`; a seção `seeds from calls` passou para o
   `parser_islands.c`.)
3. A ordem das funções dentro de cada arquivo novo é a ordem do arquivo
   antigo.
4. Atualize a tabela de `tools/cgen/src/engine/README.md` (uma linha por
   arquivo novo, dizendo as etapas que ele cobre). O `Makefile` acha os
   `.c` sozinho (`find`), e não precisa mudar.

## Arquivos

Criar os cinco `parser_islands_*.c` e o `parser_islands_internal.h`; editar
`parser_islands.c` e `tools/cgen/src/engine/README.md`;
`tools/agents/reports/T3-islands-split.md`. Nada mais.

## Aceitação

As três têm de passar:

```sh
python3 tools/agents/check_split.py HEAD:tools/cgen/src/engine/parser/parser_islands.c \
    tools/cgen/src/engine/parser/parser_islands.c tools/cgen/src/engine/parser_islands_*.c \
    tools/cgen/src/engine/parser/parser_islands_internal.h
#   -> "ok   split: all N non-blank lines ... the rest is glue"
sh tools/agents/verify.sh compare /tmp/keel-snap-T3          # -> ALL OK
wc -l tools/cgen/src/engine/parser_islands*.c                 # nenhum acima de 1400
```

`check_split.py` confere que toda linha antiga está nos arquivos novos (menos
o `static`) e que o que sobra é cola: comentário, diretiva, protótipo e
`extern`. Se ele acusar `LOST` ou `UNEXPLAINED`, o corte mudou código:
corrija o corte, não o script.

## Relatório

`tools/agents/reports/T3-islands-split.md`, em pt-BR: a partição feita
(arquivo → seções → linhas), quantas funções cada arquivo exporta e quais, se
houve renomeação ou junção de seções e por quê, a saída das três aceitações e
as dúvidas.
