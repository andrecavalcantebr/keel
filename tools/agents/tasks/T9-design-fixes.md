# T9 — aplicar as correções diretas da auditoria T8

## Objetivo

Aplicar nos designs as correções da seção 1 de
`tools/agents/reports/T8-consolidado.md`, aprovadas pelo André em 2026-10-23.
As citações completas de cada achado estão em `T8a-design-audit.md` e
`T8b-design-audit.md` (mesma pasta). **Só edição de prosa em Markdown.**

## Arquivos

Pode editar, e só estes:

- `design/codegen-design.md`
- `design/lexer-design.md`
- `design/parser-design.md`
- `design/cgen-tool.md`
- `design/diag-design.md`
- `design/requisitos-v0.md`
- `tools/cgen/diag-messages.md` — **só a linha 29** (prosa); as tabelas não
  se tocam.

Cria `tools/agents/reports/T9-design-fixes.md`. Não edite normativos
(`keel-spec.md`, `keel-c-backend.md`, `cgen-tool-spec.md`,
`keel-rationale.md`), código, golden. Não commite.

## Regras de edição

- Prosa em pt-BR, no estilo do documento à volta. Código e nomes em inglês.
- Antes de cada edição, **releia o trecho** com o número de linha atual: os
  números abaixo são do início da tarefa e mudam à medida que você edita.
  Edite de baixo para cima dentro de um arquivo, ou use a busca pelo texto.
- Mantenha as tabelas Markdown bem formadas (mesmo número de colunas).
- Se o texto atual não for o que o cartão descreve, não adivinhe: anote no
  relatório e pule o item.

## As correções

### `design/codegen-design.md`

**A1 — sai o `arena.c`.** `alloc(a, T, n)` é o parâmetro `type` geral (spec
§4.4, backend §5.16); construtores e `alloc` cru são código comum da base, que
sai do `base/keel/arena.k` como qualquer módulo. O único privilégio de emissão
é o `= {0}` implícito (backend §5.4, regra 8; ilha `implicit-init`).

- Tabela por construção (perto da linha 63-66): apague a linha do `arena.c` e
  troque a do `decl.c` por:
  `| `decl.c` | §5.1, §5.4 (regra 8), §5.16 | declarações; substituição local de nome; o `= {0}` implícito de `arena`; o parâmetro `type` apagado |`
- Tabela "o que cada construção pede" (perto da linha 452): troque a linha
  `arena` por:
  `| `arena` | §5.4 | só o `= {0}` implícito na definição sem inicializador (ilha `implicit-init`); o resto é código comum da base |`

**A2 e A3 — tabela "Escrita no `.k`" (perto das linhas 131-141).** Troque a
linha `extern_c { … }` por estas cinco, na ordem do backend §4.1:

```
| `extern_c { … }` / `pub extern_c { … }` | — | conteúdo intacto | — |
| `extern_c [type_h] { … }` / `pub extern_c [type_h] { … }` | conteúdo intacto | — | — |
| `priv extern_c { … }` | — | — | conteúdo intacto |
| diretiva de topo | — | preservada | — |
| C de topo que não é diretiva nem construção keel (`static_assert`) | conteúdo intacto | — | — |
```

**A4 — tabela por construção (perto das linhas 61-72).** Acrescente, depois
da linha do `result.c`:

```
| `extent.c` | §5.15 | `extent`: struct de colunas e acesso por coluna |
| `check.c` | §5.17 | as verificações `debug` (`KEEL_CHECK`); o `assert` de índice continua em `index.c` |
| `binder.c` | §5.18 | o binder de dimensão |
| `protocol.c` | §5.19 | as instâncias de função sobre protocolo |
```

(o §5.16 já entrou no `decl.c`, item A1).

**A5 — tabela "o que cada construção pede" (perto das linhas 447-462).**
Acrescente, depois da linha `outcome`, `corot`:

