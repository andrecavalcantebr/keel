# T39 — instância de módulo genérico: declaração que cita só parâmetro, e `priv inline`

## Objetivo

Faça o cgen produzir, byte a byte nos dois perfis, os casos **`059-generic-param-only`** e **`060-generic-priv-inline`** (oráculos escritos à mão, revisados por mim; leia o `.k`, o `NOTES` e o `expected/c23/` de cada um antes de tudo). Hoje os dois saem com `implementation-limit` (`app_pbox_box_3_i32` / `app_peek_box_i32`: "the emitter does not cover this instance yet").

Regras que valem (backend, recém-escritas, `keel-c-backend.md`): **§4.3 regra 1** (a unidade de header é a instância do módulo; o `constexpr`/`typedef` que cita só parâmetro sai no mesmo par `.type.h`/`.h` do modificador, com o nome do header como prefixo: `app_pbox_box_3_i32_CAP`, `app_pbox_box_3_i32_Pair`), **§4.3 regra 6** (a `priv inline` de genérico sai `static inline` no `.h` da instância: protótipo na seção 2, corpo na seção 4, nome `<instância>_<verbo>`, sem marca de `priv`), §4.4 r1.

## Arquivos

Pode editar `tools/cgen/src/engine/emit/` (particularmente `generic.c`, `decls.c`, `files.c`, `supported.c`; `k_em_generic_out` em `generic.c:58` é a partição de que a instância parte) e o parser se a forma exigir. Pode apagar o `|wip` das linhas `059-…` e `060-…` de `test/cgen/gen/golden-cases.txt`. Não toque: normativos, `golden/**` (inclusive `expected/`, `proof.c`, `line-check.allow`), outros casos. Sem `git add` nem commit. Se um teste de `test/cgen/unit` ou `cli` afirmar o contrário, não o edite: guarde o patch fora do repositório e diga no relatório.

**Se o esperado divergir da spec ou de um golden que já passa, pare nesse ponto e diga no relatório (o orquestrador arbitra).** Em particular não mude a saída de nenhum golden que já passa (007, 010, 051 e os demais): `golden/run.sh` e o `gen_golden` têm de continuar com as mesmas contagens.

## Passos sugeridos

1. Rode `tools/cgen/cgen -std=c11 --base-dir base --stop-after=gen -I golden/cases/060-generic-priv-inline --dest-dir /tmp/t39 golden/cases/060-generic-priv-inline/app/peek_use.k` para ver a recusa e ache de onde ela vem (`supported.c`).
2. Comece pelo 060 (a `priv inline` na instância, que o 051 e o 007 já mostram para `pub`), depois o 059 (o `constexpr` e o `typedef` que citam só parâmetro; para o 059 o nome da constante e do tipo leva o nome do header como prefixo, e o C11 leva o `static const …__chk` como no §9.2).
3. Construa a cada passo (`make -C tools/cgen -j8`), sem warning novo.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # 059 e 060 saem de wip, 0 fail; demais casos inalterados
golden/run.sh                   # "114 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
```

## Relatório

`tools/agents/reports/T39-generic-param-only-priv-inline.md`, em pt-BR: o que mudou arquivo a arquivo, a saída dos comandos, dúvidas. Entregue UMA vez e termine.
