# T53 — relatório: função sobre protocolo é sempre inline (§4.14, regra 8)

## O que mudou (arquivo a arquivo)

- `tools/cgen/src/engine/emit/files.c`, `header_body`: se a instância tem
  `proto_fn`, o corpo vai para o `.h` (a única função que `h_body` deixa passar
  por `skip` é a própria função sobre protocolo).
- `tools/cgen/src/engine/emit/decls.c`, `k_em_decl_edits`: a função sobre
  protocolo de uma instância recebe a edição de visibilidade `pub` -> `static`
  (ou `static inline` quando a palavra `inline` não está escrita). Sem isso o
  protótipo e o corpo saíam sem `static inline`. A forma com `[[nodiscard]]`
  em C23 segue a mesma regra.
- `tools/cgen/src/engine/emit/uses.c`, `k_em_h_body_includes`: o corpo da
  função sobre protocolo conta como "corpo no `.h`" mesmo sem `inline`, de modo
  que o `#include` do `.h` dos módulos que ele chama (seção 3) é emitido como no
  caso com `inline`.
- `test/cgen/gen/golden-cases.txt`: tirado o `|wip` do 072. Nenhum outro arquivo
  de golden ou normativo foi tocado.
- Criado este relatório.

Nota sobre o cartão: a correção do `header_body` sozinha não basta. Faltavam
também a edição `static inline` (decls.c) e a condição do `body` em uses.c. O
cartão pedia para confirmar a causa antes de copiar, e foi isso que mostrou a
lacuna.

## Investigação dos cinco pontos de `instance.c`

O cartão apontava `is_inline` em cinco pontos de `instance.c`. Lidos um a um:

- Linha 443, `emit_proto`: só é chamada de dentro do ramo `else` de `emit_file`
  (linha 595 em diante). Esse ramo é inalcançável: `emit_file` só é chamada com
  `K_EMIT_TYPE_H` (linha 753-760 do `k_emit_instance` manda `K_EMIT_H` para
  `k_em_instance_h` e `K_EMIT_C` para `k_em_instance_c`). Não afeta o 072.
- Linhas 499-510, `has_function` e `has_inline`: usadas só no mesmo ramo morto.
  Não afetam o 072.
- Linha 605, o laço de `emit_body` com `is_inline`: no mesmo ramo morto. Não
  afeta o 072.
- Linha 726, o laço de `emit_body` com `!n->is_inline` em `k_em_placement_c`:
  roda só para `placed(c)`, que exige `c->kind == K_CLOSURE_INSTANCE`. Função
  sobre protocolo tem `K_CLOSURE_PROTOCOL_FUNCTION`, então não entra. Não afeta o
  072 nem os goldens (ver suspeita abaixo).
- Linha 768, `k_emit_instance_redundant`: chamada só de `stop_gen.c:672`, dentro
  do bloco de instância ordinária (`!proto_fn.len`). Não foi alterada, e o
  resultado não muda com a correção.

Conclusão: nenhum dos cinco pontos afeta o 072 nem algum golden existente, então
nenhum foi alterado. Não toquei em `instance.c`.

Suspeita registrada (não investigada com probe, não há golden): o laço de
`k_em_placement_c` (linha 726) não filtra funções sobre protocolo. Um módulo
genérico com `instance` **e** uma função sobre protocolo `pub` sem `inline` teria
o corpo dessa função copiado no `.c` de cada instância colocada. Isso pode ser
um defeito separado, do mesmo tipo que o do 072. Vale abrir um caso novo se o
André quiser, mas não foi tratado aqui, conforme o cartão.

## Saída dos comandos de aceitação

`make -C tools/cgen -j8 2>&1 | grep -i "warning\|error"`: nenhuma linha.

`sh tools/agents/verify.sh`:

```
ok   build (0 warnings, baseline 0)
ok   make check (59 lines ok)
ok   golden: 144 ok, 0 fail, 0 xfail, 0 xpass, 0 wip
ALL OK
rc=0
```

`test/run.sh > /tmp/t53.out 2>&1; tail -8 /tmp/t53.out`:

```
ok     072-protocol-function-implicit-inline c23  (run)
ok     072-protocol-function-implicit-inline c11  (run)
ok     layout-check                   (codegen E6)
ok     line-check                     (backend §6)

144 ok, 0 fail, 0 xfail, 0 xpass, 0 wip
docs: 45 ok, 0 fail
EXIT=0
```

Comparação byte a byte do 072, os dois perfis (fora `keel.h` e `keel.type.h`,
que são o prelúdio): sem diferenças em `c23` nem em `c11`.

Verificações adicionais:

- 065 e 071: continuam iguais aos `expected/` (o `gen_golden.sh` compara byte a
  byte, e `make check` passou).
- Probe com `pub inline` explícito sobre a mesma forma do 072
  (`/tmp/t53-probe/pf2.k`): gera o mesmo C que o oráculo sem a palavra, nos dois
  perfis (sem diferenças fora o prelúdio).

## Pontos abertos

- A suspeita do laço de `k_em_placement_c` (acima).
- O ramo `else` de `emit_file` em `instance.c` (linhas 595-608) parece código
  morto. Não o removi: fora do escopo do cartão.
