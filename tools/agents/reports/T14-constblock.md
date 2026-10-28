# T14 — `constexpr` de bloco sob C11: relatório

## O que mudou

Arquivo único: `tools/cgen/src/engine/emit/constexpr_block.c`. Nenhum outro
arquivo foi editado. Nada foi commitado. O oráculo `test/cgen/gen/constblock/`
não foi alterado.

Funções preenchidas:

- `k_em_supported_constblock`: sob C11, `true` só se todo `constexpr` do corpo
  tem a forma `constexpr T N = INIT;` numa linha (tipo com pelo menos um token,
  identificador com até 200 bytes, `=`, inicializador com pelo menos um token,
  `;`).
- `k_em_constblock_rename`: devolve o nome `keel__N_<k>` quando o token é um uso
  de `N` no escopo (regra 2 do cartão). Sob C23 devolve `false`.
- `k_em_edits_constblock`: no `constexpr` troca `constexpr` por `static const`,
  troca o nome por `keel__N_<k>__chk` e insere ` (void)keel__N_<k>__chk;` logo
  após o `;`. Os usos viram `EDIT_TOKS` de um token, e quem renomeia é o
  `k_em_put_range`.
- `k_em_pre_lines_constblock`: escreve os `#define` da linha do `constexpr` e os
  `#undef` do `}` que é o primeiro token da linha. Tudo numa só chamada a
  `k_emit_at`, na linha `ln`.
- `k_em_post_lines_constblock`: escreve os `#undef` dos `}` que não são o
  primeiro token da linha, com `ln + 1`.
- Sob C23 os ganchos não fazem nada, e `supported` continua `true`.
- `k_em_handles_constblock` continua `false`: o parser não emite ilha para o
  `constexpr` de bloco (conferido com `--stop-after=parse` em `cb.k`, que mostra
  só `module` e as duas `decl`).

Funções auxiliares internas: `body_of`, `cb_parts`, `cb_ordinal`, `cb_name`,
`cb_use`, `put_define`, `put_undefs`, `first_tok_from`, `lines_constblock`.

Não houve falta na API de `emit_internal.h`. Não foi preciso contornar nada em
`emit.c`, `emit_core.c` ou `emit_internal.h`.

## Divergência do ordinal (peça 1 do cartão)

O cartão pede o ordinal `<k>` como posição entre os `constexpr` de bloco do
módulo. O backend §2.3 regra 2 diz "contador por função, na ordem de emissão".
O oráculo `cb.c` e o golden 008 (`app_cx.c`) mostram um contador do módulo: o
`N` de `twice` é `_0` e o de `other` é `_1`. Segui o oráculo e o golden, como o
cartão manda. A divergência fica para o orquestrador decidir se o §2.3 regra 2
muda ou se o golden cai. Não toquei em normativo.

Outras incoerências encontradas:

- Nenhuma no §9.2 que afete este caso. O exemplo do §9.2 usa `keel__N_0` numa
  única função, então não distingue os dois contadores.
- O golden 008 usa `N` nas duas funções também, e o resultado é o mesmo do
  oráculo.

## Saída das aceitações

`sh test/cgen/gen_golden.sh` (antes da mudança, para comparação):

```
wip    003-alignment
wip    006-dim
wip    007-instance
wip    008-block-constexpr
wip    test/cgen/gen/array
wip    test/cgen/gen/foreach
wip    test/cgen/gen/index
wip    test/cgen/gen/constblock
2 ok, 0 fail, 8 wip
```

`sh test/cgen/gen_golden.sh` (depois):

```
wip    003-alignment
wip    006-dim
wip    007-instance
wip    008-block-constexpr
wip    test/cgen/gen/array
wip    test/cgen/gen/foreach
wip    test/cgen/gen/index
3 ok, 0 fail, 7 wip
```

`test/cgen/gen/constblock` saiu de `wip` e está `ok` nos dois perfis. Os outros
casos estão como estavam. `008-block-constexpr` continua `wip` porque depende
de `array`, `foreach` e índice, que são de outros cartões.

Compilação do C11 gerado (`cb.c`), com `-std=c11 -Wall -Wextra`:

```
rc=0
```

Sem aviso.

Teste de borda, fora do oráculo, em `/tmp/t14-edge/`: sombreamento de `N` num
bloco interno e INIT de mais de um token. A saída ficou:

```c
#define keel__N_0 ((size_t)8)
#line 4 "ed.k"
    static const size_t keel__N_0__chk = 8; (void)keel__N_0__chk;
    size_t a = keel__N_0;
#define keel__N_1 ((size_t)(1 << 2))
#line 6 "ed.k"
    { static const size_t keel__N_1__chk = 1 << 2; (void)keel__N_1__chk; a = a + keel__N_1; }
#undef keel__N_1
#line 7 "ed.k"
    return a + keel__N_0;
#undef keel__N_0
#line 8 "ed.k"
```

`sh tools/agents/verify.sh`: `ALL OK` (ver "Saída do verify" abaixo).

## Saída do verify

```
ok   build (2 warnings, baseline 4)
ok   make check (48 lines ok)
ok   golden: 46 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

Os 2 avisos de build estão abaixo da linha de base (4). Nenhum aviso novo.

## Dúvidas abertas

1. O ordinal: §2.3 regra 2 (por função) contra o oráculo e o golden 008
   (por módulo). Segui o oráculo.
2. Colisão com `RENAME` de módulo: `node_edits` renomeia tokens de função cujo
   nome coincide com uma declaração do módulo (`k_em_declared_at`, sem escopo).
   Se um `constexpr N` de bloco sombreia um `N` de módulo, o token de uso recebe
   duas edições no mesmo `at` (o `RENAME` e o `EDIT_TOKS`). Isso não aparece no
   oráculo, e não testei. Se acontecer, o `copy_edited` sai errado. É a limitação
   de escopo registrada em `emit_core.c` (`k_em_declared_at`), não algo deste
   cartão.
3. Nome longo: o nome do `constexpr` é limitado a 200 bytes em
   `k_em_supported_constblock`, para que o nome gerado caiba em `NAME_CAP`.
   Acima disso a função é recusada, em vez de sair truncada.
4. Cada `#define`/`#undef` de uma linha é escrito numa só chamada a `k_emit_at`.
   Assim não aparecem vários `#line` seguidos na mesma linha do `.k`.
