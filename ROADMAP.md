# keel — Roteiro

Documento **não normativo**. Diz em que ordem o trabalho vem e o que decide
cada passo. Não tem datas e não promete nada: o que vira contrato passa pela
discussão de sempre (proposta de texto, "sim", spec). As ideias em si, com o
argumento, moram em [`design/possibilidades.md`](design/possibilidades.md); aqui
só a sequência e os gatilhos.

## Onde estamos

A linguagem v0 está fechada: os quatro normativos estão completos, a base está
escrita em keel e o `cgen` emite C11 e C23 para todos os casos do `golden/`
(71 casos, nos dois perfis). A versão corrente é a 0.2.0, em `master`
(2026-11-14); os normativos são versionados e o `CHANGELOG.md` registra o que
mudou. Trabalho rotineiro continua na branch de longa duração da versão
seguinte.

## 1. Agora: o que fica depois da 0.2.0

- **Manual (`keel-manual.md`).** Exemplos soltos 3.5, 3.8 e 3.9 e a
  reverificação dos exemplos *(verificado)* contra o cgen 0.2.0 (foram
  compilados com o 0.1.1).
- **Publicar a 0.2.0 no GitHub** (tag `v0.2.0`, `make release VERSION=0.2.0`).
  Depois, as bibliotecas passam a citar a tag nos resultados.
- **Bibliotecas.** Atualizar `check_probes.py` e os resultados de `keel_maths`
  (dois probes de lacuna agora compilam) e rodar `make clean` ao trocar de
  compilador.

## 2. Em seguida: uso real alimenta o cgen

Os achados mais valiosos vieram de usar keel em bibliotecas de verdade
(`../keel_libs`, `../keel_maths`), não de teste sintético. A regra é a mesma de
sempre: cada achado vira um caso no `golden/` (ou um teste em `test/cgen/`)
antes do conserto, e o que é fronteira da linguagem volta como prosa, sem
mudar o normativo às pressas.

- Coleções genéricas em `keel_libs/keel_coll` como carga de prova.
- Matrizes e tensores em `keel_maths`.
- Lacunas de cobertura dos diagnósticos e novas auditorias externas.

## 3. v1: biblioteca padrão

Nenhum item pede trabalho de núcleo: cabem em `module`/`modifier`. A ordem
proposta segue a dependência, e cada item só começa com um caso real que o peça:

1. `keel.pair` — sem dependências, pequeno, serve de primeiro exercício.
2. `keel.buffer(N)` / `keel.slice(N)` e `keel.slice.from`.
3. `keel.coll` (`stack`, `queue`, `ring`) — depende de buffer/slice. Já existe
   uma versão independente em `keel_libs/keel_coll` para comparar.
4. `keel.string` / `keel.strview`.
5. `keel.bitbuffer(W)` / `keel.bitslice(W)`.
6. `keel.grid` (`box(N)`, `index(N)`) — depende de buffer/slice.
7. Cooperativo: `keel.atomic`, `keel.chan`, `keel.barrier`.

## 4. v2+: extensões da linguagem

Cada uma tem seu gatilho de reabertura em `possibilidades.md`; nenhuma entra
sem caso real que o dispare.

| Item | Gatilho |
| --- | --- |
| Protocolo nominal | verbo de protocolo sobre `T` no corpo de um genérico (hoje `T` é opaco) |
| `source` / `pub import` | listas de imports longas e repetidas em tarefas comuns (ex.: Tensor) |
| Intervalo inclusivo `a..<b`, `a..=b` | uso frequente do padrão meio-aberto escrito à mão |
| Moldes (`keel_code`) | engavetado; o caso real (conjunto ordenado) apontou antes para o comparador como eixo de instanciação |
| Handle (ponteiro com geração), regiões e arena na base | decisão de incorporar o texto candidato da §2 de `possibilidades.md` |

## Fora do escopo

O que foi recusado, com o motivo, está na seção "Recusadas" de
`design/possibilidades.md`. Reabrir exige o gatilho registrado lá.
