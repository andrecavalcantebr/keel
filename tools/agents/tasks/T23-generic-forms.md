# T23 — módulo genérico: modificador em várias linhas, comentário na declaração e função `priv`

## Objetivo

Oráculo: **`golden/cases/051-generic-forms`** (escrito à mão: `gx.k`, `inst.k`, `expected/c23/`, `expected/c11/`, `proof.c`). Leia os dois `.k` e todo o `expected/c23/` antes de tudo. O módulo genérico `gx` tem: um `modifier` escrito em várias linhas (`stack`), um com comentário dentro da declaração (`cell`), e uma função `priv` (`peek`) chamada por uma `pub` sem `inline` (`get`); `inst.k` coloca `instance gx.stack i32;` e `instance gx.cell i32;`. Formas hoje recusadas (`implementation-limit`, em `instance.c`):

1. **Modificador em várias linhas** → o `typedef struct` da instância conserva as quebras do autor (`typedef struct gx_stack_i32 {` / `    size_t cap, len;` / `    i32 *ptr;` / `} gx_stack_i32;`), com um `#line 3` só, porque as linhas são consecutivas (1:1).
2. **Comentário dentro da declaração**: o comentário sai e os brancos em volta ficam como escritos (`typedef struct gx_cell_i32 {  i32 v; } gx_cell_i32;`, dois espaços).
3. **Função `priv` num genérico**: pertence à instância (cita `cell`), não vai ao header, e o corpo vai ao `.c` de quem declara `instance` (`inst.c`), como `static`: `static i32 gx_cell_i32_peek(gx_cell_i32 *c) { return c->v; }` com `#line 10 "gx.k"`, e logo depois o corpo de `get` com `#line 12 "gx.k"`.

## Arquivos

Pode editar, em `tools/cgen/src/engine/emit/`: `instance.c`, `emit.c`, `emit_internal.h`. Pode apagar o `|wip` da linha `051-generic-forms` de `test/cgen/gen/golden-cases.txt`. Pode criar `tools/agents/reports/T23-generic-forms.md`.

Não toque: normativos, `golden/**/expected/**`, nenhum `proof.c`, nenhum outro caso. Não faça commit nem `git add`. Se um teste de unit afirmar o contrário, não o edite: guarde o patch fora do repositório e diga no relatório.

## Contrato

- Código existente: o comentário do alto de `instance.c` (lista "Refused"), `put_gap` ("A comment would carry its text into a line of C: refused"), e o emissor do `.c` de quem declara `instance` (007: `golden/cases/007-instance/expected/c23/instances.c`).
- **Fica recusado** (sem regra no backend; não implemente): tipo ou constante de um genérico que cita um parâmetro mas **nenhum modificador** (por exemplo `pub constexpr size_t CAP = N;`), porque o nome na instância é ambíguo. Recuse com `implementation-limit`.

## Passos sugeridos

1. `sh test/cgen/gen_golden.sh`: veja o `wip 051-generic-forms`; gere a saída e compare. **Não copie a saída para o esperado.**
2. Uma forma por vez; `make -C tools/cgen -j8` e `sh test/cgen/gen_golden.sh` inteiro a cada passo (007 e `base.sh` usam genéricos: rode também `sh test/cgen/base.sh`).
3. `sh tools/agents/survey/run.sh`: g02, g03 e g04 devem ir a `OK` (g01 fica `LIMIT`).

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # 051 sai de wip; 0 fail
sh test/cgen/base.sh            # "124 ok, 0 fail, 20 wip"
golden/run.sh                   # "100 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
sh tools/agents/survey/run.sh   # g02, g03, g04 com OK nos dois perfis
```

## Relatório

`tools/agents/reports/T23-generic-forms.md`, em pt-BR: o que mudou; a saída final exata dos comandos; toda dúvida ou incoerência. Entregue o relatório UMA vez pelo SubagentHandback e termine.
