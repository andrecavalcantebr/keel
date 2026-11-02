# T20 — `match`: comentário na linha do braço, rótulos empilhados na linha, `match` numa linha, operando de campo de outro módulo

## Objetivo

Oráculos: **`golden/cases/047-match-more`** (`app/mm.k`) e **`golden/cases/052-match-foreign`** (`app/lib/other.k` + `app/fm.k`), escritos à mão, com `expected/c23/` e `expected/c11/` e `proof.c`. Leia os `.k` e os `expected/c23/` antes de tudo. Formas hoje recusadas (`implementation-limit`), todas em `match.c`:

1. **Braço na linha do rótulo com comentário** (`A: r = 1;   // one`): o comentário sai (`comment_edits`), e o resto é o braço de uma linha do T13 (`keel__m0_A: { r = 1; }`).
2. **Rótulos empilhados na mesma linha** (`A: B:` e o corpo na linha seguinte): `keel__m0_A: keel__m0_B: {` numa linha só (backend §5.6 r4), com o `#line` do rótulo.
3. **`match` inteiro numa linha** (`match (s) { A: r = 1; B: r = 2; }`): o despacho (6 linhas, como sempre) e depois os braços **numa linha de saída só**: `keel__m0_A: { r = 1; } goto keel__m0_end; keel__m0_B: { r = 2; } keel__m0_end: ;`; sem `#line` entre o despacho e essa linha, e o `#line` da linha seguinte do fonte depois dela.
4. **Operando que é campo de struct de outro módulo** (`match (g->ph)` com `Gate` de `app.lib.other`): os rótulos são do conjunto desse módulo (`case app_lib_other_Phase_ON: …`), e o `switch` lê `g->ph.tag`. Hoje o campo só é achado no próprio módulo; ache-o na AST do módulo importado (o carregador as retém: veja como `uses.c` ou `emit.c` acham a declaração de um tipo de outro módulo, por exemplo `k_em_h_forward_decls`, e `resolve_operand` em `match.c`).

## Arquivos

Pode editar: `tools/cgen/src/engine/emit/match.c`; se for inevitável, `emit.c`, `uses.c` e `emit_internal.h`. Pode apagar o `|wip` das linhas `047-match-more` e `052-match-foreign` de `test/cgen/gen/golden-cases.txt`. Pode criar `tools/agents/reports/T20-match-more.md`.

Não toque: normativos, `golden/**/expected/**`, nenhum `proof.c`, nenhum outro caso. Não faça commit nem `git add`.

## Contrato

- Código existente: o comentário do alto de `match.c` (lista "Refused"), `analyze`, `resolve_operand`, `k_em_edits_match` (registros `EDIT_LINES` `H|`/`L<n>|`/`T|`, formato em `emit_internal.h`), e o braço inline do T13.
- A largura do `case` segue a regra de `match.c` (a maior etiqueta); o esperado de 052 já a tem.

## Passos sugeridos

1. `sh test/cgen/gen_golden.sh`: veja os `wip`; gere a saída e compare com o esperado. **Não copie a saída para o esperado.**
2. Uma forma por vez; `make -C tools/cgen -j8` e `sh test/cgen/gen_golden.sh` inteiro a cada passo (009, 029, 040 usam `match`).
3. `sh tools/agents/survey/run.sh`: t02, t04, t05 e y01 devem ir a `OK`.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # 047 e 052 saem de wip; 0 fail
golden/run.sh                   # "102 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
sh tools/agents/survey/run.sh   # t02, t04, t05, y01 com OK nos dois perfis
```

## Relatório

`tools/agents/reports/T20-match-more.md`, em pt-BR: o que mudou; a saída final exata dos quatro comandos; toda dúvida ou incoerência. Entregue o relatório UMA vez pelo SubagentHandback e termine.
