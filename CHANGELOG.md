# keel — registro de mudanças

Este arquivo registra, por versão, o que mudou na linguagem, no backend e na
ferramenta. Os quatro documentos normativos (`keel-spec.md`, `keel-rationale.md`,
`keel-c-backend.md`, `cgen-tool-spec.md`) descrevem a versão que o cabeçalho de
cada um diz. Uma regra, um diagnóstico ou uma opção que entrou ou mudou depois
da 0.1.2 leva a marca *(desde X.Y.Z)* no próprio texto; o que existia antes não
leva marca.

Este arquivo não é normativo. Em caso de dúvida, vale o normativo.

## 0.3.0 (em andamento)

**Linguagem** (`keel-spec.md`)

- §4.3: o uso de um modificador de módulo com `dim` leva um valor para cada
  `dim`. Diagnóstico novo: `modifier-dimension-count` (erro). Antes, o uso sem
  as dimensões passava pelo cgen e falhava no compilador C
  (`unknown type name`).

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
