# T51 — o argumento da chamada que consome não é uso posterior; `*p = v;` não é atribuição a `p`

## Objetivo

Dois bugs antigos da análise de procedência (keel-spec §4.12), os dois em
`child-region-after-invalidation`. **Nenhum normativo muda**: a spec já diz o que
é certo, e o cgen diverge dela.

1. **O argumento da chamada que invalida é lido como uso posterior.**
   `ring.drop(r)` com `drop(consumes Ring *r)` aplica a invalidação ao chamar, e
   depois o token `r` do próprio argumento é visitado e acusado ("`r` is used
   after `drop` invalidated…"). Mas os argumentos são avaliados *antes* do efeito
   da chamada: não são uso posterior. Pior: o símbolo é acusado uma vez só
   (`told`), então esse falso positivo gasta o aviso do uso verdadeiro que vem
   depois (`ring.touch(r)`), que fica sem diagnóstico.
2. **`*p = 1;` é tratado como atribuição a `p`.** O código decide por "o token
   seguinte é `=`", sem olhar o que vem antes. `*p = 1` escreve *através* de
   `p`, não reatribui `p`: a regra 10 (atribuição sem papel descarta o estado)
   não se aplica, e escrever por ponteiro inválido é uso de `p` e deve ser
   acusado.

O oráculo, escrito à mão, é o caso `test/cgen/diag/use-after-effects/`
(`ring.k` e `use.k`, registrado em `test/cgen/diag/cases.txt`). **Não edite** esse
caso, nem `cases.txt`, nem nenhum outro teste. Hoje ele falha assim, e é o que
tem de passar:

```
use-after-effects: diagnostics differ from the markers
  missing  use.k:12 [child-region-after-invalidation]
  missing  use.k:27 [child-region-after-invalidation]
  unwanted use.k:6 [child-region-after-invalidation]
  unwanted use.k:11 [child-region-after-invalidation]
```

## Arquivos

- Editar: `tools/cgen/src/engine/parser/parser_islands_internal.h` (o `Local`),
  `tools/cgen/src/engine/parser/parser_islands_closure.c` (`invalidate` e seus dois
  chamadores), `tools/cgen/src/engine/parser/parser_islands_flow.c` (o ponto que
  acusa e o que decide a atribuição).
- Escrever: `tools/agents/reports/T51-consumes-arg-and-deref-store.md`.
- Não editar: nenhum teste, nenhum `golden/`, nenhum normativo
  (`keel-spec.md`, `keel-rationale.md`, `keel-c-backend.md`, `cgen-tool-spec.md`),
  nenhum outro arquivo. Não faça commit.

## Contrato

### Bug 1 — a invalidação vale a partir do fim dos argumentos

Código atual, `parser_islands_closure.c:254-262`:

```c
/* `x` and, when `self`, `x` itself become invalid with everything that comes from it */
static void invalidate(Ctx *c, int x, bool self, keel_slice_char by) {
    for (size_t l = 0; l < c->local_count; l++)
        if (((int)l == x && self) || descends(c, (int)l, x, 0)) {
            c->locals[l].invalid = true;
            c->locals[l].told = false;
            c->locals[l].invalid_by = by;
        }
}
```

Os dois chamadores, em `isl_roles_at_call` (`parser_islands_closure.c:475-476`):

```c
if (roles.p[k] == K_ROLE_INVALIDATES) { own_invalidation(c, isl_tok(a, callee), x, false); invalidate(c, x, false, verb); }
if (roles.p[k] == K_ROLE_CONSUMES) { own_invalidation(c, isl_tok(a, callee), x, true); invalidate(c, x, true, verb); }
```

`args` é o `const Range *` de `isl_roles_at_call`, com `argc` elementos; cada
`Range` é `{ first, end }` em tokens, `end` exclusivo (`parser_islands_internal.h`,
`typedef struct { size_t first, end; } Range;`).

O `Local` (`parser_islands_internal.h:34-39`):

```c
    int deps[8];
    int ndeps;
    bool local_origin;          /* depends on an automatic `array` of the function */
    bool invalid;
    bool told;                  /* its use after invalidation was reported */
    keel_slice_char invalid_by; /* the verb that invalidated it */
} Local;
```

O ponto que acusa, `parser_islands_flow.c:1094-1098`, dentro do laço por token `i`
de `isl_walk`:

```c
} else if (c->locals[x].invalid && !c->locals[x].told) {
    isl_diag3(c, K_DIAG_CHILD_REGION_AFTER_INVALIDATION, isl_tok(a, i), isl_tok(a, i), c->locals[x].invalid_by,
          (keel_slice_char){0});
    c->locals[x].told = true;
}
```

Desenho pedido:

1. Acrescente ao `Local` um campo `size_t invalid_from;` com o comentário "the
   invalidation counts as a use only from this token on (the call's own
   arguments are evaluated before its effect)".
2. `invalidate` ganha um parâmetro `size_t from` e põe
   `c->locals[l].invalid_from = from;` em cada local que invalida.
3. Nos dois chamadores, `from` é o maior `args[k].end` entre os `argc` argumentos
   da chamada (o token logo depois do último argumento). Calcule uma vez, antes
   dos laços, e reuse. Com `argc == 0`, use `callee`.
4. No ponto que acusa, só acuse se `i >= c->locals[x].invalid_from`. O token do
   argumento da própria chamada fica de fora; qualquer uso depois do `)` da
   chamada continua sendo acusado, uma vez.

Exemplo de aritmética: em `ring.drop(r);` os tokens são `ring`(0) `.`(1) `drop`(2)
`(`(3) `r`(4) `)`(5) `;`(6). O argumento é `{4, 5}`; `from = 5`. O `r` no índice 4
não é acusado (4 < 5). Um `ring.touch(r)` na linha seguinte tem `r` num índice
maior que 5 e é acusado.

### Bug 2 — `* p =` nunca é atribuição a `p`

Código atual, `parser_islands_flow.c:1088`:

```c
bool assigned = isl_punct(a, i + 1, "=");
```

Em C válido, `expr * p = valor` não existe (o produto não é lvalue). Logo, um `*`
logo antes do identificador, seguido de `=`, ou é um declarador (`T *p = init;`,
que o código já exclui antes com `c->locals[x].decl_at != i`) ou é um `*p = v;`
(escrita através de `p`). Nos dois casos não é atribuição a `p`. Troque por:

```c
bool assigned = isl_punct(a, i + 1, "=") && !(i > 0 && isl_punct(a, i - 1, "*"));
```

Com isso `*p = 1;` cai no `else if` seguinte e é acusado se `p` for inválido, e
não zera o estado de `p`. `p = expr;` continua sendo atribuição.

## Passos sugeridos

1. `sh tools/agents/verify.sh snapshot /tmp/keel-snap-T51` NÃO se aplica (não é
   refactor: o comportamento muda de propósito).
2. Rode `python3 test/cgen/diag.py` antes de editar e confirme que a saída é a
   do bloco acima.
3. Faça o bug 1, compile (`make -C tools/cgen -j8`), rode o `diag.py`: devem sumir
   os dois `unwanted` e o `missing use.k:12`.
4. Faça o bug 2, compile, rode o `diag.py`: deve sumir o `missing use.k:27`.
5. Rode a aceitação.

## Aceitação

Rode, nesta ordem, da raiz do repositório:

```sh
python3 test/cgen/diag.py
sh tools/agents/verify.sh
./test/run.sh > /tmp/t51-run.log 2>&1; echo "exit=$?"
```

- `diag.py`: a última linha é `56 failure cases; the catalog is covered 142/150`
  e não há bloco "differ from the markers".
- `verify.sh`: termina em `ALL OK` e o número de warnings do build não passa do
  baseline (`tools/agents/warnings.baseline`, hoje 0). Ele faz `make clean`;
  não rode outro build ao mesmo tempo.
- `test/run.sh`: leva alguns minutos (use `timeout` generoso ou rode em segundo
  plano e espere). **Rode inteiro e isolado, e leia o exit code do próprio
  comando** (`cmd; echo $?`), nunca de um pipe com `tail`. O esperado é `exit=0`.
  Se algum caso do `golden` ou do `diag` passar a falhar, a causa é a sua mudança:
  não enfraqueça o teste; descreva no relatório.

## Relatório

`tools/agents/reports/T51-consumes-arg-and-deref-store.md`, em pt-BR: o que mudou
(arquivo por arquivo), a saída final exata dos três comandos de aceitação, e todo
ponto de dúvida ou incoerência que achar (por exemplo, outro lugar que escreva
`invalid = true` ou leia `invalid` e precise do mesmo cuidado). Não faça commit.
