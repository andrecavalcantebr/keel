# T50 — relatório do oráculo do caso 071

Autor do oráculo, sem implementação. Não rodei `tools/cgen/cgen` nem li
`tools/cgen/src/`. O esperado saiu da spec (§4.14, §5.1), do backend (§2.1.1,
§4.1, §4.3, §4.3.2, §4.4, §5.19, §6, §9.1) e dos análogos 065 e 067. Não
commitei.

## Arquivos criados

Em `golden/cases/071-protocol-function-instance/`:

- `app/ibox.k` e `app/ibox_use.k`, exatamente como no cartão.
- `proof.c`: retorna 1 se `app_ibox_use_probe_count() != 3`, senão 0.
- `NOTES`: o que o caso afirma, arquivo por arquivo, e a explicação de cada esperado.
- `expected/c23/app/` e `expected/c11/app/`, nove arquivos cada, escritos separadamente:
  `app_ibox.{type.h,h}`, `app_ibox_box_i32.{type.h,h}`,
  `app_ibox_use_count_app_ibox_box_i32.{type.h,h}`,
  `app_ibox_use.{type.h,h}` e `app_ibox_use.c`.

Não há `WIP` nem `line-check.allow` (não há código injetado).

Editado: `test/cgen/gen/golden-cases.txt`, com a linha
`071-protocol-function-instance|app/ibox_use.k|-|wip`, no formato do cartão.

## Saída do `golden/run.sh`

Saída completa em `/tmp/t50-run.txt`. Exit 0.

```
ok     071-protocol-function-instance c23  (run)
ok     071-protocol-function-instance c11  (run)
ok     layout-check                   (codegen E6)
ok     line-check                     (backend §6)

142 ok, 0 fail, 0 xfail, 0 xpass, 0 wip
```

Como o caso não tem `|wip` no `run.sh` (ele não tem `WIP`), foi executado e compilado normalmente, nos dois perfis. Também conferi, à parte, que os três headers do 071 compilam isolados com `-pedantic-errors -Wall -Wextra`.

## Decisões de texto (o que o esperado afirma)

- Nome da instância: `app_ibox_use_count_app_ibox_box_i32`. Prefixo do módulo da função, sem instância (backend §5.19, regra 1), aridade única sem sufixo (§2.1.1, regra 1).
- Diretório `app/`: pelo componente-pai de `app.ibox_use` (§4.1, regra 5). O exemplo de §5.19, regra 4, escreve `keel/` porque é o caso de `keel.slice`.
- `.h` da instância, nas quatro seções de §4.3.2, regra 3: próprio `.type.h` e `app/app_ibox_box_i32.type.h` (o parâmetro é por valor, §4.14, regra 7); protótipo; `#include "app/app_ibox_box_i32.h"` (o corpo chama `length`); corpo `return app_ibox_box_i32_length(&items);` (§4.4, `&` pelo verbo `box *`).
- `app_ibox_box_i32.h` tem dois verbos (`length` e `ptr`), ambos `pub inline` e ambos mencionam `box`, então ambos são da instância (§4.3, regra 1). `ptr` tem aridade única, sem sufixo. Não é chamado, mas a regra o põe no header.
- `app_ibox_use.h` declara só `probe_count`; `count` não tem C no módulo (§4.14, regra 1).
- `#line`: `count` na linha 4 do `.k`, `probe_count` na 6, `length` e `ptr` nas 5 e 6 de `app/ibox.k`, o `typedef` na 3. No `.c`, `#line 1` seguido de cinco linhas em branco, que são as linhas 1 a 5 do `.k` (§6, regra 3).
- Perfis: iguais em conteúdo, exceto o nome do perfil no cabeçalho. Nenhum verbo desta forma pode falhar, então sem `[[nodiscard]]` (§9.1).

## Dúvidas e achados

1. **Achado, fora do escopo, não alterado: o header de instância do 065 não se compila sozinho.** `golden/cases/065-protocol-function/expected/c23/pf_span_pf_box.h` declara `static inline i32 pf_span_pf_box(pf_box items);` antes de qualquer include que defina `pf_box`. Seu `.type.h` inclui só `keel.type.h`, e `pf.h` entra depois do protótipo. Compilado isolado, o gcc dá `pf.k:12:34: error: unknown type name ‘pf_box’`. O `run.sh` passa porque `pf.c` e `proof.c` incluem `pf.h` antes. Isso contraria §4.3.2, regra 3, item 1 (o `.type.h` do tipo por valor entra antes dos protótipos) e §4.3, regra 3 ("não depende de ordem de inclusão"). A verificação I2 do `run.sh` não pega, porque só compara a ordem dos `.type.h` entre `.h`s. O 071 segue a spec. Vale decidir se o 065 é corrigido.

2. **`app_ibox_use.c` não inclui `app/app_ibox_box_i32.h`.** O `.c` declara uma variável local `app_ibox_box_i32 b`, mas não chama nenhum verbo desse tipo. §4.3.2, regra 4 diz que o `.c` inclui o `.h` de "cada módulo ou instância que usa", e a palavra "usa" pode ler-se como uso de tipo. A regra 7 diz que os `#include` são derivados das chamadas já resolvidas. Escolhi a regra 7, por ser a mais específica. O tipo chega ao `.c` pelo `.type.h` de `count`, que é transitivo. Se a leitura certa for a de tipo, o `.c` ganha uma linha, e o caso muda.

3. **Campo e verbo com o mesmo nome (`length`).** O modificador tem `size_t length;` e o módulo tem o verbo `length(box *b)`. A spec não proíbe, e em C não há colisão (membro de struct e função em espaços distintos). Registro só para revisão: o caso depende disso, e nenhum golden anterior exercitava.

4. Nada indeterminado nos dois perfis: não há diferença de perfil além do cabeçalho.
