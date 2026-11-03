# T36 — refatoração de `emit/emit.c` por partes (sem mudar comportamento)

## Objetivo

`tools/cgen/src/engine/emit/emit.c` tem ~1200 linhas e quatro responsabilidades misturadas. Separe-as em arquivos, **sem mudar uma linha do C gerado nem de qualquer diagnóstico**. É a refatoração do snapshot (skill `haiku-task`): o snapshot do comportamento do `--stop-after=parse` já foi tirado em `/tmp/claude-1000/-home-andre-code-cprojects-keel/6df5c4e9-8d62-4717-9b96-cb209ab7b812/scratchpad/snap-refactor`, e o oráculo principal é o `gen_golden` (byte a byte, mais a compilação do C escrito) com a base e a suíte inteira.

Leia `emit.c` inteiro e `emit_internal.h` antes de tudo. Os blocos, e o arquivo novo de cada um:

1. **`lines.c`** — a mecânica de linhas e edições: `copy_edited`, `emit_line`, `emit_split`, `split_line`, `comment_edits`, `drop_nested`, `emit_range` e o que for só delas.
2. **`files.c`** — a montagem dos arquivos: `type_h_body`, `h_body`, `emit_header`, `emit_entry`, `emit_c`, `type_h_has_content`, `h_has_content`, `k_emit_file` e o que for só delas.
3. **`decls.c`** — as declarações de nível de arquivo: a parte de declarações de `node_edits` (visibilidade, `typedef`, `tags`, variáveis…) se ficar separável sem mudar a ordem das edições, `emit_extern`, `emit_proto`, `emit_import`, `emit_constexpr_c11`, `first_of_span`, os auxiliares de `extern_c` (`extern_c_type_h`, `extern_c_priv`, `extern_c_lines`).
4. **`generic.c`** — a partição de módulo genérico: `k_em_generic_out`, `k_em_over_protocol`, `k_em_skip`, `k_em_other_modifier`, `uses_share`, `names_binder_or_modifier`.
5. **`supported.c`** — `supported()` e `islands_supported()` **movidas como estão** (o redesenho em aceitação por construção fica para outro cartão).

`emit.c` deve ficar só com o que sobrar de verdade (a ligação, o que as outras construções chamam) e **abaixo de ~400 linhas**.

## Regras da mecânica

- **Um bloco por vez**, na ordem acima. Depois de **cada** bloco: `make -C tools/cgen -j8` (0 warnings novos), `sh test/cgen/gen_golden.sh` (tem de continuar `65 ok, 0 fail, 0 wip`) e `sh test/cgen/base.sh` (`124 ok, 0 fail, 20 wip`). Se algo mudar, o defeito é seu: volte o bloco e refaça.
- Uma função `static` movida para outro arquivo perde o `static` e ganha prefixo `k_em_`; declare-a em `emit_internal.h` **só se** outro arquivo a usa. Estruturas e macros compartilhadas (`Emit`, `Edit`, `LINE_CAP`, `MAX_EDITS`…) já estão ou vão para `emit_internal.h`.
- Não renomeie nem reordene nada além do necessário; não reformate; não mude comentários, salvo o cabeçalho de cada arquivo novo (uma frase do que ele faz, com a referência à seção do backend, no molde dos outros `emit/*.c`) e o de `emit.c`.
- Regra de E/S do engine (`make boundary`): nada de `stdio` em `engine/`.
- Cada arquivo novo em `tools/cgen/src/engine/emit/`; o `Makefile` pega por curinga (confirme).

## Arquivos

Pode criar `lines.c`, `files.c`, `decls.c`, `generic.c`, `supported.c` em `tools/cgen/src/engine/emit/`, editar `emit.c` e `emit_internal.h`, e criar `tools/agents/reports/T36-refactor-emit.md`. Não toque: nada fora de `emit/`, normativos, golden, testes, outras construções (`match.c` etc.), `design/` (eu atualizo o mapa). Não faça commit nem `git add`. Se um teste (unit/ ou cli/) falhar por causa do que você moveu (um teste que inclui um arquivo por nome), guarde o patch fora do repositório e diga no relatório.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # 65 ok, 0 fail, 0 wip
sh test/cgen/base.sh            # 124 ok, 0 fail, 20 wip
golden/run.sh                   # "110 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh compare /tmp/claude-1000/-home-andre-code-cprojects-keel/6df5c4e9-8d62-4717-9b96-cb209ab7b812/scratchpad/snap-refactor   # ALL OK, comportamento igual ao snapshot
wc -l tools/cgen/src/engine/emit/emit.c   # abaixo de ~400
```

## Relatório

`tools/agents/reports/T36-refactor-emit.md`, em pt-BR: arquivo por arquivo, o que foi movido (nomes das funções), as linhas de cada arquivo, a saída final dos comandos, e toda função que **não** pôde ser movida e por quê. Entregue UMA vez pelo SubagentHandback e termine.
