# T50 — estouro de capacidade do emissor tem mensagem própria

## Objetivo

Hoje todo `e->ok = false` do emissor sobe como `K_EMIT_UNSUPPORTED`, e o
`stop_gen.c` imprime "the emitter does not cover this module's constructions
yet [implementation-limit]". Isso engana quando a causa é um buffer interno
(`LINE_CAP 1024`, `MAX_EDITS 32`, o scratch de 2048 bytes, `NAME_CAP`): a
construção está coberta, o que estourou foi a capacidade. A
`cgen-tool-spec.md` §7.1 já manda: `implementation-limit` "informa qual limite,
seu valor, ... e a configuração a ajustar". **Nenhum normativo muda.**

Reprodutor (verificado): um módulo `lc` com
`pub i32 big(i32 argument_number_0, ..., i32 argument_number_59)` dá hoje
`cgen: error: lc.k: the emitter does not cover this module's constructions yet [implementation-limit]`.
O oráculo, escrito à mão, é `test/cgen/cli/emit_limit.sh` (não edite).

## Arquivos

- Editar: `tools/cgen/src/engine/emit/emit.h`, `emit_internal.h`,
  `emit_core.c`, `files.c`, `instance.c`, `decls.c`, `parallel.c`,
  `tools/cgen/src/tool/stop_gen.c`; outros arquivos de `engine/emit/` só se um
  `e->ok = false` de capacidade morar neles.
- Escrever: `tools/agents/reports/T50-emit-limit-message.md`.
- Não editar: `test/cgen/cli/emit_limit.sh`, golden, normativos.

## Contrato

1. Em `emit.h`, junto de `K_EMIT_UNSUPPORTED ((size_t)-1)`:

   ```c
   #define K_EMIT_LIMIT_LINE    ((size_t)-2)   /* LINE_CAP: one line or prototype */
   #define K_EMIT_LIMIT_EDITS   ((size_t)-3)   /* MAX_EDITS: edits of one line */
   #define K_EMIT_LIMIT_SCRATCH ((size_t)-4)   /* the scratch of edit text (2048) */
   #define K_EMIT_LIMIT_NAME    ((size_t)-5)   /* NAME_CAP: a generated name */
   #define K_EMIT_IS_FAILURE(n) ((n) >= (size_t)-5)
   const char *k_emit_limit_text(size_t code);   /* the text of the message, or NULL */
   ```

   Pode simplificar os códigos se achar melhor, mantendo: um código por limite,
   e `k_emit_limit_text` devolvendo o texto com **nome e valor** do limite
   (use os `#define`s, não números copiados), p.ex.
   `"a generated line needs more than LINE_CAP = 1024 bytes (engine/emit/emit_internal.h); shorten the declaration"`.
   O texto da linha tem de conter `LINE_CAP` e `1024`.
2. `Emit` ganha o campo `size_t limit` (0 = nenhum). Os sítios que hoje fazem
   `e->ok = false` **por capacidade** passam a gravar também `e->limit` com o
   código: `k_em_put_bytes` e `k_em_put_mangled` (LINE), `k_em_push_edit` e
   `decls.c:223` (EDITS), `k_em_scratch` (SCRATCH), `decls.c:120` (NAME),
   `parallel.c:367/382/400` (cabeçalho do bloco passa de 1000: SCRATCH ou LINE,
   conforme o que de fato se esgota; leia). Os `e->ok = false` que **não** são
   capacidade (construção não coberta: `parallel.c:479`, `constexpr_block.c`,
   `subscript.c`...) ficam como estão. Audite **todos** os `ok = false` do
   diretório e liste a classificação no relatório.
   Se um estouro for seguido de outro sítio que põe `ok = false` por
   consequência, vale o primeiro código gravado (`if (!e->limit) e->limit = x`).
3. `k_emit_file` e `k_emit_instance`, no `if (!e.ok)` final, devolvem
   `e.limit ? e.limit : K_EMIT_UNSUPPORTED`. O `emit_one` e os 4 outros pontos
   de `stop_gen.c` (linhas ~77, 250, 303, 654) que testam
   `== K_EMIT_UNSUPPORTED` passam a testar `K_EMIT_IS_FAILURE(need)` e, quando
   for um limite, imprimem:

   ```c
   fprintf(stderr, "cgen: error: %.*s: %s [implementation-limit]\n", ..., k_emit_limit_text(need));
   ```

   mantendo o `%.*s` (caminho ou símbolo) que cada ponto já usa; para
   `K_EMIT_UNSUPPORTED` a mensagem de hoje não muda. Cuidado: `need` sem teste
   seria usado como tamanho em `cgen_alloc` — nenhum caminho pode alocar com
   `(size_t)-2..-5`.
4. Código novo em inglês; comentários dizem o porquê.

## Passos sugeridos

1. `make -C tools/cgen -j8`; `sh test/cgen/cli/emit_limit.sh` falha hoje.
2. Audite os `ok = false` (grep em `engine/emit/`), classifique.
3. Implemente 1 a 3 e rode `emit_limit.sh` até `ok`.
4. Confira à mão um segundo limite: um corpo de função com mais de 32 edições
   numa linha só, se conseguir provocar; se não, diga no relatório.

## Aceitação

- `sh test/cgen/cli/emit_limit.sh` imprime `ok`.
- `sh tools/agents/verify.sh` termina `ALL OK` (sem warnings novos).
- `sh test/run.sh` termina sem falhas (roda isolado, demora vários minutos;
  não o pipe em `tail` para medir o exit code).
- `git status` lista só os arquivos do cartão e o relatório.

## Relatório

`tools/agents/reports/T50-emit-limit-message.md`, em pt-BR: arquivo por
arquivo o que mudou; a tabela de classificação dos `ok = false`
(capacidade × construção); a saída final dos comandos de aceitação; dúvidas.
