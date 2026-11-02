# T15 — WP5: cláusula `else` com a instrução na linha seguinte

## O que mudou

- `tools/cgen/src/engine/emit/else.c` (único arquivo de código editado; `emit.c` não foi tocado):
  - **Causa do erro.** O edit do cabeçalho ia de `fim do token antes de else` até o início do token seguinte
    (`return`), que estava na linha seguinte. Esse edit atravessava a quebra de linha, `emit_range` tomava a
    linha seguinte como já escrita (`spanned_end`) e a apagava, e o `#line` aparecia no lugar dela. O resultado
    era `return keel_outcome_i32_value(v);` no lugar de `return -1;`.
  - **Correção.** Quando o token seguinte ao `else` não está na linha do `else` (`to >= b`), o edit para no fim
    dessa linha (sem o `\n`). O cabeçalho sai como `; if (F(x))`, sem espaço depois, e a quebra de linha e a
    linha seguinte saem como linha própria. Quando o token seguinte está na mesma linha, o texto continua
    `; if (F(x)) ` com o espaço, como antes.
  - **Recusa nova (não estava no cartão).** Um `else` com default (`x = e else v;`) cujo `;` cai em outra linha
    é recusado com `implementation-limit` (`k_em_handles_else` responde falso; `same_line` novo). Antes, esse
    caso saía como C quebrado: o `)` que fecha `W(&x, ` era inserido na linha do `;`, que não era visitada pelo
    `else`. Testado com um `.k` avulso (fora do repositório): `cgen: error: ... [implementation-limit]`, rc=2.
    É a única mudança de comportamento além do caso pedido; se não a quiser, diga e eu retiro.
  - Comentário do alto do arquivo atualizado com a forma da quebra de linha e com a nova recusa.
- `test/cgen/gen/golden-cases.txt`: apagado só o `|wip` da linha `042-else-lines`.

Os arquivos de `golden/**/expected/**`, os `proof.c`, os normativos e os outros casos não foram tocados.
Nada foi commitado nem adicionado ao índice.

Verificação da saída do cgen: `cgen --stop-after=gen` do `app/es.k` nos dois perfis produz `app_es.c`,
`app_es.h` e `app_es.type.h` idênticos aos esperados (o `keel/` que o cgen escreve a mais não é julgado pelo
oráculo).

## Saída final dos comandos de aceitação

`sh test/cgen/gen_golden.sh`:
```
wip    043-extern-c
wip    044-array-range
50 ok, 0 fail, 2 wip
```
042 saiu de `wip`: 0 fail. Os dois `wip` são de outros casos (ver abaixo), não do 042.

`golden/run.sh` (última linha):
```
84 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
```

`sh tools/agents/verify.sh`:
```
ok   build (0 warnings, baseline 0)
ok   make check (51 lines ok)
ok   golden: 84 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
rc=0
```

`sh tools/agents/survey/run.sh` (e01):
```
e01 [c23 OK] [c11 OK]
```

## Incoerências e dúvidas

1. **O número do cartão está desatualizado.** O cartão espera `80 ok, 0 fail, 0 xfail, 0 xpass, 3 wip` e
   `0 wip` no `gen_golden.sh`. Durante o trabalho, o HEAD andou: `cbf8cff` (backend §4.3.2, extern_c) e
   `d49eab6` (oráculos 043 e 044, ambos `wip`, e os cartões T16 e T17). Os 4 casos a mais dão os 84 ok atuais.
   Os 2 `wip` do `gen_golden.sh` são 043 e 044, não são deste cartão. A linha dos 043 e 044 em
   `golden-cases.txt` veio do commit `d49eab6`, eu não a alterei.
2. **Testes de unit.** Nenhum teste em `test/cgen/unit/` afirma o contrário do pedido. `make check` passou
   (51 linhas ok). Nada foi preciso guardar fora do repositório.
3. **Não testado (só lido no código):** (a) um `else` cujo token anterior (`at - 1`) está numa linha anterior
   à do `else`: `from` fica antes de `a`, `push_in_line` descarta o cabeçalho e o C sai quebrado; a forma do
   cartão não cai nesse caso, e eu não mexi nele. (b) Um comentário depois do `else` com a cauda na linha
   seguinte: o `drop_nested` deve descartá-lo junto com o edit, mas não rodei isso.
4. **Mesmo diretório.** Por engano, escrevi uma primeira saída de geração dentro do repositório
   (`tmp-unused/`); apaguei antes de qualquer commit. `git status` agora mostra só
   `test/cgen/gen/golden-cases.txt` e `tools/cgen/src/engine/emit/else.c` modificados, mais este relatório.

Saídas de geração de teste ficaram em `/tmp/claude-1000/-home-andre-code-cprojects-keel/6df5c4e9-8d62-4717-9b96-cb209ab7b812/scratchpad/t15/`.
