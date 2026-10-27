# T6 — `engine/emit/writer.c` e `engine/emit/mangle.c` (M3)

## Objetivo

Implementar as duas peças mecânicas do emissor do M3, cujos contratos já
estão escritos:

- `tools/cgen/src/engine/emit/writer.h`: o `KWriter`, o único ponto que
  escreve `#line` (codegen-design §8, decisão E4; backend §6);
- `tools/cgen/src/engine/emit/mangle.h`: os nomes de nível de arquivo
  (backend §2.1, §4.1, §4.2, §5.8; cgen design §5.3).

Os oráculos também já existem e são a aceitação:
`test/cgen/unit/emit/emit_writer_main.c` e `emit_mangle_main.c`, rodados por
`emit_writer.sh` e `emit_mangle.sh`. Hoje os dois `.c` estão vazios, e o
`make check` mostra os testes como `wip`.

## Arquivos

Escrever `tools/cgen/src/engine/emit/writer.c` e `mangle.c`, e o relatório
`tools/agents/reports/T6-writer-mangle.md`. Nada mais: **não edite os
headers nem os testes**. Se achar que um deles está errado, pare e diga no
relatório.

## Regras do motor (valem para os dois arquivos)

- `engine/` não faz E/S: nada de `<stdio.h>`, `<stdlib.h>`, `<unistd.h>`,
  `<fcntl.h>` nem `<sys/*.h>` (o `make boundary` recusa). `<string.h>`,
  `<stddef.h>` e `<stdbool.h>` podem.
- Sem alocação. A saída vai para o `keel_slice_char out` de quem chama, e o
  que não cabe é contado e descartado, como no `snprintf`. O modelo é o
  `KOut` de `tools/cgen/src/engine/parser/parser_dump.c:5-9`:

  ```c
  typedef struct { keel_slice_char out; size_t n; } KOut;
  static void put_c(KOut *o, char c) {
      if (o->n < o->out.len) o->out.ptr[o->n] = c;
      o->n++;
  }
  ```

  `keel_slice_char` é `{ size_t len; char *ptr; }`, nessa ordem:
  `(keel_slice_char){ 3, (char *)"MAX" }`.
- Nomes em inglês, `static` para tudo que não está no header, e comentários
  no estilo do resto do motor (curtos, citando a seção do documento).
- Para escrever o número do `#line` sem `snprintf`, converta à mão (o
  `put_uint` de `parser_dump.c:14` faz isso).

## O contrato, em resumo (o header é a fonte)

**writer.** `k_writer_init` zera `n` e `mapped` e marca `at_bol`.
`k_emit_raw` copia o texto, nunca escreve `#line`, avança `mapped` a cada
`'\n'` quando `mapped != 0`, e mantém `at_bol` (verdadeiro se o último byte
escrito foi `'\n'`). `k_emit_at(w, L, text)` recusa (devolve `false` e não
escreve nada) se `!at_bol` ou se `text` não termina em `'\n'`. Senão, quando
`line_on` e `mapped != L`, escreve `#line L "<src_path>"\n` antes; depois o
texto. `mapped` passa a ser `L` mais o número de `'\n'` do texto. A linha do
`#line` em si **não** avança `mapped`, porque é ela que o fixa.

Exemplo numérico (caso 4 do teste): `k_emit_at(45, "f() {\n")` escreve
`#line 45` (mapped era 0) e deixa `mapped = 46`. `k_emit_raw("    CHECK;\n")`
deixa `mapped = 47`. `k_emit_at(46, "    body;\n")`: 47 ≠ 46, então escreve
`#line 46` e deixa `mapped = 47`. `k_emit_at(47, "}\n")`: 47 = 47, sem `#line`.

**mangle.** Concatenação, sem tabela:
- módulo: `.` vira `_`;
- declaração: `<módulo>_<nome>`; com `is_type` e o nome igual ao último
  componente do módulo, só `<módulo>`; no módulo `keel` (o nome inteiro é
  `keel`), só `<nome>`;
- caminho: os componentes-pai do módulo viram diretórios, separados por `/`,
  e o arquivo é o símbolo do módulo mais `ext` (`"keel"` não tem diretório);
- guarda: maiúsculas, e todo byte fora de `[A-Z0-9]` depois da conversão vira
  `_`;
- entrada: `main_` + símbolo do módulo + `.c`.

## Aceitação

```sh
sh test/cgen/unit/emit/emit_writer.sh     # -> ok
sh test/cgen/unit/emit/emit_mangle.sh     # -> ok
sh tools/agents/verify.sh                  # -> ALL OK (o build não pode ganhar aviso)
make -C tools/cgen check | grep emit_      # os dois "ok", nenhum "wip"
```

Os testes rodam com ASan e UBSan (`-fsanitize=address,undefined`): nenhuma
escrita fora do `out.len`.

## Relatório

`tools/agents/reports/T6-writer-mangle.md`, em pt-BR: o que cada arquivo
faz, as decisões que o header deixou para você (e por quê), a saída das
aceitações e as dúvidas sobre o contrato.
