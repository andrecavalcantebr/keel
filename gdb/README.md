# gdb — depuração pelo nome keel

**Estado: esqueleto, não funcional.** `keel_gdb.py` só carrega o
`.keelsyms`; o resto é marcação de "falta fazer" para o cartão que
implementa.

## O que isto é

Um script para o GDB que lê o `.keelsyms` que o `cgen` escreve ao lado de
cada `.c`/`.h`/`.type.h` (`cgen-tool-spec.md` §4.11) e usa os dois nomes — o
keel e o manglado — para anotar `bt`/`frame`, e para resolver um nome keel
num breakpoint.

**Não é normativo.** O contrato é o formato do `.keelsyms`, que a
`cgen-tool-spec.md` já especifica por inteiro. O que vive aqui é só o
consumidor — do mesmo jeito que `editors/generate.py` lê o vocabulário da
spec sem ser ele mesmo normativo (`editors/README.md`).

## Uso previsto

```
(gdb) source gdb/keel_gdb.py
(gdb) keel-load <diretório de --dest-dir>
keel: 3 símbolos carregados de gen/
(gdb) bt
#0  coll.stack(i32)::push (keel_coll_stack_i32_push) (coll.c:42)
(gdb) keel-break coll.stack(i32)::push
```

`keel-load` já funciona (lê todo `.keelsyms` sob o diretório, recursivo). O
`bt` anotado e o `keel-break` ainda são só a intenção — ver "O que falta".

## O que falta

- **Frame filter** (`gdb.FrameDecorator`): anotar `bt`/`frame` com os dois
  nomes, sem substituir um pelo outro (quem só programa em keel ignora o
  manglado; quem importa C existente precisa dele).
- **`keel-break <nome>`**: aceitar tanto o nome bonito quanto o manglado,
  resolver pelo dicionário e chamar `gdb.Breakpoint` sobre o símbolo real —
  o manglado é sempre quem recebe o breakpoint de fato.
- **Script de verificação**, no molde de `editors/verify.mjs`: carregar o
  `.keelsyms` do exemplo da `cgen-tool-spec.md` §4.11 (`app_cfg.keelsyms`) e
  conferir o dicionário resultante, sem precisar de um binário de verdade
  para rodar o gdb.
- **Pretty-printers** dos tipos de runtime (`keel_slice_char`,
  `keel_outcome_u32`, um `keel_routine_slot_*`) são um eixo separado — mostram
  *valor*, não *nome* — e não têm cartão ainda.

## Por que fica fora dos quatro documentos normativos

`keel-spec.md`, `keel-rationale.md`, `keel-c-backend.md` e
`cgen-tool-spec.md` têm dono: a linguagem, a razão, o que o backend tem que
produzir, o que a invocação faz. Nenhum desses donos é "como um depurador
externo lê o que o cgen produziu" — é o mesmo motivo por que `editors/` não
está em nenhum deles. O que entra no normativo é só o formato do
`.keelsyms` em si (`cgen-tool-spec.md` §4.11), porque esse é o contrato real
entre o cgen e qualquer consumidor, este script incluído.
