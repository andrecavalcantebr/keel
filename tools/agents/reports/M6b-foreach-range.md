# M6b — `foreach` sobre `range` numa variável: relatório

## Estado

Parcial. O código de `foreach.c` foi estendido e a verificação passa (`ALL OK`), mas o
caso `015-foreach-range` continua `wip`: a saída difere do `expected/` em dois pontos,
e um deles é uma contradição entre o golden e o backend que eu não posso resolver dentro
do cartão. Veja "Dúvidas abertas".

## O que mudou

Só `tools/cgen/src/engine/emit/foreach.c` (e este relatório). Nada commitado.

- Cabeçalho do arquivo: a lista do que o módulo baixa agora inclui `foreach (T b : r)` e
  `foreach (T v, I i : r)` sobre `range` por valor.
- `Over` ganhou `value_copy`: `analyze_over` marca `true` quando o detalhe da ilha é
  `(range)`. Para `range`, `edits_over` não emite `*` nem `&`: sai `keel_range keel__c<N> = r;`.
  Um `range` passado por ponteiro é recusado.
- Novo `RangeVar` e `analyze_range_var`: lê o cabeçalho `( T b : r )` (exatamente um binder,
  `:` e a variável), exige o detalhe terminado em `(range)` e a declaração `range r` por
  valor na função, e lê o corpo com `read_body`.
- Novo `edits_range_var` (com o auxiliar `put_s`): `foreach` vira
  `{ size_t keel__f<N> = keel_range_first(r); size_t keel__l<N> = keel_range_limit(r); for`,
  `auto` vira `size_t`, `: r` vira `= keel__f<N>; b < keel__l<N>`, e o `)` ganha `; b++`.
  O corpo é tratado como no `foreach` sobre contêiner (bloco ou uma instrução).
- `k_em_edits_foreach` tenta `analyze_over`, depois `analyze_range_var`, depois o literal.
- `k_em_handles_foreach` aceita a forma de uma variável `range` (cabeçalho com um ou dois binders).

## Saída final dos comandos

`sh tools/agents/verify.sh`:

```
ok   build (0 warnings, baseline 0)
ok   make check (48 lines ok)
ok   golden: 46 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

Os `wip` são 011, 013 e 018/021 do golden (fora deste cartão). O `015` não aparece nessa
tabela da worktree (ela é mais antiga que a da `master`); na `master` ele está `wip`.

Caso 015, gerado à mão a partir do `.k` e comparado com `expected/`:

- Só as linhas 27 e 28 de `app_ap.c` diferem, e só nos números dos contadores:
  o cgen emite `keel__f0`/`keel__l0` e `keel__c1`/`keel__n1`; o `expected/` tem
  `keel__f2`/`keel__l2` e `keel__c3`/`keel__n3`. O restante das duas linhas bate byte a byte
  nos dois perfis (depois de renomear os contadores, `diff` fica vazio).
- O `.h` e o `.type.h` batem.

## Dúvidas abertas (decisão do orquestrador)

1. **Numeração do contador `keel__<x><N>`: por função ou por módulo.** O `expected/` do 015
   numera por módulo: `traverse` usa 0 e 1, e `sum_range` (outra função) começa em 2 e 3. O
   golden 019-walk (não listado na tabela) também numera por módulo (c0, c1, c2 em três
   funções). Mas o backend, §2.3, regra 2, diz "um contador por função", e
   `k_em_capture_ordinal` (`emit_core.c`) implementa por função. Corrigir exige mudar
   `emit_core.c`, fora da lista deste cartão, e mudaria o contador do `defer` também. Não
   mexi. Preciso de decisão: ou o backend passa a dizer "por módulo" (e `emit_core.c` muda),
   ou o golden 015 e o 019 são corrigidos para "por função" (`f0`/`l0`, `c1`/`n1`).
2. **Cópia do `range` no temporário de dois binders.** O golden emite
   `keel_range keel__c3 = r;` (valor). O backend §5.7, regra 1, diz que o contêiner "vai para
   um ponteiro temporário `keel__c<N>`", e a regra 6 manda o `range` de dois binders pela
   regra 1. Implementei a cópia por valor porque é o que o `expected/` pede; o texto do
   backend diz outra coisa. Isso é normativo: precisa de decisão.

Não encontrei a implementação do cartão para a forma de um binder com `range` por ponteiro:
ela é recusada de propósito (`implementation-limit`), como antes.

## Observação

Enquanto eu rodava `verify.sh`, outro `verify.sh` de outra sessão (M6a) estava ativo no
mesmo sistema. Não o toquei.
