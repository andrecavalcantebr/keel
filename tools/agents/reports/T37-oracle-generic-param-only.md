# T37 — relatório: oráculo do caso 059 (constante e tipo de módulo genérico que citam só parâmetro)

## O que foi feito

Escrito à mão o caso `golden/cases/059-generic-param-only/`, sem rodar `tools/cgen/cgen` e sem ler `tools/cgen/src`. Fontes consultadas: `keel-c-backend.md` (§2.1, §4.1, §4.3, §4.3.2, §4.4, §4.4.1, §9.2, a regra recém-escrita de §4.3), `golden/README.md`, `golden/run.sh`, `golden/line-check.allow` e os goldens análogos `006-dim`, `007-instance`, `051-generic-forms`.

### Arquivos criados

Fonte:

- `golden/cases/059-generic-param-only/app/pbox.k` — módulo genérico `module app.pbox dim N type T;`, com `pub constexpr size_t CAP = N;`, `pub typedef struct { T first; T last; } Pair;`, `pub modifier box byref { T v[N]; }` e os `pub inline` `cap` e `ends`.
- `golden/cases/059-generic-param-only/app/pbox_use.k` — cliente `module app.pbox_use;`, com `import app.pbox types;`, que instancia `box(3) i32` e expõe `probe_cap`, `probe_first` e `probe_last` (todos `pub`, não `inline`).
- `golden/cases/059-generic-param-only/proof.c` — arnês; usa só `app/app_pbox_use.h` e afirma `cap == 3`, primeiro extremo `4`, último `6`.
- `golden/cases/059-generic-param-only/NOTES` — prosa dos esperados, no formato dos vizinhos.

Esperados (escritos à mão, um arquivo de cada vez, nos dois perfis):

- `expected/c23/app/app_pbox.type.h`, `app_pbox.h` — par do genérico importado, vazio (só guarda, `keel.type.h` e o `#include` do próprio `.type.h`).
- `expected/c23/app/app_pbox_box_3_i32.type.h`, `app_pbox_box_3_i32.h` — instância.
- `expected/c23/app/app_pbox_use.type.h`, `app_pbox_use.h`, `app_pbox_use.c` — cliente.
- Os mesmos nomes em `expected/c11/`, com o `constexpr` sob C11 como macro mais objeto de conferência (§9.2).

### Decisões de sintaxe e de nome

1. **Nome da instância: `app_pbox_box_3_i32`.** Segui a linha `M.mod(D)` aplicada a `A` da tabela do §2.1 (`M_mod_<D>_<A>`), com `M = app.pbox`, `mod = box`. Como `box` não é o último componente de `app.pbox`, não há a supressão da regra 2. Arquivos em `app/`, pelo diretório do módulo. Os nomes derivados são `app_pbox_box_3_i32_CAP`, `app_pbox_box_3_i32_Pair`, `app_pbox_box_3_i32_cap` e `app_pbox_box_3_i32_ends`.
2. **`constexpr` e `typedef` que citam só parâmetro** saem no `.type.h` da instância, conforme a regra 1 de §4.3 (rascunho em `git diff`, ainda não commitado).
3. **Par do genérico** existe e é vazio, conforme §4.4.1 regra 5 e §4.1 regra 2 (módulo importado). Não há `app/app_pbox.c`, conforme §4.4.1 regra 6.
4. **Sem `.c` de instância**, porque o `.c` de instância só existe quando a invocação a compila (§4.1 regra 1).
5. **Corpo do `ends` com o índice substituído pelo valor**: `b->v[3 - 1]`, conforme §4.9 (a substituição é de valor, não de grafia).
6. **Cliente sem `.h` de instância no `.h`**: as funções `pub` não são `inline`, então os corpos vão ao `.c`, e o `.c` inclui `app/app_pbox_box_3_i32.h`. O `.h` do cliente inclui só o próprio `.type.h`, porque nenhuma assinatura usa tipo por valor da instância.
7. **Padding de linha do `.c` do cliente**: `#line 1` seguido de linhas em branco, como em `006` e `007`.

