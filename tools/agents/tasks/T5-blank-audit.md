# T5 — auditoria das linhas vazias nos arquivos gerados do golden

## Objetivo

O M3 compara a saída do cgen com o golden **byte a byte**, e o golden não tem
regra única para linha vazia. Depois do `#include` sintético do topo, 53
`.type.h` têm linha vazia e 10 não têm; nos `.h`, são 22 com e 42 sem. O
`design/cgen-tool.md` §8.3 (o caso `hello`) não tem nenhuma.

O André decidiu pela alternativa **(b)**: **uma linha vazia fixa separa o
bloco de `#include` sintéticos do conteúdo.** Esta auditoria mede o que existe
hoje, para que a regra seja escrita sem ambiguidade e o golden seja
normalizado por script. **Ela não muda o golden.**

## Arquivos

- Criar: `golden/blank-check.py` (a ferramenta) e
  `tools/agents/reports/T5-blank-audit.md` (o relatório).
- Só leitura em todo o resto. Pode rodar `golden/run.sh` e
  `python3 golden/line-check.py`, que só leem. Não rode `make` nem
  `verify.sh`.

## Contexto que a ferramenta precisa

- **Includes sintéticos** são os `#include "…"` que o gerador escreve por
  conta própria: o do próprio `.type.h` (num `.h`), os `.type.h` e `.h` de
  outros módulos ou instâncias, o `keel.type.h`, e os de perfil (`<stdbool.h>`,
  `<assert.h>` sob C11, só no prelúdio). **Não** são sintéticos os
  `#include <x.h>` que vêm de `import_c`: esses têm `#line` antes (backend §6,
  regra 4).
- Num `.h` há quatro seções (backend §4.3.2, regra 3; `codegen-design.md`
  §4.1): (1) includes de `.type.h`, (2) protótipos e `extern`, (3) includes de
  `.h` que os corpos chamam, (4) corpos. A seção 3 também é sintética, mas fica
  no meio do arquivo.
- Uma linha vazia sob o regime de um `#line` **conta** para o contador
  (backend §6). Tirar ou pôr uma linha vazia ali desloca as linhas seguintes,
  e aí um `#line` fica faltando ou sobrando. `golden/line-check.py` é quem
  mede isso.

## Contrato da ferramenta `golden/blank-check.py`

Python 3, só biblioteca padrão. `python3 golden/blank-check.py [--verbose]`, a
partir da raiz. Cobre os mesmos arquivos que `golden/line-check.py`: os `.c` e
`.h` de `golden/cases/*/expected/{c11,c23}/` e de `golden/{c11,c23}/`.

Para **cada linha vazia** de cada arquivo, classifique numa destas categorias,
na ordem, ficando com a primeira que vale:

| Categoria | Definição |
| --- | --- |
| `TOP-AFTER-SYNTH` | logo depois do último include sintético do topo (o bloco que segue a guarda) |
| `SECTION-3` | imediatamente antes ou depois do bloco de includes de `.h` da seção 3 de um `.h` |
| `BEFORE-ENDIF` | a linha seguinte é o `#endif` da guarda |
| `MAPPED-SOURCE` | está sob um `#line` e a linha do `.k` que o contador atribui a ela é vazia ou só comentário no `.k` |
| `MAPPED-OTHER` | está sob um `#line` e a linha do `.k` correspondente tem código |
| `UNMAPPED` | qualquer outra: antes do primeiro `#line`, ou num arquivo sem `#line` |

Para **cada arquivo**, diga também se o bloco sintético do topo é seguido de
linha vazia (`top-blank`) ou não (`top-noblank`), e o que vem depois dele: um
`#line`, código, outro bloco ou o `#endif`. Um arquivo sem nada além dos
includes (só a guarda) é um caso à parte: registre quantos há.

## Simulação da regra (b), sem aplicar

Para cada arquivo, calcule o que a regra (b) mudaria, **numa cópia em
`/tmp`**:

1. garantir exatamente uma linha vazia entre o último include sintético do
   topo e a primeira linha seguinte;
2. a seção 3 de um `.h`: medir a forma que mais aparece hoje (vazia antes,
   depois, nas duas pontas, em nenhuma) e simular a regra "uma linha vazia
   antes e uma depois do bloco da seção 3". Reporte as duas contagens;
3. não mexer em nenhuma linha vazia `MAPPED-*`.

Depois, rode `golden/line-check.py` sobre a cópia (aponte `GOLDEN` para ela,
ou copie a árvore inteira) e diga se a cópia continua passando em
`--check`. Se alguma mudança da simulação quebrar o mapeamento, diga qual.

## Saída esperada no relatório (`tools/agents/reports/T5-blank-audit.md`, pt-BR)

1. A tabela categoria → quantidade, por perfil, com dois exemplos
   (`arquivo:linha`) de cada.
2. `top-blank` contra `top-noblank`, por tipo de arquivo (`.type.h`, `.h`,
   `.c`, instância contra módulo contra prelúdio), e o que vem depois do bloco.
3. A seção 3: as formas que existem hoje, com a contagem.
4. As linhas vazias `MAPPED-OTHER` e `UNMAPPED` uma a uma: são as que nenhuma
   regra explica.
5. O resultado da simulação: quantas linhas cada regra põe ou tira, e se a
   cópia passa no `line-check.py --check`.
6. As perguntas que sobram para o André fixar a regra. Por exemplo: arquivo
   só com includes leva linha vazia antes do `#endif`? Um `.c` também segue
   a (b)?

Não proponha mudança nos normativos. O relatório é a evidência para a decisão.
