# T7 — `--cgen-help` no driver (M0)

## Objetivo

O cgen não tem ajuda. A spec já prevê a opção, e com este nome:
`cgen-tool-spec.md` §4.1 lista `--cgen-version`, `--cgen-help` no conjunto
fechado das opções do cgen, e o design (`design/cgen-tool.md` §2.2, §2.6) diz:

> | `--cgen-help` | — | imprime a sinopse e esta tabela, sai 0 | — |
>
> `--version` e `--help`, sem o prefixo, **não são do cgen**: vão para o `cc`.

> | só `--cgen-version`/`--cgen-help` | imprime e sai 0 |

Só `--cgen-version` foi implementada. Esta tarefa implementa `--cgen-help`.
**`--help` e `-h` continuam sendo repassados ao compilador C**: não os trate.

## Arquivos

- Editar: `tools/cgen/src/tool/main.c`.
- Escrever: o relatório `tools/agents/reports/T7-cgen-help.md`.
- Nada mais. **Não edite** `test/cgen/cli/help.sh` nem
  `test/cgen/cli/help.expected` (o oráculo, escrito à mão), nem
  nenhum documento. Os arquivos `tools/cgen/src/engine/emit/writer.c`,
  `mangle.c` e `tools/transform/README.md`, que aparecem no `git status`,
  são de outra tarefa: não mexa neles.

## Contrato

1. `--cgen-help`, em qualquer posição da linha, imprime em **stdout** o
   texto de `test/cgen/cli/help.expected`, byte a byte (inclusive o
   `\n` final da última linha), e sai com código 0. Nada em stderr, e o
   compilador C não é chamado.
2. Como `--cgen-version`, a opção age **dentro do laço** de `argv`, no
   momento em que é vista: o que vem depois não é examinado, e a validação
   dos valores (`--checks=maybe`), que é feita depois do laço, não chega a
   rodar. O código existente, em `tools/cgen/src/tool/main.c:63-66`:

   ```c
   if (strcmp(w, "--cgen-version") == 0) {
       printf("cgen 0.1.0\n");
       exit(0);
   }
   ```

   Ponha o `--cgen-help` logo depois desse bloco, no mesmo molde.
3. O texto fica numa `static void print_help(void)` em `main.c`, escrita com
   `fputs`/`printf` sobre `stdout` (uma string literal concatenada, uma linha
   do `.expected` por linha de código, é o mais fácil de conferir). Copie o
   texto do `.expected`; não o reescreva de memória.
4. `--help` não muda: cai no ramo `w[0] == '-'` e vai para o `cc`, como hoje.

## Passos sugeridos

1. `make -C tools/cgen -j8` e `sh test/cgen/cli/help.sh`: hoje falha
   com `FAIL: help — alone: exit code 1, want 0`.
2. Escreva `print_help` e o ramo do `--cgen-help`.
3. Rebuild, e rode `help.sh` até dar `ok`. Se o `cmp` acusar diferença, o
   script mostra o `diff`: a linha com `<` é a esperada.

## Aceitação

```
make -C tools/cgen -j8                 sem erro e sem aviso novo
sh test/cgen/cli/help.sh         última linha começa com "ok: help"
sh tools/agents/verify.sh              termina em "ALL OK"
git status --short                     só main.c (M) além do que já estava
```

## Relatório

`tools/agents/reports/T7-cgen-help.md`, em pt-BR: o que mudou em `main.c`,
a saída final exata das quatro aceitações, e toda dúvida ou incoerência
encontrada (por exemplo, se o texto do `.expected` diverge da tabela da spec
§4.1 em alguma opção).
