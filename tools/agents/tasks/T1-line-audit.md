# T1 — auditoria do `#line` no golden: onde os contadores divergem

## Objetivo

Responder com evidência mecânica, não com opinião: **nos arquivos esperados
do golden, quais diretivas `#line` são necessárias, quais são redundantes e
onde falta uma.** E mapear os **pontos de divergência** por categoria de
construção.

Contexto: o backend §6 (`keel-c-backend.md`, linhas 1857–1888) enuncia uma
invariante: o gerador mantém dois contadores, a linha do `.k` em tradução e a
da saída, e **emite `#line` sempre que eles divergem**. Na tabela, a
"declaração levada a header" leva `#line` "antes de cada declaração que não
seja a linha seguinte da anterior". O golden, por outro lado, põe um `#line`
antes de **toda** declaração de header. Exemplo: `golden/cases/019-walk/expected/c23/lst.type.h`
tem `#line 8`, `#line 9` e `#line 10` seguidos. Numa sessão anterior, o
André viu as declarações divergindo do `.k` sem eles. A auditoria decide
qual leitura os dados sustentam.

Divergências já vistas, que a auditoria tem de confirmar ou refutar:
- `golden/c23/keel.type.h` e `golden/c11/keel.type.h`: o `#include <stdint.h>`
  (de `import_c`, `base/keel.k:12`) sai sem `#line`, enquanto
  `golden/cases/016-line/expected/c23/app/app_ml.type.h` põe `#line 3` antes do
  `#include <stdio.h>`.
- No mesmo `keel.type.h`, os `static_assert` (`base/keel.k:27–30`) vêm depois
  do `#line 23` e de uma linha vazia, sem `#line` próprio. O contador da saída
  diria 25, mas o fonte está em 27.
- `design/cgen-tool.md` §8.3 (`hello.type.h`) não tem linha vazia depois do
  `#include "keel.type.h"`; os casos 016 e 019 têm.

## Arquivos

- Criar: `golden/line-check.py` (a ferramenta) e
  `tools/agents/reports/T1-line-audit.md` (o relatório).
- Só leitura em todo o resto. **Não rode** `make`, `verify.sh` nem
  `golden/run.sh`: outra tarefa está compilando a árvore ao mesmo tempo.
  `gcc -E` sobre um arquivo esperado é permitido (não toca em `obj/`).
- Leia como modelo `golden/line-audit.py`, a heurística já existente, mas não
  o edite.

## Contrato da ferramenta `golden/line-check.py`

Python 3, só biblioteca padrão. Roda a partir da raiz do repositório:
`python3 golden/line-check.py [--verbose]`. Cobre:
- todo `.c` e `.h` de `golden/cases/*/expected/{c11,c23}/`: o `.k` é o
  caminho do `#line`, relativo ao diretório do caso; se não existir ali, é
  relativo a `base/`;
- todo arquivo de `golden/c11/` e `golden/c23/` (base e prelúdio): o `.k` é
  relativo a `base/`.

Para cada arquivo, simule o contador do compilador C (C11 §6.10.4):
`#line N "f"` faz a **linha seguinte** ser N em `f`, e cada linha física
depois disso soma 1. Outras diretivas (`#include`, `#if`, `#define`…) ocupam
linha e incrementam como qualquer outra. Linhas antes do primeiro `#line`
pertencem ao próprio arquivo gerado (não mapeiam para o `.k`).

Classifique:

1. **Cada `#line N "f"`**:
   - `REDUNDANT`: já havia um `#line` anterior para o mesmo `f` neste
     arquivo, e o contador simulado, sem esta diretiva, já valeria N na
     linha seguinte;
   - `REQUIRED`: qualquer outro caso. Registre também o motivo: `first`
     (primeiro do arquivo), `jump` (o contador valeria M ≠ N; registre M e o
     salto N−M) ou `file` (outro `.k`).
2. **Cada linha de código não vazia mapeada** (não diretiva, não comentário)
   cujo texto não corresponde à linha do `.k` para onde ela aponta:
   `MISSING`. A correspondência é a de `golden/line-audit.py`:
   identificadores da linha C, sem palavras-chave, casam com algum
   identificador da linha `.k`, aceitando o prefixo de mangling
   (`lst_Node` casa com `Node`). Diretivas de pré-processador geradas de
   `import_c` (`#include <x.h>`) contam como código: comparam `x.h` com a
   linha `import_c <x.h>;`.

A prova empírica, obrigatória para pelo menos três arquivos (um `.type.h` de
caso, um `.h` de caso e o `keel.type.h` do prelúdio): apague do arquivo,
numa cópia em `/tmp`, todas as diretivas classificadas `REDUNDANT`; rode
`gcc -E` nas duas versões (com `-I` para o diretório do perfil e para o
`expected/<perfil>` do caso) e compare, para cada linha de código do arquivo
auditado, o par (arquivo, linha) que os linemarkers do `gcc -E` atribuem.
**Se a classificação estiver certa, os pares são iguais.** Mostre o comando e
o resultado no relatório.

## Saída esperada no relatório (`tools/agents/reports/T1-line-audit.md`, pt-BR)

1. Totais: arquivos, diretivas `REQUIRED` (por motivo), `REDUNDANT` e
   linhas `MISSING`, por perfil.
2. **Mapa de divergências por categoria.** Para cada `REQUIRED jump` e cada
   `MISSING`, diga o que no `.k` causa o salto: linha de comentário ou em
   branco suprimida, declaração de várias linhas que vira uma, declaração
   reordenada, `pub`/`priv` que foi para outro arquivo, `import_c`, código
   injetado, etc. Dê a tabela categoria → quantidade → dois exemplos
   (`arquivo:linha`).
3. Para os três pontos vistos acima (prelúdio, `static_assert`, linha vazia
   depois do include): confirmado ou refutado, com a evidência.
4. A conclusão em uma frase por pergunta: (a) a regra "um `#line` antes de
   cada declaração de header" do golden é necessária, ou a invariante dos
   dois contadores basta? (b) o golden tem `#line` faltando, e onde?
5. A saída final de `python3 golden/line-check.py`.

Não proponha mudança no golden nem nos normativos. O relatório é a evidência
para o André decidir.
