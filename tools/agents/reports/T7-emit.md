# T7 — relatório: `engine/emit/emit.c` (M3)

**Estado: aceitação passa (atualizado em 2026-10-26; o bloqueio abaixo foi resolvido pela opção 1 em `c337f85`).** Texto original do agente:

**Estado: aceitação NÃO passa.** `emit.c` está escrito e o oráculo de asserções
passa, mas `tools/cgen/test/unit/emit.sh` falha por LeakSanitizer, que vem do
próprio oráculo (`tools/cgen/test/unit/emit_main.c`), fora da lista de arquivos
do cartão. Não alterei o oráculo nem o `emit.sh`. Precisa de decisão (ver
"Bloqueio").

## O que mudou

- `tools/cgen/src/engine/emit/emit.c` (único arquivo alterado). Sem `stdio`,
  sem alocação, `static` para tudo que não está no header. Compila limpo com
  `-Wall -Wextra`.

O que o arquivo faz, por `k_emit_file`:

- **Moldura** dos headers: comentário com `—`, guarda por `k_mangle_guard`,
  `R0` sempre, bloco sintético (`keel.type.h`; no keel C11, `stdbool`/`assert`;
  no keel C23, nada), `R1` só com bloco e conteúdo, `R5` salvo quando o arquivo
  está vazio. Essas regras batem com os dois `golden/<p>/keel.h`, que não têm
  linha vazia sobrando.
- **`.type.h`**: `import_c` vira `#include <x>` (a partir do fonte, sem
  alocação); `typedef`, `static_assert` (vira `_Static_assert` só no C11) e o
  corpo de `extern_c [type_h]` saem com a linha do `.k`. Duas `typedef` na mesma
  linha saem numa só linha, com todos os `pub` retirados.
- **`.h`**: protótipo de cada função pública, uma por linha de fonte, nome por
  `k_mangle_decl`.
- **`.c`**: todo o fonte, linha a linha; linha de construção que não vai ao `.c`
  vira linha vazia; função sai sem visibilidade e com o nome mangled.
- **`K_EMIT_ENTRY`**: texto fixo de `main_<s>.c`.
- **Fora da fatia**: `K_EMIT_UNSUPPORTED` para qualquer nó que não seja
  `module`, `import_c`, `typedef` de uma linha, `static_assert`/C opaco,
  `extern_c [type_h]`, `protocol`, `func`.

## O que medi na AST (com `k_parse_ast` sem tabela de símbolos)

Medido com um programa de `/tmp` (ligado aos fontes de `engine/`, sem `emit.c`):

- `pub` é o primeiro token do nó (`first`) de `typedef` e de função. Função
  **sem** palavra também tem `is_public = 1`: o default é `pub` (`keel-spec.md`
  linha 627: "Sem `pub` nem `priv`, a declaração é `pub`").
- Comentários `//` e `/* */` não são tokens e ficam fora de qualquer nó. A
  quebra de linha também fica fora do nó; por isso usei intervalos de linhas
  (`line_start`/`line_end`), e não só os tokens.
- `import_c <stdint.h>;` tem os tokens `<`, `stdint`, `.`, `h`, `>`, `;`. A
  fatia `<...>` vai do token `first+1` ao fim do token `end-2` e é contígua no
  fonte.
- `extern_c`: `[pub] extern_c [ type_h ] {` ocupa `first..body_first-1`; o `{`
  é `body_first-1`, o `}` de fechamento é `body_end`. As diretivas `#ifndef`
  etc. dentro do corpo são **um token só** cada, com a quebra final dentro do
  token.
- `static_assert` do prelúdio é o primeiro token do nó `K_AST_OPAQUE` e o nó vai
  até o `;` da linha 28 (linhas 27-28).
- Função: `body_first-1` é o `{` e `body_first-2` é o `)` que fecha os
  parâmetros; o nó vai até o `}` de fechamento, inclusive.
- Função com parâmetros em duas linhas: o protótipo sai com `\n` interno (ver
  dúvida 4).

## Saída das aceitações

- `sh tools/cgen/test/unit/emit.sh` (rodado sem o detector de vazamento,
  `ASAN_OPTIONS=detect_leaks=0`): `ok     emit: hello (4 files) and the prelude (2 headers), c23 and c11`, exit 0.
  Com o detector normal (o comando da aceitação), falha:
  `LeakSanitizer: ... 73697 byte(s) leaked in 8 allocation(s)`, todos em
  `parse` e `slurp` de `emit_main.c` (linhas 14, 22, 23, 61, 62, 90).
  `emit.sh` saiu com `FAIL: timed out or crashed (exit 1)`.
