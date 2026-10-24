# T7 — `engine/emit/emit.c` (M3): o emissor de módulo

## Objetivo

Implementar `k_emit_file`, declarada em `tools/cgen/src/engine/emit/emit.h`
(o contrato, já escrito): da AST de um módulo, produzir **um** arquivo gerado
por chamada. A fatia da linguagem é a que dois oráculos exigem, e nada mais:

- `hello` (cgen design §8.3): `module`, `import_c`, uma função pública;
- o prelúdio `base/keel.k`: `import_c`, `typedef`, C opaco de topo
  (`static_assert`), `extern_c [type_h]` e protocolos.

O oráculo é `tools/cgen/test/unit/emit_main.c` (rodado por `emit.sh`), escrito
à mão: o esperado do `hello` está em `tools/cgen/test/gen/hello/expected/<p>/`
e o do prelúdio em `golden/<p>/keel.type.h` e `keel.h`. Hoje `emit.c` está
vazio e o `make check` mostra `wip unit/emit`; quando ele ganhar conteúdo o
teste passa a valer.

## Arquivos

Escrever **`tools/cgen/src/engine/emit/emit.c`** e o relatório
`tools/agents/reports/T7-emit.md`. Nada mais: não edite `emit.h`, o teste, o
`writer`, o `mangle`, os normativos, o golden nem os esperados. Se achar que
um deles está errado, pare e diga no relatório, com a evidência.

Regras do motor: nada de `<stdio.h>`, `<stdlib.h>`, `<unistd.h>`, `<fcntl.h>`
nem `<sys/*.h>` (o `make boundary` recusa); sem alocação; `static` para o que
não está no header; nomes e comentários em inglês, curtos, citando a seção
do documento, no estilo de `writer.c`.

## O que já existe (use, não reescreva)

O `KWriter` é o único que escreve `#line` e conta linhas
(`engine/emit/writer.h`):

```c
void k_writer_init(KWriter *w, keel_slice_char out, keel_slice_char src_path, bool line_on);
void k_emit_raw(KWriter *w, keel_slice_char text);                     /* sem linha do .k; nunca escreve #line */
bool k_emit_at(KWriter *w, size_t src_line, keel_slice_char text);     /* #line só se mapped != src_line */
size_t k_writer_len(const KWriter *w);
```

O mangling (`engine/emit/mangle.h`), todas devolvem o tamanho necessário e
descartam o excedente:

```c
size_t k_mangle_decl(keel_slice_char module, keel_slice_char name, bool is_type, keel_slice_char out);
size_t k_mangle_path(keel_slice_char module, const char *ext, keel_slice_char out);   /* "hello",".h" -> "hello.h" */
size_t k_mangle_guard(keel_slice_char path, keel_slice_char out);                     /* "hello.h" -> "HELLO_H" */
```

Os nós da AST (`engine/storage_types.h`, `KAstNode`): `kind`, `first`/`end`
(intervalo **semiaberto** de índices em `ast->tokens`), `name_first`/`name_end`,
`body_first`/`body_end`, `is_public`. Um `KToken` é uma fatia do fonte
(`KToken = keel_slice_char`): o deslocamento de um token no fonte é
`token.ptr - ast->source.ptr`. Os tipos de nó que a fatia usa: `K_AST_MODULE`,
`K_AST_IMPORT_C`, `K_AST_TYPE` (o `typedef`), `K_AST_OPAQUE` (C opaco de topo),
`K_AST_EXTERN_C`, `K_AST_PROTOCOL`, `K_AST_FUNCTION`. Veja como o
`parser_dump.c` percorre os nós e como o `emit_main.c` constrói a AST.
Meça antes de supor: o intervalo `first..end` de cada nó, e se a quebra de
linha e os comentários entram nele, **imprimindo-os** num programa de três
linhas (`/tmp`), e não pelo que o comentário do header diz.

O que o `emit_main.c` mostra: `k_parse_ast` sozinho (sem tabela de símbolos)
já dá os nós certos para `hello.k` e `base/keel.k`.

## Contrato

`k_emit_file(ast, module, src_path, which, opt, out)` devolve o tamanho
necessário (mesma convenção do `k_dump_ast`), `K_EMIT_UNSUPPORTED` se o
módulo tem um nó fora da fatia (aqui: `K_AST_IMPORT`, `K_AST_MODIFIER`,
`K_AST_TAGS`, `K_AST_CONSTEXPR`, `K_AST_VARIABLE`, `K_AST_INSTANCE`; e
`K_AST_TYPE` que não seja `typedef` de uma linha como os do prelúdio).

### Moldura dos headers (`K_EMIT_TYPE_H` e `K_EMIT_H`), tudo por `k_emit_raw`

```
/* <caminho> — generated from <src_path> by cgen, C23 profile. */     (ou C11)
#ifndef <GUARDA>
#define <GUARDA>
<linha vazia>                                                          R0: sempre
<bloco sintético, se houver>
<linha vazia, só se houve bloco>                                       R1
<conteúdo>
<linha vazia>                                                          R5: sempre
#endif /* <GUARDA> */
```

`<caminho>` e `<GUARDA>` vêm de `k_mangle_path` (`.type.h` ou `.h`) e
`k_mangle_guard`. O travessão do cabeçalho é o `—` (U+2014). Linhas vazias
sintéticas não se somam: sem bloco e sem conteúdo sai uma só entre a guarda e o
`#endif`.

