# Levantamento das formas que o emissor ainda recusa (2026-11-02)

Método: um `.k` mínimo por forma em [`tools/agents/survey/app/`](survey/app/), rodado por
`sh tools/agents/survey/run.sh` (cada linha: `OK`, `LIMIT` = `implementation-limit`, ou `ERR` = o
`.k` do levantamento está errado). Os 28 `.k` abaixo davam `LIMIT` nos dois perfis; cada pacote de
trabalho deve levar os seus a `OK`. Fora deste levantamento, o golden (34 casos) e a base (`base.sh`)
já passam.

| Pacote | `.k` | Forma | Origem da recusa |
| --- | --- | --- | --- |
| **WP1 declarações** | d01 | `pub struct Box { … };` (decl-struct, sem `typedef`) | `emit.c`, `supported()`: `K_AST_TYPE` exige `typedef` |
| | d02 | `pub union U { … };` | idem |
| | d03 | `typedef struct Node Node;` + `struct Node { … };` | idem |
| | d04 | `typedef struct { … } P, *PP;` (vários declaradores) | `emit.c`, "um declarador por typedef" |
| | d05 | `import_c "stdio.h";` (forma com aspas) | `emit.c`, só `<…>` |
| | d06 | `priv struct Local { … };` | idem d01 |
| | d07 | `pub int a = 1, b = 2;` (vários declaradores de variável) | `emit.c`, "um declarador" |
| | d08, d09 | `extern_c { … }` e `priv extern_c { … }` | só `[type_h]` é suportado; **a posição do conteúdo no `.h` não é especificada** (ver abaixo) |
| **WP2 arrays** | a01–a04 | `b->v[2]`, `b.v[2]`, `b->v[i]`, `b->v[1, 2]` (campo `array` de struct) | `subscript.c`: contêiner que não é nome simples |
| | a05 | `v[1..4]` sobre `array` (range-index) | idem |
| | a06–a08 | `keel.length/dim/capacity` sobre `array` local | `core.c`: só `length` sobre `array` de módulo com dimensões literais |
| | a09, a10 | `keel.length/dim` sobre parâmetro com binder (§5.18 r5) | idem |
| | a11 | `keel.dim` sobre `array` de módulo | idem |
| | a12 | `array i32 a[3], b[4];` | `array.c`: um declarador |
| **WP3 match** | m01 | operando `h->state` (campo `tagged` de struct) | `match.c`: só variável simples |
| | m02 | rótulo e instrução na mesma linha (`A: r = 1;`) | `match.c`: rótulo com a linha só para ele |
| | m03 | `break` de braço em braço que tem laço | `match.c`: `break` + laço |
| **WP4 parallel** | p01 | cabeçalho em duas linhas | `parallel.c` |
| | p02 | workers `0..W` com `W` `constexpr` | idem (constante por nome) |
| | p03 | `{` em linha própria | idem |
| **WP5 else** | e01 | `else` com a instrução na linha seguinte | `else.c` |
| **WP6 ferramenta** | — | `--instance` com mais erros de `invalid-instance`; critério de atualização da ferramenta §5 | `stop_gen.c`; `main.c` (`-f` lido e ignorado) |

**Pergunta ao André (extern_c).** O backend §4.3.2 põe `extern_c { … }` no `.h` e `priv extern_c { … }`
no `.c`, "o conteúdo, intacto", mas não diz **onde** no `.h`: antes dos protótipos, na ordem do
fonte entre eles, ou depois (com os corpos `static inline`). Os casos 022–024 (`linux-list`, WIP)
são o oráculo natural, e foram deixados para depois da v0. d08 e d09 ficam fora do WP1 até haver essa
regra.

**Pergunta ao André (tag do `typedef` anônimo com vários declaradores).** `typedef struct { … } P, *PP;`
recebe a tag do primeiro declarador (`struct app_P`), como o `typedef struct {…} cursor;` de 019
recebe `lst_cursor`. O backend não diz isso para vários declaradores.

## Plano de aplicação

Cada pacote: (1) o oráculo, um caso novo do golden escrito à mão por mim, nos dois perfis, com
`proof.c` que roda; (2) um cartão em `tools/agents/tasks/` para o Haiku; (3) a revisão do diff e
dos três comandos (`gen_golden`, `golden/run.sh`, `verify.sh`); (4) `sh tools/agents/survey/run.sh`
mostrando as formas do pacote em `OK`; (5) commit. Dois pacotes não rodam ao mesmo tempo (o
`verify.sh` faz `make clean`). Ordem, pelo que um programa real mais escreve:

1. **WP1** declarações (struct/union, typedefs, `import_c` com aspas, variáveis): caso 038.
2. **WP2** arrays e verbos `keel.*` (§5.3, §5.18): caso 039. O mais pesado: `subscript.c`,
   `core.c` e `array.c`.
3. **WP3** `match`: caso 040.
4. **WP4** `parallel` e **WP5** `else`: casos 041 e 042 (formas de escrita; mexem na leitura de
   linhas como o `defer` mexeu).
5. **WP6** ferramenta: testes em `test/cgen/cli/`, sem golden.

## Resultado (2026-11-02, noite)

As 28 formas do levantamento dão `OK` nos dois perfis (`sh tools/agents/survey/run.sh`). Pacotes e
commits: WP1 `001c36d` (T11), WP2 `512b7c1` (T12) e WP2b `4829f6b` (T17), WP3 `1e7164d` (T13),
WP4 `cd943d4` (T14), WP5 `ed6de62` (T15), WP1b `a34bb6b` (T16, `extern_c`), WP6 `760cf83`
(`source-not-found`, `-f`, recusa do `extern_c [type_h]` de uma linha). Casos novos do golden: 038 a 044.
Ficam recusados, de propósito ou por falta de oráculo: `pub extern_c { … }`, as formas abertas
`v[a..]` e `v[..]` sobre `array`, `0..W` com `W` importado, campo de `tagged` de outro módulo, e
`typedef` e variável com `array` de vários declaradores em campo de struct.
