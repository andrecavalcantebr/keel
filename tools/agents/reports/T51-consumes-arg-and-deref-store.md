# T51 — relatório: o argumento que consome não é uso posterior; `*p = v;` não é atribuição a `p`

## O que mudou (arquivo por arquivo)

### `tools/cgen/src/engine/parser/parser_islands_internal.h`

- `Local` ganhou o campo `size_t invalid_from;`, com o comentário pedido pelo
  cartão: a invalidação só conta como uso a partir desse token (os argumentos da
  própria chamada são avaliados antes do efeito dela).

### `tools/cgen/src/engine/parser/parser_islands_closure.c`

- `invalidate` ganhou o parâmetro `size_t from` e grava
  `c->locals[l].invalid_from = from;` em cada local que invalida.
- `isl_roles_at_call`: `from` é calculado uma vez, antes dos laços, como o maior
  `args[k].end` entre os `argc` argumentos (começando em `callee`, então com
  `argc == 0` vale `callee`). Os dois chamadores de `invalidate` passam `from`.

### `tools/cgen/src/engine/parser/parser_islands_flow.c`

- Bug 1: o ponto que acusa passou a exigir `i >= c->locals[x].invalid_from`.
  Um uso depois do `)` da chamada continua sendo acusado, uma vez (o `told`
  continua valendo).
- Bug 2: `assigned` passou a ser
  `isl_punct(a, i + 1, "=") && !(i > 0 && isl_punct(a, i - 1, "*"))`, com um
  comentário explicando que `*p = v;` escreve através de `p`. Assim `*p = 1;` cai
  no `else if`: é acusado se `p` for inválido e não zera o estado de `p`.
  `p = expr;` continua sendo atribuição.

Nenhum teste, `golden/`, normativo ou outro arquivo foi editado por mim. Não houve
commit.

## Saída final dos comandos de aceitação

Antes das edições, `python3 test/cgen/diag.py` deu exatamente o bloco do cartão
(`missing use.k:12`, `missing use.k:27`, `unwanted use.k:6`, `unwanted use.k:11`).

Após as edições e a compilação (`make -C tools/cgen -j8`, exit 0, sem warnings):

1. `python3 test/cgen/diag.py` (exit 0):

```
56 failure cases; the catalog is covered 142/150
```

   Sem bloco "differ from the markers".

2. `sh tools/agents/verify.sh` (exit 0):

```
ok   build (0 warnings, baseline 0)
ok   make check (59 lines ok)
ok   golden: 140 ok, 0 fail, 0 xfail, 0 xpass, 0 wip
ALL OK
```

3. `./test/run.sh > /tmp/t51-run.log 2>&1; echo "exit=$?"` (rodado inteiro):

```
exit=0
ok     layout-check                   (codegen E6)
ok     line-check                     (backend §6)

140 ok, 0 fail, 0 xfail, 0 xpass, 0 wip
docs: 45 ok, 0 fail
```

## Pontos de dúvida e incoerências

- **Estado anterior ao cartão, fora do escopo:** `git status` já mostrava
  `test/cgen/diag/cases.txt` modificado e o diretório
  `test/cgen/diag/use-after-effects/` como não rastreado, antes de eu começar.
  Não os toquei; o diff desse arquivo é de uma linha que não é minha.
- **Efeito colateral intencional, a conferir na revisão:** com a mudança do bug 2,
  `*p = v;` deixa de zerar `ndeps` e `local_origin` de `p`. Antes, uma escrita
  através de `p` apagava as dependências de `p` como se fosse reatribuição. Isso
  é o que a regra 10 pede, mas é uma mudança de comportamento para qualquer caso
  que dependesse do efeito antigo; os testes existentes continuam verdes.
- **`invalid_from` não é zerado em `isl_produce` nem na atribuição:** não precisa,
  porque a acusação só olha `invalid_from` quando `invalid` é verdadeiro, e
  `invalid_from` é sempre regravado por `invalidate` antes de `invalid` virar
  verdadeiro. Vale registrar caso alguém mude isso no futuro.
- **Locais derivados herdam o mesmo `from`:** pelo `descends`, um local que
  depende de `x` também recebe `invalid_from` da chamada. Um argumento derivado
  que aparece na mesma chamada, antes de `from`, não é acusado, o que é
  consistente com o bug 1.
- Nenhuma outra escrita de `invalid = true` existe fora de `invalidate`; a única
  leitura de `invalid` para acusar é a do `parser_islands_flow.c`. Nada mais
  precisou do mesmo cuidado.
