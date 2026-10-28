# T17 — `defer`, degrau D1: saída pelo fim natural do bloco

Leia primeiro `T13-16-contexto.md` (a mecânica de edições e a API; vale igual).

## Objetivo

Preencher `tools/cgen/src/engine/emit/defer.c` (backend §5.5): o `defer` cujo
escopo só termina pelo **fim natural do bloco**. É o caso `golden/cases/014-defer-later`.
`return`, `break`, `continue` e `goto` no escopo são os degraus D2 e D3: nesses
casos o módulo **continua recusado** (`implementation-limit`), nunca emitido errado.

## Arquivos

Edita só `tools/cgen/src/engine/emit/defer.c`; cria
`tools/agents/reports/T17-defer-d1.md`. Oráculo (não altere):
`golden/cases/014-defer-later/` (`app/dl.k` e `expected/{c23,c11}/app/`), já na
tabela de `test/cgen/gen_golden.sh` (`014-defer-later`). Os ganchos
(`k_em_edits_defer`, `k_em_handles_defer`) já estão ligados no `emit.c`.

## Contrato

O fonte (`app/dl.k`, linhas 10 a 29) e o esperado (`expected/c23/app/app_dl.c`,
linhas 13 a 31). **Leia os dois arquivos, são a sua verdade**; o resumo:

```keel
pub void late(void) {                 // 10
    int x = 1;                        // 11
    defer note(x);                    // 12   ← registro, sem lista
    x = 2;
    x = 3;
}                                     // 15
pub void early(void) {                // 17
    int x = 1;
    defer [now int x] note(x);        // 19   ← registro com cópia
    x = 2;
    x = 3;
}                                     // 22
```
```c
void app_dl_late(void) {
    int x = 1;
                                      // a linha do defer fica VAZIA (backend §5.5, regra 2)
    x = 2;
    x = 3;
    app_dl_note(x); }                 // o cleanup e o `}` na mesma linha, a do `}` do fonte
...
void app_dl_early(void) {
    int x = 1;
    struct { int x; } keel__c0 = { x };   // [now]: na linha do defer
    x = 2;
    x = 3;
    app_dl_note(keel__c0.x); }        // o corpo usa as cópias
```

Regras de saída (todas por edição de token, como os outros arquivos):

1. **O registro.** A instrução `defer …;` inteira (da palavra `defer` ao `;`), com a
   indentação que a precede, sai da linha:
   - sem lista, ou `[later]`: a linha fica **vazia** (apague indentação e
     instrução; o comentário final já é apagado pelo emissor, com os brancos);
   - `[now T1 a, T2 b]`: a instrução vira, **mantendo a indentação**,
     `struct { T1 a; T2 b; } keel__c<N> = { a, b };` (`T` e nome verbatim, como
     escritos na lista). `<N>` é o contador **por função** de capturas, de 0, na
     ordem do fonte (backend §2.3, regra 2).
2. **O cleanup.** No `}` que fecha o bloco do `defer` (o `}` do bloco léxico mais
   interno aberto no registro), antes do `}` e na mesma linha, sai o corpo de cada
   `defer` **pendente nesse bloco, em ordem inversa de registro**, cada um
   terminando em `;`, separados por um espaço, e depois um espaço e o `}`:
   `<indent><corpo1> <corpo2> }`. A indentação `<indent>` é a **do `defer`** (4 no
   golden), não a do `}`. O `}` tem de ser o primeiro token da linha; senão, o
   módulo continua recusado.
3. **O corpo do cleanup** é o texto do `defer` depois da lista, verbatim (módulo os
   renomes do módulo: `note` sai `app_dl_note`, o `k_em_put_range` já faz isso),
   sem o `;` final do fonte se houver (acrescente um `;` no fim do cleanup).
   Com `[now]`, cada ocorrência de um nome capturado no corpo (identificador, não
   precedido de `.` nem `->`) sai `keel__c<N>.<nome>`.
4. Só os `defer` pendentes **no ponto de saída** entram: aqui, os que estão
   registrados no bloco (o fim natural vem depois de todos). Um `defer` em bloco
   aninhado cleana no `}` do bloco aninhado.
5. Texto construído vai em `k_em_scratch` (copie do `e->buf` se montar por
   `k_em_put_*`, e **zere `e->n`** depois; a linha em construção do emissor usa
   `e->buf`). O que não couber no scratch (1024 bytes) recusa o módulo.

`k_em_handles_defer(ast, ilha)` — `true` somente para ilhas `K_ISLAND_DEFER` cujo
`defer` está numa função **sem** `return`, `break`, `continue` nem `goto` no
corpo (qualquer um desses tokens em qualquer lugar do corpo da função: por ora,
recusa). Detalhes das ilhas: `defer later` (sem lista ou `[later]`) e
`defer now int x` (com `[now …]`); a âncora é a palavra `defer`.

## Passos sugeridos

1. `tools/cgen/cgen --stop-after=parse -I golden/cases/014-defer-later --base-dir base
   golden/cases/014-defer-later/app/dl.k` e imprima os tokens das três funções.
2. O registro sem lista (linha vazia); depois o cleanup no `}`; depois `[now]`
   (struct + `keel__c0.x`).
3. `sh test/cgen/gen_golden.sh` a cada passo (a linha `014-defer-later`).
4. Recusas: um `.k` em `/tmp` com `return` no corpo, e outro com um `defer` e um
   `}` que não é o primeiro token da linha; confira `implementation-limit`.
5. Compile o gerado: `gcc -std=c2x -Wall` e `-std=c11`.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # -> 014-defer-later ok (c23 e c11); os demais como estavam
sh tools/agents/verify.sh       # -> ALL OK
```

## Relatório

`tools/agents/reports/T17-defer-d1.md`: edições, saídas coladas, dúvidas. Registre
como achou o bloco do `defer`, e qualquer coisa em que o backend §5.5, o §6 e o
golden não concordem.