- `sh tools/agents/verify.sh`:
  - `ok   build (4 warnings, baseline 4)`: nenhum aviso novo.
  - `FAIL make check`: só `unit/emit` (o vazamento acima). `unit/emit_mangle` e
    `unit/emit_writer` dão `ok`. O `wip gen` que sobra é de `--stop-after=gen`,
    não de `emit`.
  - O script para antes do golden por causa do `make check`.
- `make -C tools/cgen check`: `unit/emit` falha como acima; `wip gen`
  (`--stop-after=gen` ainda não existe); demais `ok`.
- `golden/run.sh` (rodado à parte): `46 ok, 0 fail, 0 xfail, 0 xpass, 3 wip`.

## Bloqueio (preciso de decisão)

O oráculo `emit_main.c` nunca libera os buffers de `parse` nem de `slurp`. O
`emit.sh` não define `ASAN_OPTIONS=detect_leaks=0`, ao contrário de
`unit/tool_load.sh` e `unit/loader_regressions.py`, que definem. Duas saídas:

1. Acrescentar `export ASAN_OPTIONS=detect_leaks=0` em `emit.sh`, como os
   outros scripts; ou
2. Liberar os buffers em `emit_main.c`.

Não fiz nenhuma das duas, porque o cartão não me deixa tocar o oráculo. A
opção 1 é a mais coerente com o resto do repositório.

## Dúvidas e desvios (para revisão)

1. **Uma chamada por linha, não `k_emit_at(1, todo_o_fonte)`.** O cartão pede
   uma só chamada para o `.c`. Sem alocação não existe buffer do arquivo inteiro.
   Cada linha é uma chamada a `k_emit_at(n, linha)`, e o `KWriter` produz bytes
   idênticos, porque só escreve `#line` quando o contador difere. Verifiquei no
   `hello.c` esperado.
2. **Linha editada cabe em buffer fixo de 1024 bytes.** `KWriter` só aceita um
   texto contíguo por `k_emit_at`; uma linha com `pub` retirado, função
   renomeada, `static_assert` ou `import_c` precisa ser montada. Linha maior que
   1024 vira `K_EMIT_UNSUPPORTED`, nunca truncamento. **Isso é uma lacuna do
   `writer.h`**: falta uma forma de emitir texto editado (uma função que receba
   segmentos). Não alterei o writer.
3. **Typedef não recebe mangling.** A tabela do cartão só manda tirar `pub`, e o
   `typedef` de um módulo que não é o prelúdio sairia com nome sem o prefixo do
   módulo (`backend §2.1`). Só o prelúdio e o `hello` estão no oráculo, e neles
   os nomes ficam como estão. Precisa decidir antes do M5.
4. **Protótipo multilinha.** Função com parâmetros em mais de uma linha gera
   protótipo com `\n` interno (`k_emit_at` conta as quebras, então o `#line`
   continua certo). O cartão diz "uma linha por função"; não ocorre no oráculo.
5. **`priv` também sai do `.c` e do `.type.h`.** O cartão cita só `pub`, mas
   `priv int f()` deixaria `priv` no C. Tratei `priv` junto. Não ocorre no
   oráculo.
6. **Corpo de função é copiado verbatim.** Uma chamada a outra função do módulo
   (`helper(a)`) sairia sem o prefixo do módulo no `.c`. O cartão pede corpo
   verbatim, mas isso deixa o `.c` errado para qualquer módulo com dois nomes
   internos. Precisa de decisão de escopo (provavelmente o M5, com `island` de
   chamada).
7. **Comentário fora de construção vai para o `.c`.** O cartão diz "todo o
   fonte": o comentário de cabeçalho do `keel.k`, por exemplo, sai no `.c`.
   Não é problema para o oráculo (o prelúdio não tem `.c`), mas não está
   decidido se deveria.
8. **Recusas extras, além da lista do cartão.** `extern_c` sem `[type_h]` (vai
   para `.h` pela P16, ainda sem implementação aqui), `import_c "..."` (só
   `<...>`), `typedef` que ocupa mais de uma linha, e não recuso `inline`
   (função com `inline` sai como no fonte; não testado).
9. **Entrada `main_<s>.c`.** O comentário diz `--main <nome>` com o nome como
   foi dado (`app.cx`), não o símbolo mangled, porque é o que o usuário digitou.
   O cartão manda usar `k_mangle_*` ali; decidi pelo nome dado. Confirmar.
10. **Quebra de linha compartilhada com `}` de `extern_c`.** Se uma `typedef`
    começar na mesma linha em que fecha o `extern_c [type_h]`, a linha inteira
    vai para o `.type.h`. Não ocorre no oráculo.

## Não feito

- Não commitei nem dei push.
- O `emit.sh` continua falhando até a decisão do bloqueio.
