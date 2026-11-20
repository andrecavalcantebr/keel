# keel_libs — especificação das bibliotecas

Documento **normativo para as bibliotecas** de `keel_libs`, e **não normativo
para a keel**: ele diz o que as bibliotecas mantidas por nós prometem, e não o
que a linguagem exige. A linguagem é a dos quatro documentos normativos
(`keel-spec.md`, `keel-rationale.md`, `keel-c-backend.md`, `cgen-tool-spec.md`),
e este documento não os altera. Rascunho de 2026-11-20. Sai da §1 de
[`possibilidades.md`](possibilidades.md), que passa a apontar para cá, e junta o
que as duas bibliotecas de `../keel_libs` já provaram: `coll` (coleções) e
`maths` (matrizes, e depois tensores).

Os módulos de uma biblioteca são módulos comuns (`module`/`modifier` com `dim`,
`type` e `tags`, spec §4.3), e o que falta à linguagem para eles volta ao
repositório da keel como relatório de bug
([`achados/README.md`](../achados/README.md)), não como texto neste documento.

## 1. Estatuto

1. **A biblioteca padrão da keel é a base**, e mais nada: `base/` deste
   repositório (`keel.k`, `keel/*.k`), normativa pela spec §5.2–§5.8 e pelo
   backend §5, porque as construções do núcleo dependem dela. Todo o resto é
   biblioteca, e este documento trata dele.

   | Camada | Onde mora | Quem manda |
   | --- | --- | --- |
   | **Base** (a biblioteca padrão) | `base/` deste repositório | a spec e o backend |
   | **Bibliotecas mantidas** | pastas de `keel_libs` (`coll`, `maths`, ...) | este documento e o `SPEC.md` de cada pasta |
   | **Bibliotecas de terceiros** | repositório de cada autor | o autor; o índice e a conformidade estão na §2 |
   | **Candidatos** | só neste documento (§7) | nada: viram módulo quando um caso real o pedir |

2. **As bibliotecas não entram no cgen nem na base.** Os módulos da base estão
   aqui porque as construções da keel os exigem, e por nenhum outro motivo
   (ROADMAP §3). Uma biblioteca vive fora, é distribuída **como código-fonte** e
   consumida com `-I`: keel é uma linguagem de fonte, e um módulo genérico ou uma
   instância só existem a partir do `.k`. Uma instalação da keel pode não trazer
   nenhuma.
3. **Servem também de carga de prova.** Cada biblioteca é uma usuária grande da
   linguagem, e por isso depura o cgen. Um achado vale se reproduzir como bug do
   cgen ou lacuna da spec, com repro mínimo.
4. **O que "mantida" promete.** Nomes e verbos estáveis dentro de uma versão
   menor, o contrato comum da §3 cumprido por todos os módulos, e testes de
   execução, de contrato e de benchmark (§6). Não promete completude: um módulo
   só existe depois do caso real.

## 2. Nomes e distribuição

As bibliotecas se chamam `coll` e `maths`, sem prefixo `keel`: os módulos `keel`
e `keel.<nome>` são a base (ferramenta §8), donos dos nomes `keel_`/`KEEL_`
(spec §4.2, `reserved-name`), e as bibliotecas não os ocupam.

| Família | Projeto | Módulos hoje |
| --- | --- | --- |
| `coll` | `keel_libs/coll` | `coll.stack`, `coll.queue`, `coll.set`, `coll.pairs`, `coll.pair`, `coll.map`, `coll.list`, `coll.bstree`, `coll.btree`; vistas `coll.segments`, `coll.chain`, `coll.treeview`, `coll.readspan` |
| `maths` | `keel_libs/maths` | `maths.matrix`, `maths.mview`, `maths.mpart`, `maths.storage`, `maths.layout`; de apoio: `scalar`, `floating`, `lu`, `direct`, `blocked`, `typed_matmul`, `matmul_policy`, `row_output` |

- Imports qualificados. Cada módulo de coleção exporta um tipo `cursor`, então
  `types` em vários deles colide.
- Os módulos de apoio de `maths` são `pub` só porque a keel não tem
  visibilidade entre módulos além de `pub`/`priv`; **não são API**.
- Todos os verbos genéricos são `inline`: nenhum módulo da biblioteca pede
  `instance`.

**Distribuição.** Independente não quer dizer que a equipe não a construa e a
disponibilize: as bibliotecas mantidas são versionadas à parte do cgen e
publicadas num lugar conhecido, **como fonte**. O onde e o como estão na decisão D2.

