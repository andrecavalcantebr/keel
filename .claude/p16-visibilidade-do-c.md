# P16 — Visibilidade do C que atravessa um módulo keel

Briefing para discutir a pendência P16 (`design/cgen-tool.md` §13). Não é
normativo. Nada daqui vai para a spec ou para o backend sem o "sim" explícito
do André (`.claude/memory/discutir-antes-de-editar-spec.md`).

## A pergunta

Um módulo keel pode trazer C de três jeitos, e a norma dá um destino a dois
deles:

| Forma | Destino hoje | Onde está escrito |
| --- | --- | --- |
| `import_c "x.h";` | `#include` no `.type.h`, a camada mais baixa | backend §4.1; spec §4.1 |
| `extern_c { … }` | o `.c`, inteiro, sem mangling | backend §4.1; spec §4.1 ("um tipo declarado em `extern_c` não é visível na interface") |
| `#define` / diretiva de nível de arquivo, fora de `extern_c` | **não especificado** | — |

A pergunta é se esse corte está certo, e para onde vai o terceiro.

## O que expôs a pendência

O primeiro caso de C real do golden: o `list.h` do Linux, em três variantes com
o mesmo `main` (`golden/cases/022`–`024-linux-list-*`, marcados `WIP`).

- **022, `import_c`**: funciona pela norma atual.
- **023, `extern_c`**: o texto do `list.h` colado no módulo. `pub typedef
  struct task { i32 id; struct list_head link; } task;` quebra no `.type.h`,
  porque `struct list_head` só existe no `.c`. O erro vem do compilador C, e
  não do keel, que não conhece o tipo.
- **024, módulo `linux.list`**: `list_for_each_entry` e as demais macros só
  servem a quem importa o módulo se saírem no `.h`. E a norma não diz para
  onde elas vão.

## As duas leituras

**A do André: em C, tudo é público.** O que vem de C deveria ser tratado como
público. `extern_c` é o meio-termo entre C e keel, e pode ter essa mesma
leitura. A norma atual (import_c no `.h`, extern_c no `.c`) falhou já no
primeiro caso de C real.

**A que está escrita (backend §4.1):** "o conteúdo é opaco e pode misturar
tipo com corpo de função; num header, os corpos dariam definição múltipla, e o
keel não tem como separar um do outro sem entender o C." A saída prescrita é:
"tipo C que atravessa a interface mora num header, e entra por `import_c`".

O `list.h` passa no teste do "tudo público" porque só tem `static inline` e
macro. Um `extern_c` com `int f(void) { … }` não passa: no header, cada unidade
que o inclui define `f`, e o link falha.

## Opções, sem ordem de preferência

1. **Manter o corte e especificar o `#define`.** O 023 vira demonstração da
   regra ("tipo que atravessa vai por `import_c`"), e o 024 exige uma regra
   para as diretivas de nível de arquivo, provavelmente "vão para o `.h`".
2. **`extern_c` inteiro na interface.** É o "tudo público" literal. Custo:
   definição múltipla para corpo não-`inline`, e a questão de em qual camada
   ele entra. O `.type.h` só pode incluir `.type.h`, e corpos na camada de
   tipo invertem a ordem do backend §4.3.2.
3. **Visibilidade escrita pelo programa**: `pub extern_c { … }` para a
   interface e `extern_c { … }` (ou `priv`) para o `.c`, reaproveitando
   `pub`/`priv`, que já existem para as declarações. A responsabilidade pela
   definição múltipla passa a ser de quem escreveu `pub`, como em C.
4. **Separar o conteúdo** (tipos e macros no header, corpos no `.c`). Exige
   entender C, o que contraria o parser de ilhas (spec §1.2). Está listada só
   para ser descartada com a razão à vista.

Em qualquer delas, falta decidir onde a diretiva de nível de arquivo cai: no
`.type.h`, no `.h` ou no `.c`. O `#define` de uma macro que cita tipo (como
`LIST_HEAD`) precisa estar onde o tipo está.

## O que ler

- `keel-spec.md` §4.1 (módulos, `import_c`, `extern_c`), §1.2 e §2.3.
- `keel-c-backend.md` §4.1 (destino de cada construção) e §4.3.2 (ordem das
  camadas).
- `golden/cases/022-linux-list-import-c/NOTES` e os NOTES do 023 e do 024.

## Como validar a decisão

Os casos 023 e 024 escrevem o `expected/` segundo a regra escolhida, e o 024
mede o custo em número de edições no `list.h` para virar módulo. A contagem
já visível no primeiro corte: 26 `static inline` sem visibilidade
(`inline-without-visibility`), 2 macros que chamam `list_empty` pelo nome C, e
a tag `struct list_head`, que entra no nome canônico (spec §4.2).
