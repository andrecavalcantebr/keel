# T20 — relatório: `match` com comentário, rótulos empilhados, `match` numa linha, campo de outro módulo

Estado: **parcial.** As quatro formas foram implementadas e o oráculo do 047 passa
byte a byte nos dois perfis. O 052 passa nas formas de `match` (o `app/app_fm.c` e o
`app/app_fm.h` saem idênticos ao esperado), mas falha em **um** arquivo que não tem
`match` nenhum: `app/lib/app_lib_other.c`. Essa diferença já existe no HEAD (ver abaixo).
Por isso o 052 ficou com `|wip` em `test/cgen/gen/golden-cases.txt`. Não o tirei,
porque o cartão exige 0 fail e a decisão sobre o `#include` é normativa (ver "Dúvidas").

## O que mudou

**`tools/cgen/src/engine/emit/match.c`** (único arquivo de código tocado):

1. **Braço na linha do rótulo com comentário** (`A: r = 1;   // one`). O ` }` do braço
   vai no fim do código (depois do `;`), e o comentário é removido por `comment_edits`
   (que já existia). `analyze` deixou de olhar o último caractere da linha: agora confere o
   último token da instrução (antes do próximo rótulo ou do `}`), que tem de ser `;` ou `}`
   e estar na linha do rótulo. Assim o comentário no fim não atrapalha.
2. **Rótulos empilhados na linha** (`A: B:`, corpo na linha seguinte). Um rótulo que não
   começa a linha, mas segue imediatamente o `:` do anterior sem quebra de linha entre
   eles, é aceito: sai como ` keel__m0_B: {` logo depois do `:` de `A`, e o rótulo do
   primeiro sai sem chave. `inl` passou a valer só para o último rótulo do grupo, que é
   quem tem a instrução na mesma linha.
3. **`match` inteiro numa linha** (`match (s) { A: r = 1; B: r = 2; }`). `analyze`
   detecta `oneline` (nenhuma quebra entre `{` e `}`). O despacho engole o `{` e o rótulo
   do primeiro braço, numa única edição. Os braços seguintes vêm com `} goto keel__m0_end; `
   antes do rótulo, e o `}` final vira `} keel__m0_end: ;`. Tudo sai numa linha de saída só,
   sem `#line` entre o despacho e ela. Um `match` numa linha com coluna (`P->col[i]`) é recusado.
4. **Operando que é campo de struct de outro módulo** (`Gate *g` com `g->ph`, `Gate` trazido
   por `import ... types`). `field_operand` procura o struct na AST do módulo importado
   (`ts->origin->ast`, pelo símbolo resolvido) e o conjunto em `origin->symbols`
   (`field_set`). Os rótulos saem como `app_lib_other_Phase_ON`, e o `switch` lê
   `g->ph.tag`. Campo ponteiro sai `g->ph->tag`.

Também: comentários do cabeçalho de `match.c` atualizados (lista de recusas e a descrição
das formas), e o `struct_type` / `field_set` novos com comentário próprio.

**`test/cgen/gen/golden-cases.txt`**: apaguei `|wip` apenas da linha `047-match-more`.
A linha `052-match-foreign` continua com `|wip`. Nenhuma outra linha mudou.

Não toquei em normativos, em `golden/**/expected/**`, em `proof.c`, em testes, em
`emit.c`, `uses.c` nem `emit_internal.h`. Não fiz commit nem `git add`.

## Verificação dos casos de borda (fora do oráculo)

Um fonte temporário com estas formas passou por `cgen --stop-after=gen` nos dois perfis
e saiu como esperado (não entrou no repositório):

- `A: B: r = 1;   /* comentário */` em `match` multilinha: sai `keel__m0_A: keel__m0_B: { r = 1; }`.
- Dois grupos na mesma linha: `match (s) { A: B: r = 1; C: r = 2; }` sai
  `keel__m0_A: keel__m0_B: { r = 1; } goto keel__m0_end; keel__m0_C: { r = 2; } keel__m0_end: ;`.
- Campo ponteiro de outro módulo, `Gate *g` com `tagged Phase *ph`: sai `switch (g->ph->tag)`.

