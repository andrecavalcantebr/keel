# T15 — `foreach` sobre intervalo literal: relatório

## O que mudou

Só `tools/cgen/src/engine/emit/foreach.c` (e este relatório). Nada commitado.

**`foreach.c`** (preenchido; antes eram dois ganchos vazios):

- `analyze(ast, kw, &f)`: lê o `foreach` na âncora `kw` e confirma a forma
  `( T b : A .. B )` (exatamente um `..` no cabeçalho, `A` e `B` não vazios,
  `T` e `b` palavras, `:` na quarta posição após o `foreach`), e o corpo:
  bloco `{ … }` (casado por contagem de chaves) ou uma instrução simples até o
  primeiro `;` de nível zero, sem `{`, `}`, `if`, `for`, `while`, `foreach`,
  `switch`, `do`, `else`, e sem parênteses ou colchetes desbalanceados.
- `k_em_handles_foreach`: `true` só para `K_ISLAND_FOREACH` cujo detalhe contém
  `..` e cuja forma passa em `analyze`. Container, dois binders, `range` em
  variável e corpo com `if` continuam recusados.
- `k_em_edits_foreach`: para cada `foreach` tratado do corpo da função, empilha
  as edições (só as com `at` em `[a, b)`):
  - `foreach` → `{ for` (`EDIT_TEXT`, `skip` = 7);
  - `auto` → `size_t` (um tipo escrito fica);
  - `:` → `=`;
  - `..` → `; <b> < ` (texto em `k_em_scratch`, `skip` = 2);
  - `)` do cabeçalho → `; <b>++)` (texto em `k_em_scratch`, `skip` = 1);
  - bloco: ` }` inserido depois do `}` que fecha o corpo;
  - instrução simples: `{ ` inserido antes do primeiro token do corpo, e
    ` } }` inserido depois do `;`.
- Se o nome do binder for também um nome declarado do módulo
  (`k_em_declared_at`), a edição marca `e->ok = false` (ver dúvida 1).

Nenhum outro arquivo foi tocado. `emit.c`, `emit_core.c`, `emit_internal.h`,
o oráculo `test/cgen/gen/foreach/` e os demais golden não foram alterados.

## Medições (antes de escrever)

`cgen --stop-after=lex` sobre `loop.k`:

- `0..n`: `number "0"`, `punct ".."`, `ident "n"`. O `0` não vira `0.`.
- `1..4`: `number "1"`, `punct ".."`, `number "4"`.
- `auto` é `cword`; `u32` é `ident`.

`cgen --stop-after=parse` sobre `loop.k` dá três ilhas
`ilha foreach i : 0..n` (`loop.k:5:5`), `j : 2..n` (`6:5`) e `k : 1..4`
(`7:5`). A âncora é o token `foreach`, e o detalhe não tem parênteses de
contêiner.

## Saída do oráculo `gen_golden.sh`

O script é chamado pelo caminho absoluto (`sh <abs>/test/cgen/gen_golden.sh`),
porque a forma relativa foi recusada pelo verificador de comandos deste
ambiente. Saída:

```
wip    003-alignment
wip    006-dim
wip    007-instance
wip    008-block-constexpr
wip    test/cgen/gen/array
wip    test/cgen/gen/index
wip    test/cgen/gen/constblock
3 ok, 0 fail, 7 wip
```

O script só imprime as linhas `wip` e `FAIL`; `test/cgen/gen/foreach` não
aparece em nenhuma, e por isso fica entre os 3 `ok` (os outros `ok` são
`shapes` e `tags`). Para ver o caso diretamente, gerei `loop.k` nos dois
perfis e comparei byte a byte com `expected/`:

```
c23: loop.c, loop.h, loop.type.h iguais ao oráculo
c11: loop.c, loop.h, loop.type.h iguais ao oráculo
```

Saída do `loop.c` (C23, idêntica à do C11):

```c
size_t loop_sum(size_t n) {
    size_t t = 0;
    { for (size_t i = 0; i < n; i++) { t += i; } }
    { for (size_t j = 2; j < n; j++) { t += j; } }
    { for (u32 k = 1; k < 4; k++) { t += k; } }
    return t;
}
```

