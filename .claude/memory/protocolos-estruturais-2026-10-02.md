---
name: protocolos-estruturais-2026-10-02
description: "Decisões de 2026-10-02 sobre protocolos, papéis e base: conformidade estrutural sem implement, papéis herdados, §4.14 na v0, slice.of sobre Sliceable, from_stack removido"
metadata:
  node_type: memory
  type: project
  modified: 2026-10-02
---

Revisão de 2026-10-02, sobre a revisão de 01/10 (papéis, protocolos e moldes).
Tudo está na `master` (commits até `e565a5e`).

**Conformidade estrutural, sem `implement`.** Os protocolos são nomeados em
`keel.protocols` (`IndexPtr`, `IndexGet`, `Countable`, `Traversable`,
`Partitionable`, `Sliceable`, `Taggable`, `Failable`, `Winnable`) e servem às
construções e como tipo de parâmetro (§4.14, como uma interface Java). Um tipo
atende quando o seu módulo declara os verbos; não há declaração de
conformidade. A exigência nominal foi incorporada e revertida no mesmo dia:
não verificava nada que o uso não verificasse e puxava para "um tipo por
módulo, um protocolo por módulo". O André: "o ganho é bem pouco", e
`implement` "basicamente vira um comentário". O histórico está em
`design/proposta-implement-no-modulo.md`.

**Papéis.** Os do protótipo valem para o implementador que não os escreve
(§5.1, regra 10); escrever papéis diferentes é `protocol-role-mismatch`.
As construções valem como as chamadas que traduzem (§4.12, regras 14 e 15):
`x[a..b]`, `foreach` por ponteiro, `walk` e `parallel` fazem do binder, do
cursor, da parte ou do símbolo que recebe `x[a..b]` um `child` do contêiner.
O André aceitou o custo: "símbolo é chamada". As palavras de papel ficam fora
de `define-over-keel-name` e de `keel-name-shadowed`, porque não chegam ao C.

**Diagnósticos unificados.** `protocol-not-satisfied` substitui `not-iterable`,
`not-cursor-iterable`, `not-partitionable`, `not-countable`,
`no-range-index-verb`, `match-without-tag`, `else-on-infallible-type` e
`else-default-without-win`. Também saíram `ambiguous-match-tags`: o conjunto do
`match` é o tipo do operando ou o `tagset` que `tag` devolve.

**§4.14 na v0.** `slice.of` é `C.view of(Sliceable C x)` em `keel.slice`, nas
três aridades; o backend §5.19 nomeia a instância `keel_slice_of<aridade>_<tipo>`,
num par de headers próprio, e um `array` entra com binder (nome canônico
`keel_array_<T>`). `corot.tag` devolve `Status`; `tagged.tag` devolve `E`.
`keel.array` ganhou `length` (o binder); `keel.length` continua constante do
núcleo — o André: o que precisa da informação do parser fica no núcleo.

**`from_stack` saiu.** O armazenamento no frame é um `array u8` declarado e
passado a `from_array`, que recebe o vetor por binder; a dimensão entregue é a
declarada (§5.18), não `sizeof`.

**Estado do cgen (M2, só parse).** Alinhado: catálogo regenerado, `slice.of`
emitido como a instância da §5.19 (ainda por caso particular, pelo nome; vira
regra geral quando a §4.14 for implementada), `from_stack` fora. Ainda não
implementados: emissão de C, análise de papéis, registro de protocolos e
verificação de conformidade fora de `Sliceable`.

Ver [[base-real-keel-em-progresso]], [[cgen-implementacao]],
[[discutir-antes-de-editar-spec]].
