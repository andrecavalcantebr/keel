# T13 — WP3: `match` com operando de campo, braço na linha do rótulo e `break` ao lado de laço

## Objetivo

`match.c` recusa três formas válidas (`implementation-limit`). O oráculo é o caso
**`golden/cases/040-match-forms`** (escrito à mão: `app/mf.k`, `expected/c23/`, `expected/c11/`,
`proof.c`); leia o `mf.k` e o `expected/c23/app/app_mf.c` inteiros antes de tudo. As três formas:

1. **Operando que é campo** de struct: `match (h->state)`, com `Holder { tagged Cycle void state; }`.
   O que o `switch` lê é o campo de etiqueta do campo: `h->state.tag` (backend §5.6 r3; por valor, `.tag`;
   se o campo for ponteiro para o `tagged`, `->tag`). Hoje só o operando de nome simples é aceito.
2. **Instrução na linha do rótulo**: `A: r = 1;` (função `inline_arms`). Saída: o bloco do braço fica na
   própria linha, `keel__m0_A: { r = 1; }`, seguido da linha `goto keel__m0_end;` (exceto no último
   braço). Nada de `{` sozinho em linha: a escrita do autor, uma linha de fonte, uma linha de saída. O
   `}` que fecha o `match` escreve só `keel__m0_end: ;` (o último braço já se fechou). Os `#line` são os do
   esperado.
3. **`break` de braço com um laço no mesmo braço** (função `with_loop`): o `break;` no nível do braço
   vira `goto keel__m0_end;` (backend §5.6 r6); o `break;` **dentro** do `for` escrito pelo programa
   continua `break;`. Hoje o emissor recusa qualquer `break` num braço que tem laço, `switch` ou `match`;
   passe a decidir pela profundidade: um `break` é do braço quando não está dentro de chaves de laço,
   `switch` ou `match` aninhado dentro do braço (rastreie a pilha de blocos, como o `defer.c` faz com
   `exit_target`).

## Arquivos

Pode editar: `tools/cgen/src/engine/emit/match.c`; se for inevitável, `emit.c` e `emit_internal.h`. Pode
apagar o `|wip` da linha `040-match-forms` de `test/cgen/gen/golden-cases.txt` quando fechar. Pode
criar `tools/agents/reports/T13-match.md`.

Não toque: normativos, `golden/**/expected/**`, nenhum `proof.c`, nenhum outro caso. Não faça commit.

## Contrato

A saída exata é a do esperado; as regras que você precisa respeitar:

- O tipo do campo vem da declaração do `struct` (`Holder`), que está no módulo: ache o campo pelo tipo
  declarado da variável do operando (`h`), como o `analyze` faz hoje para variáveis, e leia o tipo do
  campo (`tagged Cycle void`) da declaração dele. Campo de outro módulo ou tipo que você não resolva:
  recuse (`implementation-limit`) em vez de adivinhar.
- O conjunto e o formato do `case` não mudam: `case app_mf_Cycle_A: goto keel__m0_A;` etc.
- O contador `N` (`keel__m<N>`) é por função e conta cada `match` — como hoje.
- Um rótulo que divide a linha com a instrução só vale quando a instrução termina com `;` ou `}` na
  mesma linha. Se a instrução continuar na linha seguinte, recuse.

Código existente (leia no arquivo): o comentário do alto de `match.c` (a lista "Refused"), `analyze`
(`Match`, `starts_line`/`ends_line`, a varredura de `break`/`inner`), `resolve_operand`, e
`k_em_edits_match` (os registros `EDIT_LINES` com `L<n>|`/`T|`, formato em `emit_internal.h`).

## Passos sugeridos

1. `sh test/cgen/gen_golden.sh`: veja o `wip 040-match-forms`; gere a saída numa pasta temporária e
   compare com o esperado. **Não copie a saída para o esperado.**
2. Uma forma por vez: o `break`, o rótulo na linha, o operando de campo. Depois de cada uma:
   `make -C tools/cgen -j8` e `sh test/cgen/gen_golden.sh` inteiro (009 e 029 também usam `match`).
3. `sh tools/agents/survey/run.sh`: m01, m02 e m03 devem ir a `OK`.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # 040 sai de wip; 0 fail; os outros wip (041, 042) continuam
golden/run.sh                   # "80 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
sh tools/agents/survey/run.sh   # m01, m02, m03 com OK nos dois perfis
```

Se algum caso que passava deixar de passar, o defeito é seu. Se o esperado parecer errado, pare e diga
no relatório, com a evidência.

## Relatório

`tools/agents/reports/T13-match.md`, em pt-BR: o que mudou; a saída final exata dos quatro comandos; toda
dúvida ou incoerência.
