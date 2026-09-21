---
name: base-real-keel-em-progresso
description: "/base (keel real, não $T) — completa; golden re-derivado dela em 2026-09-20 (P15 fechada)"
metadata: 
  node_type: memory
  type: project
  originSessionId: eaa22aed-ef9c-4f55-828b-7c4fe65d59ac
  modified: 2026-09-20T22:07:56.741Z
---

`/base`, na raiz do repositório, é o alvo final: fonte keel de verdade que o
futuro `cgen` processa. **Não confundir** com `tools/codegen/base` nem com
`tools/cgen/gen`, que são do bootstrap `$T`. Ver [[keel-quatro-documentos]].

9 arquivos: `keel.k` e `keel/{arena,buffer,slice,outcome,corot,range,tagged,
parallel,routine}.k`. Cobre as §§5.2–5.7 da spec. Nunca foi executada — não há
parser; a verificação é manual contra `golden/c{11,23}/keel/*.h`.

**P15 fechada em 2026-09-20** (commits `bbc59c5` e `5fe7fea`). O golden deixou
de ser aproximação: é o que o cgen deve emitir a partir da `/base`. A regra que
governou tudo, e que o André enunciou, é **"a emissão segue o `.k` SEMPRE"** —
onde golden e `.k` divergiam, o `.k` mandou.

**Correção na própria base:** `arena.k` tinha duas macros (`alloc`, `from_stack`)
que eram resíduo do bootstrap `$T`. `alloc` virou `pub inline void *alloc(arena
*a, size_t n, size_t sz, size_t align)` — verbo comum; o programa escreve
`arena.alloc(a, T, n)` e o backend materializa `sizeof`/`alignof`/cast.
`from_stack` não declara função: cria armazenamento no escopo do **chamador**,
então é reescrita do backend.

**A instância sai inteira**, pelo backend §7.2 (o header é função *apenas do
próprio nome*). Recortar "ao que o caso usa" fazia o arquivo ser função de quem
o usa e quebrava o no-op da segunda escrita sob `make -j` (§7.1). Isso gerou o
fecho transitivo: `at` devolve `outcome T` e `clone` devolve `outcome <cont> T`,
então 7 instâncias novas apareceram só para fechar o grafo.

**Instanciação degenerada** (spec §4.3, escrita nesta rodada): com `void` somem
os verbos que mencionam o parâmetro em posição de valor; com `const` somem os
que escreveriam através dele; e **a omissão é transitiva** — `slice const char`
perdeu `set`, `clone` e, por consequência, `at`, porque `at` chama o `win1` que
`outcome const char` já tinha perdido. Chamar um verbo ausente é o
`verb-not-in-instance`.

Ver [[cgen-implementacao]] e [[golden-perfis-autorais]].
