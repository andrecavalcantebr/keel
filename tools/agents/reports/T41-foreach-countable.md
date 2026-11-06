# T41 — `foreach` de um binder sobre `Countable`: relatório

## O que mudou

**`tools/cgen/src/engine/emit/foreach.c`** (único arquivo de código):

- Cabeçalho do arquivo atualizado: a forma `Countable` deixa de ser "recusada".
- Nova struct `Countable` e `analyze_countable_var` (logo após `analyze_range_var`).
  Reconhece o cabeçalho `foreach (T b : c)` (mesma checagem de `analyze_range_var`),
  lê o texto entre parênteses do detalhe da ilha (`b : c (window)`), recusa
  `range` (fica para `analyze_range_var`), resolve o tipo com
  `k_em_type_symbol` e decide `by_pointer` com o mesmo laço de declaração de
  `analyze_over`, adaptado a um binder.
- Novo `edits_countable_var` (logo após `edits_range_var`). Gera
  `{ size_t keel__f<N> = S_first(c); size_t keel__l<N> = S_limit(c); for (T b = keel__f<N>; b < keel__l<N>; b++) body }`,
  com os verbos resolvidos por `verb_symbol(e, sym, sym_len, "first"/"limit", ...)`
  e o argumento `c` (ponteiro) ou `&c` (por valor). `auto` vira `size_t`.
  A sequência de edições é a mesma de `edits_range_var`.
- Despacho: `k_em_edits_foreach` chama o par novo depois de `analyze_range_var` e
  antes da forma literal. `k_em_handles_foreach` também passa a aceitar a forma.
  Esse é o gate usado por `supported.c:29`, que decide se a construção é recusada
  com `implementation-limit`. Sem essa segunda alteração, o cgen continuaria
  recusando o caso mesmo com o emissor pronto.

**`test/cgen/gen/golden-cases.txt`**: só a linha do 064, com `|wip` removido
(`064-foreach-countable|win.k|-|`). Nenhum outro caso foi tocado.

Nenhum arquivo de `parser/`, nenhum oráculo, nenhum normativo.

## Por que não precisa de valor fixo

- O nome do tipo vem do detalhe da ilha (`win.k:13:5 ... i : w (window)`) e é
  resolvido para o símbolo manglado por `k_em_type_symbol` (`uses.c:511`), que
  procura a ilha de tipo com o nome escrito. Para `window` isso dá `win_window`.
- Os verbos saem de `verb_symbol` (`foreach.c`, função `verb_symbol`), que
  primeiro procura instância de closure (`<sym>_first`) e depois a função do módulo
  chamada `first`/`limit`, manglada com `k_mangle_decl`. Para `window` isso dá
  `win_first` e `win_limit`. Nenhum nome está escrito à mão.

## Saída dos oráculos

Saída de `sh tools/agents/verify.sh` (final):

```
ok   build (0 warnings, baseline 0)
ok   make check (54 lines ok)
ok   golden: 128 ok, 0 fail, 0 xfail, 0 xpass, 0 wip
ALL OK
```

Saída de `sh test/cgen/gen_golden.sh`:

- com o emissor novo e a linha do 064 ainda com `|wip`: `74 ok, 0 fail, 0 wip`,
  e o 064 não aparece na saída (só casos fora de `ok` são impressos). O cartão
  dizia 73 → 74; não medi o total anterior ao emissor novo, então esse "antes"
  vem do cartão, não de uma execução minha.
- depois de tirar o `|wip`: `74 ok, 0 fail, 0 wip`, `rc=0`, mesma leitura.

Saída de `sh test/run.sh`: `128 ok, 0 fail, 0 xfail, 0 xpass, 0 wip`,
`docs: 45 ok, 0 fail`, `rc=0`, zero linhas com `FAIL`. Os dois perfis do 064
aparecem como `ok`.

Saída direta, antes de qualquer oráculo:

```
./tools/cgen/cgen -std=c23 ... --stop-after=gen -I golden/cases/064-foreach-countable --dest-dir /tmp/t41out golden/cases/064-foreach-countable/win.k
diff expected/c23/win.c /tmp/t41out/win.c   -> sem diferença (rc=0)
diff expected/c11/win.c ...                 -> sem diferença
```

## Pontos em aberto

1. **Sondagem de borda, fora do golden.** Um módulo com dois tipos contáveis,
   cada um com seu `first`/`limit` (`wa`, `wb`), é recusado pelo parser com
   `protocol-not-satisfied`: "its module does not declare first, limit". A
   checagem de protocolo do parser parece ser por nome no módulo, não por tipo.
   Por isso `verb_symbol`, que pega a primeira função `first` do módulo, não
   chega a escolher o errado nessa configuração. Um struct sem `first` também é
   recusado pelo parser, como esperado. A checagem por nome no parser é
   observação de parser: não toquei o parser, conforme o cartão.
2. `verb_symbol` tem o mesmo ponto frágil para tipos não genéricos (escolhe a
   primeira função do nome no módulo, sem olhar o tipo do receptor). Hoje isso
   só é inofensivo por causa do ponto 1. Não é deste cartão, mas quem mexer em
   `verb_symbol` deve saber.
3. Nenhuma pendência de parser foi encontrada para a forma geral: as ilhas
   `foreach` já trazem `(<tipo>)` no detalhe para variáveis locais e parâmetros
   (`win.k:13:5 foreach i : w (window)`), e o parser não precisou de mudança.

Não houve commit: a alteração está no working tree para revisão do orquestrador.
