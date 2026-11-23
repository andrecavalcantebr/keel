# E01 — o nome gerado tem três limites e três diagnósticos

Estudo aberto em 2026-11-23, cgen 0.4.0-dev. Origem: auditoria de limites
(`tools/agents/reports/limites-auditoria.md`, dúvida 3), reproduzida à mão.

## O que a spec promete

Backend §2.4, regra 1: *nome gerado acima de 255 caracteres é erro:
`name-too-long`*. É o único teto, e o texto do §2.4 diz que ele não é sentido na
prática.

## O que acontece

Um modificador instanciado vira um **arquivo** (`<instância>.type.h`, `.h`) e um
**símbolo**. Três limites incidem sobre o mesmo nome, e o que dispara primeiro
depende do tamanho:

| Ordem | Limite | Onde | Diagnóstico |
| --- | --- | --- | --- |
| 1º | arquivo temporário `<arquivo>.tmp.<pid>` passa de 255 bytes (o `NAME_MAX` do sistema de arquivos) | `tool/output.c:44` | `write-failure` ("cannot write '…'"), sem dizer que é o nome |
| 2º | caminho do arquivo gerado passa de `NAME_CAP` = 256 (inclui a pasta, `app/`) | `engine/emit/instance.c:552-556`, `emit_internal.h:13` | `implementation-limit` ("a generated name or path needs more than NAME_CAP = 256 bytes") |
| 3º | símbolo da instância passa de 255 | `engine/instances.c:98` | `name-too-long` |

Medido com `app.pbox` (`module app.pbox dim N type T; pub modifier box byref { T v[N]; }`)
e `box(3) T…` em `app.use`, com o tipo `T` de L caracteres (`/tmp/sweep`):

| L | resultado |
| --- | --- |
| até 214 | ok |
| 216 a 220 | `write-failure`: arquivo de 246 bytes, mais `.tmp.<pid>` |
| 224 a 232 | `implementation-limit` (`NAME_CAP`) |
| 233 em diante | `name-too-long` |

O comportamento só coincide com a spec no terceiro degrau. Um programa com
nome legal pela spec (até 255) falha entre 216 e 232 com um diagnóstico que não
diz "o nome é longo": o `write-failure` parece problema de disco.

## Análise

1. O limite real não é 255 do símbolo. É o menor entre o símbolo (255), o nome do
   arquivo com sufixo (`.type.h` = 7 → 248) e o caminho (256 com a pasta).
2. O `.tmp.<pid>` é do cgen, não do usuário: gasta de 6 a 12 bytes do `NAME_MAX`
   à toa.
3. Nome de arquivo acima de 255 bytes não é escrevível em ext4, NTFS e APFS, então
   o teto de 255 do §2.4, por si, não garante um arquivo.

## Caminhos (nenhum é decidido)

- **A. Uma só regra, um só diagnóstico (recomendada).**
  - O temporário passa a um nome curto e fixo na mesma pasta
    (`.keel-<pid>.tmp`), de modo que não gasta o `NAME_MAX` do arquivo.
  - O `NAME_CAP` sobe (por exemplo, 512), para o caminho nunca disparar antes.
  - O `name-too-long` passa a medir o **arquivo**: instância de mais de 248
    caracteres (255 menos `.type.h`). Texto do §2.4, regra 1: "nome gerado, ou
    arquivo gerado, acima de 255 caracteres". Muda um normativo: precisa de
    "sim" e da marca *(desde 0.4.0)*.
- **B. Só consertar a mensagem.** O `write-failure` e o `implementation-limit`
  dizem "o nome gerado é longo demais" e apontam a instância. Não muda a spec, e
  o teto efetivo continua sendo uma consequência do sistema de arquivos.
- **C. Nome curto por hash.** Recusado em §2.4, regra 3 (sem truncamento nem
  hash); só reabriria com caso real.

## Oráculo, antes de qualquer conserto

Teste em `test/cgen/cli/` com dois programas: instância de 248 caracteres
(passa) e de 249 (`name-too-long`, na opção A), nos dois perfis; mais um com
nome de pasta longo, para o `NAME_CAP`. Escrito à mão.
