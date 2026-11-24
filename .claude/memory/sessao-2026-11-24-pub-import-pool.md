---
name: sessao-2026-11-24-pub-import-pool
description: "2026-11-24: texto normativo de pub import e keel.pool fechado; medição da arena do cgen por categoria; E03 pausado; ordem de trabalho para a implementação"
metadata:
  node_type: memory
  type: project
  originSessionId: 1451d4d2-0852-4fbf-9ab0-bb54be16a69e
  modified: 2026-11-24T22:34:26.535Z
---

Branch `0.4.0`. Continuação de [[sessao-2026-11-23-estado]] e
[[decisoes-v1-mais-2026-11-23]].

## Texto normativo fechado nesta sessão (commitado, ainda sem push)

- **`pub import`** (spec §4.1 regra 7a; rationale; cgen-tool-spec §4.6):
  publica um alias transparente para o módulo, cadeia de alias livre em
  profundidade mas cortada onde um elo não publicou; `indirect-import` não
  dispara por esse alcance; sem `priv import` (redundante com `import` puro,
  padrão oposto ao da regra 2 — isso foi corrigido no texto depois de o André
  apontar a leitura cruzada ambígua); busca de todo import relativa ao
  diretório de quem importa, antes das raízes `-I`; pacote = pasta com um ou
  mais módulos de fachada, sem conceito novo na ferramenta. Exemplos com
  `keel_libs/matrix` (`matrix.matrix` como fachada de `matrix.linalg`,
  `matrix.product`).
- **`keel.pool`** (spec §5.9, desde 0.4.0): índice geracional sobre `pool`,
  não o handle com ponteiro sobre arena (descartado,
  `design/possibilidades.md` marcado). `pool T byref`, capacidade fixa, sem
  `realloc`; `entity {u32 index, u32 gen}`, 8 bytes. `alloc`/`free`/`reset`
  verbos distintos; `free(parent pool *p, consumes entity e)` — `consumes`,
  não `invalidates` (o próprio valor fica inválido, é free, não reset);
  `alloc` devolve `outcome entity`; `ptr` verifica a geração em debug
  (`entity-generation-mismatch`, como `buffer.ptr`), `at` verifica em toda
  build (como `buffer.at` — acrescentado por mim, não estava no desenho
  original, para ter o par debug/sempre que `buffer` já tem); geração `u32`,
  satura em `UINT32_MAX` em release, `pool-generation-overflow` em debug
  (decisão do André: documentar e travar em debug, não alargar a geração).
  Import explícito, como `arena`/`buffer` (regra 15 do §4.1, que ganhou
  `array`, `bitbuffer`/`bitslice`/`bits` e `pool`/`entity`, faltantes desde
  que `bitbuffer` entrou — gap pré-existente, corrigido de passagem).
- **Rationale novo:** por que alias e não cópia de símbolos; por que a
  profundidade do alcance é livre; por que não há `priv import`; por que
  pacote não é conceito novo; **por que a base não publica um módulo seu
  por dentro de outro** — a motivação do `pub import` é o recorte
  interno/externo de uma lib de domínio (`matrix`/`tensor`); a base não tem
  esse recorte, e trazer tudo por padrão penalizaria quem só queria `arena`.
- **ROADMAP.md**: tirou as linhas de `pub import` e "Handle" da tabela de
  v2+ (gatilhos dispararam); ficou só protocolo nominal e comparador como
  eixo; nota de que a tabela toda vai ser reestruturada (muita coisa já
  obsoleta — implementada ou descartada), mas isso é tarefa futura.

Commits: `800e4b4` (pub import), `c75373c` (keel.pool), `a621fbe` (correção
da regra 2 + rationale da base).

## Medição da arena do cgen (implementado nesta sessão)

`tools/cgen/src/tool/memory.c` (novo) + `memory.h`: toda alocação já passa
por `cgen_alloc`, então uma tabela simples (`CgenStat`, até 64 rótulos,
`strcmp` linear) totaliza bytes e chamadas por `what`. `cgen_memory_error`
(estouro da arena) imprime a tabela ordenada por bytes automaticamente — é
exatamente "saber de qual limite estamos a falar" que o André pediu. Fora do
estouro, `CGEN_ARENA_STATS=1` no ambiente imprime a mesma tabela num run
normal. **Pegadinha corrigida:** a tabela não pode ser `static` no header —
cada `.c` que inclui `memory.h` ganharia sua própria cópia vazia; por isso
`memory.c` tem a definição única e `memory.h` só declara `extern`.

Ainda não medido com o cenário de estresse (≈400 módulos, ≈10-11 mil
funções) do `achados/E03-recusas-silenciosas-e-escala.md`. **Decisão do
André:** não chutar o novo tamanho da arena (dobrar é fácil mas arbitrário);
medir primeiro com a tabela nova, decidir o número depois.

## E03: pausado

