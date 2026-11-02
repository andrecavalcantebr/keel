# T24 — terceiro levantamento: relatório (auditoria)

Auditoria: o emissor não foi tocado, nem o parser, os normativos, o golden ou
os testes. Nenhum `make` nem `verify.sh` foi rodado; o binário usado é a cópia
da scratchpad (`cgen-t24`, de 14:07). Nada foi commitado.

## O que foi feito

- `tools/agents/survey/app3/*.k` (75 arquivos: 73 válidos e 2 negativos,
  `ng01.k` e `ng02.k`) e `tools/agents/survey/app3/lib/` (`other.k`, `box.k`,
  `grid.k`, módulos genéricos e de tags, importados pelos casos).
- `tools/agents/survey/run3.sh`: varre `app3/` nos dois perfis. Por perfil:
  `OK`, `WARN` (só warnings), `LIMIT` (implementation-limit), `ERR` (outro
  erro: o `.k` pode estar errado) e `BAD` (o cgen aceitou, e o `cc
  -fsyntax-only -Wall` do C gerado falha). `c23` usa `-std=c2x`; `c11`, `-std=c11`.
- Resultado nos dois perfis, idêntico em todas as 75 linhas:

| Classe | Contagem (c23 = c11) |
| --- | --- |
| OK (aceito, C compila) | 53 |
| WARN (aceito com warning, C compila) | 1 (`df02`) |
| LIMIT | 14 |
| BAD (aceito, C não compila) | 7: 5 `.k` válidos, mais os 2 negativos |
| ERR inesperado após correção | 0 |

Durante a varredura, 6 `.k` meus estavam errados (`a03`, `a08`, `a09`, `gn01`,
`mt04`, e `oc03`, que usava `int` onde a spec pede tipo keel). Foram corrigidos e
não entram nas tabelas, exceto como nota na seção 3.

## 1. LIMIT (recusa com implementation-limit)

Todos os `.k` abaixo são válidos (salvo onde dito). Os minimizados estão nos
arquivos indicados. "Documentada" quer dizer que o cabeçalho do emissor já
lista a recusa.

