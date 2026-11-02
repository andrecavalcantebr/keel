# T31 — relatório: keel.length de campo, dimensão por constexpr, keel.dim sem binder, range-index de campo

## O que mudou

**`tools/cgen/src/engine/emit/core.c`** (verbos do núcleo sobre `array`):

- `read_call` aceita o argumento como campo de struct: `b->v` e `b.v` (o recipiente é o
  nome `b`; o campo é lido do tipo dele por `k_em_array_of` → `find_decl` → `field_decl`).
  Vale para `keel.length` e `keel.dim`.
- `plan`, ramo de parâmetro sem binder: `keel.dim` deixou de ser recusado. Ele cai no ramo
  comum de `dim` (dimensão literal `k`, escrita como número inteiro). `keel.capacity` sobre
  parâmetro sem binder continua recusado.
- `plan`, ramo de array de módulo: cada dimensão passa por `k_em_array_dim` (aceita literal
  ou `constexpr` de literal), e não mais só por literal de token único. Antes, `keel.length(tab)`
  com `priv array i32 tab[K]` era recusado.
- Comentário de cabeçalho atualizado (formas novas, recusas restantes).

**`tools/cgen/src/engine/emit/subscript.c`** (`x[a..b]` sobre array):

- `read_sugar` aceita o campo de um array como recipiente: `b->v[1..4]` (e `b.v[...]`). O
  colchete é `lb = t + 3` nesse caso; `closing_of`, a busca do `..` e as flags `low`/`high`
  passam a partir de `lb`. A forma só vale quando `s->dim` (campo array): campo de módulo
  (`b->f[i]`) continua recusado (`if (field && !s->dim) return false;`).
- Comentário de cabeçalho atualizado.

**`test/cgen/gen/golden-cases.txt`**: apagado o `|wip` da linha `056-length-fields`. Nenhuma outra linha.

Nada mais foi tocado: nem normativos, nem `golden/**/expected/**`, nem `proof.c`, nem testes unit/cli.
Nenhum arquivo de teste ficou dentro do repositório. Os temporários de cgen foram na scratchpad.

## Saída dos comandos de aceitação

`sh test/cgen/gen_golden.sh`:

    63 ok, 0 fail, 0 wip

`golden/run.sh` (última linha):

    106 ok, 0 fail, 0 xfail, 0 xpass, 3 wip

`sh tools/agents/verify.sh`:

    ok   build (0 warnings, baseline 0)
    ok   make check (53 lines ok)
    ok   golden: 106 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
    ALL OK

Verificação extra: os seis arquivos de `golden/cases/056-length-fields/expected/{c23,c11}/app/`
são idênticos byte a byte aos gerados pelo cgen (`cmp`, com `--stop-after=gen`).

## Testes (unit/ e cli/)

Nenhum teste afirmou o contrário do que o cartão pede. `make check` passou (53 linhas ok), então
não houve patch guardado fora do repositório.

## Dúvidas e pontos abertos

1. `keel.capacity` sobre parâmetro sem binder continua recusado (`implementation-limit`). O
   cartão não pedia; ficou como estava e está dito no comentário de `core.c`.
2. O campo de módulo com forma `b->f[i]` (não array) segue recusado em `subscript.c`, de propósito.
3. Incidente de processo, não relacionado ao código: tentei um `cp` de backup para
   `$TMPDIR/core.c.orig` com `TMPDIR` vazio, o que apontava para `/core.c.orig`. A checagem de
   segurança bloqueou a tentativa de remoção. `ls /core.c.orig` confirma que o arquivo não existe
   (a cópia não chegou a ser criada), então não há nada a limpar. O backup correto ficou em
   `/tmp/claude-1000/-home-andre-code-cprojects-keel/6df5c4e9-8d62-4717-9b96-cb209ab7b812/scratchpad/core.c.orig`
   e `.../subscript.c.orig`.
4. Nenhum commit, nenhum `git add`. `git status`: `M` em três arquivos (os dois `.c` e
   `golden-cases.txt`), sem arquivos não rastreados.