Limite conhecido: o tipo qualificado no parâmetro (`other.Gate *g`) continua recusado por
`field_operand`, como antes ("um nome qualificado é escrito com o módulo: recusado").
O 052 usa o nome sem qualificação, então isso não bloqueia o cartão.

## Saída final dos comandos de aceitação

`sh test/cgen/gen_golden.sh` (rc 0):

```
wip    048-parallel-more
wip    049-adapt-foreach
wip    050-else-default
wip    051-generic-forms
wip    052-match-foreign (differs)
wip    053-generic-module-instance
55 ok, 0 fail, 6 wip
```

`golden/run.sh` (última linha, também impressa pelo verify):

```
102 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
```

`sh tools/agents/verify.sh` (rc 0):

```
ok   build (0 warnings, baseline 0)
ok   make check (53 lines ok)
ok   golden: 102 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

`sh tools/agents/survey/run.sh` (as quatro linhas pedidas):

```
t02 [c23 OK] [c11 OK]
t04 [c23 OK] [c11 OK]
t05 [c23 OK] [c11 OK]
y01 [c23 OK] [c11 OK]
```

Antes da mudança, as mesmas quatro linhas estavam `LIMIT` nos dois perfis, e o gen tinha
6 wip (047 a 052). Nos testes `make check` nenhum teste afirmou o contrário do cartão.

## Dúvidas e incoerências (para decisão do orquestrador)

1. **`app/lib/app_lib_other.c` do 052 (bloqueio do cartão).** O oráculo (escrito à mão)
   não tem `#include "keel/keel_tagged_app_lib_other_Phase_void.h"` no `.c` de
   `other.k`, um módulo só com `pub tags` e um `typedef struct` que tem `tagged Phase void ph`.
   O cgen emite esse include. Verifiquei que é **anterior** a esta tarefa: com o `match.c`
   do HEAD, o mesmo `cgen --stop-after=gen` gera o include (`grep` acha 1 ocorrência). A
   causa está em `k_em_use_includes` (`uses.c`), que inclui o `.h` de todo tipo island
   (inclusive o tipo de campo por valor). A regra 4 de §4.3.2 de `keel-c-backend.md` diz
   "`.c` inclui o próprio `.h` e o `.h` de cada módulo ou instância que **usa**", e não
   deixa claro se um campo por valor conta como uso. Não mexi em `uses.c` nem no oráculo:
   é leitura de normativo, e o cartão não autoriza. Se a decisão for "uso é chamada no corpo",
   o ajuste é em `uses.c`, e o 052 passa sem mais nada.
2. **Forma de uma linha sem norma.** O `keel-c-backend.md` §5.6 regra 12 descreve o braço
   com o `goto` na linha seguinte. O `match` inteiro numa linha (`keel__m0_A: { r = 1; } goto ...;`
   tudo numa linha de saída) é só o que o 047 fixa. Vale uma regra normativa para ela; eu não
   a escrevi.
3. **Comentário de bloco no fim da linha** (`/* ... */`) é tratado por `comment_edits` como
   qualquer comentário da linha. Não testei comentário de bloco que atravessa quebras de linha
   dentro de um braço; `analyze` recusa esse caso porque a instrução precisa estar na linha
   do rótulo.
4. **Estado do repositório durante o trabalho.** O HEAD andou enquanto eu trabalhava
   (`006c600`, `4869f9c`, `18a0579`, `df10743`). O 102 de `golden/run.sh` do cartão é o
   número com esses commits. Não fiz commit nem toquei em nada deles.
5. **Coisas que não são minhas e que vi no `git status`:** `tools/agents/survey/app3/` e
   `tools/agents/survey/run3.sh` (não rastreados), e `y02` no survey, com `ERR` por
   `other.WORKERS` (paralelo, sem `match`). Não mexi em nada disso.

## Arquivos

- Código: `tools/cgen/src/engine/emit/match.c`
- Índice de casos: `test/cgen/gen/golden-cases.txt` (só `047-match-more` sem `|wip`)
- Relatório: `tools/agents/reports/T20-match-more.md`
