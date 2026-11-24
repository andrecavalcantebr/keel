# keel — registro de mudanças

Este arquivo registra, por versão, o que mudou na linguagem, no backend e na
ferramenta. Os quatro documentos normativos (`keel-spec.md`, `keel-rationale.md`,
`keel-c-backend.md`, `cgen-tool-spec.md`) descrevem a versão que o cabeçalho de
cada um diz. Uma regra, um diagnóstico ou uma opção que entrou ou mudou depois
da 0.1.2 leva a marca *(desde X.Y.Z)* no próprio texto; o que existia antes não
leva marca.

Este arquivo não é normativo. Em caso de dúvida, vale o normativo.

## 0.4.0 (em andamento)

**Linguagem** (`keel-spec.md`)

- §5.8, novo: `keel.bitbuffer(W) T` e `keel.bitslice(W) T`, sequência e vista
  de elementos de `W` bits, e `keel.bits`, o empacotamento que os dois
  compartilham. Diagnósticos novos: `bitbuffer-width-invalid` (erro, pelo
  compilador C) e `bit-value-out-of-width` (debug). A §5.8 "Biblioteca padrão"
  passou a §5.9.
- §4.2, regra 3 e `base-type`: as palavras C de um tipo aritmético (`unsigned char`, `long long`)
  são argumento de modificador e normalizam para o tipo keel. Diagnóstico novo: `c-type-spelling`
  (`warning`); `c-type-as-argument` fica para o que não tem tipo keel (`long`).
- §4.5, `..<` e `..=`: o limite superior de um intervalo leva um marcador; `a..<b` é `a..b`, e `a..=b` inclui `b`. Fim aberto (`a..`, `a..<`, `a..=`) é uma forma só. Vale em `x[...]`, `foreach` e no literal `range`. Debug novo: `inclusive-range-overflow`. Léxico e gramática (`dots`) ajustados.

**Backend** (`keel-c-backend.md`)

- §5.20, novo: a emissão de `bitbuffer` e `bitslice`; §5.17 ganha as três
  verificações dos verbos novos.
- §2.1: a tabela de nomes ganha as palavras C aritméticas e `struct S`; o C emitido usa o nome
  keel e a etiqueta renomeada.
- §5.17: `keel_inclusive(b)`, o limite exclusivo de `a..=b`, com `inclusive-range-overflow`; §5.2 regra 9, §5.3 regra 7 e §5.7 regra 2a dizem como cada construção o escreve.

**Rationale** (`keel-rationale.md`)

- Seção nova, "`bitbuffer` e `bitslice`": por que não há `ptr`, o layout por
  byte, o alinhamento de `partition`, `make` e `bytes`, e o módulo `keel.bits`.
- Nota nova em "Modificador e tipo modificado": por que `unsigned char` é aceito, normalizado, e por que `long` não.
- Seção nova, "Intervalos: o marcador do limite superior": por que `a..b` não é depreciado, o fim aberto, o `b + 1` e o `SIZE_MAX`.

**Base** (`base/keel/`)

- `bitbuffer.k`, `bitslice.k` e `bits.k`.
- `keel.k`: `keel_inclusive`, ao lado de `keel_index`.

**Ferramenta**

- A ligação (cgen-tool-spec §4.1, *desde 0.4.0*): os `.c` gerados passam a vir **antes** dos `.a`, `.so`, `-l`, `-L` e
  `-Wl,` da linha, seja qual for a ordem em que o usuário os escreveu. Antes, um
  `.a` ou um `-lfoo` dava `undefined reference` (o `.o` não dependia da ordem).
  O manual ganha o §7.6 (bibliotecas: fonte, C e exportar para C).
- Dois bugs do cgen (achados A1 e A3 de `keel_libs`, golden 083 e 084): o `.type.h`
  de uma instância inclui o `.type.h` do módulo de um argumento que é `typedef`
  de outro módulo e fica por valor num campo (`outcome item`); e um módulo
  genérico com mais de um modificador resolve o verbo sobre qualquer um deles,
  não só sobre o primeiro declarado.
- `typedef` de uma instância de modificador (achado A2 de `keel_libs`, golden 085):
  `typedef buffer u8 text;` e `typedef slice.slice const u8 view;` eram tratados
  como C opaco, embora `decl-typedef` aceite um `specifier` (spec §4.2). Agora o
  nome vale como a instância: o `typedef` sai com o símbolo canônico, a instância
  entra no fecho e os verbos do módulo aceitam o nome como contêiner, também em
  quem o importa.
- Tipos C como argumento de modificador (spec §4.2, regra 3 e `base-type`; backend §2.1;
  golden 086, achado de `keel_libs`): `slice const unsigned char`, `buffer long long`
  e `slice struct Person` são aceitos. As palavras C de um tipo aritmético são lidas
  como o tipo keel que designam (`u8`, `i64`, ...), em qualquer ordem, e o C emitido usa
  o nome keel. O uso emite o `warning` novo `c-type-spelling`, com o nome idiomático
  como dica. `c-type-as-argument` passa a valer só para o que não tem tipo keel (`long`,
  `unsigned long`, `long double`). O campo de um `struct` do módulo usa a etiqueta
  renomeada (`struct recs_Person`), e não a escrita.