```
| `extent` | §5.15 | struct de colunas; acesso verificado sob `--checks` |
| parâmetro `type` | §5.16 | pares `size_t` na assinatura; `sizeof`/`alignof` e o cast no ponto de chamada |
| verificações `debug` | §5.17 | `KEEL_CHECK` injetado, e o `#line` logo depois dele (§8) |
| binder de dimensão | §5.18 | o parâmetro de dimensão na assinatura e na chamada |
| função sobre protocolo | §5.19 | uma instância por combinação de tipos concretos, sem vtable |
```

**D1 — fórmula do mangling (perto da linha 230).** Troque
`símbolo   = <módulo com . → _> …` por
`símbolo   = <nome canônico M.mod, com . → _ e o encurtamento do backend §2.1> [ _ <argumento canônico> ]* [ _<sufixo de aridade> ]`
(mantenha o alinhamento das outras duas linhas do bloco).

### `design/lexer-design.md`

- **L1** (perto da linha 116): na lista `k_contextual_words`, insira
  `"protocol",` entre `"priv",` e `"pub",`. Confira a lista contra
  `tools/cgen/src/engine/lexer/lexer.c:22-30` (a lista real); se diferirem em mais
  alguma palavra, anote no relatório, não mude.
- **L2** (perto da linha 121): "(backend §4.2)" → "(backend §2.3, regra 3)".

### `design/parser-design.md`

- **P1** (perto das linhas 120-125): no `enum KTopKind`, acrescente
  `K_TOP_PROTOCOL` depois de `K_TOP_EXTENT`. Se houver um `union` com um
  membro por espécie logo abaixo, acrescente
  `struct { keel_slice_char name; } protocol;   /* decl-protocol, spec §5.1 */`
  no mesmo estilo dos vizinhos.

### `design/cgen-tool.md`

- **B1** (perto das linhas 199-200): troque
  "e o resto é ignorado — `-o` inclusive." por
  "e o resto é ignorado, salvo o `-o`: em `lex` e `parse` ele nomeia o
  arquivo da saída (`-o -` é `stdout`); com `gen` é `output-with-gen`
  (código 2), porque a fase escreve vários arquivos."
- **B2** (§2.2, logo depois da tabela que tem a linha `--parallel-lowering`,
  perto da linha 113): acrescente o parágrafo
  "`--parallel-lowering=openmp` sem `-fopenmp` entre as opções repassadas é
  `lowering-unavailable` (código 2), e o cgen para antes de compilar
  (ferramenta §4.8)."
- **B3** (§6, perto das linhas 549-557, depois da lista de regras da chamada
  ao `cc`): acrescente os itens
  "- `--checks=off` acrescenta `-DKEEL_CHECKS=0` à chamada; com `on`, nada:
  o prelúdio já vale `KEEL_CHECKS 1` quando a macro não existe (backend
  §5.17)." e
  "- `--line=off` só muda a emissão (backend §6); a chamada ao `cc` é a
  mesma."
  no formato da lista que já existe ali.
- **B4** (§3.3, passo 10, perto da linha 400, a frase do `--main`):
  acrescente "Se o módulo não tem função de entrada pública, é
  `missing-entry-point` (código 2), antes de qualquer escrita."
- **B5** (§5.2, coluna de detalhe/conteúdo das tabelas de espécies e marcas,
  perto das linhas 478-514): troque "spec §" por "linguagem §" **somente**
  nestas referências: `§4.3, regras 11 a 13`, `§4.6`, `§5.1`, `§2.3`, `§4.8`,
  `§4.10`, `§4.11`, `§4.4`, `§5.3`. Não troque "spec da ferramenta".
- **B6** (§13, perto das linhas 907, 912, 924): troque "Spec §4.1" →
  "linguagem §4.1"; "spec §4.3" → "linguagem §4.3"; "spec §4.4" →
  "linguagem §4.4", só nesses parágrafos de P16, P15 e P14.
- **B7** (perto da linha 693): "(regra 3 do backend §4.3.2)" →
  "(backend §4.3.2, regra 4, e §5.8)".
- **B8** (§10, perto das linhas 868-870): troque o parágrafo que começa por
  "Enquanto o golden tiver prosa explicativa nos cabeçalhos" por
  "O cabeçalho de cada arquivo é a linha de geração (§5.3: uma linha, sem
  versão nem data), e o diff a compara junto com o resto."

### `design/diag-design.md` e `tools/cgen/diag-messages.md`

- **B9** (`diag-design.md`, perto da linha 111, e `diag-messages.md:29`):
  onde se diz que a condição "é o que o `--cgen-help=<classe>` mostra",
  troque por "é a descrição do diagnóstico em inglês; o cgen não tem ajuda
  por classe (`--help=<classe>` vai ao `cc`, ferramenta §4.1)". Ajuste a
  concordância à frase de cada arquivo.
- **B10** (`diag-design.md`, perto da linha 25): troque a frase "Nenhuma
  mensagem de diagnóstico do keel existe escrita em lugar nenhum do
  repositório." por "As mensagens moram em `tools/cgen/diag-messages.md`,
  uma linha por identificador, e o `gen-diags.py` as gera para
  `src/engine/diag_messages.def`."
- **B11** (`diag-design.md`): "149 identificadores" (perto da linha 23) →
  "157 identificadores (139 `error`, 7 `warning`, 2 `info`, 9 `debug`)";
  "os 7 `debug`" (perto da linha 286) → "os 9 `debug`". **Não mexa no
  "135"** (é decisão pendente).
- **B12** (`diag-design.md`, fim do parágrafo "Gerada, não escrita à mão",
  perto das linhas 103-116): acrescente "A condição e a mensagem das linhas
  novas ou alteradas passam pelo `tools/cgen/diag-sync.py`, que pede o texto
  ao modelo e guarda o hash da condição em `diag-messages.lock.json`; o
  `gen-diags.py` gera o `.def` em seguida, e o resultado se revisa com
  `git diff`."

### `design/requisitos-v0.md`

- **B14** (perto da linha 9): "coleta, resolução, expansão (vazia na v0),
  papéis, `defer`, emissão" → "coleta, resolução, expansão (vazia na v0),
  reanálise da AST expandida (vazia na v0), papéis, `defer`, emissão".

## Aceitação

Rode da raiz do repositório e cole a saída no relatório:

```sh
git status -s
git diff --stat
python3 tools/cgen/gen-diags.py --check; echo "gen-diags rc=$?"
grep -n 'arena.c\|reescrita de `alloc' design/codegen-design.md; echo "arena rc=$?"
grep -c '"protocol"' design/lexer-design.md
grep -c 'K_TOP_PROTOCOL' design/parser-design.md
grep -n 'cgen-help=<classe>' design/diag-design.md tools/cgen/diag-messages.md; echo "help rc=$?"
grep -n -- '-o` inclusive' design/cgen-tool.md; echo "o rc=$?"
```

Passa quando: `git status` lista só os arquivos permitidos (mais o
relatório, o cartão e os relatórios T8, que já estavam fora do git);
`gen-diags rc=0`; `arena rc=1`, `help rc=1` e `o rc=1` (nada achado); os
dois `grep -c` dão pelo menos 1.

## Relatório

`tools/agents/reports/T9-design-fixes.md`, em pt-BR: cada item (A1…B14) com
"feito" ou "pulado, porque…", a saída da aceitação, e qualquer incoerência
nova que você tenha visto ao editar (só anote, não corrija).
