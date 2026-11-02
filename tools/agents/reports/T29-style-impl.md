# T29-style-impl — relatório

Caso: `golden/cases/054-style-forms`. Três formas de estilo: `constexpr` que cita `constexpr`, `foreach` com a `{` do corpo em linha própria, `walk` com o cursor qualificado pelo nome do módulo.

## O que mudou (arquivo por arquivo)

1. `tools/cgen/src/engine/parser/parser_islands.c`: nova `constexpr_names`. Cada nome que aparece no inicializador de um `constexpr` de módulo recebe a ilha de nome (`isl_name_island`), como um uso em corpo de função. O laço que a chama roda depois das variáveis globais, com `locals` = globais (escopo do módulo). Membro após `.` ou `->` e chamada de C ficam de fora.
2. `tools/cgen/src/engine/emit/emit.c`, `emit_constexpr_c11`: o inicializador sai por `k_em_put_range` (por tokens), e não mais por `k_em_put_source` (texto cru). Assim os nomes do módulo são renomeados nos dois usos (macro e objeto de conferência). Remove as variáveis `is` e `ie`, que não têm mais uso.
3. `tools/cgen/src/engine/emit/foreach.c`:
   - `array_value` e `container_value` extraem o texto do binder (`T v = …;`), antes inline em `edits_over_array` e `edits_over`.
   - `body_brace_line` (novo): quando a `{` do corpo está numa linha própria (Allman), essa linha recebe ` T v = …;` após a `{`. Vale para `foreach` sobre contêiner e sobre `array`. A linha do `foreach` continua com o bloco externo `{ S keel__c0 = …; for (…)`, como no golden. A `}` já recebia ` }` pelo ramo existente.
   - `names_module` (novo): verifica se os tokens `a.b.c` soletram o nome dotado do módulo (`e->module`).
   - `edits_walk`: o cursor com tipo qualificado de vários segmentos (`app.sf.cursor`, `app3.wk01.cursor`) passa a ser reconhecido pelo nome do módulo, e sai só o nome mangled (`app_sf_cursor`). Antes só reconhecia um ou três tokens.
4. `test/cgen/gen/golden-cases.txt`: apenas o `|wip` da linha `054-style-forms` foi removido. Nenhum `golden/**/expected/**` ou `proof.c` foi tocado.

Nenhum normativo editado. Nenhum teste (unit/ ou cli/) afirmou o contrário do cartão: `make check` passou, então não há patch guardado. Nada foi adicionado à árvore além das alterações acima, e não houve `git add` nem commit.

## Verificação

Survey (`CGEN=$PWD/tools/cgen/cgen sh tools/agents/survey/run3.sh`, filtrado):

```
ce04 [c23 OK] [c11 OK]
fe05 [c23 OK] [c11 OK]
wk01 [c23 OK] [c11 OK]
```

Antes: as três eram BAD nos dois perfis.

Caso 054, antes e depois do `|wip` sair: a saída de `cgen --stop-after=gen` (com `-fopenmp`, como o `gen_golden`) é byte a byte igual a `expected/c23/app/*` e `expected/c11/app/*`.

`sh test/cgen/gen_golden.sh`:

```
61 ok, 0 fail, 2 wip
wip    056-length-fields
wip    055-defer-return-index
```

`golden/run.sh`:

```
106 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
```

`sh tools/agents/verify.sh`:

```
ok   build (0 warnings, baseline 0)
ok   make check (53 lines ok)
ok   golden: 106 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

## Pontos em aberto

- **`walk` com a `{` em linha própria (Allman) continua quebrado, e já estava assim antes desta tarefa.** A linha da `{` não recebe a declaração `i32 *p = S_next(...)`, e o C gerado não compila (`p` indeclarado). O cartão só pede Allman para `foreach`, então não mexi. Quem quiser fecha com a mesma ideia de `body_brace_line`, usando o texto do `walk` (`T *p = S_next(keel__c<N>, &c);`).
- **Redação do cartão sobre a linha do `foreach`.** O cartão diz "a linha do `foreach` sai sem chave", mas o golden tem `{ keel_slice_i32 keel__c0 = s; … for (…)` nessa linha: o bloco externo fica. Segui o golden, que é o oráculo; o trecho do cartão deve ser lido como "sem a chave do corpo". O §5.7 r5 da spec diz "sai na sua linha sem chave", com a mesma ambiguidade.
- **Efeito colateral no C11.** `emit_constexpr_c11` passou a renomear também usos de nomes de outros módulos dentro do inicializador (antes saíam crus). Nenhum golden mudou, mas é uma mudança de comportamento.
- **Comentário stale em `emit.c` (`supported()`, caso `K_AST_CONSTEXPR`).** Diz "INIT not naming the module", mas o código nunca testou isso. Não o alterei.
