# keel — Roteiro

Documento **não normativo**. Diz em que ordem o trabalho vem e o que decide
cada passo. Não tem datas e não promete nada: o que vira contrato passa pela
discussão de sempre (proposta de texto, "sim", spec). As ideias em si, com o
argumento, moram em [`design/possibilidades.md`](design/possibilidades.md); aqui
só a sequência e os gatilhos.

## Onde estamos

A versão publicada é a 0.4.0 em `master`; a `0.4.1` está em andamento (os
achados da terceira revisão de código, sem mudança normativa). O `cgen` emite C11
e C23 para os 90 casos do `golden/`. Os normativos são versionados e o
`CHANGELOG.md` registra as mudanças. O que vem depois da 0.4.1 está na §5: uma
0.5.0 só de estabilidade, a `keel_libs` fechada e, em seguida, o `cgen` escrito em
keel.

## 1. Agora: documentação e bibliotecas

- **Manual (`keel-manual.md`).** Exemplos soltos 3.5, 3.8 e 3.9 e a
  reverificação dos exemplos *(verificado)* contra o cgen 0.2.0 (foram
  compilados com o 0.1.1).
- **Bibliotecas.** Atualizar `check_probes.py` e os resultados de `keel_maths`
  (dois probes de lacuna agora compilam) e rodar `make clean` ao trocar de
  compilador.

## 2. Em seguida: uso real alimenta o cgen

Os achados mais valiosos vieram de usar keel em bibliotecas de verdade
(`../keel_libs`: `coll`, `maths`), não de teste sintético. A regra é a mesma de
sempre: cada achado vira um caso no `golden/` (ou um teste em `test/cgen/`)
antes do conserto, e o que é fronteira da linguagem volta como prosa, sem
mudar o normativo às pressas.

- Coleções genéricas em `keel_libs/coll` como carga de prova.
- Matrizes e tensores em `keel_libs/maths`.
- Lacunas de cobertura dos diagnósticos e novas auditorias externas.

## 3. v1: casos que depuram a linguagem

A linguagem está fechada, salvo bug ou erro de spec; funcionalidade nova é
futuro (§4). A v1 **não** é uma biblioteca padrão canônica a construir. Com
protocolos e papéis conferidos em todos os módulos, qualquer biblioteca pode
ser "da stdlib", então o que se chamar de stdlib é só a escolha de um conjunto
de casos de uso que servem para depurar keel e o cgen.

As bibliotecas vivem em projetos independentes (`../keel_libs`, um repositório com `coll`,
`maths`). Os módulos de `base/` estão neste repositório porque as próprias
construções da keel dependem deles, e por nenhum outro motivo. O que as
bibliotecas acham volta para cá como relatório de bug (`achados/README.md`),
nunca como pedido de módulo novo.

Candidatos a caso de uso, em ordem de dependência. Cada um só se torna caso
quando uma biblioteca real o pedir, e o que importa é o que ele revela na
linguagem:

1. `pair` — sem dependências; o primeiro exercício.
2. `buffer(N)` / `slice(N)` e `slice.from` — o que a `keel_coll` hoje resolve
   com `coll.span` e `coll.readspan`.
3. Coleções (`stack`, `queue`, `ring`) — já em `keel_libs/coll`.
4. `string` / `strview`.
5. `bitbuffer(W)` / `bitslice(W)`.
6. `grid` (`box(N)`, `index(N)`) — em parte em `keel_libs/maths`.
7. Cooperativo: `atomic`, `chan`, `barrier`.

## 4. v2+: extensões da linguagem

Cada uma tem seu gatilho de reabertura em `possibilidades.md`; nenhuma entra
sem caso real que o dispare.

`pub import` e `keel.pool` (índice geracional) entraram na 0.4.0.
O desenho anterior, com ponteiro e geração sobre arena, está arquivado em
`design/possibilidades.md`.

| Item | Gatilho |
| --- | --- |
| Protocolo nominal | verbo de protocolo sobre `T` no corpo de um genérico (hoje `T` é opaco); ainda por estudar a fundo |
| Moldes (`keel_code`) | engavetado; o caso real (conjunto ordenado) apontou antes para o comparador como eixo de instanciação |
| Comparador como eixo de instanciação (`instance ... by cmp`) | chave que `==`/`<` direto e a coluna-índice não cobrem (string, ordem do usuário); medido em 2026-11-15: 5× sobre o ponteiro guardado (`achados/`, B3/B6) |

Esta tabela está defasada e vai ser reestruturada: vários itens antigos já
foram descartados ou implementados. Até lá, só o que está acima é atual.

## 5. A 0.5.0 e o cgen em keel

Plano de 2026-11-25, ainda sem datas. Duas etapas, nesta ordem.

### 5.1 A 0.5.0: estável, sem funcionalidade nova

A 0.5.0 não traz construção nova: fecha o que a revisão e o uso real acharam e
deixa o `cgen` em C num estado que valha como ponto de partida. O que entra:

- **O `transform` endurecido** (achados 2026-11-25, X1): limite em `load_file`
  (T1), valor ausente e `else` no laço de opções (T2), cabeçalho que não cabe em
  `head` recusado em vez de descartado (T3), retorno de `fwrite` e `fclose`
  verificado. Ele continua sendo o bootstrap da base do compilador (`gen/`);
  só deixa de errar em silêncio. Fica até o fim do §5.2.
