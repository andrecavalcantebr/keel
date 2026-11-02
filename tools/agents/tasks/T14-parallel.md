# T14 — WP4: `parallel` com cabeçalho em duas linhas, `{` em linha própria e workers por `constexpr`

## Objetivo

`parallel.c` recusa três formas de escrita válidas (`implementation-limit`). O oráculo é o caso
**`golden/cases/041-parallel-forms`** (escrito à mão: `app/pf.k`, `expected/c23/app/`,
`expected/c11/app/`, `proof.c`); leia o `pf.k` e o `expected/c23/app/app_pf.c` inteiros antes de tudo. As
três funções têm o mesmo corpo; o C do gestor é o mesmo do caso 004 (`golden/cases/004-parallel`):

1. **Cabeçalho em duas linhas** (`two_lines`): `parallel v ALL (size_t w : 0..4;` + linha seguinte
   `slice.slice i32 part : xs) {`. O gestor substitui o cabeçalho **inteiro**, e a segunda linha do
   cabeçalho **não produz linha de saída**: o `#line` que vem depois do gestor é o da primeira linha
   do corpo (`#line 11`), e o do `return` depois do bloco é `#line 15`.
2. **Workers por `constexpr`** (`named`): `0..W`, com `pub constexpr size_t W = 4;`. O C usa o **literal**
   resolvido (backend §5.9 r4: `num_threads(k)` sai com o literal): `.workers = 4`, `num_threads(4)`,
   `w < 4`. Resolva o nome com a mesma busca que `index.c` usa para a dimensão decimal por `constexpr`
   (o comentário do alto de `index.c` descreve: "o nome de um constexpr cujo inicializador é um literal
   decimal, de bloco ou de módulo"); inicializador que não seja literal decimal: recuse.
3. **`{` em linha própria** (`brace_below`): a chave que abre o corpo está na linha seguinte ao
   cabeçalho; o gestor cobre cabeçalho e chave, e o `#line` seguinte é o da primeira linha do corpo
   (`#line 30`).

## Arquivos

Pode editar: `tools/cgen/src/engine/emit/parallel.c`; se for inevitável, `emit.c`, `index.c` (só para
expor a busca do `constexpr`) e `emit_internal.h`. Pode apagar o `|wip` da linha `041-parallel-forms`
de `test/cgen/gen/golden-cases.txt` quando fechar. Pode criar `tools/agents/reports/T14-parallel.md`.

Não toque: normativos, `golden/**/expected/**`, nenhum `proof.c`, nenhum outro caso. Não faça commit.

## Contrato

- O texto C é o do 004, com `N` por função; não mude o gestor, só a leitura do cabeçalho e a contagem
  de linhas.
- `--parallel-lowering=serie` continua omitindo as duas linhas da diretiva (oráculo
  `test/cgen/gen/serie`); não o quebre.
- O que continua recusado: política que não é ALL/ANY/decimal, binder de partição com mais de um nome,
  captura não declarada na função, e workers que não sejam `0..K` com K decimal ou `constexpr` decimal.

Código existente (leia no arquivo): o comentário do alto de `parallel.c` (a lista "Refused"), a análise do
cabeçalho (`Par`), a linha `else return false; /* a constant by name is not lowered yet */`, e o ponto em
que o texto do gestor substitui `[kw_off, end)` com `push_in_line`.

## Passos sugeridos

1. `sh test/cgen/gen_golden.sh`: veja o `wip 041-parallel-forms`; gere a saída numa pasta temporária e
   compare com o esperado. **Não copie a saída para o esperado.**
2. Uma forma por vez; depois de cada uma `make -C tools/cgen -j8` e `sh test/cgen/gen_golden.sh` inteiro
   (004, 012 e 031 usam `parallel`/`routine`).
3. `sh tools/agents/survey/run.sh`: p01, p02 e p03 devem ir a `OK`.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # 041 sai de wip; 0 fail; o outro wip (042) continua
golden/run.sh                   # "80 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
sh tools/agents/survey/run.sh   # p01, p02, p03 com OK nos dois perfis
```

## Relatório

`tools/agents/reports/T14-parallel.md`, em pt-BR: o que mudou; a saída final exata dos quatro comandos;
toda dúvida ou incoerência.
