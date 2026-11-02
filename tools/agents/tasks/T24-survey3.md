# T24 — terceiro levantamento: o que o emissor ainda recusa ou traduz mal

## Objetivo

Auditoria (não muda o emissor). O emissor ainda recusa formas válidas (`implementation-limit`) e, pior, pode **aceitar e gerar C inválido** (aconteceu: um argumento ponteiro passado a verbo por valor saía sem `*`). Ache o que resta, por **construção da spec**, com `.k` mínimos:

1. Para cada construção de `keel-spec.md` §4 e §5 (declarações, `array`/`ref`/modificadores, `defer`, `foreach`, `walk`, `parallel`, `match`/`tags`, `else`, `outcome`/`corot`/`routine`, `extent`, módulos genéricos e `instance`, `protocol`, funções com `type`/`dim`, `import_c`/`extern_c`, `constexpr`) escreva 2 a 5 `.k` **válidos** que variem a forma de escrita (uma linha / várias linhas, com / sem chaves, com comentário, com vários declaradores, via ponteiro ou valor, em módulo / em bloco, dentro de laço / de `defer`, combinadas entre si). Leia também as seções 5.x do `keel-c-backend.md` para saber a forma suportada.
2. Rode cada um nos dois perfis e classifique (`OK`, `LIMIT`, `ERR`). Para cada `OK`, **compile o C gerado**: `cc -std=c11 -fsyntax-only -Wall -I <dest> <dest>/<caminho>.c` (e `-std=c2x` no perfil C23; o compilador daqui não conhece `c23`). `OK` que não compila é **tradução errada**: o achado mais importante.
3. `ERR` é, em geral, um `.k` seu errado (leia a mensagem e corrija o `.k`); só reporte `ERR` que pareça defeito do cgen (diagnóstico falso sobre `.k` válido).

## Arquivos

Pode criar `tools/agents/survey/app3/*.k` (e `app3/lib/*.k`), estender `tools/agents/survey/run.sh` (ou criar `run3.sh` ao lado) para varrer `app3/` e para compilar o C gerado, e criar `tools/agents/reports/T24-survey3.md`. Não edite mais nada: emissor, parser, normativos, golden, testes. Não faça commit nem `git add`.

**Não rode `make` nem `verify.sh`** (outro agente constrói o cgen agora). Na primeira ação, **copie o binário** `tools/cgen/cgen` para a pasta de trabalho e use a cópia (`cp tools/cgen/cgen /tmp/claude-1000/-home-andre-code-cprojects-keel/6df5c4e9-8d62-4717-9b96-cb209ab7b812/scratchpad/cgen-t24`); se o binário não existir naquele instante, espere alguns segundos e tente de novo. A cópia é de um momento: tudo bem.

Escreva os temporários só em `/tmp/claude-1000/-home-andre-code-cprojects-keel/6df5c4e9-8d62-4717-9b96-cb209ab7b812/scratchpad/` (nunca `rm -rf` com variável no caminho).

## Contrato do relatório

`tools/agents/reports/T24-survey3.md`, em pt-BR, com três tabelas:

1. **`LIMIT`**: `.k`, a forma em uma frase, o mínimo que reproduz, e onde o emissor recusa (`grep -rn implementation-limit` não basta: procure o `return false` do `supported()` / dos `k_em_handles_*` em `tools/cgen/src/engine/emit/` e cite arquivo e função).
2. **Tradução errada** (aceita, mas o C não compila): o `.k`, o trecho gerado, o erro do `cc`.
3. **`ERR` suspeito de defeito do cgen**.

E uma lista curta do que você **não cobriu** e por quê. Mire em **40 a 80 `.k`** no total; priorize formas que um programa real escreveria. Entregue o relatório UMA vez pelo SubagentHandback e termine.