- O `implementation-limit` diz o que estourou (E02, E03): o catálogo passa a `implementation limit: %s`, e cada
  ponto de emissão (binders, variáveis visíveis, delimitadores, tabelas de módulo, ilhas, fecho, raízes de busca)
  dá a própria frase, com o valor e o caminho. O que antes saía como `unexpected-token` ou "does not cover" por
  capacidade (`protocol` com mais de 16 itens, `array` com mais de 8 declaradores, `defer [now ...]` com mais de 8
  entradas, `parallel` com mais de 16 capturas) nomeia o limite; o de `match` com muitos rótulos não manda mais
  "encurtar a linha". Teste: `cli/limit_messages`.
- `..<` e `..=` (golden 087, `cli/range_marker`): o lexer junta o marcador às pontos (`..<`, `..=`, e `..<=` continua `..` e `<=`); `isl_punct` e o emissor leem `..` como as pontos com ou sem marcador; `subscript.c`, `foreach.c`, `rangelit.c` e `parallel.c` (`0..=3` são quatro workers) tratam o `..=`; `base/keel.k` ganha `keel_inclusive`; `inverted-range-index` conta `a..=b` como `[a, b+1)`.
- `--cgen-version` diz `cgen 0.4.0-dev`; `test/run.sh AREA...` roda só uma fatia dos oráculos.
- Um conjunto `tags` passa de 16 a 128 constantes (o 17º item dava
  `unexpected-token`, apontando o início da declaração; achado 1 do xprk). Acima
  de 128, o diagnóstico novo `tags-limit-exceeded` diz o conjunto e o limite. É
  do cgen, não da linguagem: está só no `diag-messages.md`.
- O tamanho de uma declaração deixa de limitar a geração: uma tabela `pub array`
  com 30 ou mais referências a `tags` dava `a source line needs more than
  MAX_EDITS = 32 edits` (achado 2 do xprk). A causa estava no `.h`: o `extern`
  calculava as edições da declaração inteira para descartar as do inicializador.
  Agora só as de fora dos inicializadores são calculadas, e `MAX_EDITS` deriva
  de `LINE_CAP` (512), de modo que o limite de edições não vem antes do de linha.
  Caso 081 do golden. Resta o `LINE_CAP` (1024 bytes de C por linha física do
  fonte); o `implementation-limit` de capacidade passa a apontar a linha do `.k`
  (`arq.k:4: error: ...`) e a dizer como sair: quebrar a linha de origem. O limite está no `cgen-tool-spec.md` §7.1, na linha do `implementation-limit`.
- A recusa de uma instância pelo emissor (`implementation-limit`) passa a dizer
  em que construção e em que linha.
- `foreach` e `walk` sobre contêiner de módulo com `dim` (`bitbuffer(4) u8`,
  `m(4) u32`): o emissor lia o tipo pelo último `(` do detalhe da ilha e
  recusava. É o que faltava do A7 da `keel_coll`.
- Função de um módulo genérico que não cita parâmetro nem modificador (regra 20
  da §4.3): chamada qualificada de fora dava `from-without-target`, e de dentro
  de uma instância saía com o nome cru. Agora é chamada como a de um módulo
  comum.
- `static_assert` de topo num módulo genérico que cita um parâmetro: o emissor
  recusava (`implementation-limit`). Agora cada instância o leva no `.type.h`,
  com os valores substituídos.
- `x[a..b]` sobre modificador de módulo com `dim`: não era reconhecido e o `cc`
  recusava a expressão. O `range-index` usa uma dimensão (§4.5), e o acessor
  de posto completo, `x[i, j]`, continua à parte.
- `x[i, j]` sobre modificador de módulo com `dim` (spec §4.5, regra 1; backend
  §5.3.1): o cgen deixava a forma passar como veio (operador vírgula de C) e o
  `cc` recusava ou, pior, aceitava o que não devia. Agora sai
  `*ptr_N(x, (size_t[N]){i, j})`, com `&` quando o contêiner é valor, e um número
  de índices diferente de `N` dá `no-ptr-for-arity`. Caso 082 do golden, e
  `index-rank` em `test/cgen/diag/`. Não havia caso do golden que cobrisse a forma.
- `extent` com mais de 4 grupos `[k, n]` saía com código 0 e deixava o texto keel
  no `.type.h`, para o compilador C recusar (E03 dos achados). Agora é
  `extent-limit-exceeded` (do cgen, só no `diag-messages.md`), como o
  `tags-limit-exceeded`. Teste `test/cgen/cli/extent_limit.sh`.