Compilação: `gcc -std=c2x -Wall -Wextra -fsyntax-only` e
`gcc -std=c11 -Wall -Wextra -fsyntax-only` sobre o `loop.c` gerado: sem aviso.

## Formas recusadas e aninhadas (`/tmp/t15/refuse/`)

- `foreach (auto x : xs) t += x;` (contêiner): `implementation-limit`, rc 2.
- `foreach (auto i : 0..n) if (i > 2) t += i;` (corpo com `if`): `implementation-limit`, rc 2.
- `pub size_t i(...)` com `foreach (auto i : 0..n) t += i;` (binder que é nome
  do módulo): `implementation-limit`, rc 2.
- `foreach` aninhado, com bloco externo e instrução simples interna
  (`foreach (auto i : 0..n) { foreach (auto j : 0..i) t += j; }`): gerado e
  correto. Saída: `{ for (size_t i = 0; i < n; i++) {` /
  `{ for (size_t j = 0; j < i; j++) { t += j; } }` / `} }`. Compila nos dois
  perfis (`gcc -fsyntax-only`).

## Saída do `verify.sh`

```
ok   build (2 warnings, baseline 4)
ok   make check (48 lines ok)
ok   golden: 46 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

Nenhum aviso do `foreach.c` no build (o total de 2 avisos é o que já havia;
a base é 4).

## Dúvidas e pontos abertos

1. **Binder com nome de declaração do módulo.** `k_em_handles_foreach` não
   recebe o `Emit`, então não pode checar isso; a checagem fica em
   `k_em_edits_foreach`, que recusa a chamada (`e->ok = false`, UNSUPPORTED).
   A causa: o texto `; i < ` é escrito cru, mas `node_edits` renomeia os
   tokens do binder no cabeçalho e no corpo. Um `handles` com acesso ao nome
   declarado tornaria a recusa uma recusa de ilha, o que seria mais limpo.
   Peço decisão (não é normativo, mas é contrato do emissor).
2. **Limites `A` e `B` de vários tokens** são aceitos (copiados como o autor
   escreveu, com os renomes das outras edições). `foreach (auto i : a+1..n)`
   passa, porque `a+1` não é tocado. O cartão falava em "literal", e não
   restringe a um token; deixei aberto.
3. **Forma sem tipo** (`foreach (i : 0..n)`, sem `auto` ou tipo) é recusada
   porque o cabeçalho exige duas palavras antes do `:`. Não sei se a gramática
   do keel permite essa forma; se permitir, o `handles` a recusa, o que é
   seguro, mas deixa o caso sem cobertura.
4. **Oráculo.** Não encontrei nada errado em `test/cgen/gen/foreach/`; o
   resultado bate com o cartão e com a tabela da §5.3/§5.7 (`{ for`, `size_t`,
   `=`, `; i < `, `; i++)`, ` }` e ` } }`).
5. **Uma pergunta sobre o cartão:** o cartão pede `handles` que olhe "o detalhe
   **e** os tokens". Olho o detalhe só para o `..` (um contêiner não o tem);
   a forma exata vem dos tokens. Se quiserem a checagem de sufixo `(T)`, é
   uma linha a mais, mas não a acrescentei para não recusar `0..f(x)` por
   engano.
6. **Não rodei `golden/run.sh` nem `sh tools/agents/verify.sh` como comando
   separado**: o `verify.sh` inclui o `golden/run.sh`, e ele deu
   `golden: 46 ok, 0 fail, 0 xfail, 0 xpass, 3 wip` (ALL OK).
7. **Comandos recusados pelo verificador.** Esta sessão foi recusada ao
   executar `sh test/cgen/gen_golden.sh` (caminho relativo), ao usar `PWD`
   dentro de comandos e ao encadear `git` com `cd`. Rodei as mesmas
   operações com caminhos absolutos e comandos simples, sem `git`. Não houve
   nenhuma operação de git além de `git status`/`git diff` no worktree.
