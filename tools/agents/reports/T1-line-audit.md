# T1 — auditoria do `#line` no golden

Auditoria somente leitura. Foram criados `golden/line-check.py` e este relatório.
Nada mais foi editado, e não rodei `make`, `verify.sh` nem `golden/run.sh`. As
provas com `gcc -E` usaram cópias em `/tmp/t1/` (fora do repositório), e os
scripts de apoio também estão lá.

## 1. A ferramenta

`python3 golden/line-check.py [--verbose]`, a partir da raiz. Só biblioteca padrão.

Cobre os 310 arquivos: os `.c`/`.h` de `golden/cases/*/expected/{c11,c23}/`
(155 por perfil, recursivo) e os de `golden/c11/` e `golden/c23/` (base e
prelúdio, também 155). O `.k` de cada `#line` é resolvido no diretório do caso
e, se não existir ali, em `base/`.

O que ela faz:

- simula o contador do compilador: a linha seguinte a `#line N` é N, e cada linha
  física depois soma 1 (diretivas, comentários e linhas em branco contam);
- classifica cada `#line` como `REQUIRED first|file|jump` ou `REDUNDANT`
  (a definição do card: mesmo `.k` e o contador já valeria N sem a diretiva);
- marca `MISSING` as linhas de código mapeadas cujo texto não casa com a linha
  do `.k` (regra de `golden/line-audit.py`: identificadores, com o prefixo
  `keel__` e `KEEL_` fora do lado C; `#include <x.h>` casa com `import_c <x.h>;`
  ou com `#include <x.h>` bruto no `.k`). Comentários de várias linhas são
  tratados nos dois lados;
- contabiliza `sem-id` (linha C sem identificador, como `}`, ou linha do `.k`
  sem identificador): não verificável, não entra em MISSING.

Limitações, que valem para ler os números:

- **A checagem por nome é fraca, e por isso é um limite inferior.** Um casamento
  por sufixo passa sem olhar a posição: `keel_arena_from_array` casa com a linha
  `*a = (arena){0};` porque `keel_arena` termina em `_arena`. O `golden/c*/keel_arena.h`
  está deslocado em +1 (seção 5), e a ferramenta só o marca nas linhas de corpo.
- Linhas antes do primeiro `#line` não são mapeadas, como manda o card. Por isso
  os `#include` do topo de `keel.type.h` não são checados (ver seção 5).
- Os `REDUNDANT` e `REQUIRED` não dependem da checagem por nome: vêm só da
  aritmética do contador.

## 2. Totais

Saída de `python3 golden/line-check.py` (sem `--verbose`):

```
== totais por perfil ==
c11: 155 arquivos; REQUIRED first=128 file=0 jump=314; REDUNDANT=352; MISSING=72; sem-id (nao verificavel)=191
c23: 155 arquivos; REQUIRED first=128 file=0 jump=302; REDUNDANT=351; MISSING=64; sem-id (nao verificavel)=190
total: 310 arquivos, 1575 #line, 703 REDUNDANT, 136 MISSING, 0 sem .k
```

Por perfil: 128 arquivos com `#line` (27 sem nenhum). Dos 1575 `#line`, 703
(44,6%) são `REDUNDANT`. Os `REQUIRED` são `first` (128), `jump` (616) e nenhum
`file`: nenhum arquivo muda de `.k` sem `#line` antes.

## 3. Prova empírica

Para cada arquivo: tirei as diretivas `REDUNDANT` numa cópia em `/tmp`, rodei
`gcc -E -x c -std=c2x` (ou `c11`) com `-I` no perfil e no caso, e comparei, para
cada linha de código do arquivo original, o par (arquivo, linha) que os
linemarkers atribuem. A correspondência entre a linha do fonte e a saída é feita
por `difflib` sobre o texto com espaço normalizado; um par conta só quando as
duas versões o atribuem.

Comando: `python3 -I /tmp/t1/proof.py` (script em `/tmp/t1/proof.py`).

```
golden/cases/019-walk/expected/c23/lst.type.h: REDUNDANT removidas=2; linhas de codigo=3; casadas no original=3; na copia=3; em comum=3; pares diferentes=0
golden/cases/020-extent/expected/c23/app/app_pos.h: REDUNDANT removidas=2; linhas de codigo=36; casadas no original=28; na copia=28; em comum=28; pares diferentes=0
golden/c23/keel.type.h: REDUNDANT removidas=4; linhas de codigo=13; casadas no original=10; na copia=10; em comum=10; pares diferentes=0
golden/cases/019-walk/expected/c11/lst.type.h: REDUNDANT removidas=2; linhas de codigo=3; casadas no original=3; na copia=3; em comum=3; pares diferentes=0
```

Os três arquivos pedidos (um `.type.h` de caso, um `.h` de caso e o
`keel.type.h` do prelúdio) mais um `.type.h` em c11 têm zero pares diferentes.
A primeira versão do script comparava texto cru e caminho absoluto; o resultado
não mudou em nenhum dos pares, só a cobertura (o espaço era normalizado de
forma diferente pelo `gcc -E`). A cobertura não é total (28 de 36, 10 de 13)
porque as linhas com macro expandida (`KEEL_CHECK`) não batem com o texto.

