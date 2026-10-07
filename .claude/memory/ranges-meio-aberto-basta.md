---
name: ranges-meio-aberto-basta
description: "2026-10-07: `a..b` meio-aberto segue como padrão; `a..<b` / `a..=b` (marcador só no limite superior, sem mudar o lexer) é solução pronta, a adotar só se o inclusivo valer a pena"
metadata:
  node_type: memory
  type: project
  modified: 2026-10-07
---

André viu, em outra abordagem de keel, `a..<b` (exclusivo) e `a..=b`
(inclusivo). **Decisão de 2026-10-07: por ora `a..b` meio-aberto, `[a, b)`,
continua o padrão.** Ainda falta decidir se o intervalo inclusivo vale a pena;
se valer, a solução abaixo já está desenhada. Nada foi escrito nos normativos; o
registro está em `design/possibilidades.md` ("Intervalo inclusivo").

**Por que lá a notação faz sentido.** Lá a ferramenta atua depois do
preprocessador (PPC) e `0..N`, com `N` macro, **não expande**: o PPC não entende
`N` como macro, só `0..N` como um número. Esse é o problema. Se expandisse, só
seria preciso uma notação para distinguir o inclusivo do exclusivo.

**Por que em keel não é urgente.** keel atua antes do PPC: `N` chega como região
C opaca e sai como está (`keel_range_of(0, N)`; backend §5.x, regra 9).

**A solução pronta (marcador só no limite superior).**
`a..<b`, `a..=b`, `a..`, `..<b`, `..=b`, `..`. `a..b` sem marcador vira erro
(`range-bound-unmarked`): assumir limite aberto por omissão forçaria espaço em
`a.. N` com `N` macro; com o erro, os espaços são só estilo.

- **O lexer não muda.** `..`, `<` e `=` já são tokens distintos
  (`k_lexer_scan_number` para antes de `..`; `..` está na tabela de pontuação).
  O parser junta `..` com o marcador; depois de `..` só vêm `]`, `)`, `,`, `;`
  ou o marcador, então não há ambiguidade. `a..<=b` é rejeitado.
- **Emissão do inclusivo:** `assert(b != SIZE_MAX); limit = (b) + 1;`, com os
  parênteses (`b` é C opaco) e o assert só em debug. O `range` guarda `limit`
  exclusivo, então o intervalo inclusivo completo não é representável.
- **Custo:** `a..b` aparece em cerca de 45 arquivos (spec 30 vezes, golden,
  testes, `parser_islands.c`); os perfis c11/c23 são escritos à mão.
- **Alternativa sem sintaxe:** verbo `range.through(a, b)`.

**Critério:** retomar quando houver caso de uso concreto de inclusivo cuja
forma em C (`i <= b`, ou `a..b+1`) fique pior de ler ou manter.
