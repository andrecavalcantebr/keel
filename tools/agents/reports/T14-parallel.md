# T14 — WP4: `parallel` com cabeçalho em duas linhas, `{` em linha própria e workers por `constexpr`

## O que mudou

**Causa real da recusa.** Não era a análise do cabeçalho (`analyze`, em `parallel.c`) que recusava as
três formas: `analyze` passava nelas. A recusa vinha de `emit.c`: a edição do gestor
(`[kw_off, end)`) atravessa a quebra de linha do cabeçalho. Em `copy_edited`, o texto restante da
linha (`b - pos`) ficava negativo (`size_t`), o construtor de linha estourava `LINE_CAP` e
`e->ok = false`, e `k_emit_file` devolvia `K_EMIT_UNSUPPORTED`. Além disso, a segunda linha do
cabeçalho (e a linha só com `{`) seria emitida como linha própria, o que o card proíbe.

Arquivo a arquivo:

1. `tools/cgen/src/engine/emit/emit.c` (o card permite se for inevitável; foi):
   - `copy_edited`: copia o resto da linha só se `pos < b`; guarda em `spanned_end` até onde o
     texto foi consumido (além do fim da linha, quando uma edição atravessa a quebra).
   - `emit_line`: zera `spanned_end` antes de copiar.
   - `emit_range` e o laço de `emit_c` (o caminho do `.c` de corpo de função): não emitem as linhas
     cujo início está dentro de uma edição que já as consumiu (`written`). O `#line` seguinte sai
     do próprio escritor, pelo número da linha seguinte (`#line 11`, `#line 30`, como no oráculo).
2. `tools/cgen/src/engine/emit/parallel.c`:
   - `Par` ganha `workers` (K como número). Em `analyze`, K é o literal decimal ou um `constexpr`
     cujo inicializador é literal decimal (busca de `k_em_constexpr_value`); qualquer outra coisa
     recusa.
   - `k_em_edits_parallel` usa `p.workers` (antes lia o token decimal direto).
   - Comentário do topo atualizado: cabeçalho em várias linhas e `{` em linha própria agora são
     aceitos; a lista de recusas passa a ter só: `{` que não termina a linha, workers que não sejam
     `0..K` com K decimal ou constexpr decimal, política que não é ALL/ANY/decimal, binder com mais
     de um nome, captura não declarada.
3. `tools/cgen/src/engine/emit/index.c` e `emit_internal.h`: novo `k_em_constexpr_value`, um
   invólucro de `constexpr_value` (a mesma busca que `index.c` usa para as dimensões). Só exposição,
   sem mudança de lógica.
4. `test/cgen/gen/golden-cases.txt`: apagado o `|wip` da linha `041-parallel-forms`. Nenhuma outra
   linha tocada.

Não foram tocados: normativos, `golden/**/expected/**`, nenhum `proof.c`, nenhum outro caso, nenhum
teste de unit. Sem commit, sem `git add`.

## Verificação

Ao longo do trabalho, a saída de `041` foi gerada num diretório temporário e comparada byte a byte
com `expected/c23/` e `expected/c11/` (arquivos `app_pf.c`, `app_pf.h`, `app_pf.type.h`): iguais nos
dois perfis. Nenhum esperado foi copiado da saída.

Saída final dos quatro comandos de aceitação:

```
$ sh test/cgen/gen_golden.sh
wip    042-else-lines (differs)
49 ok, 0 fail, 1 wip
(rc=0)

$ golden/run.sh | tail -1
80 ok, 0 fail, 0 xfail, 0 xpass, 3 wip

$ sh tools/agents/verify.sh
ok   build (0 warnings, baseline 0)
ok   make check (51 lines ok)
ok   golden: 80 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
(rc=0)

$ sh tools/agents/survey/run.sh
a01 [c23 OK] [c11 OK]
a02 [c23 OK] [c11 OK]
a03 [c23 OK] [c11 OK]
a04 [c23 OK] [c11 OK]
a05 [c23 LIMIT] [c11 LIMIT]
a06 [c23 OK] [c11 OK]
a07 [c23 OK] [c11 OK]
a08 [c23 OK] [c11 OK]
a09 [c23 OK] [c11 OK]
a10 [c23 OK] [c11 OK]
a11 [c23 OK] [c11 OK]
a12 [c23 OK] [c11 OK]
d01 [c23 OK] [c11 OK]
d02 [c23 OK] [c11 OK]
d03 [c23 OK] [c11 OK]
d04 [c23 OK] [c11 OK]
d05 [c23 OK] [c11 OK]
d06 [c23 OK] [c11 OK]
d07 [c23 OK] [c11 OK]
d08 [c23 LIMIT] [c11 LIMIT]
d09 [c23 LIMIT] [c11 LIMIT]
e01 [c23 OK] [c11 OK]
m01 [c23 OK] [c11 OK]
m02 [c23 OK] [c11 OK]
m03 [c23 OK] [c11 OK]
p01 [c23 OK] [c11 OK]
p02 [c23 OK] [c11 OK]
p03 [c23 OK] [c11 OK]
```

`p01`, `p02` e `p03` estão em `OK` nos dois perfis. `a05`, `d08` e `d09` continuam em `LIMIT`: estão
fora deste card e não foram tocados.

## Dúvidas e pontos em aberto

- **Desvio do card (permitido):** a correção de `emit.c` é a causa da recusa, e o card a previa
  como "se for inevitável". Mudei também `index.c` e `emit_internal.h` só para expor a busca do
  `constexpr`.
- **Caminho de `emit_range` sem oráculo:** o `written` em `emit_range` (usado para funções que vão
  ao `.h`, como as `pub inline`) não é exercitado por nenhum caso. O `041` cobre só o caminho do
  `.c` (`emit_c`). Vale um caso com `parallel` de cabeçalho em duas linhas dentro de função `pub
  inline`, se quiserem a cobertura; não o criei, por estar fora do card.
- **Recusa do `{` seguido de código na mesma linha** (`ends_line`) continua, e não tem oráculo
  próprio. Não foi alterada.
- **Constexpr importada:** `k_em_constexpr_value` só enxerga constantes do próprio módulo (e blocos
  do corpo). Um `0..W` com `W` importado é recusado pelo emissor. Não testei esse caso; o parser
  (`isl_decimal_of`) pode aceitar, o que deixaria a recusa do emissor como a única barreira.
- **Lixo de depuração:** para achar a causa, instrumentei uma cópia de `tools/cgen` fora do
  repositório (`scratchpad/t14`). O repositório foi restaurado antes das alterações finais; conferido
  com `grep` (sem `fprintf`/`DBG` em `parallel.c`, `index.c`, `emit.c`).
