---
name: cgen-ligacao-e-tags-2026-11-20
description: dois bugs do cgen fechados na 0.4.0-dev: conjunto tags >16 itens (agora 128, tags-limit-exceeded) e ordem de ligação (.a/-l depois dos .c gerados); versão 0.4.0-dev em ~/Apps
metadata:
  type: project
---

Fechados na branch `0.4.0` em 2026-11-20 (achados do xprk e da revisão de distribuição, registrados em `achados/README.md`):

1. **`tags` com 17+ itens** dava `unexpected-token` no início da declaração: `K_TAGS_MAX_ITEMS` era 16 (`parser.h`). Agora 128, e acima disso o diagnóstico novo **`tags-limit-exceeded`** (id do cgen, só em `diag-messages.md` e `diag.h`, não está em normativo), com a posição no nome do conjunto e o limite na mensagem. Teste `test/cgen/cli/tags_many.sh`.
2. **Ordem de ligação**: o cgen punha o `.a`/`-l` do usuário antes dos `.c` gerados, e o ligador dava `undefined reference`. Agora `.a`, `.so`, `-l`, `-L`, `-Wl,`, `-Xlinker` (e o valor separado de `-l`/`-L`) vão depois das unidades geradas, em ordem relativa. `run_cc.c`, `is_late_input`; cgen-tool-spec §4.1 *(desde 0.4.0)*; manual §7.6; teste `test/cgen/cli/link_order.sh`.

`--cgen-version` diz `cgen 0.4.0-dev` (string em `main.c` e `args_partition.sh`). `~/Apps/keel-0.4.0-dev` é o build instalado (`make release VERSION=0.4.0-dev` + tar + install.sh; o symlink `~/.local/bin/cgen` aponta para ele; `keel-0.3.0` continua lá). Refazer a dist depois de mudar o cgen.

**Why:** xprk (../xprk) e as libs usam o cgen instalado; o `cgen` no PATH é o de ~/Apps, não o de `tools/cgen/cgen` (cuidado ao testar um conserto: usar o binário do repo com `--base-dir base`).
**How to apply:** ao testar correção no cgen, rodar `tools/cgen/cgen`, não `cgen`. Ver [[oraculo-isolado-e-exit-code-real]].
