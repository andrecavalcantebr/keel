---
name: golden-perfis-autorais
description: "No golden do keel, c11/ e c23/ são árvores autorais independentes; não derivar uma da outra"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 6686ee1b-2ce1-44eb-8aca-16c163421dc6
  modified: 2026-09-16T19:56:28.214Z
---

Em 2026-09-16 o André decidiu que a árvore `golden/c11/` **não** deve ser
derivada de `golden/c23/`: "isso seria realmente opção do backend escolher o que
e como". O `derivar-c11.sh`, que gerava uma da outra por `sed`, foi removido.
Cada perfil é saída esperada escrita à mão.

**Why:** o que cada perfil emite é decisão do backend, não uma reescrita textual
fixa do outro. A lista de diferenças do `keel-c-backend.md` §9.1 descreve a
versão de hoje, não um contrato. Na prática o derivador já mentia em dois
arquivos mantidos à mão — `c11/keel/prelude.h` (precisa de `<stdbool.h>` e
`<assert.h>`) e `casos/008-constexpr-bloco/esperado/c11/app/cx.c` (`constexpr`
de bloco vira macro com nome reescrito, §9.2) — e os apagava quando rodado.

**How to apply:** ao mexer num caso golden, editar os dois perfis; não escrever
script que gere um a partir do outro. O `README.md` de `golden/` registra a
decisão. Ver [[keel-quatro-documentos]].
