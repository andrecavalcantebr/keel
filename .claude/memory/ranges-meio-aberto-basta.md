---
name: ranges-meio-aberto-basta
description: "2026-10-07: por que `a..b` meio-aberto basta em keel e não se adota `a..<b` / `a..=b`; quando um inclusivo valeria"
metadata:
  node_type: memory
  type: project
  modified: 2026-10-07
---

André viu, em outra abordagem de keel, a notação `a..<b` (exclusivo) e
`a..=b` (inclusivo) e perguntou se valia adotá-la. Conclusão: **não, aqui
`a..b` meio-aberto, `[a, b)`, continua sendo o único sentido.** Nada foi
escrito nos normativos.

**Por que lá faz sentido.** Nessa outra abordagem a ferramenta opera depois do
preprocessador (PPC) e `0..N`, com `N` macro, **não expande**: o PPC não
entende `N` como macro, só `0..N` como um número. Esse é o problema, e é ele que
pede uma notação própria. Se expandisse, não se precisaria de outra notação, a
não ser para distinguir o inclusivo do exclusivo.

**Por que aqui não.** keel é parser de ilhas e atua sobre o texto antes do PPC:
`N` chega como região C opaca e sai como está (`keel_range_of(0, N)`; backend
§5.x, regra 9). O problema que motiva a notação não existe. E o meio-aberto é o
idioma de C (`i < n`), é o regime de `x[a..b]` → `as_slice(x, a, b)`, e as
formas abertas `x[a..]`, `x[..b]`, `x[..]` ficam limpas; `x[..<b]` e `x[a..<]`
ficariam piores.

**Se um dia o inclusivo for necessário.** `..=` custaria um `+1` escondido; com
`N` opaco ele pode estourar em `SIZE_MAX`, e `range` guarda `limit` exclusivo,
então o intervalo inclusivo completo não é representável. O caminho mais barato
seria um verbo (`range.through(a, b)`), não sintaxe nova, e deixaria o lexer
(M1 fechado) intacto. Ideia registrada, não pendência.