| `.k` | Forma (uma frase) | Mínimo que reproduz | Onde recusa | Observação |
| --- | --- | --- | --- | --- |
| `df06`, `w7` | `return` com índice num escopo com `defer` | `int f(void){ int v[3]={0}; defer v[1]=0; return v[0]; }` | `defer.c`, `plan_of`, linhas 457-460: `return` com cleanup inline não aceita `[` | Não documentada no cabeçalho de `defer.c`. Afeta qualquer `return` com índice sob `defer` |
| `a11` | `keel.length` sobre campo de struct | `typedef struct { array i32 v[5]; } Box; size_t f(Box *b){ return keel.length(b->v); }` (também `b.v` por valor) | `core.c`, `k_em_handles_core` (223) → `plan()` | Não documentada: o cabeçalho lista só arrays de módulo e parâmetros sem binder |
| `ce01`, `a07` | `keel.length` sobre array de módulo com dimensão não literal | `constexpr size_t K = 3; pub array i32 v[K];` + `keel.length(v)`; ou `pub array char name[] = "keel";` | `core.c`, cabeçalho linhas 36-39 | Documentada (recusa intencional) |
| `a03` | `keel.dim` sobre parâmetro sem binder | `pub size_t f(array i32 m[2, 4]) { return keel.dim(m, 1); }` | `core.c`, cabeçalho linhas 36-38 | Documentada |
| `a08` | range-index sobre campo de struct | `slice i32 s = v->v[1..4];` com `v` do tipo `Vec` (campo `array i32 v[6]`) | `subscript.c`, `k_em_handles_subscript` (201), ramo do range-index (166-170) | Não documentada. Sobre array de parâmetro com binder (`v[..]`, caso `i8`) o caso passa |
| `mt02`, `mA` | braço de `match` com bloco em várias linhas, seguido de outro rótulo | `A: {` / `r = 1;` / `}` / `B: r = 2;` | `match.c`, `k_em_handles_match` (485) | Com o bloco em uma linha (`A: { r = 1; }`), o caso passa |
| `cb03`, `dC` | `defer` com `break` dentro do corpo de `foreach` | `foreach (i32 x, size_t i : s) { defer t++; if (x == 0) break; }` | não localizado (suspeita: `foreach.c`, `k_em_handles_foreach` (637), ou `defer.c`, `k_em_handles_defer` (868)) | `defer` sem `break` (`dD`) e `break` sem `defer` (`dE`) passam; `defer` num `for` com `break` (`dA`) também passa |
| `el03`, `eC`, `eD` | `else` com default na linha seguinte | `outcome i32 v = read(n) else` / `-1;` (`eD`) | `else.c`, `k_em_handles_else` (98) | Não isolado. `else` em uma linha, encadeado (`eA`), passa; `else` com `slice.from(...)` na linha seguinte (golden 050) passa; o caso de `-1` não |
| `gn01`, `gn02`, `gn03`, `gn04`, `cb04` | `instance` de módulo genérico com `modifier` em várias linhas | `module app3.lib.bx6 type T;` / `pub modifier stack byref {` / `size_t cap, len;` / `T *ptr;` / `}`; e `instance m.stack i32;` em outro módulo | mensagem `... does not cover this instance yet`, de `stop_gen`/instância; não localizei a função | Mesmo modifier em uma linha (`bx1`, `bx2`) passa; o golden 051 tem modifier em várias linhas, mas com funções, e passa. Não isolado além disso |
| `i5` (extra, fora do `app3`) | `match` sobre campo de tipo qualificado | `pub int f(other.Gate *g) { match (g->ph) {...} }` | `match.c`, `k_em_handles_match` (485) | Com `Gate *g` (tipo injetado por `types`) passa (`k1`) |

Nota: `df06` (`return p->v[i];`) e `w7` são o mesmo caso.

## 2. Tradução errada (aceita, o C não compila)

Aqui o `.k` é válido (exceto onde dito), o cgen não diagnostica e o `cc` falha.
São os achados mais importantes.

| `.k` | `.k` (trecho) | Trecho gerado | Erro do `cc` |
| --- | --- | --- | --- |
| `ce04` (e `ce2` nos testes) | `pub constexpr size_t A = 2;` / `pub constexpr size_t B = A * A;` | `constexpr size_t app3_ce04_B = A * A;` (o `A` não recebe o prefixo do módulo) | `‘A’ undeclared here (not in a function)`; no perfil c11 sai o mesmo problema (`#define` com `A`) |
| `fe05` (mínimo `feB`) | `foreach (i32 x, size_t i : s)` com o `{` na linha seguinte | `for (...)` com corpo `{ t += x; }`, sem `i32 x = keel_slice_i32_get(...)` (o binder some) | `‘x’ undeclared`. Com o `{` na mesma linha (`feA`), o binder sai |
| `wk01` (mínimos `wkA`, `wkB`) | `walk (i32 *p, app3.wk.cursor c : l)`, ou `wk.cursor` | `app3.wk.cursor c = app3_wk_begin(...)` (qualificador não renomeado) | `‘app3’ undeclared` / `‘wk’ undeclared`. Com `cursor` sem qualificador (`wkC`) passa. O golden 019 usa `lst.cursor` com módulo de um segmento |
| `a09` | `pub size_t f(array i32 v[size_t n]) { slice i32 s = v[..]; ... }` com `import keel.array as array;` | `#include "keel/keel_array_i32.h"`, que o cgen não gera no `--dest-dir` | `keel/keel_array_i32.h: No such file`. Só `keel_array.h` aparece |
| `mt04` | `pub int f(other.Phase p) { match (p) { ON: ...; OFF: ...; } }`, com `other` = `app3/lib/other.k` (`pub tags Phase [ON, OFF];`) | o header de `other` (`app3/lib/app3_lib_other.type.h`) inclui `keel/keel_tagged_app3_lib_other_Phase_void.type.h`, que não é gerado | `keel/keel_tagged_app3_lib_other_Phase_void.type.h: No such file`. Pelo `.k` do lib (`pub typedef struct { tagged Phase void ph; } Gate;`) |

