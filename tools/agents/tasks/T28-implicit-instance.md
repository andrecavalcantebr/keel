# T28 — instância implícita de `keel.array` no range-index

## Objetivo

`v[a..b]` e as formas abertas sobre um `array` chamam `keel_array_<T>_as_slice2` e fazem o `.c` incluir `keel/keel_array_<T>.h`, mas o cgen **não escreve** esse header: o range-index não registra a instância `keel.array <T>` no fecho. O `gen_golden.sh` agora compila o C que cada caso escreveu (`cc -fsyntax-only -I <árvore gerada>`) e os casos **044-array-range** e **046-array-leftovers** falham por isso (estão com `|wip`; o conteúdo dos arquivos esperados já bate). Faça o cgen escrever os headers da instância usada.

Oráculo: `sh test/cgen/gen_golden.sh` com `044` e `046` **sem** `|wip` e sem `FAIL`; e `sh tools/agents/survey/run3.sh` (com `CGEN=$PWD/tools/cgen/cgen`) com `a09` em `OK` (hoje `BAD`: o C inclui `keel/keel_array_i32.h`, que não existe). Os headers escritos têm de ser os mesmos de `golden/c23/keel/keel_array_i32.{h,type.h}` e `golden/c11/…` (a base já os escreve por `--instance "keel.array i32"`).

## Arquivos

Pode editar: o que registra o fecho de instâncias (`tools/cgen/src/engine/parser/parser_islands*.c`, `instances.c`) e/ou o ponto de escrita (`tools/cgen/src/tool/stop_gen.c`, `emit_instances`); se for inevitável, `emit/uses.c`. Pode apagar o `|wip` das linhas `044-array-range` e `046-array-leftovers` de `test/cgen/gen/golden-cases.txt`. Pode criar `tools/agents/reports/T28-implicit-instance.md`. Não toque: normativos, `golden/**/expected/**`, `proof.c`, outros casos. Não faça commit nem `git add`. Se um teste (unit/ ou cli/, por exemplo `test/cgen/cli/islands.py`, que fixa o texto das ilhas) afirmar o contrário, não o edite: guarde o patch fora do repositório e diga no relatório.

## Contrato

- O range-index sobre `array` usa a **família de verbos de um tipo de elemento** (`keel.array` não declara tipo nem modificador: backend §5.2 regra 8). A instância é `keel.array <T>`, com `T` o tipo do elemento do `array` indexado (`i32` nos casos; o mesmo vale para os outros tipos que o gerador já sabe nomear).
- Onde o parser já registra a instância de uma chamada de verbo `array.get(v, i)` (golden 025 usa `array.get`): o range-index deve registrar a mesma entrada de fecho.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # 044 e 046 sem wip; 0 fail
golden/run.sh                   # "106 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
CGEN=$PWD/tools/cgen/cgen sh tools/agents/survey/run3.sh | grep a09   # OK nos dois perfis
```

## Relatório

`tools/agents/reports/T28-implicit-instance.md`, em pt-BR: o que mudou, a saída dos comandos, dúvidas. Entregue UMA vez pelo SubagentHandback e termine.
