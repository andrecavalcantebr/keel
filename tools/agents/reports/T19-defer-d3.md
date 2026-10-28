# T19 — `defer`, degrau D3: `break`, `continue` e `return` em laço: relatório

## O que mudou

Arquivo único: `tools/cgen/src/engine/emit/defer.c`. Nada mais foi editado: nem
`emit.c`, `emit_internal.h`, golden, oráculos, tabela `test/cgen/gen/golden-cases.txt`
ou normativos. Não há commit.

`git status --short` ao final:

```
 M tools/cgen/src/engine/emit/defer.c
```

### `defer.c`, por partes

- **Recusa** (`has_goto`, `has_jump`): antes, `break`, `continue` e `goto` recusavam a
  função; agora só `goto` recusa (`k_em_handles_defer`). `break` e `continue` passam a
  ser tratados, desde que tenham um alvo (`exit_target`); sem laço (ou sem `switch`,
  para `break`), o módulo é recusado em `plan_of`.
- **Blocos de laço e switch** (`block_kind`, `exit_target`): `block_kind` diz se um `{`
  abre corpo de `for`/`while`/`do` (laço) ou de `switch`, olhando o `(` que casa com o
  `)` antes dele. `exit_target` acha o `{` do laço mais interno que contém o `break` ou
  o `continue`; para `break`, também o `switch`.
- **Pendentes** (`pending_at`, `pending_count`): um `defer` j está pendente na saída p,
  com alvo `target`, quando o bloco de j é o alvo ou está dentro dele, e encerra depois
  de p (j < p). Para `return` o alvo é o `{` da função; para `break`/`continue`, o `{`
  do alvo. A ordem de cleanup (mais interno primeiro, em ordem inversa de registro) sai
  da varredura de j para trás, porque blocos aninhados que contêm p têm os seus `defer`
  registrados depois dos dos blocos de fora.
- **Saídas inline** (`inline_exit`, antes `inline_return`): a mesma `EDIT_LINES` serve
  a `return` e a `break`/`continue`. `break;`/`continue;` → `H|{`, os `L`, `T|<indent>`,
  e `EDIT_TEXT " }"` depois do `;`, como o `return;` de `void` do T18. `return expr;` →
  a forma do T18, com o temporário `keel__rv<n>`; `n` é o número de `return expr;`
  anteriores na função que têm pendentes (`inline_ordinal`), de 0. Uma saída sem nenhum
  pendente não muda (regra 4 do cartão: `return total;` da linha 26).
- **Plano** (`plan_of`): `break` ou `continue` na função força `RET_INLINE` (regra 1). Um
  `defer` aninhado é aceito com `break`/`continue`; sem eles, com `return`, continua
  recusado (escolha do T18, mantida). `Plan` ganhou `ends_ret` (o corpo termina em
  `return`), que antes era decidido só no ramo `RET_INLINE`. `break;`/`continue;` com
  forma diferente (sem `;` logo depois) são recusados.
- **Fim do corpo** (`k_em_edits_defer`): no `}` da função, em modo `RET_INLINE`, o
  cleanup natural só é pulado quando o corpo termina em `return` (`ends_ret`). Sem isso,
  um corpo que termina em laço, sem `return` final, perderia os `defer` de nível da
  função. Em `RET_LADDER` não muda.
- Ajuste de forma: `Plan p = { RET_NONE, true, false };` (sem aviso novo).

## Saídas

### `sh test/cgen/gen_golden.sh` (final)

```
wip    003-alignment
wip    006-dim
wip    007-instance
wip    005-defer-loop (differs)
9 ok, 0 fail, 4 wip
```

Antes da alteração: `9 ok, 0 fail, 4 wip` (005 era `wip`, sem `differs`, que é
mostrado aqui porque agora o `.c` bate e o que difere é o `.h`; ver dúvida 1).
014 e 017 continuam `ok` nos dois perfis.

**005 não sai `ok`.** O `.c` gerado é byte a byte igual ao oráculo nos dois perfis
(`c23` e `c11`). O que difere é o `.h` (e o `.type.h`, no guard). Ver dúvida 1.

### `sh tools/agents/verify.sh` (final)

