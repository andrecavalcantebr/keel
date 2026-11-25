# E03 — recusas silenciosas, mensagens enganosas e escala (medição de 2026-11-23)

**Situação.** Fechados: o `extent` silencioso (`extent-limit-exceeded`, 0bd6f3c), as mensagens enganosas de `match` (SCRATCH), `protocol` (17 itens), `array` (9 declaradores), `defer [now]` (9) e `parallel` (17 capturas), com `test/cgen/cli/limit_messages.sh`, e a emissão de agregados nomeados com declaradores (golden 090). Abertos: o tempo quadrático, a arena e a cobertura do protótipo público sem corpo.

Estudo aberto a partir de `tools/agents/reports/limites-medicao.md` (Haiku; os
achados 1 e 14 reproduzidos por mim, cgen 0.4.0-dev, build de desenvolvimento
`-O0`).

## Bug: `extent` com 5 ou mais grupos sai sem diagnóstico

    pub extent struct s [d0, 2][d1, 3][d2, 4][d3, 5][d4, 6] { array i32 *v; size_t d0, d1, d2, d3, d4; };

`cgen` sai com 0 e o `.type.h` traz a linha keel intacta (`pub extent struct s
[d0, 2]…`), que o compilador C recusa depois. Causa: `k_em_extent_at`
(`engine/emit/extent.c:96`) devolve falso quando `groups >= MAX_GROUPS` (4), e
falso quer dizer "não é um extent que eu traduza". A mesma saída vale para mais
de 8 colunas e para um `k` depois do 32º campo (este com mensagem errada:
`extent-count-not-field`). É a classe da §6.4: o limite tem de sair como
`implementation-limit`, nunca como saída errada. Conserto: um limite do emissor
(`k_em_limit`) quando a forma é um `extent` que excede a capacidade. As
funções que decidem hoje não recebem o `Emit`, então o conserto pede passar a
capacidade para quem emite. Caso a escrever antes: `extent` de 5 grupos, com
`implementation-limit` esperado.

## Mensagens enganosas (do mesmo molde do E02)

- `match` com 15 rótulos curtos (ou 10 de 27 caracteres): "shorten the line", mas
  o que cresce é o número de rótulos, não a linha. O limite real é
  `SCRATCH_CAP` (2048) sobre o texto das edições da linha do `match`.
- `protocol` com 17 itens, verbo com 9 parâmetros e `array` de topo com 9
  declaradores: `unexpected-token` ("malformed or truncated declaration").
- `defer [now …]` (9), `parallel` (17 capturas), `extent` (9 colunas), `x[i, …]`
  com 9 índices: "the emitter does not cover this construction yet", sem o
  limite.
- Globais: 128 globais apontam `g0`, não a 128ª.
- `routine` com 18 ou mais campos: o preenchimento `= {0}` some por inteiro, sem
  diagnóstico. Efeito nulo em C, mas é divergência do desenho.

## Escala

- **Tempo quadrático no número de linhas.** 1 000 funções: 0,53 s; 4 000: 6,9 s;
  10 000: 42 s; 100 000 linhas de comentário não terminaram em 15 min. Programas
  reais (`coll`: 12,8 MiB, segundos) não sentem, mas um arquivo gerado por
  máquina sente.
- **A arena (64 MiB) acaba antes dos tetos de arquivos.** Com cerca de 400
  módulos importados, e com 10 000 a 11 000 funções num módulo. `WRITTEN_CAP`
  (1024) e `CGEN_SOURCES_CAP` (1024) nunca disparam. 100 000 linhas de fonte
  esgotam a arena em 0,1 s.
- Para decidir: se `maths` (e o `tensor`) passam de uns 100 módulos, a medição de
  pico se repete e a arena sobe a 128 MiB.

## Cobertura do emissor

`pub struct S { int x; } s0;` e `pub i32 h(void);` já são admitidos pela
gramática da spec §2.2. Na declaração de agregado, o tipo e os objetos
compartilham o trecho de fonte. O golden 090 verifica a separação entre
a tag no `.type.h`, os `extern` no `.h` e as definições no `.c`, tanto
para `struct` quanto para `union` nomeadas.

O protótipo público sem corpo ainda sai com `implementation-limit`
porque o emissor extrai a assinatura a partir da posição do corpo.

## Caminho

1. E03a: o `extent` (bug, o único que dá saída errada) com caso à mão.
2. E03b: as mensagens, junto do E02 (mesmo molde).
3. E03c: o tempo quadrático: perfil (`perf`) de `--stop-after=gen` com 4 000
   funções, antes de supor a causa.
4. E03d: a cobertura dos agregados nomeados está resolvida. Falta o protótipo público sem corpo.