**Controle negativo.** `python3 -I /tmp/t1/control.py` remove do `keel.type.h`
uma diretiva `REQUIRED jump` (linha 23, `#line 42`) e compara:

```
em comum 10 diferentes 3
   35 ('keel.k', 53) ('keel.k', 40) static inline size_t keel_index(size_t i, size_t d
   37 ('keel.k', 55) ('keel.k', 42) return i;
   38 ('keel.k', 56) ('keel.k', 43) }
```

Remover um `REQUIRED` muda os pares; remover um `REDUNDANT` não muda nada. Logo a
classificação é a que o `gcc -E` confirma.

## 4. Mapa de divergências por categoria

### 4.1 `REQUIRED jump` (c23: 302; c11: 314)

Medido pelo que fica entre o último `.k` mapeado e o `N` do salto. Contagem c23
(c11 entre parênteses). `/tmp/t1/jumps.py` e `/tmp/t1/gaps.py` fazem a medição.

| categoria | c23 (c11) | exemplos |
|---|---|---|
| avanço: só linhas em branco e comentários do `.k` suprimidas | 138 (138) | `cases/005-defer-loop/expected/c23/app/app_loop.h:12` (M=12, N=16); `cases/007-instance/expected/c23/coll_stack_i32.h:8` (M=11, N=13) |
| avanço com código do `.k` que não sai neste arquivo: corpo que vai para o `.c`, declaração que vai para o `.type.h` | 115 (115) | `cases/001-arena-buffer-defer/expected/c23/app/app_cfg.h:10` (N=30, M=12: `if (!fp)`, `return outcome.win(...)` vão para `app_cfg.c`); `cases/020-extent/expected/c23/app/app_pos.h:48` (M=25: `pub extent struct image`, que está no `.type.h`) |
| recuo: a saída põe o trecho do `.k` fora de ordem (instância de genérico, lowering de `parallel`/`match`, `typedef` depois de `constexpr`) | 49 (61) | `cases/006-dim/expected/c23/grids_grid_3_f32.h:11` (N=5, M=12: `length` sai depois do bloco de `grid`); `cases/004-parallel/expected/c23/app/app_val.c:24` (N=12, M=18: o corpo sai depois do gerente); `cases/008-block-constexpr/expected/c23/app/app_cx.type.h:11` (N=4, M=8: `typedef struct` depois do `constexpr`) |

Em c23, 170 dos 302 saltos estão no prelúdio/base (`golden/c23/keel/*`, `golden/c11/keel/*`),
o resto nos casos. A distribuição é a mesma nos dois perfis.

### 4.2 `MISSING` (c23: 64; c11: 72)

| categoria | c23 (c11) | exemplos | veredito |
|---|---|---|---|
| acessores de `extent` (`app_pos_*_ptr`, corpo com `KEEL_CHECK`) | 18 (18) | `cases/020-extent/expected/c23/app/app_pos.h:8` (`array f32 *x;`), `:34` | esperado: o acessor não tem linha própria no `.k` (já anotado em `line-audit.py`) |
| `keel_arena.h` em +1 em relação a `base/keel/arena.k` (seção 5) | 14 (14) | `golden/c23/keel/keel_arena.h:27` (`#line 19` antes do cabeçalho de `from_array`, que está na linha 18 do `.k`); `:79` (`#line 74`, arquivo com 73 linhas) | **erro do golden** |
| lowering gerado (gerente de `parallel`, despacho de `match`, `keel__rv0`) | 7 (7) | `cases/004-parallel/expected/c23/app/app_val.c:18`, `cases/009-match-tags/expected/c23/ag.c:27`, `cases/012-parallel-any/expected/c23/app/app_bs.c:18` | esperado (código gerado) |
| membros de instância de genérico (`slot`/`slice`/`array` de `ag2_Ag`, `routine`) | 10 (10) | `cases/010-routine/expected/c23/keel/keel_routine_ag2_Ag.type.h:11`, `:12` | esperado, mas precisa ser lido à parte |
| `tags` que vira enum de quatro linhas | 4 (4) | `golden/c23/keel/keel_corot.type.h:10`–`:13` (`#line 12` cobre só a primeira linha) | **falta `#line`**, ou decisão (seção 6) |
| prelúdio: `static_assert` depois de `#line 23` | 2 (2) | `golden/c23/keel.type.h:19`, `:20` | **falta `#line 27`** (seção 5) |
| chave `}` de fechamento mapeada a linha em branco (instâncias de `slice`/`buffer`/`array`, `021`) | 7 (7) | `cases/010-routine/expected/c23/keel/keel_slice_keel_routine_slot_ag2_Ag.h:66`, `cases/010-routine/expected/c23/keel/keel_slice_keel_routine_slot_ag2_Ag.type.h:11` | contagem divergente; ver seção 6 |
| `typedef struct` com nome na linha de fecho (anônimo no `.k`) e `keel_parallel` | 2 (2) | `cases/021-else-assign/expected/c23/app/app_reg.type.h:8`; `golden/c23/keel/keel_parallel.type.h:8` | esperado (nome só no fecho) |
| `__chk` de `constexpr` sob C11 | 0 (8) | `cases/001-arena-buffer-defer/expected/c11/app/app_cfg.type.h:11`, `cases/020-extent/expected/c11/app/app_pos.type.h:13` | esperado (já anotado em `line-audit.py`) |

