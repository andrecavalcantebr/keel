---
name: biblioteca-padrao-planejamento
description: "Catálogo do estado da biblioteca padrão do keel — módulos já especificados/implementados vs. candidatos em discussão, e a leitura ECS/DOD do projeto"
metadata: 
  node_type: memory
  type: project
  originSessionId: 35c40a48-8f0d-4825-9f4c-93daf4674b56
  modified: 2026-09-17T18:57:14.439Z
---

## Já especificado (spec §5, inventário em §5.1)

| Módulo | O que declara | Contrato |
| --- | --- | --- |
| `keel.arena` | tipo `arena`, construtores, verbos | §5.2 |
| `keel.buffer`, `keel.slice`, `keel.range` | sequência, trecho, intervalo + protocolos de acesso/cursor/partição | §5.3 |
| `keel.tagged` | modificador etiqueta+valor | §5.4 |
| `keel.outcome`, `keel.corot` | resultado de dois estados / estado cooperativo de três | §5.5 |
| `keel.routine` | tabela de participantes, `seq` e `par` | §5.6 |
| `keel.parallel` | símbolo de controle e consultas | §5.7 |

## Implementado de verdade em `base/keel/*.k` (estado em 2026-09-17, commit `36611dc`)

- `arena.k`, `buffer.k`, `slice.k`, `outcome.k` — 4 arquivos com fonte keel real.
- Faltando, mesmo já especificado: `range` (nenhum dos quatro arquivos menciona), `keel.tagged`, `keel.corot` (a outra metade do módulo de resultado), `keel.routine`, `keel.parallel`, e o próprio `keel.k` (prelúdio que gera `keel.h` — pendente, é onde mora a tensão `of`/`as_slice` não resolvida, ver [[base-real-keel-em-progresso]]).

## Escrito na spec (2026-09-17, commits `577b7dd`, `3d53e88`, revisado no mesmo dia)

`soa` entrou como construção de núcleo em **keel-spec.md §4.11**, com seção correspondente em **keel-rationale.md** ("`soa`: da recusa à admissão"). **Forma final, depois de reconsideração no mesmo dia: sem nenhum verbo sintetizado** — só a inversão `array T campo → T *campo` e a reescrita `var[i].campo → var.campo[i]`/`param->campo[i]`. Nada de `keel.soa_from`/`length`/`capacity`/`push`/`pop`/`clear`/`slice_of`; a ponte pro resto do keel é o verbo já existente `slice.from(T,p,n)`. Detalhe completo e o porquê da reversão em [[ideias-pendentes-soa-e-cooperativo]] (seção "Decisão final sobre soa"). Caso golden `020-soa` reescrito para bater com essa forma.

Um catálogo leve dos demais candidatos (não implementados, sem contrato fechado) está em **keel-spec.md §5.8**: `buffer(N)`/`slice(N)` (soa homogêneo), `strbuf`/`string`, `bitbuffer(W)`/`bitslice(W)`, `atomic`/`chan`/`barrier`.

## Candidatos em discussão — o que ainda não está na spec; nada mais se escreve sem "sim" explícito (ver [[discutir-antes-de-editar-spec]])

Detalhe de cada um em [[ideias-pendentes-soa-e-cooperativo]]:

1. **`soa`** — linha viva: `soa struct NAME { array T campo; ... }`, declaração concreta reconhecida pelo núcleo (não módulo genérico), marcador `array` decide o que vira coluna. A ideia mais madura e mais promissora das cinco; sobreviveu a três tentativas fracassadas de generalização (introspecção de tipo C, aridade variável, template).
2. **`buffer(N)` / `slice(N)`** — nomes preferidos no lugar de `tensor(N)` / `view(N)` para variantes de capacidade/extensão fixa em tempo de compilação.
3. **`strbuf` / `string`** — typedefs de conveniência (`buffer char` / `slice const char`), não módulos novos.
4. **`bitbuffer` / `bitslice`** — array de bits compacto. Confirmado por busca no repo (2026-09-17): não há nenhuma menção em nenhum arquivo — é ideia nova, não retomada de lugar nenhum.
5. **Cooperativo:** `keel.atomic type T`, `keel.chan type T`, `keel.barrier` — só esboço inicial, pouco discutido ainda.

## Digressão registrada (2026-09-17): keel como linguagem DOD, e o nome "cdod"

Andre observou que o conjunto já reunido (soa, buffer/slice, foreach/walk/apply/parallel+partition, tags/match) é, na prática, uma linguagem DOD (data-oriented design) sobre C — e cogitou, como digressão filosófica, batizar o projeto de **cdod** (C + DOD) em vez de keel. Não é decisão, é musing explicitamente rotulado como tal — registrado para não se perder, sem nenhuma ação tomada (nenhum arquivo, license ou nome de repo foi tocado). Retomar só se Andre trouxer de novo com intenção de decidir; se isso acontecer, lembrar que é mudança de grande superfície (toca license, docs, nome do repo/remote, tooling) e vale tratar como decisão à parte, não de passagem.

## Digressão registrada (2026-09-17): leitura ECS do que já existe / falta

Mapeamento que Andre fez entre keel e um ECS (entity-component-system) clássico:

| Conceito ECS | Equivalente em keel |
| --- | --- |
| Entidade (id) | índice (`i` num `soa`/`buffer`/`slice`) |
| Componente (struct) | campo de `soa struct` (layout SoA) |
| Busca e travessia | `foreach`/`walk`/`apply`/`parallel` + `slice` (já existem) |
| Bitmask / presença de componente | `bitbuffer`/`bitslice` (candidato, item 4 acima) |
| Query / reduce / filter | sem módulo dedicado; Andre propôs simular com `apply` |

**Lacuna que Claude apontou (2026-09-17) — retratada no mesmo dia, com correção do Andre:** eu tinha apontado "índice geracional" (recicla o slot de uma entidade removida, detecta referência obsoleta ao slot reciclado) como peça faltando. Andre corrigiu: isso pressupõe um modelo de alocador com free-list e reuso de slot com identidade nova, que não é como arena/buffer funcionam aqui — **arena não gerencia elementos individualmente**; invalidação é lógica (encolher `len`, ou uma flag), e a liberação de memória é sempre no fim do ciclo de vida da arena inteira, não por slot. Não há "slot reciclado com identidade diferente" nesse modelo, então geração não tem o que resolver. Retirado da lista de lacunas — não é um gap real dado como o keel trata memória.

**Componentes esparsos:** também não é lacuna urgente — Andre não tem (ainda) caso de uso de componente opcional por entidade, então bitmask de presença não é necessidade hoje, só possibilidade futura (ver nota sobre `bitbuffer(W)` abaixo).

**Peça nova que surgiu na mesma conversa (2026-09-17): materializar uma entidade inteira, ou um lote delas, a partir do soa.** Com os verbos por campo já discutidos (`soa.get(s, campo, i)`), dá pra montar uma entidade completa manualmente: `T var = { soa.get(s1,campo1,i), soa.get(s1,campo2,i), ... };` — um gather explícito, escrito pelo programa, não automático (compatível com a negação de `p[i]` sozinho: quem quer a struct inteira, monta com a mão, não pede pro `soa` materializar). Isso funciona bem para **uma linha por vez**. Para um **lote de N linhas** (gather de um bloco contíguo em `T batch[N]`, útil para processamento tipo SIMD por blocos: reunir N linhas em AoS, processar, espalhar de volta), a peça que falta é o `slice(N) T`/`buffer(N) T` já candidato (item 2 de [[ideias-pendentes-soa-e-cooperativo]]) — um `slice(N)` daria o tamanho fixo em compilação que permite desenrolar o loop de gather sem variável de tamanho. Ainda não fechado — vale confirmar essa leitura com o Andre antes de aprofundar, porque a frase original ("não conseguiria fazer isso com um subconjunto") admite também a leitura de "subconjunto de campos" (projeção parcial de T), que é um problema diferente (e mais parecido com os já fechados: exigiria keel conhecer os campos de T para gerar um tipo NOVO, menor — mesma categoria de recusa que soa heterogêneo genérico already fechou).

**`bitbuffer`/`bitslice` deveriam nascer parametrizados por largura de bit, não como tipo booleano fixo:** `bitbuffer(W)`/`bitslice(W)` = array compacto de inteiros de W bits; `bitbuffer(1)`/`bitslice(1)` é só o caso particular que dá bitmask/bitset clássico — não precisa de um tipo à parte pra isso. Conexão observada pelo Andre: `keel.routine` já tem algo parecido na saída de `seq`/`par` hoje, mas representado como `buffer i32 states` (um int inteiro por slot, não compacto) — se `bitbuffer(1)` existisse, essa representação interna poderia trocar para algo mais compacto (o `mask` de 64 slots já documentado em §5.6 é derivado dessa tabela). Não é urgente, só uma consistência possível de aproveitar depois.

**Nomes genéricos vs. vocabulário de domínio:** Andre observou, de passagem, que talvez os nomes das construções (buffer, slice, tags) não "pareçam" DOD/ECS — resposta: isso é decisão de design, não lacuna. `import X as Y types;` já permite renomear qualquer módulo pro vocabulário que o programa quiser (`import keel.buffer as component types;`, por exemplo) sem nenhum mecanismo novo — o núcleo fica genérico de propósito, e o vestir de domínio é responsabilidade do módulo do programa, não da base.

**Why:** consolida num único lugar o estado real da stdlib (o que já está escrito/especificado vs. o que é ideia solta), para não perder o fio entre sessões — a lista de candidatos cresce rápido em conversa e não está em nenhum documento do repo.

**How to apply:** antes de sugerir "vamos adicionar X à stdlib", checar esta lista — pode já estar aqui como candidato com análise feita, ou pode ser genuinamente novo. Ao retomar qualquer candidato para decisão, seguir [[discutir-antes-de-editar-spec]].

**2026-09-19:** o catálogo passou a morar no repositório, em
`design/possibilidades.md` (não normativo), organizado por horizonte que André
fixou: **v0 = núcleo e base; v1 = stdlib; v2+ = protocolos**. A spec §5.8 virou
só um apontamento. Esse documento é agora a fonte; esta memória é histórico.