"Precisa de estudos... vamos deixar quieto por hora." Tempo quadrático
(perfilar com `perf` antes de supor causa) e a cobertura de
`pub struct S {...} s0;`/protótipo sem corpo (lacuna da spec ou do emissor,
a decidir por leitura) ficam para depois da implementação abaixo.

## Implementação do `keel.pool` — feita (2026-11-24, commits a621fbe..7f96a20)

`base/keel/entity.k` e `base/keel/pool.k`, golden 088 (feliz caminho +
`entity-generation-mismatch` em debug), suíte inteira verde (176 ok golden,
0 fail em tudo, docs 45 ok). Escrevi eu mesmo (não delegado a Haiku): a
implementação estava presa demais ao desenho ainda sendo fechado (ex.: achei
e corrigi, no meio do caminho, que `pool.ptr` tinha `T *ref` no protótipo da
spec — inválido, `ref` só vale em declarador local com inicializador).

**Dois bugs reais do cgen achados e corrigidos**, nenhum exercitado antes:
- `consumes` vazava para o C gerado (`engine/emit/roles.c`: lista de papéis
  a apagar esquecia `consumes`).
- `priv inline` de módulo genérico que não menciona `T` nem o modificador
  não ia para lugar nenhum (nem `.h` de instância, nem `.h` nu do módulo) —
  `engine/emit/files.c`, `header_function`/`header_body` só admitiam
  inline-sem-pub dentro de uma instância. Corrigido para compartilhar o
  `.h` nu do módulo (como um `pub inline` não-paramétrico já fazia); testei
  à mão que duas instâncias do mesmo genérico na mesma unidade não duplicam
  a definição.
- De passagem: 4 scripts de teste unitário não linkavam `tool/memory.c`
  (novo na sessão anterior) — corrigido.

## Geração na arena-mãe: feita (2026-11-24, commit 371b3c0)

O André apontou o ponto certo: comptime-only não bastava — faltava saber em
*runtime* se a arena de um `pool` moveu debaixo dele (não é o mesmo que
liberar um slot). Pediu custo controlado, O(1).

Desenho: `arena` ganha `generation` (sobe em `reset`/`restore`/cada
construtor, nunca em `alloc`/`mark`); verbo só-leitura `arena.generation(a)`
(não pode ler o campo direto — `instance-field-access`, já previsto no
estudo do handle). `pool` ganha `owner`/`stamp`; `check_region` confere os
dois a O(1) em `alloc`/`free`/`reset`/`ptr` (debug); `at` dobra a mesma
conferência na condição que já decide `NONE`, sem `KEEL_CHECK`, porque `at`
promete nenhum UB em release. Só alcança a arena **direta** — uma avó
(`from_parent`) resetada não é vista; encadear deixaria de ser O(1).

**Bug achado no caminho** (não só lacuna): `pool.from_arena` estava com
`parent pool *p, parent arena *a` — os dois `parent`, nenhum `child`, então
`p` nunca ficava ligado a `a` na análise estática. Corrigido para `child
pool *p, parent arena *a` (molde de `arena.from_array`); a propagação
transitiva da regra 4 do §4.12 resolve o resto sem mecanismo próprio (`reset(a)`
invalida `p` invalida qualquer `entity`). Caso novo em
`test/cgen/diag/provenance/v.k`.

golden 088 ganhou o `DEBUG` 2 (`pool-region-invalidated`); golden 026 teve
uma linha de `DEBUG` deslocada (`arena.k` cresceu) e precisou de ajuste.
Manual e `keel-llm-context` atualizados com o mecanismo novo.

## `pub import`: não implementado nesta sessão

Descoberta ao tentar: `pub`/`priv` antes de `import` **já faz parse hoje**
(zero trabalho de gramática), mas o resolvedor **não encadeia dois pontos**
— testei (`b.c.value()` produz `bb_c.value()`, lixo). A regra 8 do §4.1
(`indirect-import`, já testada pelo golden 069) só alcança um nível de
indireção, e não distingue import publicado de não publicado. Implementar a
cadeia de alias de profundidade livre que o texto da spec promete é cirurgia
no núcleo do resolvedor (`isl_module_alias_ex`), não um ajuste pequeno.
Decisão pendente com o André: reduzir o alcance da spec para um nível só
(perto do que a regra 8 quase já faz) ou implementar a cadeia completa com
mais tempo dedicado. Fica para a próxima sessão.

## Ordem de trabalho (pedida pelo André)

1. Golden primeiro, 2. implementação (Haiku quando possível — aqui não deu),
3. suíte inteira, 4. commit e push. Feito para `keel.pool`; falta para
`pub import`.

Depois de `pub import`, volta para o E03.

**Why:** o André quer o "fio" da sessão escrito à parte, para não se perder
entre a decisão de design e a implementação.
**How to apply:** ler este arquivo e os dois linkados antes de retomar;
checar `git log --oneline -10` e `git status -sb` primeiro.
