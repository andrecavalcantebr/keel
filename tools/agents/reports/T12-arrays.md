# T12 — relatório: arrays em campos de struct, verbos `keel.*`, declaradores múltiplos

Cartão: `tools/agents/tasks/T12-arrays.md`. Caso: `golden/cases/039-array-forms` (saiu de `wip`).

## O que mudou, arquivo por arquivo

- **`tools/cgen/src/engine/emit/index.c`** — o índice de `array`:
  - `read_decl` virou `read_decl_at(…, lbr, …)` mais `read_decl` (primeiro declarador). Um
    `array` local de função pode ter vários declaradores; módulo e campo de struct não (refusal
    mantida).
  - `declarator_named`: acha o declarador de um nome, percorrendo `, name[...]` só em locais.
  - Campo por ponteiro ou valor: `use_name` reconhece `b->v` / `b.v` (o island do índice é ancorado
    no contêiner `b`, não em `v`; ver `parser_islands_index.c:253`). `field_decl` resolve o campo
    pelo tipo declarado do contêiner (`container_type`, `module_type`).
  - Exporta `k_em_array_of` e `k_em_array_dim` para `core.c`.
- **`tools/cgen/src/engine/emit/core.c`** — reescrito para os três verbos (`keel.length`,
  `keel.capacity`, `keel.dim`) sobre `array` de módulo, local e parâmetro com binder. Os casos
  de binder saem como texto inteiro (`keel__n`, `(keel__n * 3)`, literal de `dim`); os de local e
  módulo continuam `sizeof(x)/sizeof(T)` (o `keel.length` de módulo mantém a regra antiga de
  dimensões literais).
- **`tools/cgen/src/engine/emit/array.c`** — (a) declaradores múltiplos de local: a palavra `array`
  sai uma vez e as vírgulas de cada declarador viram `][`; (b) a declaração é lida a partir do token
  (`array_decl(ast, arr, …)`), não só do island, porque campo de struct não tem island; (c) novo
  `k_em_edits_struct_arrays` e `k_em_supported_struct_arrays`: campos `array` de `typedef struct`
  (que antes ficavam `array i32 v[4];` no `.type.h`); (d) islands duplicados do mesmo `array`
  (um por declarador) são pulados.
- **`tools/cgen/src/engine/emit/emit.c`** — `node_edits` chama `k_em_edits_struct_arrays` para
  `K_AST_TYPE` que não é `extent` (com `k_em_sort_edits` do próprio nó), e `supported()` recusa a
  struct se algum campo `array` não for uma forma que o arquivo lowera.
- **`tools/cgen/src/engine/emit/emit_internal.h`** — declarações de `KArrayRef` (`K_ARRAY_RANK`),
  `k_em_array_of`, `k_em_array_dim`, `k_em_edits_struct_arrays`, `k_em_supported_struct_arrays`.
- **`test/cgen/gen/golden-cases.txt`** — só o `|wip` da linha `039-array-forms` foi apagado. Nenhum
  outro caso foi tocado. `subscript.c` não foi editado (não era necessário).

Nenhum commit, nenhum `git add`. Nenhum arquivo de `golden/**/expected/**`, nenhum `proof.c`, nenhum
normativo, nenhum teste de `test/cgen/unit/` foi editado.

## Critério de resolução do campo (card: "relate qual critério usou")

O campo `campo` de `X->campo[...]` / `X.campo[...]` é resolvido assim:

1. Acha-se a declaração de `X` na função (parâmetro ou local), a ocorrência mais próxima que tenha
   a forma `T X` ou `T *X` seguida de `,` `)` `=` ou `;`, com `T` nome de um `typedef` do módulo
   (K_AST_TYPE com nome). Se não houver, **recusa** (`implementation-limit`).
2. Varre-se o tipo `T` (todos os tokens do nó, `[first, end)`) à procura de declarações `array` cujo
   nome seja `campo`. Se houver mais de uma com esse nome, **recusa**. Se a achada tiver segundo
   declarador, ou for binder, **recusa**.

