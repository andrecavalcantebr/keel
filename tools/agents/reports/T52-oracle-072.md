# T52 — relatório

O agente Haiku foi interrompido (pausa da sessão) antes de escrever este
relatório, mas tinha terminado o trabalho do cartão. Eu (Opus) conferi o
resultado e completo o relatório.

## Arquivos criados

- `golden/cases/072-protocol-function-implicit-inline/pf2.k`: espelha
  `golden/cases/065-protocol-function/pf.k`, com `span` **sem** `inline`.
- `proof.c`, `NOTES`.
- `expected/c23/` e `expected/c11/`: `pf2.h`, `pf2.type.h`, `pf2.c`,
  `pf2_span_pf2_box.h`, `pf2_span_pf2_box.type.h`.

Conferi `expected/c23/pf2.h` e `pf2_span_pf2_box.h` contra os equivalentes do
065: a única diferença de forma é o deslocamento de linha (o 072 não tem o
comentário de bloco do 065 antes de `span`, e `span` não escreve `inline`,
mas a assinatura C emitida é idêntica — `static inline i32
pf2_span_pf2_box(pf2_box items)`), exatamente como a regra 8 exige.

## Edição

`test/cgen/gen/golden-cases.txt`: linha
`072-protocol-function-implicit-inline|pf2.k|-|wip` acrescentada.

## Aceitação

```
$ golden/run.sh
...
ok     072-protocol-function-implicit-inline c23  (run)
ok     072-protocol-function-implicit-inline c11  (run)
ok     layout-check                   (codegen E6)
ok     line-check                     (backend §6)

144 ok, 0 fail, 0 xfail, 0 xpass, 0 wip
```

## Dúvidas

Nenhuma: a forma é uma cópia direta do 065, só sem a palavra `inline`.