Observação sobre `ce04` e `ce2`: `D = (B + 1)` com `B` constante nomeada
também sai `B` sem prefixo (`ce2`). `constexpr` sem referência a outra constante
(`ce05`, `ce02`) passa.

## 3. ERR suspeito de defeito do cgen (aceito indevidamente, ou diagnóstico enganoso)

| Caso | `.k` | Situação |
| --- | --- | --- |
| `ng01` (negativo) | `array routine.slot int steps[2] = ...;` (o `int` é palavra reservada de C; a spec exige tipo keel como argumento) | aceito sem diagnóstico; o `array` sai sem tradução e o `cc` falha (`unknown type name ‘array’`). Compare com `dB`: `slice int` recebe `c-type-as-…` (ver a mensagem na saída do `run3`). O mesmo diagnóstico faltou em `routine.slot` |
| `ng02` (negativo) | `outcome u32 r = ...;` sem `import keel.outcome` | aceito sem diagnóstico; o `cc` falha (`unknown type name ‘outcome’`) |
| `mt04` (original, com `other.ON:` e `other.OFF:`) | rótulos de `match` qualificados | o cgen diz `ON of Phase has no arm in this match [tag-without-label]`, mas o braço existe (com o nome qualificado). A spec (§4.9) diz que dentro de `match` os rótulos são escritos sem qualificação. Diagnóstico enganoso, não bug de tradução |
| `df02` | `defer` dentro de `if` | `WARN` (`defer-in-control-block`), correto: não é erro |
| `gn04` | acesso a `s.cap`, `s.len`, `s.ptr` de instância | `WARN` (`instance-field-access`), correto (e o uso era meu) |

## 4. Não coberto, e por quê

- Molde (§4.13, v1): fora do escopo deste levantamento (a spec o marca v1).
- Protocolo definido pelo usuário (§5.1): não escrevi `protocol`.
- `parallel` com mais de um binder, e `extent` com três grupos: não cobertos.
- `alignas`, `ref` sobre parâmetro, e `tags` com valores explícitos: só o que
  o golden já cobre, sem variação.
- `corot` com `routine.seq`/`routine.par` sobre tipo que não seja `i32`:
  coberto só com `i32`.
- Instâncias de genéricos com `dim` (`gn03`, `grid.cells(4) f32`): recusadas, mas
  não isoladas.
- Os módulos de biblioteca (`lib/box.k`, `lib/grid.k`) não são compilados sozinhos
  (o cgen exige `--instance`, `LIB`); são exercitados pelos `gn*`/`cb04`.
- Não rodei o C gerado com `-O2` nem executei; só `-fsyntax-only -Wall`.
- Não variei a ordem de declaração (uso antes da declaração) nem nomes que
  colidem com o prefixo do módulo.

## Ferramentas e onde estão

- Binário usado: `/tmp/claude-1000/-home-andre-code-cprojects-keel/6df5c4e9-8d62-4717-9b96-cb209ab7b812/scratchpad/cgen-t24`.
- Saída do levantamento: `/tmp/claude-1000/-home-andre-code-cprojects-keel/6df5c4e9-8d62-4717-9b96-cb209ab7b812/scratchpad/run3-3.txt`.
- Mínimos e variantes: pasta `/tmp/claude-1000/-home-andre-code-cprojects-keel/6df5c4e9-8d62-4717-9b96-cb209ab7b812/scratchpad/m` e `.../scratchpad/m2`.