Não há adivinhação: o tipo do contêiner decide a struct, então a ambiguidade do card (duas structs
com mesmo campo) não chega a ser escolhida; o caso sem tipo conhecido é recusado.

## Saída final dos comandos de aceitação

`sh test/cgen/gen_golden.sh`:

```
wip    040-match-forms
wip    041-parallel-forms
wip    042-else-lines
47 ok, 0 fail, 3 wip
```

(039 saiu de `wip`; 040 a 042 continuam `wip`, como o cartão pede.)

`golden/run.sh` (última linha):

```
80 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
```

`sh tools/agents/verify.sh` (ALL OK, 0 warnings, baseline 0):

```
ok   build (0 warnings, baseline 0)
ok   make check (51 lines ok)
ok   golden: 80 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

`sh tools/agents/survey/run.sh`:

```
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
(d01–d07 OK; d08, d09, e01, m01–m03, p01–p03 continuam LIMIT, como antes)
```

A saída de `cgen` para `golden/cases/039-array-forms/app/ar.k` foi comparada byte a byte com
`expected/c23/app/app_ar.c` e `expected/c11/app/app_ar.c`: iguais.

## Dúvidas e incoerências

1. **Declaradores múltiplos só em local.** O cartão pede `array i32 a[3], b[4];` no corpo de função.
   `test/cgen/unit/emit/emit_main.c:93` afirma que `pub array i32 a[2], b[3];` *de módulo* é
   `K_EMIT_UNSUPPORTED`. Não é o contrário do que o cartão pede (que fala de local), então não parei,
   e **não editei o teste**. A recusa de módulo foi mantida; o teste passa. Se André quiser declaradores
   múltiplos também em variável de módulo, esse teste tem de mudar, e é decisão dele.
2. **O cartão diz que o campo de struct "já funciona".** Não funciona: o `.type.h` saía com
   `array i32 v[4];` dentro da struct (o caso 039 não passava por isso só porque era `wip`).
   Campo `array` de struct não tem island (o parser não gera). A solução foi a passagem por tokens
   descrita acima. Vale corrigir a afirmação do cartão nas próximas vezes.
3. **Recusas que o cartão não pede** (todas `implementation-limit`):
   - `keel.capacity` sobre parâmetro com binder (extensão de um binder não aparece no cartão);
   - `keel.length` / `keel.dim` sobre parâmetro sem binder (`i32 v[static 4]` não tem extensão no C;
     o parser já recusa `array-1d-as-parameter` no caso 1-D, mas `array i32 v[4][3]` passaria);
   - `keel.dim(x, k)` com `k` não literal, ou `k` ≥ número de dimensões;
   - campo `array` de struct com segundo declarador (`array i32 a[3], b[2];` em `typedef struct`).
4. **`keel.dim` com dimensão nomeada** (constexpr de módulo ou de bloco) sai com o valor da constante
   (`keel.dim(x, 0)` com `constexpr N = 5` vira `5`). `keel.length` de módulo mantém a regra antiga
   de dimensões literais; para local, `sizeof` não exige literal.
5. **Campo de tipo não-módulo** (`i32 *q; q->v[1]`) não gera island do parser, então passa sem
   tradução. É forma inválida em keel (o tipo não tem campo `v`); não tratei, mas vale o parser recusar.
6. **Snapshot de comportamento não comparado.** Tirei o snapshot do parser (`verify.sh snapshot`) no
   início, na pasta de scratch (não em `/tmp`), mas não rodei `compare`: a mudança é de emissão, não
   de refatoração, e o parser não mudou.
7. **Testes de sonda fora do repositório.** Os casos de borda (binder com `[size_t n, 3]`, campo
   em struct com declaradores múltiplos, `capacity` de binder, `dim` fora do intervalo, campo com
   `Box *q = 0;` local) foram rodados numa pasta de scratch da sessão, não estão no repositório.
