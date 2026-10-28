# T19 — `defer`, degrau D3: `break`, `continue` e `return` dentro de laço (inline)

Leia primeiro `T13-16-contexto.md`, `T17-defer-d1.md` e `T18-defer-d2.md` (relatórios
de `tools/agents/reports/`), e o `defer.c` atual: o D1 e o D2 estão integrados.

## Objetivo

Estender `tools/cgen/src/engine/emit/defer.c` para o `defer` registrado **dentro de um
bloco aninhado** (corpo de laço) com saídas `break`, `continue` e `return`
(backend §5.5, regras 1, 4, 5; §5.5.2, regras 1, 2, 5). É o caso
`golden/cases/005-defer-loop`. `goto` continua **recusado**.

## Arquivos

Edita só `tools/cgen/src/engine/emit/defer.c`; cria
`tools/agents/reports/T19-defer-d3.md`. Oráculo (não altere):
`golden/cases/005-defer-loop` (`app/loop.k`, `expected/{c23,c11}/app/app_loop.c`).
Na tabela de `gen_golden.sh` o caso leva o campo `wip` (imprime `wip` se difere,
nada se bate); **você não edita a tabela**: o orquestrador tira o marcador. O seu
alvo é `005-defer-loop` sair sem `wip` nos dois perfis. `014` e `017` continuam ok.

## Contrato (leia o `expected` inteiro, é a verdade)

```keel
pub int run(int n) {                        // 16
    int total = 0;
    for (int i = 0; i < n; i++) {           // 18
        int h = acquire(i);
        defer [now int h] release(h);       // 20
        if (h == 0) continue;               // 21
        if (h < 0)  break;                  // 22
        if (h > 90) return -1;              // 23
        total += h;                         // 24
    }                                       // 25
    return total;                           // 26
}
```
```c
        struct { int h; } keel__c0 = { h };         // [now] na linha do defer (D1)
        if (h == 0) {
#line 20 "app/loop.k"
        app_loop_release(keel__c0.h);                // cleanup: indentação e linha do defer
        continue; }                                  // sem #line: o contador já vale 21
        if (h < 0)  {
#line 20 "app/loop.k"
        app_loop_release(keel__c0.h);
#line 22 "app/loop.k"
        break; }
        if (h > 90) { int keel__rv0 = -1;
#line 20 "app/loop.k"
        app_loop_release(keel__c0.h);
#line 23 "app/loop.k"
        return keel__rv0; }
        total += h;
#line 20 "app/loop.k"
        app_loop_release(keel__c0.h);
#line 25 "app/loop.k"
    }
    return total;                                    // fora do bloco do defer: intocado
```

Regras (todas pelo mecanismo `EDIT_LINES` que o T18 usou; não escreva `#line`):

1. **Quando é inline.** A função tem pelo menos um `break`, `continue` ou `goto` em
   qualquer lugar do corpo → o cleanup sai **inline em cada saída** (nunca escada).
   `goto` ainda recusa o módulo.
2. **Quais `defer` estão pendentes** numa saída: os já registrados (antes dela, no
   texto) nos blocos que a saída **deixa**, do mais interno para o mais externo, e dentro
   de cada bloco em ordem inversa de registro:
   - `continue` e `break`: os blocos entre a instrução e o corpo do laço mais interno,
     **inclusive** o corpo;
   - `return`: todos os blocos até o corpo da função;
   - fim natural de um bloco (`}`): os `defer` desse bloco.
3. **A forma**, sempre com chaves no inline: `break;`/`continue;` → `EDIT_LINES` em `at`
   = o token da instrução, `H|{`, um `L<n>|<indent do defer><corpo>` por defer
   pendente, `T|<indentação da linha da instrução>`, e um `EDIT_TEXT " }"` depois do
   `;`. `return expr;` (não `void`) → como no T18 (inline): **uma só** `EDIT_LINES` que
   cobre a instrução, `H|{ T keel__rv<n> = <expr>;` … `T|<indent>return keel__rv<n>; }`,
   com `n` o contador **por função** dos temporários inline, de 0, na ordem do fonte
   (aqui há um só). `return;` de `void`: `H|{`, como no T18.
4. **Sem pendentes, intocado.** Uma saída sem nenhum `defer` pendente (o
   `return total;` da linha 26) não muda.
5. **O fim natural do corpo do laço** (`}` da linha 25) cleana como no D1: `EDIT_LINES`
   no `}` com os `L` e `T|<indentação original da linha do }>`.
6. `[now]` e `[later]`: como no D1 (a struct `keel__c<N>` na linha do `defer`, com `<N>`
   por função; o corpo do cleanup usa `keel__c<N>.h`).

`k_em_handles_defer`: `true` para as ilhas das funções dentro deste escopo (inline
com blocos aninhados; `break`/`continue`/`return`); `false` com `goto`. O escopo do T18
(escada, defer no bloco da função) **não pode regredir**: 014 e 017 seguem ok. Onde o T18
recusava (`defer` aninhado numa função com `return` e sem `break`/`continue`: seria
escada por bloco), continue recusando e registre.

## Passos sugeridos

1. Leia o `defer.c` e o `expected` de 005 (c23 e c11, iguais salvo o cabeçalho).
2. Pendentes por bloco: uma função pura sobre tokens (pilha de chaves) que, dada uma
   posição, devolva os `defer` visíveis dos blocos que a saída deixa.
3. `continue`/`break`; depois `return`; depois o fim do corpo do laço.
4. `sh test/cgen/gen_golden.sh` a cada passo.
5. Fora do oráculo, em `/tmp`: laço aninhado com dois `defer` em blocos distintos e um
   `break` do laço interno (só cleana o interno); `goto` (deve recusar). Compile o gerado
   com `-std=c2x` e `-std=c11`.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # -> 005, 014, 017 sem wip (c23 e c11); os demais como estavam
sh tools/agents/verify.sh       # -> ALL OK
```

## Relatório

`tools/agents/reports/T19-defer-d3.md`: edições, saídas coladas, dúvidas.
