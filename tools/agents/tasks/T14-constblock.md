# T14 — `constexpr` de bloco sob C11

Leia primeiro `T13-16-contexto.md`.

## Objetivo

Preencher `tools/cgen/src/engine/emit/constexpr_block.c`: o `constexpr` em escopo
de bloco, dentro de função (backend §9.2, regras 4 a 6). Sob **C23** a declaração
sai verbatim e nada muda (as funções não fazem nada, e `supported` já é `true`).
Sob **C11** vira macro de nome gerado, com os usos reescritos.

## Arquivos

Edita só `tools/cgen/src/engine/emit/constexpr_block.c`; cria
`tools/agents/reports/T14-constblock.md`. Oráculo (não altere):
`test/cgen/gen/constblock/` (`cb.k`, `expected/{c23,c11}/`), já na tabela.

## Contrato (C11)

Fonte e esperado (o `cb.c` do oráculo; leia-o inteiro, é a sua verdade):

```keel
pub size_t twice(void) {                     // linha 3
    constexpr size_t N = 8;                  // linha 4
    int buf[N];
    return sizeof(buf) / sizeof(int) + N;
}                                            // linha 7
pub size_t other(void) {                     // linha 9
    { constexpr size_t N = 2; return N; }    // linha 10
}                                            // linha 11
```

```c
size_t cb_twice(void) {
#define keel__N_0 ((size_t)8)
#line 4 "cb.k"
    static const size_t keel__N_0__chk = 8; (void)keel__N_0__chk;
    int buf[keel__N_0];
    return sizeof(buf) / sizeof(int) + keel__N_0;
#undef keel__N_0
#line 7 "cb.k"
}
...
size_t cb_other(void) {
#define keel__N_1 ((size_t)2)
#line 10 "cb.k"
    { static const size_t keel__N_1__chk = 2; (void)keel__N_1__chk; return keel__N_1; }
#undef keel__N_1
#line 11 "cb.k"
}
```

Quatro peças:

1. **O ordinal.** `keel__N_<k>`: `k` é a posição do `constexpr` entre **todos** os
   `constexpr` de corpo de função do módulo, em ordem de tokens (no oráculo, os
   dois `N` são `_0` e `_1`; igual ao golden 008). Calcule-o contando tokens
   `constexpr` desde o começo do módulo (um `constexpr` de arquivo é nó
   `K_AST_CONSTEXPR` de topo, e não conta). *Atenção: o backend §2.3 regra 2 diz
   “contador por função”, e o golden 008 mostra contador do módulo. Siga o
   oráculo e registre a divergência no relatório.*
2. **Os usos.** Todo token `N` (mesmo texto do nome) **depois** do statement e
   até o `}` que fecha o bloco que contém o `constexpr`, **exceto** quando o
   token anterior é `.` ou `->` (membro) — vira `keel__N_<k>` (`EDIT_TEXT`, o
   texto em `k_em_scratch`). Fora desse intervalo `N` não muda. O bloco é o do
   `{` mais interno ainda aberto na posição do `constexpr` (percorra os tokens
   contando chaves).
3. **O statement** `constexpr T N = INIT;` (numa só linha) vira, por edições:
   `constexpr` → `static const`; o nome `N` → `keel__N_<k>__chk`; e logo depois
   do `;` insira ` (void)keel__N_<k>__chk;`. (`T` e `INIT` ficam como estão,
   salvo os renomes que outras edições façam.)
4. **As linhas inteiras.** `k_em_pre_lines_constblock(e, ln, a, b)`, chamado antes
   do texto da linha `[a, b)` (que é a linha `ln` do `.k`): se a linha contém um
   `constexpr` de bloco, escreva antes dela, com
   `k_emit_at(&e->w, ln, texto)` (texto montado em `e->buf` com
   `k_em_put_bytes`, terminado em `\n`):
   `#define keel__N_<k> ((T)INIT)`, onde `T` são os tokens entre `constexpr` e o
   nome (sem o branco final) e `INIT` os tokens depois do `=` até antes do `;`,
   **entre parênteses se forem mais de um token**, copiados por
   `k_em_put_range` (que já aplica os renomes de módulo e de outros blocos).
   Para o `#undef`: no `}` que fecha o bloco, se ele é o **primeiro token da
   linha**, escreva `#undef keel__N_<k>` em `k_em_pre_lines_constblock` dessa
   linha; senão (o `}` está no fim de uma linha que tem mais coisa, como em
   `other`), em `k_em_post_lines_constblock` **dessa** linha, com o número de
   linha `ln + 1`. Quem escreve o `#line` de ressincronização é o `KWriter`
   (`k_emit_at` o escreve só quando o contador diverge): **você nunca escreve
   `#line` à mão**.

Acrescente `k_em_constblock_rename(e, tok, out, cap, len)`: devolve `true` e o
nome `keel__N_<k>` se o token `tok` é um uso de `N` no escopo (a mesma regra da
peça 2), `false` caso contrário e sempre sob C23. É o que o `k_em_put_range` usa
(o T16 copia dimensões `N` por ele).

`k_em_supported_constblock`: sob C11, `true` se **todo** `constexpr` do corpo é
da forma `constexpr T N = INIT;` numa só linha; `false` nos outros casos (a
função continua recusada).

## Passos sugeridos

1. Rode `--stop-after=parse` e imprima os tokens do `cb.k` (índices, offsets).
2. Ordinal e escopo (funções puras sobre tokens); depois o rename; depois o
   statement; depois as linhas inteiras. `sh test/cgen/gen_golden.sh -std` dos
   dois perfis a cada passo.
3. Compile o gerado: `gcc -std=c11 -Wall -Igolden/c11 -I<dest> -c cb.c`.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # -> test/cgen/gen/constblock ok (c23 e c11); os demais como estavam
sh tools/agents/verify.sh       # -> ALL OK
```

## Relatório

`tools/agents/reports/T14-constblock.md`, com a divergência do ordinal (peça 1)
e qualquer outra incoerência entre o oráculo, o §9.2 e o golden 008
(`golden/cases/008-block-constexpr/expected/c11/app/app_cx.c`).
