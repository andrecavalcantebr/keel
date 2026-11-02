# T32 — relatório do autor de oráculo: `walk` com a `{` do corpo em linha própria

## O que foi escrito (tudo novo, nada existente alterado)

- `golden/cases/057-walk-allman/app/wa.k` — módulo `app.wa`. Lista ligada com `begin`, `has_next`
  e `next` (mesmo cursor de 054), e `pub i32 sum(List *l)` com a `{` da função em linha própria e
  o `walk (i32 *p, app.wa.cursor c : l)` com a `{` do corpo em linha própria.
- `golden/cases/057-walk-allman/proof.c` — arnês: lista de três nós (soma 8), lista vazia (soma 0).
  Inclui `keel.type.h` e `app/app_wa.h`, como 054.
- `golden/cases/057-walk-allman/expected/c23/app/app_wa.{type.h,h,c}` — escritos à mão.
- `golden/cases/057-walk-allman/expected/c11/app/app_wa.{type.h,h,c}` — escritos à mão.
- `test/cgen/gen/golden-cases.txt` — acrescentada a linha `057-walk-allman|app/wa.k|-|wip`.
- `golden/line-check.allow` — **não alterado**: `line-check.py` não aponta nenhuma linha de 057 como
  `MISSING`, e `--allow` não lista nada do caso. O `keel__c0`, o `begin`, o `has_next` e o `next`
  estão na própria linha do cabeçalho do `walk`, que já tem linha no `.k`; a linha `{ i32 *p = …`
  fica na linha 16 do `.k`, que é a `{` sozinha.

Não rodei `tools/cgen/cgen`, não li `tools/cgen/src/`, não rodei `make` nem `verify.sh`. Não fiz
`git add` nem commit.

## Regras seguidas

- Cabeçalho do `walk` sai sem a `{` do corpo, com o `while (…)` no fim da linha; a `{` do corpo sai
  na linha seguinte, com a declaração do binder (`    { i32 *p = app_wa_next(keel__c0, &c);`).
  A `}` do corpo fecha também o bloco do `walk` (`    } }`). Regra 5 do §5.7 (texto novo), e o
  mesmo formato do `foreach` Allman de 054.
- Cada linha do `.k` continua com a sua no C: `.c` com `#line 1` e linhas em branco até a linha 12
  (`i32 app_wa_sum`), depois 1:1 até a `}` da linha 20.
- `.h` e `.type.h` no formato de 054: guarda, `#include` do `.type.h` (e nenhum `.h` de módulo, pois
  não há `slice`), `#line` antes de cada declaração cujo número do `.k` diverge do contador, a
  seção dos protótipos, a dos corpos `static inline` (`#line 8` de novo) e o `#endif`.
- Nomes locais não sombreiam símbolos do módulo (`p`, `c`, `t`, `cur`, `l`).

## Verificação

`sh golden/run.sh` (rodado uma vez, após as escritas):

```
ok     057-walk-allman          c23  (run)
ok     057-walk-allman          c11  (run)
...
ok     line-check                     (backend §6)

108 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
```

`python3 golden/line-check.py` (o mesmo `line-check` que o `run.sh` roda): nenhum `REDUNDANT` e
nenhum `MISSING` para `057-walk-allman`. Os `#line` do caso, classificados (`--verbose`):
`REQUIRED first` no `.c` (linha 1 do `.k`), `REQUIRED first` no `.type.h` (linha 4), `REQUIRED first`
e dois `REQUIRED jump` no `.h` (8 → 12 → 8), idênticos em c23 e c11.

## Escolhas onde a spec ou o card deixaram margem

1. **Dois perfis com o mesmo corpo.** O `.c`, o `.h` e o `.type.h` de c23 e c11 são idênticos
   exceto pela linha de comentário que diz o perfil. Foi assim porque esta forma não tem nada que
   dependa do perfil (sem `constexpr`, sem `__chk`, sem `_Static_assert`, sem `_Atomic`). Mesmo assim
   os dois arquivos foram escritos separadamente e não derivados um do outro; quem revisar deve
   conferir que essa igualdade é o que a spec pede, e não uma cópia acidental.
2. **Chave da função em linha própria.** O card pede `{` na linha seguinte também na função
   `sum`. Esperei que a emissão do cabeçalho da função saia como em 054 (`allman`): a assinatura
   numa linha, a `{` na seguinte, sem `#line` extra, porque cada uma tem o mesmo número que no `.k`.
3. **Sem `NOTES`.** 054 também não tem, e o card não pede. Se quiser o caso anotado, o texto é o
   único que falta.
4. **Sem `slice`, sem `#include` de `keel/keel_slice_i32.h`.** A forma não usa `slice`, então o
   `.c` e o `.h` não incluem nada além do próprio `.h` e do `.type.h`.

## Pontos em aberto

- Nada que bloqueie. Se o `cgen` emitir a `{` do corpo do `walk` no mesmo registro do cabeçalho
  (ou não emitir o `} }`), o caso fica `wip` até a implementação alcançar a regra 5 do §5.7.
- O `#line` da `{` do corpo do `walk` no `.c` não aparece: a linha 16 do `.k` é `{`, a linha 16 da
  saída é `    { i32 *p = …`, e o contador já vale 16 ali. Está correto por regra; registro só para
  o revisor não estranhar a ausência.
