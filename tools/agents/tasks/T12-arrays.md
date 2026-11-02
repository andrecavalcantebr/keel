# T12 — WP2: arrays em campos de struct, verbos `keel.*`, declaradores múltiplos

## Objetivo

O emissor recusa (`implementation-limit`) formas válidas sobre `array`. Faça-o aceitar, como o
caso **`golden/cases/039-array-forms`** (escrito à mão: `app/ar.k`, `expected/c23/app/` e
`expected/c11/app/`, `proof.c`) mostra — leia o `ar.k` e o `expected/c23/app/app_ar.c` inteiros antes
de tudo, a saída exata está lá:

1. **Índice de campo `array` de struct**, por ponteiro ou por valor: `b->v[2]`, `b.v[2]`,
   `b->v[i]`, `gr->m[i, j]`, `gr->m[1, 2]`. Regras do backend §5.3 (3, 4 e 6): vírgula entre
   índices vira `][`; cada índice passa por `keel_index(i, d)` contra a dimensão **declarada** do campo;
   índice e dimensão decimais conhecidos dão o colchete simples (`b->v[2]`, `gr->m[1][2]`).
2. **`keel.length`, `keel.capacity` e `keel.dim`** sobre `array` local, de módulo e parâmetro com binder
   (backend §5.3 r7 e §5.18 r5):
   - `array` local ou de módulo: `length` e `capacity` saem `sizeof(x)/sizeof(<elemento>)` (o elemento vem
     da declaração); `dim(x, k)` sai o literal da dimensão `k` (`3`, `4`);
   - parâmetro com binder: `length(v)` sai `keel__n` (vezes as outras dimensões, se houver) e
     `dim(v, 0)` sai `keel__n`.
3. **Vários declaradores** em `array i32 a[3], b[4];` → `i32 a[3], b[4];` (o `array` some uma vez).

A forma `v[1..4]` sobre `array` (range-index → `keel_array_i32_as_slice2(v, 6, 1, 4)`) **não** está neste
cartão: ficou para um caso à parte.

## Arquivos

Pode editar, em `tools/cgen/src/engine/emit/`: `subscript.c`, `index.c`, `core.c`, `array.c`, `emit.c` e
`emit_internal.h`. Pode apagar o `|wip` da linha `039-array-forms` de `test/cgen/gen/golden-cases.txt`
quando fechar. Pode criar `tools/agents/reports/T12-arrays.md`.

Não toque: normativos, `golden/**/expected/**`, nenhum `proof.c`, nenhum outro caso. Não faça commit.

## Contrato

Fonte (`ar.k`) e saída por forma — todas já escritas no esperado:

| Fonte | C |
| --- | --- |
| `typedef struct { array i32 v[4]; } Box;` | `typedef struct app_ar_Box { i32 v[4]; } app_ar_Box;` (já funciona) |
| `b->v[2]`, `b.v[2]` | `b->v[2]`, `b.v[2]` |
| `b->v[i] = 1` | `b->v[keel_index(i, 4)] = 1` |
| `gr->m[i, j]` (campo `array i32 m[2, 3]`) | `gr->m[keel_index(i, 2)][keel_index(j, 3)]` |
| `gr->m[1, 2]` | `gr->m[1][2]` |
| `keel.length(x) + keel.dim(x, 1) + keel.capacity(x)` (`array i32 x[2, 3]`) | `sizeof(x)/sizeof(i32) + 3 + sizeof(x)/sizeof(i32)` |
| `keel.dim(g, 0)` (`priv array i32 g[4]` de módulo) | `4` |
| `keel.length(v) + keel.dim(v, 0)` (`array i32 v[size_t n]`) | `keel__n + keel__n` |
| `array i32 a[3], b[4];` | `i32 a[3], b[4];` |

Como achar a dimensão do campo: o índice `X->campo[…]` / `X.campo[…]` precisa da declaração do campo
`campo` dentro do `struct` do tipo de `X`. Resolva pelo tipo declarado de `X` (parâmetro ou local de um
tipo do módulo) quando for possível; se houver mais de uma struct do módulo com um campo `array` de
mesmo nome e dimensões diferentes e você não souber qual, **recuse** (`implementation-limit`) em vez de
adivinhar. Relate qual critério usou.

Código existente (leia nos arquivos; as linhas mudam):

- `index.c`: o comentário do alto descreve `Decl` (a declaração de um `array` de nome simples) e as
  recusas; `k_em_handles_index`.
- `subscript.c`: "Refused: a container that is not a plain name (`s.f[i]`), a comma between indices…".
- `core.c`: `is_core_verb`, `k_em_handles_core`, e o comentário do alto com a única forma suportada.
- `array.c`: `k_em_handles_array`, `ArrayDecl`, a recusa de "mais de um declarador".

## Passos sugeridos

1. `sh test/cgen/gen_golden.sh`: veja o `wip 039-array-forms`. Gere a saída do cgen numa pasta temporária
   e compare com o esperado — a diferença é o seu trabalho. **Não copie a saída para o esperado.**
2. Uma forma por vez, nesta ordem: declaradores múltiplos; `keel.length/dim/capacity`; índice de campo.
   A cada passo: `make -C tools/cgen -j8` e `sh test/cgen/gen_golden.sh` inteiro.
3. Rode `sh tools/agents/survey/run.sh`: a01–a04 e a06–a12 devem ir a `OK` (a05 fica `LIMIT`).

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # 039 sai de wip; os outros wip (040-042) continuam wip; 0 fail
golden/run.sh                   # "80 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
sh tools/agents/survey/run.sh   # a01-a04, a06-a12 com OK nos dois perfis
```

Se algum caso que passava deixar de passar, o defeito é seu; não altere oráculo nem baseline. Se o
esperado parecer errado, pare e diga no relatório, com a evidência.

## Relatório

`tools/agents/reports/T12-arrays.md`, em pt-BR: o que mudou, arquivo por arquivo; a saída final exata dos
quatro comandos; o critério de resolução do campo; toda dúvida ou incoerência.
