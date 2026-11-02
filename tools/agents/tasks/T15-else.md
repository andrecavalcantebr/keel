# T15 — WP5: cláusula `else` com a instrução na linha seguinte

## Objetivo

`else.c` recusa o `else` cuja instrução (ou bloco) está na linha seguinte (`implementation-limit`). O
oráculo é o caso **`golden/cases/042-else-lines`** (escrito à mão: `app/es.k`, `expected/c23/app/`,
`expected/c11/app/`, `proof.c`); leia o `es.k` e o `expected/c23/app/app_es.c` inteiros antes de tudo.

A escrita do autor é conservada (backend §5.12 r1 e r2): o texto ` else` vira `; if (M_failed(x))` **na
mesma linha**, e o resto — a quebra de linha, a indentação, a instrução ou o bloco da linha seguinte —
sai como foi escrito:

```
    outcome i32 v = read(n) else            ->    keel_outcome_i32 v = app_es_read(n); if (keel_outcome_i32_failed(v))
        return -1;                                       return -1;
```

O mesmo vale com `{` na linha seguinte (`split_block`).

## Arquivos

Pode editar: `tools/cgen/src/engine/emit/else.c`; se for inevitável, `emit.c`. Pode apagar o `|wip` da
linha `042-else-lines` de `test/cgen/gen/golden-cases.txt` quando fechar. Pode criar
`tools/agents/reports/T15-else.md`.

Não toque: normativos, `golden/**/expected/**`, nenhum `proof.c`, nenhum outro caso. Não faça commit.

## Contrato

- A forma de default (`decl = expr else valor;`) não muda.
- O `#line` não é seu: o `k_emit_at` decide pelas linhas de saída; o esperado não tem nenhum no corpo das
  funções, porque cada linha de fonte continua uma linha de saída.
- O que continua recusado: o que o comentário do alto de `else.c` lista e que não seja esta forma.

## Passos sugeridos

1. `sh test/cgen/gen_golden.sh`: veja o `wip 042-else-lines`; gere a saída numa pasta temporária e
   compare com o esperado. **Não copie a saída para o esperado.**
2. `make -C tools/cgen -j8` e `sh test/cgen/gen_golden.sh` inteiro (011 e 021 usam `else`).
3. `sh tools/agents/survey/run.sh`: e01 deve ir a `OK`.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # 042 sai de wip; 0 fail; 0 wip
golden/run.sh                   # "80 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
sh tools/agents/survey/run.sh   # e01 com OK nos dois perfis
```

## Relatório

`tools/agents/reports/T15-else.md`, em pt-BR: o que mudou; a saída final exata dos quatro comandos; toda
dúvida ou incoerência.