Os 64 de c23 somam: 18 + 14 + 7 + 10 + 4 + 7 + 2 + 2 = 64. Os 72 de c11 são os
mesmos 64 mais os 8 `__chk`.

## 5. Os três pontos vistos antes

**(1) `#include <stdint.h>` sem `#line` em `keel.type.h`: confirmado.** A saída
tem `#include <stdint.h>`, `<stddef.h>` e `<float.h>` nas linhas 4–6 de
`golden/c23/keel.type.h` (e nas mesmas linhas de `golden/c11/keel.type.h`),
antes do primeiro `#line 19`. Pelo `keel.k`, eles são as linhas 12–14 (`import_c`).
Um único `#line 12 "keel.k"` antes da linha 4 basta, porque o contador então
conta 12, 13, 14. Em `cases/016-line/expected/c23/app/app_ml.type.h:6` o caso
faz exatamente isso (`#line 3` antes do `#include <stdio.h>`). A ferramenta não
mede essas linhas por causa da regra das linhas anteriores ao primeiro `#line`.

**(2) `static_assert` depois de `#line 23` com a linha 27 do `.k`: confirmado.**
O contador põe as linhas 19–22 da saída nas posições 25–28 do `.k`, mas o fonte
está em 27–30. O `#line 23` cobre a linha `typedef f32`, e a linha em branco 24 e
os comentários 25–26 do `.k` são suprimidos. Por isso a ferramenta marca
`keel.type.h:19` e `:20` como `MISSING` (o `.k` diz 25 e 26, que são comentário
e linha em branco), e o deslocamento que casa é +2 (linha 27). Falta `#line 27`
antes de `static_assert`. O `#line 42` seguinte já está certo (`REQUIRED jump`,
M=29).

**(3) Linha em branco depois do `#include`: refutado como divergência.** O
exemplo de `design/cgen-tool.md` §8.3 (`hello.type.h`) não tem linha em branco
depois de `#include "keel.type.h"`, e os casos 016 e 019 têm. A linha em branco
antes do primeiro `#line` não corresponde a nenhum `.k`. O `#line` seguinte
reinicia o contador, então nenhum par muda. É diferença de estilo.

## 6. Respostas

**(a)** A invariante dos dois contadores basta para o mapeamento. A regra de
"um `#line` antes de cada declaração" do golden acrescenta 703 diretivas
(44,6% do total) que a prova mostra dispensáveis (seção 3: os pares não mudam).
E a própria tabela de `keel-c-backend.md` §6 já diz "antes de cada declaração
que não seja a linha seguinte da anterior", que é exatamente o `REQUIRED`: o
golden não segue a tabela no que ela exclui. Manter as redundantes pode ser uma
escolha de legibilidade, mas é decisão do André, não exigência da invariante.

**(b)** Falta `#line` em três lugares, e o golden também tem um erro de número:

- `golden/c11/keel.type.h` e `golden/c23/keel.type.h`: falta `#line 27 "keel.k"`
  antes de `static_assert` (linhas 19–20 em c23; em c11, as linhas do `static_assert` com deslocamento +2);
  e falta `#line 12 "keel.k"` antes dos três `import_c` no topo (linhas 4–6).
- `golden/*/keel/keel_corot.type.h`: a linha `pub tags` do `.k` (linha 12) sai em
  quatro linhas C. O contador dá 13 para o primeiro membro. Ou se põe `#line 12`
  antes de cada linha que sai dali, ou se decide que os membros não têm linha
  própria. Isso é decisão, não correção automática.
- **Erro, não falta:** `golden/c11/keel/keel_arena.h` e `golden/c23/keel/keel_arena.h`
  estão em +1 em relação a `base/keel/arena.k` a partir de `from_array`. O
  cabeçalho de `from_array` está na linha 18 do `.k`, e o golden diz 19; `#line 74`
  é depois do fim do arquivo, que tem 73 linhas. O histórico (`c8d3d30`, `0639ee2`)
  mostra o mesmo deslocamento, então não é deriva recente.

## 7. Saída final

```
$ python3 golden/line-check.py
== totais por perfil ==
c11: 155 arquivos; REQUIRED first=128 file=0 jump=314; REDUNDANT=352; MISSING=72; sem-id (nao verificavel)=191
c23: 155 arquivos; REQUIRED first=128 file=0 jump=302; REDUNDANT=351; MISSING=64; sem-id (nao verificavel)=190
total: 310 arquivos, 1575 #line, 703 REDUNDANT, 136 MISSING, 0 sem .k
```

Nenhuma proposta de mudança no golden nem nos normativos foi feita aqui.
