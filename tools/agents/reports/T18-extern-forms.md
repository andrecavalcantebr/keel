# T18 — relatório: `pub extern_c`, `extern_c` de uma linha e comentário no `INIT` de `constexpr`

## Resultado

**Não passou a aceitação. Parei no último estado que passa.** A implementação
das três formas está feita e verificada (o caso `045-extern-forms` sai de `wip`
nos dois perfis), mas `sh tools/agents/verify.sh` falha em `make check` por um
teste que afirma o contrário do item 2 do cartão (`cli/source_errors`, detalhe
abaixo). Por instrução do cartão, não editei esse teste.

- O patch completo está fora do repositório, em
  `/tmp/claude-1000/-home-andre-code-cprojects-keel/6df5c4e9-8d62-4717-9b96-cb209ab7b812/scratchpad/T18-extern-forms.patch`
  (inclui também as 8 linhas `|wip` que já estavam sem commit em
  `test/cgen/gen/golden-cases.txt`; o trecho que é meu é só a remoção de
  `|wip` da linha 45).
- O repositório ficou no estado original: `tools/cgen/src/engine/emit/*`
  voltou ao HEAD (`git checkout`, que só desfez as minhas alterações), e a
  linha 45 de `golden-cases.txt` voltou a `045-extern-forms|app/ef.k|-|wip`.
  O `git status` agora mostra só `golden-cases.txt`, com as linhas que já
  estavam lá antes de eu começar.

## O conflito: `test/cgen/cli/source_errors.sh`

Linhas 18 a 20:

```sh
printf 'module e;\nextern_c [type_h] { typedef int T; }\n' > "$tmp/e.k"
out=$("$BIN" --base-dir "$K" -I "$tmp" --stop-after=gen --dest-dir "$tmp/g" "$tmp/e.k" 2>&1); rc=$?
[ $rc -eq 2 ] && echo "$out" | grep -q 'implementation-limit' || bad "extern_c on one line must be refused, not lost: rc=$rc [$out]"
```

A afirmação é: `extern_c [type_h] { … }` numa linha só **deve ser recusada**
(`implementation-limit`, rc 2). O cartão pede o contrário (item 2: aceitar
essa forma e escrever o `typedef` no `.type.h`).

Observações:
- O cartão fala em "teste de unit em `test/cgen/unit/`". Este arquivo está em
  `test/cgen/cli/`, mas é o mesmo tipo de afirmação, está em `make check`
  (`test/cgen/run.sh` roda `cli/`) e bloqueia a aceitação. Por isso parei, em
  vez de editar. Se a leitura correta for outra, a decisão é sua.
- A mensagem do teste ("must be refused, not lost") protege contra perda
  silenciosa do texto. A implementação que testei não perde o texto: emite a
  linha. Se a decisão for aceitar a forma, o teste precisa mudar para afirmar
  a saída; se for recusá-la, o item 2 do cartão cai nessa parte.

Com a implementação aplicada, o resto do `make check` passou: a única linha
`FAIL` de `sh test/cgen/run.sh` foi `cli/source_errors`; todos os testes de
unit (inclusive `unit/emit`) passaram.

## O que o patch faz (para revisão)

Em `tools/cgen/src/engine/emit/`:

1. **`pub extern_c { … }`**: `supported()` deixa de recusar o `pub` explícito
   (a regra `k != n->first && !extern_c_priv` saiu). A saída é a do plano.
2. **`extern_c { … }` em uma linha** (plano, `priv`, `[type_h]`): novo
   `extern_c_inline()`. O texto sai por `emit_line()` com o número da linha do
   `{`, entre o primeiro e o último token do corpo (os brancos de borda caem).
   A recusa antiga (`memchr` de `\n`) saiu. Usado em `type_h_body` (`.type.h`),
   `h_body` (`.h`) e no laço de `emit_c` (o `priv` troca a linha do fonte).
3. **Comentário em `constexpr`**: `k_em_put_source()` (em `emit_core.c`) copia
   fonte sem comentários e, quando o comentário termina a linha, tira os brancos
   antes dele. `k_em_put_range` usa isso nas folgas entre tokens, então vale para
   `k_em_put_continued` (o `#define`) e para os demais usos. `emit_constexpr_c11`
   usa a mesma rotina no `INIT` do `#define` e no `__chk`. `k_em_continue_from`
   é a parte de `k_em_put_continued` que põe ` \` antes de cada quebra.
   `k_em_range_plain` foi removida (sem usos depois disso), e a recusa de
   comentário em `cb_parts` (`constexpr_block.c`) também.

Uma restrição **a mais** que o cartão não pede: um `extern_c { … }` de uma
linha só é aceito se nenhum outro nó ocupa a mesma linha (`alone_on_line()`).
Sem isso, o `TYPE` vizinho copiaria a linha inteira no `.type.h` (`type_h_body`
emite o intervalo da linha). Pode-se afrouxar isso, mas com essa regra a saída
fica certa.

## Saída dos comandos de aceitação

Com o patch aplicado (o estado que testei; os números são desse estado):

`sh test/cgen/gen_golden.sh`:

```
wip    046-array-leftovers
wip    047-match-more
wip    048-parallel-more
wip    049-adapt-foreach
wip    050-else-default
wip    051-generic-forms
wip    052-match-foreign
53 ok, 0 fail, 7 wip
```

045 saiu de `wip` nos dois perfis (`expected/c23` e `expected/c11` batem byte a byte).

`golden/run.sh` (última linha):

```
100 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
```

`sh tools/agents/verify.sh`:

```
ok   build (0 warnings, baseline 0)
FAIL make check
FAIL   cli/source_errors
       FAIL: extern_c on one line must be refused, not lost: rc=0 []
```

`sh tools/agents/survey/run.sh` (as linhas pedidas):

```
k01 [c23 OK] [c11 OK]
k02 [c23 OK] [c11 OK]
x01 [c23 OK] [c11 OK]
x02 [c23 OK] [c11 OK]
x03 [c23 OK] [c11 OK]
x04 [c23 OK] [c11 OK]
```

Sem o patch (HEAD, estado em que o repositório ficou): `gen_golden` dá
`52 ok, 0 fail, 8 wip`, e o survey mostra `LIMIT` para x01 a x04 e k01, e
`LIMIT` no c11 de k02.

## Dúvidas e pontos abertos

- **Decisão pedida:** o item 2 para `[type_h]` numa linha só contradiz
  `cli/source_errors.sh` (acima). Qual dos dois vale?
- O `k_em_put_source` dentro de `k_em_put_range` vale para **todos** os
  chamadores de `k_em_put_range` (foreach, parallel, defer, extent...): um
  comentário numa folga entre tokens agora sai do texto gerado. Antes ele
  passava. Nenhum golden mudou, e `test/cgen/run.sh` só falhou no teste acima.
  Vale a pena o orquestrador confirmar que isso é o desejado (é o que a regra
  de §6 pede, mas não foi escrito um caso específico para isso).
- O `INIT` de `emit_constexpr_c11` continua copiado do fonte sem renomear
  nomes do módulo (comportamento já existente, fora deste cartão). Não mexi.
- Não fiz commit, não rodei `git add`, não toquei normativos, `golden/**/expected`
  ou `proof.c`.