- **O resto dos achados baratos:** X2 (`mktemp -d` nos scripts de teste), X3
  (`git add` só do que o `install-zed.sh` copiou), X4 (linha malformada em
  `.keelsyms`) e as clarezas Q1 a Q13.
- **S2:** as quatro variáveis estáticas do motor (`spanned_end`, `pending_cap`,
  `seed_node_count`, `seed_count`) passam para `KAst` ou `Emit`. É pré-requisito
  do §5.2: um motor sem estado de arquivo é o que se porta e o que se usa como
  biblioteca.
- **Os achados abertos do `keel_libs`** (A1 a A8) que se reproduzirem como bug
  do `cgen`.
- **A suíte `asan`** (`make -C tools/cgen asan`) rodando limpa, e a lista de
  warnings extras (Q9) decidida.

Fica registrado, sem entrar na 0.5.0, o que foi medido e adiado: a tabela de
símbolos linear (`k_symtab_lookup`, parser-design §2.1; cresce ×3 por
duplicação a partir de uns 4 mil símbolos), a reserva por token de `KAstNode`
(128 bytes) e `KSymbol` (72), e o limite de arena só em tempo de compilação. O
gatilho é o mesmo de sempre: uma aplicação real que reclame do tempo ou do
teto. A solução provável da tabela é um índice por dispersão com o
armazenamento dado pelo chamador, como já é o `decl_slots`, e a troca da decisão
do §2.1.

### 5.2 O cgen em keel

**Pré-requisito: a `keel_libs` fechada.** Um compilador precisa de coleções,
texto (`string`, `strview`), tabelas e ordenação, e não adianta começar o port
sem tê-los. A `keel_libs` (coleções, `string` etc.) tem de estar pronta e
estável antes, e a base compilada em keel junto com ela já é, por si, um passo
grande na direção de keel escrita em keel: é o `cgen` usando só keel nas
bibliotecas de que depende, ainda que o motor continue em C. Os itens da §3
(`string`/`strview`, `buffer(N)`/`slice(N)`) deixam de ser "quando uma biblioteca
pedir" e passam a ser o que o port pede; o que faltar entra como caso de uso, no
fluxo de sempre (`achados/README.md`).

Com a 0.5.0 publicada e a `keel_libs` fechada, o `cgen` passa a ser escrito em
keel, módulo a módulo. É,
ao mesmo tempo, a maior aplicação para testar a linguagem e a prova do uso que o
README promete: ilhas de keel num mar de C, em adoção incremental. A linguagem
continua fechada: o que o port tropeçar volta como relatório de bug ou lacuna
da spec (`achados/README.md`), não como pedido de construção.

**A costura é a que já existe.** `src/engine/` é puro (sem I/O, sem alocação, só
o fonte e a entrada); `src/tool/` é a CLI e o sistema de arquivos. Porta-se o
`engine/` de dentro para fora, e cada passo deixa o resto em C:

1. Folhas sem estado: léxico (`lexer*`, `token_predicates*`), `names`, `diag`.
2. Símbolos e parser (`symtab`, `parser_*`, `instances`). É aqui que o índice da
   tabela de símbolos entra.
3. O emissor (`emit/*`), por último: é o maior e o que mais depende dos
   anteriores.
4. `tool/`, só se os passos acima mostrarem que vale.

Para cada módulo portado: o `.k` substitui o `.c`, o `golden/` e a suíte do
cgen continuam sendo o oráculo, e o oráculo de uma tarefa nunca é capturado da
ferramenta (regra do `AGENTS.md`).

**Bootstrap em estágios.**

- *Estágio 0:* o `cgen` em C da 0.5.0, congelado como o binário da distribuição.
- *Estágio 1:* o `cgen` em keel, compilado pelo estágio 0.
- *Estágio 2:* o estágio 1 compilando a si mesmo.

O critério de aceite **não** é o C do `cgen` ser igual entre os estágios: o jeito
de escrever em keel produz, naturalmente, um C diferente do escrito à mão, e
exigir igualdade seria exigir que o port imitasse o original. O que tem de ser
byte a byte igual é o que cada `cgen` **produz**: o estágio 0, o 1 e o 2, cada um
compilando o `golden/`, geram exatamente o `expected/` escrito à mão (perfis c11
e c23), e passam a mesma suíte (`test/cgen/`). O estágio 2 existe para provar que
o `cgen` em keel compila a si mesmo e que o binário resultante continua
passando nos dois oráculos. Resta decidir se o C do estágio 1 entra no
repositório, para quem clona sem um `cgen` anterior. Enquanto o `cgen` ainda
usar a base do bootstrap (`tools/transform/base/`), o `transform` continua; ele
sai quando o `cgen` compilar a base real (`base/keel`), e a retirada é o último
passo, sem pressa.

**O que medir antes do primeiro passo:** o que a base e a `keel_libs` já cobrem
do que o motor usa (texto, tabelas, ordenação, buffers sobre arena), e o
custo de cada módulo portado em tempo de compilação e de execução do `cgen`.

## Fora do escopo

O que foi recusado, com o motivo, está na seção "Recusadas" de
`design/possibilidades.md`. Reabrir exige o gatilho registrado lá.