```
ok   build (2 warnings, baseline 4)
ok   make check (48 lines ok)
ok   golden: 46 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

`make -C tools/cgen boundary`: `ok   engine boundary (desenho do cgen §3.1)`.

### Saída do `.c` de 005 (c23), contra o oráculo

```
$ tools/cgen/cgen -std=c23 --base-dir base --stop-after=gen -I golden/cases/005-defer-loop --dest-dir /tmp/t19/c23 golden/cases/005-defer-loop/app/loop.k
rc=0
$ diff golden/cases/005-defer-loop/expected/c23/app/app_loop.c /tmp/t19/c23/app/app_loop.c
diffrc=0
```

C11: `diff` do `.c` também sem saída (igual ao oráculo).

### Diferença restante de 005: só o `.h` (dois dos dois perfis)

```
< #ifndef APP_APP_LC_H              (oráculo)
> #ifndef APP_APP_LOOP_H            (gerado)
< #define APP_APP_LC_H
> #define APP_APP_LOOP_H
< int  app_loop_run(int n);         (oráculo: dois espaços)
> int app_loop_run(int n);          (gerado: um espaço, como no .k)
< #endif /* APP_APP_LC_H */
> #endif /* APP_APP_LOOP_H */
```

O mesmo no `app_loop.type.h`: `APP_APP_LC_TYPE_H` no oráculo, `APP_APP_LOOP_TYPE_H` no gerado.

### Casos extras, em `/tmp/t19/src/app/` (fora do repositório)

- `nest.k` (laço aninhado, dois `defer [now]` em blocos distintos, `break`/`continue`
  do laço interno, `break` do externo, `switch` com `break`, `return -1` com pendentes).
  Gerado em C23 e C11, compila sem aviso relevante (`gcc -Wall -Wextra`; só um aviso de
  parâmetro não usado de `rel`, do meu fonte de teste, com `-Wno-unused-parameter`).
  Trecho da função `run2`:

  ```c
          if (j == 2) {
  #line 11 "app/nest.k"
              app_nest_rel(keel__c1.j);
              break; }
          ...
          if (i == 5) {
  #line 9 "app/nest.k"
          app_nest_rel(keel__c0.i);
  #line 16 "app/nest.k"
          break; }
  ```

  O `break` do laço interno limpa só `j`; o `break` do externo limpa só `i`. O `break` do
  `switch` sai intocado.
- `gt.k` (com `goto done;` no laço): recusado,
  `cgen: error: app/gt.k: the emitter does not cover this module's constructions yet [implementation-limit]`, rc 2.

## Dúvidas e incoerências

1. **Oráculo 005 tem dois erros no `.h` (bloqueia 005 `ok`).** O cartão pede que 005 saia
   `ok`. O `.c` está certo, mas `expected/{c23,c11}/app/app_loop.h` (e `.type.h`) tem:
   - **guard `APP_APP_LC_H`**: o módulo é `app.loop`, e nos outros casos o guard é
     `<MOD>_H` do nome do módulo (`APP_APP_DR_H` em 017, `APP_APP_DL_H` em 014). `LC` não
     corresponde a nada do `.k`. Deveria ser `APP_APP_LOOP_H` (e `APP_APP_LOOP_TYPE_H`).
   - **`int  app_loop_run(int n);`** com dois espaços: no `.k` está `pub int run(int n)`,
     com um. "A emissão segue o `.k` sempre" aponta para o gerado (um espaço). Os outros
     protótipos de 005 (`int  app_loop_live_now`) têm dois espaços porque o `.k` tem
     `pub int  live_now`; esse é coerente.
   - Por isso não editei o oráculo. **Precisa de decisão do orquestrador:** corrigir os
     `.h` de `005` (guard e espaço) nos dois perfis, à mão, ou o `.k`. Com a correção, o
     caso deve sair `ok` sem outra mudança no `defer.c`. Não verifiquei isso com a
     correção aplicada, para não alterar o oráculo.
   - Observação: o T18 apontou o mesmo tipo de defeito em `017` (`int  app_dr_consume`,
     dúvida 1 do T18). Hoje `017` sai `ok`, mas não conferi aqui como esse ponto foi
     resolvido. Vale conferir se a regra "a emissão segue o `.k`" foi aplicada também a 005.
2. **Indentação do `break`/`continue` com `if` sem chaves.** `if (h < 0)  break;` vira
   `if (h < 0)  {`: o `{` entra antes do `break`, e o espaço entre `)` e `break` é
   mantido. Isso bate com o oráculo (linha 22), mas é uma consequência do `H|{` no ponto
   do `break`, não de uma regra escrita. Funciona para `if (x) break;` em uma linha, que
   é o que o `plain` do `defer` permite. Não testei `else break;` na mesma linha.
3. **Ordinal `keel__rv<n>` do inline.** O cartão pede o contador por função dos
   temporários inline. Implementei como o número de `return expr;` anteriores na função
   que têm pendentes (`inline_ordinal`). Com um só `return` pendente, `n` é 0, como no
   oráculo. Com dois ou mais, ainda não há caso no golden; os casos de `/tmp` usam um.
   O contador ignora `return` sem pendentes, porque não gera temporário.
4. **Natural end de função com `break`/`continue` e corpo sem `return` final.** Passei a
   emitir o cleanup de nível da função no `}` (D1), porque antes o `}` era pulado em modo
   inline. Não há caso no golden com esse formato; o `nest.k` termina em `return total;`.
   Vale um caso com corpo que termina em laço, se o orquestrador quiser o registro.
5. **`defer` aninhado com `return` e sem `break`/`continue` continua recusado**, como no
   T18 (dúvida 2 do T18 ficou com a escolha do T18). Com `break` ou `continue` na função,
   o aninhado é aceito, porque o inline cobre todos os blocos que a saída deixa.
6. **`break` dentro de `switch` em laço com `defer` no laço.** `break` procura o `switch`
   mais próximo e não tem `defer` pendente dentro dele (o parser recusa `defer` em
   `switch`, `K_DIAG_DEFER_IN_CONTROL_BLOCK`). Coberto pelo `nest.k` (`switch` com `break`,
   gerado e compilado), mas sem um `defer` dentro do `switch`, porque o parser o recusa.
7. **Não testado:** `continue` dentro de `switch` (o alvo é o laço, e o `switch` não tem
   `defer`); laço `do { } while` com `break` (o `block_kind` reconhece `do`, não testei);
   `return` com `[` (índice) num laço (recusado pelo `plan_of` como no T18).
