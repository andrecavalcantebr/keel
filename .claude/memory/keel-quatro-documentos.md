---
name: keel-quatro-documentos
description: keel — projeto de linguagem transpilada para C; quatro specs em português com divisão de propriedade rígida
metadata: 
  node_type: memory
  type: project
  originSessionId: 5c733e00-beee-44b0-a18e-ab055c4313fc
  modified: 2026-09-13T20:58:50.518Z
---

keel é uma linguagem que transpila para C (parser de ilhas: ilhas de keel num mar
de C que atravessa opaco). O repositório em `/home/andre/code/cprojects/keel` tem
os quatro documentos normativos:

- `keel-spec.md` — o que depende só do fonte (aceita/recusa, sentido do gerado)
- `keel-rationale.md` — as razões; espelha a numeração da spec, "nada aqui é regra"
- `keel-c-backend.md` — o que depende do alvo (mangling, artefatos, `#line`, perfis)
- `cgen-tool-spec.md` — o que depende da invocação (caminhos, flags, depfiles)

A divisão é normativa e usada como critério de projeto: quando uma decisão trava,
a pergunta é se a regra está no documento certo. Mudança conceitual costuma tocar
três dos quatro ao mesmo tempo, e as referências cruzadas (`§x.y`, `backend §n`)
precisam ser varridas junto. Ver [[discutir-antes-de-editar-spec]].

Em 2026-09-13 a revisão v3 foi fechada como **a** spec (versões antigas apagadas;
commits direto em master, sem branch). Além dos documentos existem `golden/`
(suíte de casos esperados, 86 headers em c11/ e c23/) e `editors/` (realce gerado
da spec).

Desde 2026-09-14 há bootstrap em `src/`: `src/codegen/` é uma ferramenta pequena
que substitui `$T`/`$E`/`$N` num `.k` quase-header e cospe o `.h`, para escrever
a base (`src/base/keel/{arena,buffer,slice,outcome}.k`) já no estilo keel e usá-la
no parser de verdade. Não é o cgen da spec — é andaime.
