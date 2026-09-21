---
name: codigo-em-ingles
description: decisão de 2026-09-19 — docs seguem em pt-BR, mas todo código (IDs de diagnóstico, base, ferramentas, golden, exemplos nos docs, mensagens do cgen) passa a inglês; de-para dos diagnósticos em revisão pelo André
metadata:
  type: project
---

André decidiu (2026-09-19), como último passo para congelar a v0: prosa dos
documentos fica em pt-BR (tradução para inglês "eventualmente"); **código em
inglês** — nomes, variáveis, funções, comentários, inclusive dentro dos docs.
Confirmou os três pontos: mensagens do cgen em inglês; diretórios/arquivos do
golden também (`casos/`→`cases/`, `esperado/`→`expected/`, `prova.c`…).

Etapas combinadas: (1) IDs de diagnóstico — é interface estável, André revisa
o de-para ("é crítico"); (2) base, transform, cgen; (3) golden; (4) exemplos
nos documentos. Suíte rodando entre etapas.

Estado: de-para de 142 IDs (130 spec + 12 ferramenta) apresentado; André já
fixou `declarador-enterrado`→`hidden-declarator`,
`instance-inutil`→`redundant-instance`,
`soa-elemento-nao-existente`→`soa-element-without-field` e o estilo
`nonconstant-X` (não `X-not-constant`). Resto ainda em revisão — nada aplicado.
Iguais nos dois idiomas: pub-static, byref-param, arena-escape, alloc-overflow.

**Why:** IDs de diagnóstico são declarados estáveis e usados em mensagens e
opções — traduzir depois quebra usuários.
**How to apply:** não aplicar o de-para antes do "sim" final do André.
Ver [[discutir-antes-de-editar-spec]].

**Etapa 2 feita (commit `14c4d9a`, 2026-09-19):** base, tools, editors,
scripts em inglês; arquivos renomeados (editors/generate.py, verify.mjs,
verify-zed.py, install-*.sh, gen-ccjson.sh; `--copiar`→`--copy`). Pendentes
para a etapa 3: `golden/gerar-ccjson.sh`, `golden/casos` citado em
verify.mjs/verify-zed.py/generate? e no golden/README. Etapa 1 (diagnósticos)
aguarda revisão; etapas 3 e 4 não começadas.
Revisão parcial 2 (André): across-branches; família injected-* para types;
specific-format-unavailable; flat-view-of-n-dim-array; constexpr-as-lvalue;
nonscalar-constexpr; família range-index-* (x[a..b]) e
open-range-outside-index. alias-com-argumento: André quer TIRAR restrição e
verificação da v0 (nome alias-with-argument guardado; revisitar na v1 com
buffer(N)) — confirmado e aplicado (commit seguinte a 14c4d9a). Resto do de-para em revisão.
Revisão parcial 3: mask-above-64-slots (não amarra u64; sobrevive a bitslice);
tag-repetida e rotulo-de-tag-repetido FUNDIDOS em duplicate-tag (mesma regra,
lugares diferentes; ambos error, logo não suprimíveis — precedente byref-param).
A linha do catálogo passa a descrever os dois lugares.

**Etapa 1 FEITA (commit seguinte a 6d9ac6e):** 128 IDs em inglês aplicados em
spec/backend/rationale/cgen-tool-spec/design/golden (400 ocorrências).
Cuidado registrado: `sombreamento` é palavra de prosa — a substituição só vale
dentro de crase. Faltam etapas 3 (golden: infra, nomes de caso, identificadores)
e 4 (código nos docs, inclusive os não-terminais da gramática da spec §2.2, que
estão em português: cabecalho-sistema, param-array, decl-funcao, stmt-c, etc.).

**Etapa 3 FEITA (commit `1c249c7`):** golden inteiro em inglês — cases/,
expected/, proof.c, NOTES, VERIFY, XFAIL, gen-ccjson.sh, diagnostics.py;
run.sh com FAIL/MISSING/STRUCT; 5 diretórios e 6 módulos renomeados
(app.align, app.loop, coll, instances, results, grids); ~100 identificadores.
Truque usado: renomear só em REGIÃO DE CÓDIGO (comentário/string fora), e
depois traduzir comentários à mão — palavras como estado/linha são prosa.
Falta só a etapa 4 (código nos documentos + não-terminais da gramática §2.2).

**Etapa 4 FEITA (commit `1ae845d`) — passagem para inglês COMPLETA.** Gramática
da spec §2.2 (51 não-terminais), ~370 identificadores nos blocos keel/c,
comentários dentro dos blocos, strings de #error, mensagens de exemplo do cgen,
módulo `ola`→`hello`. Prosa dos documentos segue em pt-BR (tradução futura).
Ferramentas de apoio ficaram no scratchpad da sessão (levanta.py, ren.py,
linhas.py, diag-map.tsv) — se precisar de novo, refazer.
Commit extra `2922ad0`: a linha de cabeçalho dos gerados também virou inglês
("generated from X by cgen, C23 profile") em 244 arquivos. O que ainda tem
prosa em português, por ser prosa ou por estar pendente: NOTES/README do
golden, os comentários dos headers da base em golden/c{11,23} (P15) e a prosa
dos quatro normativos.
