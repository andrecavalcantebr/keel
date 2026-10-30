# M6a — relatório: a cláusula `else` de resultado

Cartão: `tools/agents/tasks/M6a-else.md` (contexto comum: `M6-contexto.md`).

## Estado da árvore antes do trabalho

A worktree estava em `5727019`, 18 commits atrás de `master` (`b6accd8`), sem
commits próprios. Como o cartão e o esqueleto `else.c` estão em `master`, fiz
`git merge --ff-only master` (avanço rápido, sem commit novo). Nada mais foi
alterado fora do arquivo abaixo.

## O que mudou (arquivo por arquivo)

- `tools/cgen/src/engine/emit/else.c`: escrito do zero (era um esqueleto que
  recusava tudo). É o único arquivo alterado do repositório.
  - `read_parts`: lê o detalhe da ilha `else`
    `<alvo> (<tipo>) exit|default → <failed> [<win> &1]`. O modo é a última
    palavra antes da seta; o alvo é a primeira palavra. Recusa `win` sem `&1`
    (default), `win` no `exit`, e qualquer outra grafia.
  - `value_end`: acha o `;` que termina o valor de default (profundidade 0).
  - `k_em_handles_else`: aceita a ilha `K_ISLAND_ELSE` cuja âncora é o token
    `else`, com detalhe válido, e, no default, um `;` terminador.
  - `k_em_edits_else`: para cada ilha com a âncora em `fn` e na linha `[a, b)`:
    - o trecho entre o fim do token antes de `else` e o início do rabo vira
      `; if (F(x)) ` (exit), ou `; if (F(x)) W(&x, ` (default). Um único
      `EDIT_TEXT` com `skip` cobrindo `else` e os brancos;
    - no default, um `EDIT_TEXT` `)` na posição do `;` que termina o valor.
  - Usa só `k_em_tok`, `k_em_tok_off`, `k_em_tok_end`, `k_em_text_is`,
    `k_em_push_edit`, `k_em_scratch` e `keel_buffer_char_ptr` (`island_text`).
- `tools/agents/reports/M6a-else.md`: este relatório (novo, não rastreado).

Nenhum outro arquivo foi tocado: `golden/`, `test/cgen/gen/golden-cases.txt`,
normativos e `emit.c` ficaram como estavam.

## Verificação do que o `else.c` faz

Imprimi as ilhas dos três casos (`--stop-after=parse`), e todas saem como o
cartão descreve:

- `011-else`: `v (outcome i32) exit` (×2, linhas 20 e 26), e
  `s (outcome slice i32) default → …_failed …_win1 &1` (linha 32).
- `018-corot-outcome`: `r (outcome i32) default → keel_outcome_i32_failed keel_outcome_i32_win1 &1` (linha 26).
- `021-else-assign`: `name` exit (linha 53), `address` default (linha 54),
  `birth` default (linha 55).

Resultado de `else.c` nos trechos com `else`:

- `011`, linha 25/31/38 do esperado: `keel_outcome_i32 v = app_el_read(n); if (keel_outcome_i32_failed(v)) return -1;`,
  o bloco `{ puts("is_fault"); return -1; }`, e o default
  `if (keel_outcome_keel_slice_i32_failed(s)) keel_outcome_keel_slice_i32_win1(&s, …);`
  saem idênticos ao `expected/`, nos dois perfis, exceto pelas duas diferenças
  listadas abaixo.
- `018`: em um recorte do mesmo arquivo sem a função `evaluate_cleanup` (que é
  recusada, ver abaixo), a linha de `repair` sai exatamente como o esperado:
  `keel_outcome_i32 r = origin; if (keel_outcome_i32_failed(r)) keel_outcome_i32_win1(&r, 5);`.

## Saída dos comandos de aceitação

`sh tools/agents/verify.sh`:

```
ok   build (0 warnings, baseline 0)
ok   make check (50 lines ok)
ok   golden: 48 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

`sh test/cgen/gen_golden.sh`:

```
wip    011-else (differs)
wip    013-sugar
wip    015-foreach-range
wip    018-corot-outcome
wip    021-else-assign
18 ok, 0 fail, 5 wip
```

O critério de aceitação do cartão (`011`, `018` e `021` saírem da lista `wip`)
**não foi atingido**. Os três casos continuam `wip` por causas fora de
`else.c`, abaixo.

## Por que cada caso segue `wip` (o primeiro bloqueio)

- **`011-else`**: o `else` sai certo, mas duas diferenças são de outras
  construções:
  1. `keel_slice_i32_from(i32, base, …)` deveria ser `keel_slice_i32_from(base, …)`.
     O `i32` é o argumento de tipo, e quem o retira é `uses.c` (`marks_of3`,
     marca `type:<k>`). A ilha da chamada sai sem essa marca:
     `call slice.from/3 → keel_slice_i32_from` (app/el.k:15:27 e 32:46), e o
     `uses.c` não tem o que retirar. Gap do parser (detalhe da chamada), não do `else`.
  2. No `.h`, o esperado alinha o tipo de retorno (`int    app_el_twice(int n);`,
     quatro espaços), e o cgen sai `int app_el_twice(int n);`. Alinhamento de
     protótipo, outra construção.
  O `.c` e o `.h` diferem só nisso; o `app_el.type.h` confere. Os arquivos
  `keel/` que o cgen escreve a mais não entram na comparação (são extras, não
  do `expected/`), e não os conferi aqui.
- **`018-corot-outcome`**: a função `evaluate_cleanup` (linhas 30 a 36, com
  `defer ++*counter;` e `return corot.fault(...)`) faz o módulo inteiro ser
  recusado com `implementation-limit`. Isolei: uma função `corot` com
  `return` e sem `defer` passa; com `defer ++*counter;` e `return`, recusa. A
  recusa vem de `defer.c` (`k_em_handles_defer`), que é do cartão de `defer`,
  não deste.
- **`021-else-assign`**: o módulo é recusado antes de qualquer `else` com
  `cgen: keel_slice_const_char: the emitter does not cover this instance yet
  [implementation-limit]`. É a instância `keel.slice` de `const char`, fora de
  `else.c`.

## Dúvidas e pontos em aberto

1. O cartão pede que `else` cubra os três casos; hoje o `else` está correto nos
   três trechos, mas nenhum dos três fica verde. Recomendo que o cartão seja
   reaberto só para o `else`, e que o `slice.from(T, …)` (marca `type:` na
   ilha de chamada), o alinhamento do `.h` e o `defer` com retorno `corot`
   entrem em cartões próprios. Não decidi isso: é escopo do orquestrador.
2. `k_em_edits_else` assume que o `;` do default e o `else` estão na mesma
   linha (ou que o `;` está numa linha posterior, o que o filtro por posição
   trata). Não há caso no golden com valor de default em várias linhas, e não
   testei esse caso.
3. O nome do alvo é copiado do detalhe como escrito. Se um alvo fosse renomeado
   por outra construção (um `constexpr` de bloco, por exemplo), o `if` usaria o
   nome original. Não há caso assim nos três.

## Observações sobre o processo

- Um `verify.sh` que já estava rodando em outra worktree (`agent-adaf127e…`)
  não interferiu com o meu: são diretórios de build diferentes.
- Os arquivos temporários de diagnóstico ficaram em `/tmp/m6a-*` e `/tmp/m6a-bis/`,
  fora do repositório.
- Nada foi commitado nem enviado.