**Terceiros.** Uma biblioteca de terceiros (um aluno, um grupo, outra
instituição) mora no repositório do autor, no ritmo e com a autoria dele. Não há
gerenciador de pacotes, então a descoberta é por um **índice** (`LIBS.md`, a
criar): nome, repositório, versão do cgen testada e o resultado da conferência
da §6. Entra em `keel_libs`, por pull request, a biblioteca que cumpre os
critérios da §6 e que alguém se dispõe a manter. As duas formas usam o mesmo
molde: uma pasta com `README`, `Makefile`, `src/` e testes próprios.

**Biblioteca compilada.** Um `.a` leva só os módulos não genéricos, e um módulo
genérico ou uma instância não vêm de binário: o consumidor keel precisa dos
`.k`. O binário serve a dois casos, e o manual (§7.6) os descreve: uma biblioteca
C usada de dentro de um programa keel (`import_c` mais `-l`), e o que se exporta
de keel para quem usa C (o `.h` e o `.a` gerados, com `instance` explícito).

## 3. Contrato comum

Vale para todo módulo da biblioteca. É o que os dois projetos já cumprem.

**Princípios.**

1. **Dados vetorizáveis primeiro.** Contíguos e por colunas; estruturas de
   ponteiros só com benchmark que as justifique.
2. **Blocos do núcleo.** Sobre `extent`, `keel.buffer`/`keel.slice`,
   `keel.arena` e `keel.outcome`; não se duplica o que a base oferece.
3. **Memória e visão.** Todo dono de memória é `byref`. Toda visão é por valor,
   sem armazenamento próprio, e vem do dono (`dono → visão`, nunca o inverso).
4. **Capacidade fixa em arena.** Sem crescimento e sem `free` individual.
5. **Custo declarado e medido.** Nenhuma alegação de desempenho, vetorização ou
   cache sem benchmark (§6).

**Dono.**

- `init(c, arena, capacidade, ...)` devolve `bool`; em falha deixa a coleção
  vazia. Capacidade zero é válida.
- Todo mutador estrutural leva `invalidates`; depois de qualquer mutador, os
  empréstimos anteriores se descartam, **mesmo se a chamada falhar**.
- Remoção que pode falhar devolve `outcome T` (`NONE` se vazio); a extração é
  explícita (`outcome.value`).
- Elementos copiáveis por atribuição C, sem destrutor nem posse.
- Verificações em execução por `KEEL_CHECK`; o runner rejeita `NDEBUG`.

**Visão.**

- Copiável, sem posse, não cresce; um mutador estrutural do dono a invalida.
- `as_slice(a, b)` é `[a, b)` com `a <= b <= length` e devolve o **mesmo tipo**.
- `partition(k, w)`: quociente e resto; partes na ordem lógica, sem sobreposição;
  `k = 0` ou `w >= k` dá visão vazia.
- `begin`/`has_next`/`next` para `walk`; `get`/`ptr` quando indexável.
- **Mutabilidade.** Visão mutável quando o valor do elemento é livre (`stack`,
  `queue`, `list`); `const` quando há invariante de valor (`set`: unicidade;
  `bstree`, chaves de `pairs` e `map`: ordem e busca). `const` é superficial.

**Papéis e protocolos.** Os verbos declaram `parent`/`child`/`invalidates`, e os
protocolos nomeados do prelúdio (`Indexable`, `Sliceable`, `Traversable`,
`Partitionable`, `Countable`, ...) que cada módulo cumpre; o cgen confere no uso.
Um módulo da biblioteca cumpre os protocolos de que as construções precisam
(`foreach`, `walk`, `parallel`, `x[a..b]`) e nenhum a mais.

**Igualdade e ordem.** Hoje o comparador é ponteiro de função, que não
vetoriza. Para `T` escalar, a igualdade inline é o caminho; o comparador como
eixo de instanciação (struct, string) é decisão aberta (D5).

**Duas interfaces por operação (a decidir, D8).** `bstree` oferece `has_seq`
(previsível, pequeno) e `has_cnt` (imprevisível, grande, em lote), com `has`
escolhendo. Se for regra, as demais coleções a seguem.

## 4. `coll`: coleções

Estado: **implementado** na `coll` 0.2.0, com 79 casos de execução, golden
e contrato de segurança, e benchmarks em `bench/`. A especificação detalhada,
por coleção, está em `keel_libs/coll/SPEC.md` e `docs/`.

