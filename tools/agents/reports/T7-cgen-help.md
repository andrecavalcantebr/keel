# T7 — relatório: `--cgen-help` no driver (M0)

## O que mudou

Único arquivo editado: `tools/cgen/src/tool/main.c`.

1. **`static void print_help(void)`**, inserida antes de `int main` (linhas 31–61).
   O texto foi gerado a partir de `tools/cgen/test/cli/help.expected` por um
   script (`sed`: escapa `\` e `"`, acrescenta `\n` ao fim de cada linha), e não
   redigitado. É uma única chamada `fputs(..., stdout)` com uma literal por linha
   do `.expected`. As 26 linhas e o `\n` final batem byte a byte.
2. **Ramo `--cgen-help`** logo depois do bloco de `--cgen-version` (linhas 96–100):
   `print_help(); exit(0);`. Como o `--cgen-version`, age dentro do laço de
   `argv`, no momento em que a opção é vista.
3. `--help` e `-h` não foram tocados: continuam caindo em `w[0] == '-'` e vão
   para o `cc`.

Nenhum outro arquivo foi editado. `help.sh`, `help.expected`, a spec e os
documentos ficaram intactos.

## Saída final das aceitações

`make -C tools/cgen -j8` (última execução, sem erro; avisos iguais ao baseline,
ver abaixo):

```
gcc  ./obj/tool/read_source.o ... -lm -o cgen
make: Saindo do diretório '/home/andre/code/cprojects/keel/tools/cgen'
```

`sh tools/cgen/test/cli/help.sh`:

```
ok: help (26 lines, 5 lines of invocation, --help forwarded)
```

`sh tools/agents/verify.sh`:

```
ok   build (4 warnings, baseline 4)
ok   make check (45 lines ok)
ok   golden: 46 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

`git status --short`:

```
 M tools/cgen/src/tool/main.c
?? tools/agents/tasks/T7-cgen-help.md
?? tools/cgen/test/cli/help.expected
?? tools/cgen/test/cli/help.sh
```

Além disso, `./tools/cgen/cgen --cgen-help` sai com 0, stderr vazio e stdout
igual a `help.expected` (`cmp`).

## Avisos de compilação

Os 4 avisos são de `main.c` e já existiam antes da mudança (baseline 4):
`dest_dir` (linha 70), `main_module` (76), `pedantic_names` (80) e `f_flag` (81),
todos "set but not used". Não foram introduzidos aqui e não foram corrigidos,
porque estão fora do escopo do cartão.

## Dúvidas e incoerências

1. **Divergência da ordem de `--profile`.** O `help.expected` (e a validação do
   código, `main.c`, `profile`) usa `--profile=<auto|c11|c23>`, enquanto
   `cgen-tool-spec.md` §4.1 (linha 111) escreve `--profile=<c11|c23|auto>`. Não
   editei o oráculo, como manda o cartão. Decisão para o orquestrador: alinhar a
   spec à ordem do `.expected` (e do código), ou o contrário. Isso é normativo,
   então cabe a André.
2. **A linha `--cgen-version`, `--cgen-help` da §4.1 está em branco** (linha 113,
   sem efeito e sem padrão preenchidos). A `design/cgen-tool.md` §2.2 (linha 116–117)
   dá o efeito ("imprime ... sai 0"), e o texto do `.expected` segue essa
   descrição. Não há conflito de conteúdo, só falta de preenchimento na spec.
3. **Alteração externa no `git status`.** No início da tarefa, `tools/transform/README.md`
   aparecia como modificado (`M`). Na conferência final, ele não aparece mais.
   Não toquei nesse arquivo; a mudança deve ter vindo de fora da tarefa (outro
   agente ou commit). Os arquivos `writer.c` e `mangle.c` citados no cartão também
   não aparecem como modificados, e não foram tocados.
4. Nenhuma outra divergência encontrada entre o `.expected`, a tabela da §4.1 e o
   §2.2 do design para as demais opções.

Nada foi commitado nem enviado.
