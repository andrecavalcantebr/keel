# M6c — relatório: `x[i]` e `x[a..b]` (subscript.c)

## Base de trabalho

O cartão e o `M6-contexto.md` estavam só no `master` (commit `b6accd8`), e a worktree
estava 18 commits atrás. A worktree estava limpa e o `HEAD` era ancestral do `master`:
fiz `git merge --ff-only master` antes de começar. Nenhum commit foi feito.

## O que mudou

Um arquivo, dentro da lista do cartão:

- `tools/cgen/src/engine/emit/subscript.c` — de esqueleto (nada baixava) a emissor
  completo das ilhas `index` e `range-index`.

Não tocado: `golden/`, `test/cgen/gen/golden-cases.txt`, documentos normativos.

### Forma do arquivo

- `read_sugar` lê a ilha (`K_ISLAND_INDEX` ou `K_ISLAND_RANGE_INDEX`) e confere a forma
  antes de aceitar. O detalhe (`b → keel_buffer_i32_ptr1`, `b 5.. → … keel_buffer_i32_length`,
  com ` &1` opcional no fim) dá os símbolos; os índices e limites são copiados como estão
  escritos.
- `k_em_handles_subscript` aceita só o que `read_sugar` lê. Recusa (vira
  `implementation-limit`, o caso fica `wip`, nunca sai código errado):
  - contêiner que não é nome simples (`s.f[i]`, anchor seguido de `.`/`->`);
  - vírgula no nível dos colchetes (`x[i,j]`, §5.3.1);
  - nome precedido de `&` (regra 2 do §5.3, colapso de `&*`: ver ponto em aberto 1);
  - colchete que atravessa duas linhas;
  - ilha fora de corpo de função;
  - número de palavras do detalhe diferente do esperado pela forma.
- `k_em_edits_subscript`, por ilha dentro da função e da linha:
  - `x[i]`: insere `*sym(` (e `&` se `&1`) antes do nome; `[` vira `, `; `]` vira `)`.
  - `x[a..b]`: insere `sym(` antes do nome; `[` vira `, ` (ou `, 0` sem limite inferior);
    `..` vira `, `; sem limite superior, insere `length(` + nome copiado (`EDIT_TOKS`)
    depois do `..` e fecha com `))` no `]`; com limite superior, fecha com `)`.

Nenhuma aritmética de posição foi precisa além de `k_em_tok_off`/`k_em_tok_end`, e nenhuma
edição se sobrepõe a outra (todas as substituições cobrem tokens disjuntos).

## Saída dos comandos de aceitação

`sh tools/agents/verify.sh`:

```
ok   build (0 warnings, baseline 0)
ok   make check (50 lines ok)
ok   golden: 48 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

`sh test/cgen/gen_golden.sh` (parte do `make check`), no estado final:

```
wip    011-else
wip    013-sugar (differs)
wip    015-foreach-range
wip    018-corot-outcome
wip    021-else-assign
18 ok, 0 fail, 5 wip
```

**A aceitação do cartão não foi atingida:** `013-sugar` continua `wip (differs)`. A causa
está no `.h` esperado, não no emissor (ver divergência 1).

Verificação direta do caso 013, nos dois perfis, com `--stop-after=gen`:

- `expected/c23/app/app_ac.c` == saída: `diff` sem diferença (rc 0).
- `expected/c11/app/app_ac.c` == saída: `diff` sem diferença (rc 0).

Compilação do build do cgen: 0 avisos novos.

## Divergências e pontos em aberto

1. **Golden com erro de espaço no `.h` (pare-e-diga, não editado).** Em
   `golden/cases/013-sugar/expected/{c23,c11}/app/app_ac.h`, as linhas 9, 11 e 13 são
   `i32    app_ac_bump(...)`, `i32    app_ac_once(...)`, `i32    app_ac_cube(void)`
   (quatro espaços). O `.k` tem `pub i32 bump(` (um espaço). O `emit_proto`
   (`engine/emit/emit.c`, ~linha 388) copia o texto do `.k` do primeiro token até o `)`,
   sem visibilidade; o caso `005-defer-loop` confirma a regra (`pub int  live_now` sai
   `int  app_loop_live_now`, com os dois espaços da fonte). Logo o esperado de 013 contradiz
   o `.k` e a regra ("a emissão segue o `.k` sempre"). Saída real: `i32 app_ac_bump(...)`.
   Os `.c` dos dois perfis batem byte a byte. Para o caso sair de `wip`, o `.h` esperado
   precisa ser corrigido à mão (decisão do André/orquestrador); eu não toquei.
   (O `015-foreach-range` tem o mesmo padrão de espaçamento, mas não é deste cartão.)
2. **Regra 2 do §5.3 (`&*`) não implementada.** `&x[i]` é recusado (`read_sugar` devolve
   falso) em vez de sair `&*sym(...)`. Não está no cartão, e o caso 013 não o usa. Quem
   decide: implementar o colapso (remover o `&` e o `*`) numa tarefa própria.
3. **`&1` (contêiner por endereço) não exercitado pelo caso.** O código emite `&` antes do
   nome quando o detalhe traz ` &1`, nos dois verbos (`as_slice` e `length`), mas o 013 não
   tem `buffer` por valor. Não verificado por saída de golden.
4. **Forma `s.f[i]` recusada.** O parser já emite ilha para `x.f[i]` com o caminho no
   detalhe; o cartão não cobre, e recusar é o seguro.

## Relação com o cartão

Os passos do cartão: leitura do `.k`, do `.c` esperado e das ilhas (`cgen --stop-after=parse`);
`k_em_handles_subscript` e `k_em_edits_subscript` escritos juntos no mesmo arquivo; depois
um build (0 avisos) e o diff dos dois perfis. Não houve build intermediário entre os dois
passos de código.