- O teto de nome gerado passa de 255 a **215** caracteres (backend §2.4, regra 1
  e nova regra 5; D21; rationale "Por que o teto de nome é 215"). O símbolo de
  uma instância é também o nome dos seus arquivos, e o sistema de arquivos limita
  o componente a 255 antes dos sufixos. Medido na 0.4.0-dev, o teto de 255 era
  inalcançável: `write-failure` vinha desde 216 caracteres, `implementation-limit`
  desde 224, e `name-too-long` só aos 233 (E01 dos achados, opção A). Agora é um
  diagnóstico só, `name-too-long`, no 216º. O arquivo temporário da escrita tem
  nome curto e fixo (`.keel.<pid>.tmp`), e `NAME_CAP` sobe a 1024 para o caminho
  com a pasta não disparar antes. Testes `cli/name_limit.sh` e `cli/imports.py`.

## 0.3.0 (2026-11-19)

**Linguagem** (`keel-spec.md`)

- §4.3: o uso de um modificador de módulo com `dim` leva um valor para cada
  `dim`. Diagnóstico novo: `modifier-dimension-count` (erro). Antes, o uso sem
  as dimensões passava pelo cgen e falhava no compilador C
  (`unknown type name`).
- §4.1, regra 5: sem alias, o qualificador de um import é o nome do módulo por
  inteiro (`import m.t;` então `m.t.algo`). A regra não estava escrita; o cgen
  não resolvia nem essa forma nem a de um só segmento.
- §4.1, regra 6: `types` também injeta nu o nome de um conjunto de tags do
  módulo importado, não só tipos, modificadores e protocolos — o rationale já
  dizia isso (§4.1), mas a regra não estava na spec, e o cgen não injetava.

**Rationale** (`keel-rationale.md`)

- Seção nova, "O custo também é previsível: o laço que o compilador
  vetoriza": a previsibilidade do C gerado como justificativa de várias
  decisões, o que se mediu e o que keel recusa (`always_inline`).

**Documentação** (não normativa)

- `keel-manual.md` §3.11 e `keel-llm-context.md`: como escrever para o
  compilador C vetorizar e como conferir.

**Ferramenta** (`tools/cgen`)

- Correções dos achados da `keel_coll` sobre a 0.2.0 (`achados/README.md`): o
  ponteiro de função passado como argumento não recebe `&` (A6); a dimensão de
  `array` passada a `arena.from_array` pode ser expressão constante que começa
  por dígito (A5); `slice const T` com `T` struct sai grafado como C (A1); a
  instância de um genérico que devolve ou constrói instância de outro genérico
  pelos binders sai completa (A4).
- O `implementation-limit` de uma construção que o emissor não cobre passa a
  dizer `arquivo:linha:coluna` e a construção (A7, A9); a
  `cgen-tool-spec.md` §7.1 já pedia a posição.
- Achados do `xprk` (`achados/README.md`): `alias.Set.VALOR`, com `alias` de
  outro módulo, resolvia só até o tipo do conjunto e deixava `.VALOR` para o
  compilador C recusar (golden 075); um import sem alias não resolvia nenhuma
  forma do nome do módulo nos usos qualificados, nem o próprio por inteiro
  (regra 5); `types` não injetava o nome de um conjunto de tags (regra 6).

## 0.2.0

**Linguagem** (`keel-spec.md`)

- §4.12, regras 16 e 17: o corpo da função é conferido contra a própria
  assinatura. Diagnósticos novos: `role-missing` (erro) e `return-may-escape`
  (aviso). A regra 12 passa a remeter à regra 16.
- §4.14, regra 8: a função sobre protocolo é sempre `inline`. Escrever `inline`
  é permitido e redundante; não há forma fora de linha.
- §5.1, regra 10: o verbo de protocolo herda os papéis do protótipo. Com dois
  ou mais protocolos visíveis de mesmo nome e aridade, o verbo escreve ao menos
  a união dos papéis exigidos. Diagnóstico novo: `protocol-role-ambiguous`
  (erro).

**Backend** (`keel-c-backend.md`)

- §4.1: a invocação produz quatro arquivos por módulo (`.type.h`, `.h`, `.c` e
  `.keelsyms`), e não três. D28 acompanha.
- §2.4 e D22: `--pedantic-names` retirada; o teto de nome gerado é um só, 255.

**Ferramenta** (`cgen-tool-spec.md`)

- §4.11: tabela de símbolos `.keelsyms`, com a correspondência entre o nome
  manglado e o nome keel, para a depuração.
- §4.1: `--cgen-version` imprime, além de `cgen <versão>`, as linhas `commit`,
  `date` e `cc` (a procedência do binário).
- §4.1: `--pedantic-names` é reconhecida e recusada com `[implementation-limit]`.

**Cgen** (comportamento sem mudança de texto normativo)

- Correções vindas de duas análises externas: temporário de saída aberto com
  `O_EXCL`, mensagem de limite do emissor que nomeia o limite, erro de análise
  num `import` que já não cala o importador, e dimensões de módulo genérico no
  símbolo da instância do fecho.

## 0.1.2

Publicada em 2026-11-12. Só documentação e número da versão em relação à 0.1.1.