### Lista de linhas do line-check

Acrescentada uma linha em `golden/line-check.allow`, para o `static const ... _CAP__chk` do C11, que o README lista como código injetado. A chave é o caminho mais o texto, gerada com `python3 -I golden/line-check.py --allow`.

### Lista de casos

Acrescentada em `test/cgen/gen/golden-cases.txt` a linha `059-generic-param-only|app/pbox_use.k|-|wip`, conforme o cartão.

Nenhum `WIP` foi criado, porque o caso está completo e o runner precisa checá-lo.

## Saída do oracle

Primeira rodada, antes da entrada no allow:

```
ok     059-generic-param-only   c23  (run)
ok     059-generic-param-only   c11  (run)
ok     layout-check                   (codegen E6)
FAIL   line-check
         line-check: golden/cases/059-generic-param-only/expected/c11/app/app_pbox_box_3_i32.type.h:10: maps to app/pbox.k:3, which does not match (not in line-check.allow)

113 ok, 1 fail, 0 xfail, 0 xpass, 3 wip
```

Final, depois da entrada no allow (`sh golden/run.sh`):

```
ok     059-generic-param-only   c23  (run)
ok     059-generic-param-only   c11  (run)
ok     060-generic-priv-inline  c23  (run)
ok     060-generic-priv-inline  c11  (run)
ok     layout-check                   (codegen E6)
ok     line-check                     (backend §6)

114 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
```

Os `wip` são `022`, `023` e `024` (esqueletos já existentes, fora do escopo). Os `Aborted` na saída são as asserções de `DEBUG` dos casos 026 e 033, esperados. O caso 060 (T38) também passa agora; não toquei nele.

`python3 -I golden/line-check.py --check` sai com código 0.

## Dúvidas em que a spec não decide

1. **Nome do modificador na instância.** O exemplo da regra 1 de §4.3 (`app_g01_3_i32_CAP`) sugere que o modificador leva o mesmo nome do último componente do módulo, `g01`. Aqui usei `box`, com `app_pbox_box_3_i32`, pela tabela de §2.1. Se a intenção for outra, o caso muda de nome, e o exemplo de §4.3 deveria dizer qual dos dois se aplica.
2. **`ends(b).first`.** A gramática de expressão da spec não cobre acesso a membro de chamada; não há nenhum golden com `)` seguido de `.campo` (busca feita). Assumi que a expressão passa para o C como postfix comum. Se keel não admitir isso, a alternativa é um `Pair` nomeado no cliente, e não sei qual é a forma de nomear um tipo de instância de fora do módulo.
3. **`b.v[i] = ...` no cliente.** Escrever campo de instance fora do módulo que a declara é `instance-field-access`, um warning (§4.5 regra 2). O golden `006` faz o mesmo (`g.dims[0] = 2`), e o runner não checa warnings, então mantive por paridade. Se o oráculo deve ficar livre de warning, o genérico precisa de um verbo de preenchimento, o que é mudança de escopo do caso.
4. **Ordem dos `typedef`/`struct` no `.type.h` da instância.** Usei a ordem do fonte (`constexpr`, `Pair`, depois o modificador). A spec não diz se a ordem é do fonte ou se o modificador vem antes; o runner não distingue.

Nenhuma dessas dúvidas afeta o que o runner checa hoje. A 1 é a que mais importa para o normativo, e a 2 é a que mais pode forçar mudança no caso.

## Arquivos tocados

- `golden/cases/059-generic-param-only/**` (novo)
- `golden/line-check.allow` (uma linha acrescentada)
- `test/cgen/gen/golden-cases.txt` (uma linha acrescentada)
- `tools/agents/reports/T37-oracle-generic-param-only.md` (este relatório)

Não toquei em normativos, em `tools/cgen/src`, nem em outros casos. Não fiz commit nem `git add`.
