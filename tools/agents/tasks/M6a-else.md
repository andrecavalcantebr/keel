# M6a — a cláusula `else` de resultado

Leia antes: `.claude/skills/haiku-task/SKILL.md` (a metade do agente), `AGENTS.md`,
`tools/agents/tasks/M6-contexto.md`, e o backend `keel-c-backend.md` §5.12 ("Cláusula
`else`") inteiro, mais a spec `keel-spec.md` §4.10.

## Objetivo

Baixar a ilha `else` (o tratamento de falha de um `outcome`, na declaração e na
atribuição), para que os casos `011-else`, `018-corot-outcome` e `021-else-assign` saiam
pelo cgen iguais aos `expected/`, nos dois perfis. Se um desses casos esbarrar noutra
construção que não é `else` (outra ilha recusada), registre no relatório e siga com os
outros.

## Arquivos

Edita só `tools/cgen/src/engine/emit/else.c` (hoje um esqueleto que não baixa nada). Cria
`tools/agents/reports/M6a-else.md`. Nada mais.

## Contrato

As ilhas (detalhe exato, do parser):

```
ilha else  v (outcome i32) exit → keel_outcome_i32_failed                                            app/el.k:20:29
ilha else  s (outcome slice i32) default → keel_outcome_keel_slice_i32_failed keel_outcome_keel_slice_i32_win1 &1   app/el.k:32:41
```

Formato: `<alvo> (<tipo keel>) <exit|default> → <símbolo de failed> [<símbolo de win> <marcas>]`.
A âncora é o token `else`. `exit`: o tratamento começa por `{` ou por palavra de salto
(`return`, `break`, `continue`, `goto`); `default`: o tratamento é um valor padrão, e o
`win` o grava no alvo (`&1` = o 1º argumento leva `&`). Leia o `.k` e o `.c` esperado de
cada caso lado a lado: a forma de saída de cada variante está lá, linha por linha
(o lowering de uma linha de fonte ocupa uma linha de saída, backend §6 regra 2).

`k_em_handles_else` aceita a ilha `else` cujo detalhe tem a forma acima; `k_em_edits_else`
faz as edições nas linhas da função. Use `k_em_tok`, `k_em_tok_off`, `k_em_tok_end`,
`k_em_text_is`, `k_em_push_edit`, `k_em_scratch` (emit_internal.h). O texto do detalhe da
ilha está em `keel_buffer_char_ptr(&ast->island_text, is->text)`, com `is->text_len` bytes
(veja como `uses.c` lê um detalhe: `detail_of`, uses.c:29).

## Passos sugeridos

1. Imprima as ilhas `else` dos três casos e leia os esperados.
2. Comece pelo `011` (`exit` e `default`), depois `018` e `021`.
3. Compile a cada passo (`make -C tools/cgen -j8`), sem warning novo.

## Aceitação

`sh tools/agents/verify.sh` → `ALL OK`; `sh test/cgen/gen_golden.sh` não lista mais `011-else`
(e, se nada mais os travar, `018-corot-outcome` e `021-else-assign`) como `wip`.

## Relatório

`tools/agents/reports/M6a-else.md`: o que mudou, a saída final dos comandos de aceitação,
cada caso que ainda fica `wip` e por quê (a primeira ilha recusada), e toda dúvida.