### Donas da memória (todas `byref`)

| Coleção | Contrato | Visão |
| --- | --- | --- |
| `stack T` | LIFO: `push`, `pop`, `top` | `slice T` |
| `queue T` | FIFO circular O(1): `put`, `take` (`outcome T`), `peek`, `get`. Absorveu o `ring` | `segments T`: até dois trechos contíguos |
| `set T` | conjunto em vetor denso, ordem de inserção, comparador dado: `add`, `remove`, `has`; álgebra `unite`, `intersect`, `diff`, `subset`, `superset`, `equal` com destino do chamador | `slice const T` (hoje `readspan`) |
| `pairs K V` | duas colunas sob um `len`/`cap`; busca linear nas chaves, ordem de inserção | `slice const K` e `slice V` |
| `pair K V` | valor avulso | — |
| `map K V` | tabela hash; pede igualdade e hash na instanciação | a definir |
| `list T` | lista por índices; sem investimento | `chain T` |
| `bstree(N) T` | conjunto ordenado: B+ com nós de `N` chaves (4 a 64), pool fixo em arena, chave reservada. 3,6× a 5,7× a AVL; 1,3× a 2,6× a busca binária em vetor | cursor ordenado |
| `btree T` | AVL indexada, **só referência de benchmark** | `treeview T` (`const`) |

`ring` virou `queue` (mesmo contrato); `coll.span` virou `keel.slice T`.
`readspan` fica enquanto `slice const T` falhar com `T` struct (D6).

### Regimes, para a documentação recomendar com base

- `pairs` vence o hash com **poucas** entradas, chave pequena e muitas buscas:
  sem função de hash, sem ponteiro até o bucket, sem colisão, e a coluna de
  chaves cabe na cache. `pair u8 f64` ocupa 16 bytes; uma entrada de `pairs u8
  f64`, 9. O ponto de cruzamento com `map` é empírico, e fica fixado por
  benchmark antes de qualquer recomendação.
- `bstree` é o conjunto ordenado; `set` é o conjunto pequeno em ordem de
  inserção; `map` é o dicionário grande.

## 5. `maths`: matrizes e tensores

### 5.1 Estado: `matrix` e `mview` (implementados)

Descritores por valor sobre elementos contíguos na arena do chamador ou num
array externo; **nenhum incorpora os elementos**.

| Tipo | Papel | Tamanho (x86-64) |
| --- | --- | --- |
| `matrix(N,M) T` | descritor mutável | 16 bytes |
| `mview(N,M) T` | descritor só de leitura, com dois strides, em elementos | 32 bytes |
| `mpart T` | partição mutável por linhas, emprestada, usável em `parallel` | — |

- `N`, `M` e `T` pertencem ao tipo; os descritores não guardam linhas nem
  colunas.
- `T`: `i8..i64`, `u8..u64`, `f32`, `f64` e aliases compatíveis, conferidos por
  `_Generic` e `static_assert`. `bool`, `struct` e ponteiros são recusados. Não
  há `f16`.
- **Construção e posse.** `zeros`, `ones`, `identity`, `from`/`copy`,
  `transpose_copy` (alocam na arena indicada); `borrow` (sem cópia, sobre array
  externo mutável); `share` (alias com papel); `storage.from`/`readonly`
  (adaptam array plano).
- **Visões lógicas, O(1), sem alocar.** `mv.from`, `transpose` (troca os
  strides), `slice_rows`, `slice_cols`. Strides positivos; `is_contiguous` é
  contiguidade lógica row-major.
- **Aritmética.** `add`, `sub`, `mul` (por elemento), `add_c`, `mul_c`,
  `matmul`, `inv`, `det`. Inteiros usam aritmética verificada, e o overflow
  intermediário é erro. Flutuantes: `-ffp-contract=off`, sem `fast-math`.
- **`matmul`** escolhe i-j-k direto até 24×24×24 (ou, ordenadas as dimensões,
  16×32×64), e blocos com quatro partições `parallel` acima disso. Limiar prático
  e revisável; `matmul_direct`, `matmul_blocked`, `matmul_parallel` e as
  variantes `bt` escolhem à mão.
- **Falha.** `outcome T` em `get`/`det`; `failed`/`code` no descritor.

