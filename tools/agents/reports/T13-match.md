# T13 — WP3: `match` com operando de campo, braço na linha do rótulo e `break` ao lado de laço

## O que mudou

Arquivos tocados (nada mais; sem commit, sem `git add`):

1. `tools/cgen/src/engine/emit/match.c`
   - **Operando de campo** (`match (h->state)`, `h.state`): `analyze` aceita `ident ->|. ident )`
     (campo `m->field`). `field_operand` acha o tipo de `h` pela declaração (`decl_of`, o mesmo
     critério que o `analyze` já usava), acha no próprio módulo o `struct`/`typedef struct` com esse
     nome (`K_AST_TYPE`), acha o campo pelo nome no nível do corpo e lê `tagged <Set> <T>` (com `*`
     opcional, que vira `->tag`). O conjunto é resolvido por `k_symbol_resolve` e tem de ser
     `K_SYM_TAGS`. O `switch` lê o operando como escrito: `h->state.tag`. Campo de outro módulo,
     tipo qualificado (`other.Holder`), campo que não é `tagged` ou nome não resolvido: recusa.
   - **Instrução na linha do rótulo** (`A: r = 1;`): `Match.inl[i]`. O rótulo aceita instrução na
     mesma linha quando o último caractere não branco da linha é `;` ou `}` e o próximo token depois
     da linha é o próximo rótulo ou o `}` do `match`. Saída: `keel__m<N>_A: { r = 1; }` (um `EDIT_TEXT`
     de ` }` no fim da linha); antes do rótulo seguinte, só `goto keel__m<N>_end;` (sem `}` solto); o
     último braço inline não gera `}` na linha de fechamento, só `keel__m<N>_end: ;`.
   - **`break` de braço com laço** (`break_owner`): a pilha de blocos do braço. Um `{` consumido por
     `for`/`while`/`switch`/`foreach`/`match`/`do` é de construto; corpo sem chaves vai até o `;` (com
     `else` incluído). Um `break` é do braço (vira `goto keel__m<N>_end;`) só quando nenhum construto
     o envolve; o emissor usa a mesma função, então o que é reescrito é exatamente o que a análise
     aceitou. A análise recusa só o caso `BREAK_DEEP` (aninhamento acima de `BLOCK_CAP` = 32).
   - Cabeçalho do arquivo atualizado (a lista "Refused" e a descrição dos braços).
2. `test/cgen/gen/golden-cases.txt`: apagado só o `|wip` da linha `040-match-forms`.

Não tocados: normativos, `golden/**/expected/**`, `proof.c`, outros casos, `emit.c`,
`emit_internal.h`. Nenhum teste de unit afirma recusa de `match` (grep em `test/cgen/unit/` achou
apenas `match_option.*`, que é outro assunto): nada precisou ser guardado fora do repositório.

## Verificação

Comandos de aceitação, saída final (na ordem do cartão):

```
$ sh test/cgen/gen_golden.sh
wip    041-parallel-forms
wip    042-else-lines
48 ok, 0 fail, 2 wip

$ golden/run.sh
wip    022-linux-list-import-c       — expected/ ainda não escrito — esqueleto do caso (ver NOTES)
wip    023-linux-list-extern-c       — expected/ ainda não escrito — esqueleto do caso (ver NOTES)
wip    024-linux-list-module         — expected/ ainda não escrito — esqueleto do caso (ver NOTES)
80 ok, 0 fail, 0 xfail, 0 xpass, 3 wip

$ sh tools/agents/verify.sh
ok   build (0 warnings, baseline 0)
ok   make check (51 lines ok)
ok   golden: 80 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK

$ sh tools/agents/survey/run.sh   (linhas m0*)
m01 [c23 OK] [c11 OK]
m02 [c23 OK] [c11 OK]
m03 [c23 OK] [c11 OK]
```

- `gen_golden.sh`: 040 saiu de `wip` e as duas formas (c23 e c11) batem byte a byte com o esperado
  (`app_mf.c` comparado à mão, além da execução do gen_golden inteiro). 041 e 042 continuam `wip`.
- `golden/run.sh`: a linha exata do cartão. Os `Aborted` que aparecem ao rodar o `golden/run.sh`
  inteiro vêm dos casos de debug-checks (`KEEL_CHECK` que aborta de propósito) e já existiam antes.
- `verify.sh`: 0 warnings, o mesmo baseline (`tools/agents/warnings.baseline` = 0).
- Além do cartão, testado fora do repositório (`scratchpad/t13/edge/`): `break` de `for` sem chaves
  seguido de `break` de braço (o primeiro fica, o segundo vira `goto`); `switch` dentro do braço
  (fica); braço inline com linha de continuação (recusado); campo `int` em vez de `tagged` (recusado);
  campo ponteiro `tagged ... *state` (sai `h->state->tag`).

## Dúvidas e incoerências

1. **Regra extra além do cartão (inline).** O cartão diz só "termina com `;` ou `}` na mesma linha".
   Implementei também a exigência de que o braço inteiro caiba nessa linha (o próximo token depois
   da linha é o próximo rótulo ou o `}` do `match`). Sem isso, `A: r = 1;` seguido de `r += 2;` na
   linha de baixo ficaria com o `r += 2;` fora das chaves. Vale confirmar que é essa a intenção.
2. **Comentário no fim da linha inline.** `A: r = 1; // x` é recusado (o último caractere não é `;`).
   Conservador; se o normativo quiser aceitar, é uma mudança de uma linha em `analyze`.
3. **`k_token_is_ident` recusa palavras-chave C** (`void` não é `ident` por essa função). Achei isso
   pelo debug. Para cartões futuros: use a checagem de primeira letra (`word_like`, em `match.c`) para
   tipos C, não `ident`. Não mexi em `token_predicates_words.c`.
4. **Campo só é resolvido no próprio módulo.** `struct` ou `typedef` de outro módulo, ou tipo
   qualificado, são recusados (`implementation-limit`) por design do cartão.
5. **Os três `wip` de `golden/run.sh`** são 022, 023 e 024 (esqueletos sem `expected/`), antes e depois
   da tarefa. O 040 não é caso `wip` de `golden/run.sh`; só estava marcado em `gen/golden-cases.txt`,
   e isso está corrigido.
6. **Observação de processo.** Um `rm -rf` de limpeza de pasta de scratch foi bloqueado pela checagem
   de segurança (alvo com variável de shell); não foi executado e não fez falta. A pasta de scratch
   foi usada só com caminhos novos.
