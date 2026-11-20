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

**Backend** (`keel-c-backend.md`)

- §5.20, novo: a emissão de `bitbuffer` e `bitslice`; §5.17 ganha as três
  verificações dos verbos novos.

**Rationale** (`keel-rationale.md`)

- Seção nova, "`bitbuffer` e `bitslice`": por que não há `ptr`, o layout por
  byte, o alinhamento de `partition`, `make` e `bytes`, e o módulo `keel.bits`.

**Base** (`base/keel/`)

- `bitbuffer.k`, `bitslice.k` e `bits.k`.

**Ferramenta**

- A ligação: os `.c` gerados passam a vir **antes** dos `.a`, `.so`, `-l`, `-L` e
  `-Wl,` da linha, seja qual for a ordem em que o usuário os escreveu. Antes, um
  `.a` ou um `-lfoo` dava `undefined reference` (o `.o` não dependia da ordem).
  O manual ganha o §7.6 (bibliotecas: fonte, C e exportar para C).
- `--cgen-version` diz `cgen 0.4.0-dev`; `test/run.sh AREA...` roda só uma fatia dos oráculos.
- Um conjunto `tags` passa de 16 a 128 constantes (o 17º item dava
  `unexpected-token`, apontando o início da declaração; achado 1 do xprk). Acima
  de 128, o diagnóstico novo `tags-limit-exceeded` diz o conjunto e o limite. É
  do cgen, não da linguagem: está só no `diag-messages.md`.
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