Limites atuais: a ponte `layout.span` (metadados por valor) existe por limites do
emissor e é transitória; use variáveis nomeadas para ela. Sem broadcast, strides
negativos, `v[i,j]`, conversão entre tipos, vistas vazias ou destino externo
reutilizável nas operações aritméticas.

### 5.2 Planejado: `tensor` (sem API)

`tensor(N) T`, com `N` o **posto** (`dim` como rank), é o passo seguinte da
`maths` (ROADMAP §2). É um problema estruturalmente igual ao de `matrix` e
semanticamente maior: pede eixo, stride por eixo, `reshape`, transposição e
permutação de eixos, e uma indexação que não dependa do posto. Pontos já
decididos pelo que `mview` provou:

- descritor por valor, sem os elementos; dono `byref`, visão por valor;
- visões lógicas O(1) (strides por eixo); cópia física é explícita;
- `matrix`/`mview` são os casos de posto 2, e devem ser conversíveis, sem cópia,
  de e para `tensor(2)`.

O que o tensor pede de `grid` (§7): `index(N)` e `box(N)` para
coordenadas e extensões independentes do posto. O açúcar `v[I]` seria trabalho
de núcleo e fica fora (spec §4.5). O gatilho para `source`/`pub import`
(ROADMAP §4) é justamente a lista de imports de uma biblioteca grande como esta.

## 6. Critérios de entrada e de qualidade

1. **Caso real antes de módulo novo.** A mesma regra do ROADMAP.
2. **Testes.** Execução (inclusive UBSan), golden do que o cgen emite e
   **contratos de segurança**: para cada verbo com `invalidates`, um probe que
   usa a visão ou o ponteiro depois do mutador e deve ser recusado, e o par
   positivo. Probes que documentam lacuna do cgen compilam e nunca executam.
3. **Benchmark** antes de alegar vantagem: no mínimo 8, 64, 512 e 4096
   elementos; `T` escalar e `struct`; inserção, remoção, busca, percurso, visão e
   partição; referência escrita à mão em C; relatório com compilador e flags,
   versão do cgen, `--checks`, perfil C, mediana, aquecimento.
   "Vetorizável" só com `-fopt-info-vec` ou assembly.
4. **Os perfis c11 e c23** compilam e passam; a biblioteca não pede `auto`,
   `typeof` nem `_BitInt`.
5. **O que a biblioteca achar volta como bug do cgen** ou lacuna da spec, com
   repro mínimo (`achados/README.md`); nunca como pedido de funcionalidade.

## 7. Candidatos (sem módulo)

Cada um cabe em `module`/`modifier` com `dim`/`type`/`tags`, sem trabalho de
núcleo, e só vira módulo quando uma biblioteca o pedir.

| Candidato | O que é | Estado e pontos abertos |
| --- | --- | --- |
| `buffer(N)` / `slice(N)` | *soa* homogêneo: `N` colunas paralelas do mesmo `T` sob um `len`/`cap`; verbos de `buffer` com um índice a mais para a coluna | A `coll` resolve o caso estreito com `pairs`. Abertos: `T *col[N]` (colunas separadas) × `T dados[N]` (bloco contíguo), porque o cursor, `ptr` e `partition` supõem memória contígua; uso como lote de tamanho fixo para gather/scatter sobre `extent`; regra memória/visão (`buffer(N)` produz `slice(N)`); alias seguido de `(`, a revisar com o nome `alias-with-argument` |
| `string` / `strview` | `typedef buffer char string` e `typedef slice const char strview` | O trabalho real é o conjunto de funções (comparação, concatenação, formatação). `slice const char` só não está na base porque este par seria o nome dele; o golden 021 já o usa cru |
| `grid` | `box(N)` e `index(N)`: a extensão de `range` a `N` dimensões | Em parte em `maths`. Contável e particionável como `range`. Abertos: `box(N)` com `first`/`limit` ou como `range r[N]`; sem literal `[a,b]` (colide com designador e atributo C23): `(index(2)){a, b}` ou `index.of(a, b)` |
| `slice.from(T, p, range)` | `slice` sobre ponteiro cru descrito por `range` | Útil com coluna de `extent` |
| `atomic`, `chan`, `barrier` | o conjunto cooperativo: `_Atomic T` com `init`/`load`/`store`/`swap`; `send`/`recv` como `corot` (cooperativo e SPSC); rendezvous sem busy-wait | Diagnóstico `atomics-unavailable` sob `__STDC_NO_ATOMICS__`, no molde de `specific-format-unavailable`. Abertos: `chan` ou `channel`; MPMC fica fora; um módulo para o contexto que sobrevive à suspensão |