**Bloco sintético.**
- `.type.h` de módulo que não é `keel`: `#include "keel.type.h"` (a importação
  implícita do prelúdio). No `.h`: `#include "<caminho do .type.h>"`.
- `.type.h` de `keel` no perfil C11: `#include <stdbool.h>` e
  `#include <assert.h>`. No C23, nenhum.
- `.h` de `keel`: `#include "keel.type.h"`.

### Conteúdo do `.type.h`: cada construção com a **sua** linha do `.k`

Cada trecho sai por `k_emit_at(linha_do_fonte, texto)`, que escreve o `#line`
só quando o contador pede. O texto é o do fonte, linha a linha, a partir do
início da primeira linha do nó até o fim da última, **com `\n`**, e os nós
na ordem do fonte:

| Nó | O que sai |
| --- | --- |
| `K_AST_IMPORT_C` | a linha do fonte, `import_c <x>;` → `#include <x>` (o prelúdio e o `hello` têm só a forma `<...>`) |
| `K_AST_TYPE` (`typedef`) | as linhas do fonte, sem o token `pub` e o espaço que o segue, **cada** `pub` da linha. Dois `typedef` na mesma linha são dois nós e uma só emissão da linha |
| `K_AST_OPAQUE` | as linhas do fonte; no C11, o token `static_assert` vira `_Static_assert` |
| `K_AST_EXTERN_C` com `[type_h]` | só o **corpo**: as linhas depois da linha do `{` e antes da linha do `}`, com a numeração da primeira linha do corpo |
| `K_AST_MODULE`, `K_AST_PROTOCOL` | nada |

### `K_EMIT_H` e `K_EMIT_C`

- `.h` de função pública: o protótipo, **uma linha** por função, com a linha
  do fonte da declaração: o texto do início da declaração até o `)` que fecha
  os parâmetros, sem `pub`, com o nome por `k_mangle_decl` (função: `is_type`
  falso), e `;` no fim. Exemplo: `pub int main(int argc, char **argv) {` →
  `int hello_main(int argc, char **argv);`.
- `.c`: primeiro, pela moldura própria, **sem linha vazia**:
  `/* <caminho .c> — generated from <src_path> by cgen, C23 profile. */`,
  `#include "<caminho do .h>"`. Depois, **todo o fonte, linha a linha, num só
  `k_emit_at(1, …)`**: a linha de uma construção que não vai ao `.c`
  (`module`, `import_c`, `typedef`, …) vira linha **vazia** (a quebra fica,
  o conteúdo sai); a de uma função sai como no fonte, sem `pub` e com o nome
  mangled, e o corpo é copiado verbatim com as quebras. Mantém-se o número
  de quebras de linha do fonte: o `#line 1` único do esperado só é correto
  por isso (backend §6, regra 3).
- `K_EMIT_ENTRY` (`main_<símbolo>.c`, backend §5.8): texto fixo, ver
  `expected/<p>/main_hello.c`; o nome do módulo e o da função mangled
  entram por `k_mangle_*`. Cabeçalho próprio, **sem perfil**:
  `/* main_hello.c — entry unit, generated by --main hello. */`.

`opt.line_on == false` não escreve `#line` (o `KWriter` já cuida).

## Exemplo trabalhado (o `hello.h`, C23)

Fonte, linha 4: `pub int main(int argc, char **argv) {`.

```
/* hello.h — generated from hello.k by cgen, C23 profile. */   raw
#ifndef HELLO_H                                                  raw
#define HELLO_H                                                  raw
                                                                 raw (R0)
#include "hello.type.h"                                          raw (bloco)
                                                                 raw (R1)
#line 4 "hello.k"                                                k_emit_at(4, …): mapped era 0
int hello_main(int argc, char **argv);                           (mapped passa a 5)
                                                                 raw (R5)
#endif /* HELLO_H */                                             raw
```

## Passos sugeridos

1. Construa a AST do `hello.k` num programa de três linhas e imprima, por nó,
   o texto de `first..end`; faça o mesmo para o prelúdio. É daí que sai quem é
   quem, e onde cai a quebra.
2. A moldura dos headers e o `K_EMIT_ENTRY`; depois o `hello.type.h`/`.h`.
3. O `hello.c`.
4. O prelúdio: `import_c`, `typedef`, `static_assert`, `extern_c [type_h]`.
5. `K_EMIT_UNSUPPORTED`, `--line=off` e o buffer curto (já no teste).

## Aceitação

```sh
sh tools/cgen/test/unit/emit.sh            # -> ok
sh tools/agents/verify.sh                  # -> ALL OK (o build não pode ganhar aviso)
make -C tools/cgen check | grep unit/emit  # -> "ok" para emit, emit_mangle, emit_writer; nenhum wip
```

Os testes rodam com ASan e UBSan: nenhuma escrita fora do `out.len`.

## Relatório

`tools/agents/reports/T7-emit.md`, em pt-BR: o que o arquivo faz, o que você
mediu na AST (o `first..end` de cada tipo de nó e onde cai a quebra), a saída
das aceitações e toda dúvida sobre o contrato ou o oráculo.
