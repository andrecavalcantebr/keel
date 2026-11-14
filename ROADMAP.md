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
mudou. O trabalho rotineiro vai na branch `0.3.0`; `master` fica na versão
publicada.

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

## 3. v1: casos que depuram a linguagem

A linguagem está fechada, salvo bug ou erro de spec; funcionalidade nova é
futuro (§4). A v1 **não** é uma biblioteca padrão canônica a construir. Com
protocolos e papéis conferidos em todos os módulos, qualquer biblioteca pode
ser "da stdlib", então o que se chamar de stdlib é só a escolha de um conjunto
de casos de uso que servem para depurar keel e o cgen.

As bibliotecas vivem em projetos independentes (`../keel_libs`: `keel_coll`,
`keel_maths`). Os módulos de `base/` estão neste repositório porque as próprias
construções da keel dependem deles, e por nenhum outro motivo. O que as
bibliotecas acham volta para cá como relatório de bug (`achados/README.md`),
nunca como pedido de módulo novo.

Candidatos a caso de uso, em ordem de dependência. Cada um só se torna caso
quando uma biblioteca real o pedir, e o que importa é o que ele revela na
linguagem:

1. `pair` — sem dependências; o primeiro exercício.
2. `buffer(N)` / `slice(N)` e `slice.from` — o que a `keel_coll` hoje resolve
   com `coll.span` e `coll.readspan`.
3. Coleções (`stack`, `queue`, `ring`) — já em `keel_libs/keel_coll`.
4. `string` / `strview`.
5. `bitbuffer(W)` / `bitslice(W)`.
6. `grid` (`box(N)`, `index(N)`) — em parte em `keel_libs/keel_maths`.
7. Cooperativo: `atomic`, `chan`, `barrier`.

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
