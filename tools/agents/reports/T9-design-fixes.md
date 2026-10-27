# T9 — relatório: correções diretas da auditoria T8

Agente Haiku, cartão `tools/agents/tasks/T9-design-fixes.md`. Só edição de
prosa em Markdown, nos arquivos permitidos. Nada commitado.

## Itens

### `design/codegen-design.md`

- **A1 — feito.** Linha `arena.c` apagada da tabela por construção; linha
  `decl.c` trocada pela do cartão. Linha `arena` da tabela "o que cada
  construção pede" trocada pela do cartão.
- **A2 e A3 — feito.** Linha `extern_c { … }` da tabela "Escrita no `.k`"
  trocada pelas cinco linhas, na ordem do cartão.
- **A4 — feito.** Acrescentadas `extent.c`, `check.c`, `binder.c`,
  `protocol.c` após `result.c`.
- **A5 — feito.** Acrescentadas as cinco linhas após `outcome`, `corot`.
- **D1 — feito.** Fórmula do `símbolo` trocada; alinhamento das outras duas
  linhas do bloco mantido.

### `design/lexer-design.md`

- **L1 — feito.** `"protocol"` inserido entre `"priv"` e `"pub"`.
  Diferença restante, não alterada: a lista real de `tools/cgen/src/engine/lexer/lexer.c`
  tem também `"keel_code"` (entre `"instance"` e `"later"`), que o design não
  lista. Só anotado.
- **L2 — feito.** "(backend §4.2)" → "(backend §2.3, regra 3)".

### `design/parser-design.md`

- **P1 — feito.** `K_TOP_PROTOCOL` acrescentado depois de `K_TOP_EXTENT` no
  `enum KTopKind`. O `union as` tem um membro por espécie; acrescentado
  `struct { keel_slice_char name; } protocol;` logo depois de `extent`, no
  mesmo estilo.

### `design/cgen-tool.md`

- **B1 — feito.** Frase do `-o` trocada pela do cartão.
- **B2 — feito.** Parágrafo sobre `--parallel-lowering=openmp` inserido logo
  depois da tabela de opções do cgen, antes de "As duas formas".
- **B3 — feito.** Dois itens (`--checks=off`, `--line=off`) acrescentados ao
  fim da lista da §6.
- **B4 — feito.** Frase do `missing-entry-point` acrescentada ao passo 10 da
  §3.3.
- **B5 — feito, com uma ressalva.** Trocado "spec §" por "linguagem §" nas
  referências listadas: §4.3 regras 11 a 13, §4.6, §5.1, §2.3, §4.8, §4.10,
  §4.11, §4.4 (no parágrafo logo após a tabela de `decl`/`ilha`) e §5.3, todas
  dentro da §5.2. Não trocada: a referência "spec §4.3, regra 19" na linha da
  espécie `decl instance`, que não está na lista do cartão (ver pendências).
  "spec da ferramenta" não foi tocado.
- **B6 — feito, com uma ressalva.** P16: "Spec §4.1" → "linguagem §4.1". P15:
  "spec §4.3" → "linguagem §4.3". P14: "spec §4.3" → "linguagem §4.3". Não
  feito: o cartão pede "spec §4.4" → "linguagem §4.4" em P15, mas o texto
  atual é "o `verb-not-in-instance` / da §4.4" (sem "spec", com quebra de
  linha). Não adivinhei; ver pendências.
- **B7 — feito.** "(regra 3 do backend §4.3.2)" (com quebra de linha no meio)
  → "(backend §4.3.2, regra 4, e §5.8)".
- **B8 — feito.** Frase do golden trocada pela do cartão; a marca **[P5]**
  saiu junto, como o cartão pede.

### `design/diag-design.md` e `tools/cgen/diag-messages.md`

- **B9 — feito.** Em `diag-design.md`: "que é o que o `--cgen-help=<classe>`
  mostra" virou "traduzida para o inglês. O cgen não tem ajuda por classe
  (`--help=<classe>` vai ao `cc`, ferramenta §4.1)." (a concordância foi
  ajustada, porque o "que é" ficava redundante). Em `diag-messages.md:29`:
  "e é o que o `--cgen-help=<classe>` mostra" virou "e é a descrição do
  diagnóstico em inglês; o cgen não tem ajuda por classe (`--help=<classe>`
  vai ao `cc`, ferramenta §4.1)". A tabela não foi tocada.
