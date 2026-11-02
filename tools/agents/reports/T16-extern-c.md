# T16 — WP1b: `extern_c` plano e `priv extern_c`

## O que mudou

- `tools/cgen/src/engine/emit/emit.c`:
  - Três helpers novos, antes de `type_h_body`: `extern_c_type_h` (a forma `[type_h]`),
    `extern_c_priv` (o `priv`) e `extern_c_lines` (as linhas entre as chaves: da linha após a `{`
    até a linha da `}`, sem ela).
  - `type_h_body`: o `case K_AST_EXTERN_C` só emite a forma `[type_h]`; as outras duas saem.
  - `h_body`: o laço dos protótipos ganha um ramo para o `extern_c` plano. Ele entra no ponto do seu
    nó, na ordem do fonte, com `emit_range` (que põe o `#line` quando o contador diverge). Os corpos
    `pub inline` continuam depois de todos os protótipos.
  - `type_h_has_content` e `h_has_content` consideram o `extern_c` plano como conteúdo do `.h`
    (o `[type_h]` continua sendo conteúdo do `.type.h`).
  - `emit_c`: as linhas de um `extern_c` plano ficam em branco; as de um `priv extern_c` entre as
    chaves saem intactas (`emit_line`), e as linhas de `priv extern_c {` e `}` ficam em branco.
  - `supported()`: aceita `extern_c {` e `priv extern_c {`; `pub extern_c {` não é forma e continua
    recusada. O `[type_h]` tem as mesmas verificações de antes.
- `test/cgen/gen/golden-cases.txt`: apagado o `|wip` da linha `043-extern-c`. Nada mais.

Não houve commit, `git add`, edição de normativo, de `golden/**/expected/**`, de `proof.c` ou de teste
de unit. Diff completo salvo fora do repositório em
`/tmp/claude-1000/-home-andre-code-cprojects-keel/6df5c4e9-8d62-4717-9b96-cb209ab7b812/scratchpad/T16-extern-c.patch`.
`git status --short` mostra só os dois arquivos acima.

## Decisão além do cartão: `{` e `}` na mesma linha

Uma forma plana de uma linha só, `extern_c { static int f(void) { … } }`, perderia o texto em
silêncio: a faixa de linhas entre as chaves fica vazia. Por isso `supported()` recusa a forma plana e a
`priv` quando as duas chaves estão na mesma linha (`implementation-limit`). Verificado:
`extern_c { static int twice(int v) { return v * 2; } }` dá `implementation-limit`.

Não mexi no `[type_h]`, que tem o mesmo defeito (ver "Pontos abertos").

## Acceptance

Saída final dos quatro comandos:

```
$ sh test/cgen/gen_golden.sh
wip    044-array-range
51 ok, 0 fail, 1 wip

$ golden/run.sh
84 ok, 0 fail, 0 xfail, 0 xpass, 3 wip

$ sh tools/agents/verify.sh
ok   build (0 warnings, baseline 0)
ok   make check (51 lines ok)
ok   golden: 84 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK

$ sh tools/agents/survey/run.sh
a01 [c23 OK] [c11 OK]
a02 [c23 OK] [c11 OK]
a03 [c23 OK] [c11 OK]
a04 [c23 OK] [c11 OK]
a05 [c23 LIMIT] [c11 LIMIT]
a06 [c23 OK] [c11 OK]
a07 [c23 OK] [c11 OK]
a08 [c23 OK] [c11 OK]
a09 [c23 OK] [c11 OK]
a10 [c23 OK] [c11 OK]
a11 [c23 OK] [c11 OK]
a12 [c23 OK] [c11 OK]
d01 [c23 OK] [c11 OK]
d02 [c23 OK] [c11 OK]
d03 [c23 OK] [c11 OK]
d04 [c23 OK] [c11 OK]
d05 [c23 OK] [c11 OK]
d06 [c23 OK] [c11 OK]
d07 [c23 OK] [c11 OK]
d08 [c23 OK] [c11 OK]
d09 [c23 OK] [c11 OK]
e01 [c23 OK] [c11 OK]
m01 [c23 OK] [c11 OK]
m02 [c23 OK] [c11 OK]
m03 [c23 OK] [c11 OK]
p01 [c23 OK] [c11 OK]
p02 [c23 OK] [c11 OK]
p03 [c23 OK] [c11 OK]
```

Antes da mudança, `gen_golden.sh` dava `50 ok, 0 fail, 2 wip` (043 e 044), e `d08`/`d09` davam
`LIMIT` nos dois perfis. Agora `d08` e `d09` dão `OK`. O `a05` era `LIMIT` antes e continua, fora do
escopo.

O oráculo 043 foi comparado pelo `gen_golden.sh` byte a byte nos dois perfis (`.h`, `.c`,
`.type.h`), sem copiar saída para o esperado. `proof.c` não foi compilado por mim: ele é exercido pelo
`golden/run.sh`, que passou.

## Unit tests

`test/cgen/unit/parser/parser_extern_c.sh` testa só o parser (`k_scan_extern_c`) e não afirma recusa
de emissão. Nenhum teste de unit contradiz o cartão, então nenhum patch foi guardado.

## Pontos abertos

1. **Defeito pré-existente, não corrigido: `extern_c [type_h] { typedef int T; }` numa linha só perde o
   texto em silêncio.** O `.type.h` sai sem o `typedef` e o `cgen` retorna 0. Reproduzi com
   `--stop-after=gen` numa pasta de teste. A causa é a mesma faixa de linhas (`extern_c_lines`) do
   `type_h_body`, que já existia. Corrigir pede uma decisão: recusar a forma `[type_h]` de uma linha,
   como fiz para a plana, ou aceitá-la. Não mexi, porque está fora do cartão.
2. **`pub extern_c { … }`** continua recusada (`implementation-limit`). O cartão não cita essa forma;
   deixei como recusada.
3. **Comentário em `engine/emit/emit.h`, linha 5**: ele fala do `extern_c [type_h]` como única forma
   emitida. Ele ficou desatualizado. O arquivo não estava na lista de arquivos do cartão, então não
   foi editado.