| `file` | arquivo como `fd` mais `buffer`, não `FILE *`: abrir, ler e escrever em bloco, `size`, `seek`, `close`; leitura por linha ou bloco como `walk`/`foreach`; partição de arquivo em `parallel` | **Wrapper fino** de stdio/POSIX (D12): a convenção da biblioteca, não a reimplementação. Buffers na arena do chamador, capacidade fixa, falha em `outcome` com o `errno` preservado. Depende de `string`/`strview` |
| `path` / `dir` | caminhos e listagem de diretório | Só quando um caso pedir |
| `net` | TCP e UDP em nível de socket, mais `poll` | Wrapper fino, como `file`; bloqueante primeiro, cooperativo (`corot`, soquete não bloqueante, `chan`) depois. Casa com `atomic`/`chan` |
| `time` | relógio monotônico e de parede, espera | `poll` e timeout pedem relógio; entra junto de `net` |

`bitbuffer`, `bitslice` e `bits` **saíram da lista**: estão na
base desde a 0.4.0 (spec §5.8).

## 8. Decisões abertas

| # | Decisão | Inclinação |
| --- | --- | --- |
| D1 | ~~Prefixo das bibliotecas~~ | **Decidido (2026-11-20):** sem `keel.`; as famílias são `coll` e `maths` |
| D2 | Onde publicar `keel_libs` (um repositório, pasta por biblioteca) e como instalar | **Fonte, um repositório, pasta por biblioteca** (decidido em 2026-11-20); o nome do repositório, `keel_libs`. Falta: se a dist do cgen traz uma cópia (`lib/`) ou o usuário clona e passa `-I` |
| D3 | `list`: vetor denso com deslocamento, ou como está | Decidir com benchmark; sem investimento até lá |
| D4 | `put_over` em `queue`; `void` como valor em `pairs` (`pairs K void` como conjunto); chave duplicada em `pairs` (recusa, sobrescreve ou aceita) | Só com caso real |
| D5 | Igualdade inline para `T` escalar; comparador como eixo de instanciação para struct e string | Eixo de instanciação (ROADMAP §4, B3/B6) |
| D6 | `slice const T` em módulo genérico com `T` struct (achado A1): `readspan` até o cgen corrigir | Bug do cgen |
| D7 | Vista do `map`; nome `segments` (plural) contra a convenção singular do núcleo | A decidir |
| D8 | Duas interfaces por operação (previsível/pequeno × imprevisível/grande) como regra de API | Regra, se o benchmark confirmar nas demais |
| D9 | `buffer(N)`/`slice(N)`: núcleo, módulo da biblioteca ou depois | Depende do primeiro caso real (provável: o tensor) |
| D10 | Ordem das chaves em `pairs` (busca binária e deslocamento das duas colunas) | Terceiro modificador, se pedido |
| D11 | ~~A stdlib "oficial" contra o ROADMAP §3~~ | **Resolvida (2026-11-20):** a biblioteca padrão é a base; o resto é biblioteca. O ROADMAP §3 e a spec §5.9 já dizem isso |
| D12 | `file`, `net` e `time`: wrapper fino de C, ou implementação própria | **Decidido (2026-11-20):** wrapper fino, com a convenção da biblioteca (arena, `outcome`, `slice`/`buffer`, `walk`) em vez de buffer e ponteiro crus |
| D13 | `string`, `strview`, `atomic`, `chan`, `barrier`: base (`keel.*`) ou biblioteca | A decidir: `string`/`strview` são pré-requisito de `file`, e `atomic`/`chan` o são do cooperativo |

## 9. Pendências de edição

Fora deste arquivo, que não altera normativo da keel (regra do `AGENTS.md`):

- Criar o `LIBS.md` (índice das bibliotecas de terceiros) e o `CONTRIBUTING` de
  `keel_libs`, com os critérios da §6.
- `cgen-tool-spec.md` §4.3: dizer que as entradas de ligação do usuário (`.a`,
  `.so`, `-l`, `-L`, `-Wl,`) seguem as unidades geradas na linha do compilador
  C. Texto proposto, que pede o "sim" do André; o cgen já se comporta assim
  desde a 0.4.0-dev.
- `possibilidades.md`: a §1 deixou de existir; o que dela entrou aqui está nas
  §4, §5 e §7.