- **B10 — feito.** Frase "Nenhuma mensagem…" trocada pela do cartão.
- **B11 — feito, com uma ressalva.** "149 identificadores" → "157
  identificadores (139 `error`, 7 `warning`, 2 `info`, 9 `debug`)"; "os 7
  `debug`" (linha ~287) → "os 9 `debug`". O "135" não foi tocado. Ressalva: a
  linha ~310 (tabela de pendências, R2) diz "Os 7 `debug` do catálogo" com
  maiúscula; o cartão não a cita, então não mexi.
- **B12 — feito.** Frase acrescentada ao fim do parágrafo "Gerada, não escrita
  à mão" (`diag-sync.py`, `diag-messages.lock.json`, `gen-diags.py`, `git diff`).
  Os três arquivos existem em `tools/cgen/`.

### `design/requisitos-v0.md`

- **B14 — feito.** "reanálise da AST expandida (vazia na v0)" inserido entre
  "expansão (vazia na v0)" e "papéis".

### Itens não tratados

- **B13** não existe no cartão (numeração pula de B12 para B14). Nada feito.
- Nenhum item pulado por texto diferente do descrito, exceto as ressalvas
  de B5, B6 e B11 acima.

## Aceitação

Saída de `git status -s` e `git diff --stat` (da raiz do repositório):

```
 M design/cgen-tool.md
 M design/codegen-design.md
 M design/diag-design.md
 M design/lexer-design.md
 M design/parser-design.md
 M design/requisitos-v0.md
 M tools/cgen/diag-messages.md
?? tools/agents/reports/T8-consolidado.md
?? tools/agents/reports/T8a-design-audit.md
?? tools/agents/reports/T8b-design-audit.md
?? tools/agents/tasks/T8-design-audit.md
?? tools/agents/tasks/T9-design-fixes.md
 design/cgen-tool.md         | 47 ++++++++++++++++++++++++++++-----------------
 design/codegen-design.md    | 22 ++++++++++++++++-----
 design/diag-design.md       | 15 ++++++++-------
 design/lexer-design.md      |  4 ++--
 design/parser-design.md     |  3 ++-
 design/requisitos-v0.md     |  2 +-
 tools/cgen/diag-messages.md |  4 ++--
 7 files changed, 61 insertions(+), 36 deletions(-)
```

Os não rastreados são T8/T9 (cartões e relatórios), que já estavam fora do
git, e o relatório T9 que este arquivo é. Os arquivos editados são só os
permitidos.

```
gen-diags rc=0
arena rc=1
1
1
help rc=1
o rc=1
```

- `gen-diags.py --check`: rc=0.
- `arena`: nada achado (rc=1), como esperado.
- `"protocol"` em `lexer-design.md`: 1 ocorrência.
- `K_TOP_PROTOCOL` em `parser-design.md`: 1 ocorrência.
- `cgen-help=<classe>` em `diag-design.md` e `diag-messages.md`: nada achado (rc=1).
- `-o` inclusive em `cgen-tool.md`: nada achado (rc=1).

Passa, conforme o cartão.

## Pendências e incoerências vistas (só anotadas, não corrigidas)

1. `design/cgen-tool.md`, §5.2, espécie `decl instance`: "spec §4.3, regra 19"
   ficou. O cartão não lista essa referência. Mesmo caso das outras
   referências trocadas, então pode ser decisão do André.
2. `design/cgen-tool.md`, §13, P15: o texto diz "o `verb-not-in-instance` / da
   §4.4", sem "spec". O cartão pede "spec §4.4 → linguagem §4.4", que não
   existe literalmente ali.
3. `design/diag-design.md`, tabela de pendências (R2, ~linha 310): "Os 7
   `debug`", com a contagem antiga; o certo é 9.
4. `design/parser-design.md`, ~linha 449: "os 7 `debug`", mesma contagem antiga
   (fora do escopo do cartão, que não lista este arquivo para B11).
5. `design/lexer-design.md`: a lista de palavras contextuais não tem
   `"keel_code"`, que está no `lexer.c`.
6. `design/parser-design.md`, comentário do membro `protocol`: "spec §5.1", que
   veio do cartão e fica como está.
