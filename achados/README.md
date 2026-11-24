# Achados das bibliotecas

Este documento não é normativo. É o canal dos achados que vêm das bibliotecas
que usam keel (`../keel_libs`: `coll`, `maths`) e dos usos locais do
próprio André. Quem abre uma issue no GitHub é quem está fora do repositório;
aqui ficam os casos locais, que não precisam passar por lá.

As bibliotecas são independentes de propósito (`ROADMAP.md`, §3). O que elas
trazem para este repositório é **relatório de bug**: o cgen travou, gerou C
errado, deu um diagnóstico enganoso, ou a spec disse uma coisa e fez outra. Não
trazem pedido de módulo nem de funcionalidade; isso é futuro e tem o caminho
do `ROADMAP.md`, §4.

## Quando um achado entra aqui

Só se reproduzir como **bug do cgen** ou **lacuna ou erro da spec**, com um
repro mínimo. Fica de fora, e permanece no repositório da biblioteca:

- documentação da biblioteca desatualizada em relação ao próprio manifesto;
- o que já é fronteira documentada da linguagem (limites de `implementation-limit`
  anunciados, o que a spec recusa de propósito);
- pedido de módulo ou de construção nova.

Um relato que **não** reproduz também vale registro, com o veredito
"não reproduzido" e a versão em que se tentou, porque impede que a mesma
hipótese seja investigada duas vezes.

## O que o relato leva

É o mesmo que os formulários de issue pedem (`.github/ISSUE_TEMPLATE/bug.yml`),
para que um relato local possa virar issue sem retrabalho:

1. **Versão:** a saída completa de `cgen --cgen-version`. Ela traz `commit`,
   `date` e `cc`, e é o que separa um build de desenvolvimento do lançamento.
2. **O `.k` mínimo** que reproduz, com os módulos que ele importa.
3. **O comando** completo (`cgen ...`).
4. **O esperado e o observado:** a saída do cgen, ou o erro do compilador C
   sobre o C gerado.
5. **Ambiente:** sistema e compilador C que compilou o gerado.

O relato mora no repositório da biblioteca (um `.md` com o repro ao lado, como
`coll/docs/achados/` ou `maths/BUG-CGEN-DIMENSOES.md`). Aqui só entra
uma linha no registro abaixo, apontando para ele.

## O que acontece com um achado

Um achado confirmado vira **caso no `golden/`** (ou teste em `test/cgen/`)
**antes** do conserto, com o esperado escrito à mão, e só depois o cgen é
consertado (`AGENTS.md`, regras de trabalho). Se o conserto pede mudar um
normativo, vale a regra de sempre: discussão, texto proposto, "sim" do André, e
a marca *(desde X.Y.Z)* com a linha no `CHANGELOG.md`. O registro guarda o caso
do golden ou o commit, e a versão em que saiu.

## Registro

| Data | Origem | Achado | Veredito | Resultado |
| --- | --- | --- | --- | --- |
| 2026-11-06 | `keel_maths` | argumento de tipo apagado que precisa de renome (T40-T44) | bug | `type-arg-foreign`, spec §4.4, regra 20 |
| 2026-11-07 | `keel_libs` | `const` perdido em genérico aninhado | bug | caso 067 |
| 2026-11-07 | `keel_libs` | `parallel` sobre vista passada por valor | bug | caso 068 |
| 2026-11-09 | `keel_maths` | dimensões permutadas no retorno de módulo `dim`-genérico (`BUG-CGEN-DIMENSOES.md`) | bug | caso 070 |
| 2026-11-13 | `keel_coll` | função sobre protocolo com tipo concreto instância de outro módulo | bug | caso 071, 0.2.0 |
| 2026-11-13 | `keel_coll` | função sobre protocolo `pub` sem `inline`: aceita e falha só no link | lacuna da spec | caso 072, §4.14 regra 8, 0.2.0 |
| 2026-11-14 | `keel_coll` | `keel.slice const T` perde o `const` em módulo genérico (`docs/achados/`, item 1) | não reproduzido | repro em 0.1.2 e 0.2.0; sem ação |
| 2026-11-15 | `keel_coll` | A1: `slice const T` com `T` struct emite o tipo `const_<módulo>_<struct>`, inexistente (`docs/achados/slice_const_struct/`) | bug | `test/cgen/cli/slice_const_struct.sh`, 0.3.0 |
| 2026-11-15 | `keel_coll` | A4: tipo de modificador de outro módulo genérico (`pair.pair K V`) como retorno não é emitido (`cgen_limites/src/r3/`) | bug | `test/cgen/cli/generic_companion_return.sh`, 0.3.0 (dois defeitos: include da instância e alvo de tipo com binders) |
| 2026-11-15 | `keel_coll` | A6: parâmetro de ponteiro de função repassado como argumento recebe `&` (`cgen_limites/src/r5_fnptr_arg.k`) | bug | caso 073, 0.3.0 |
| 2026-11-15 | `keel_coll` | A8: modificador com `dim` usado sem a dimensão passa pelo cgen e falha no `cc` (`cgen_limites/src/r8/`) | bug (diagnóstico ausente) | `modifier-dimension-count`, spec §4.3 e §6.2, 0.3.0 |
| 2026-11-15 | `keel_coll` | A5, A7, A9: `implementation-limit` sem posição nem construção (`array` com expressão na dimensão passado a `arena.from_array`; `walk` sobre módulo com `dim`) | bug (contraria `cgen-tool-spec.md` §diagnósticos) | A5: caso 074 (a dimensão `1<<10` passa a ser aceita); A7 e A9: `file:line:col` e a construção na mensagem, `test/cgen/cli/emit_refusal_site.sh`, 0.3.0 |
| 2026-11-15 | `keel_coll` | A2: o cgen emite toda função `inline` da instância, usada ou não | não é bug (decisão da spec: a instância sai por inteiro, `keel-c-backend.md`) | pedido de emissão sob demanda é funcionalidade, `ROADMAP` §4 |
| 2026-11-15 | `keel_coll` | A3: módulo genérico companheiro não infere o tipo do argumento (`from-without-target`) | não é bug (o tipo vem do alvo, spec §4.4) | possível lacuna de documentação |
| 2026-11-15 | `keel_coll` | A10: `extent` com coluna de tipo genérico em módulo genérico | sem repro | pista, não investigada |
| 2026-11-15 | `keel_coll` | A11: contrato de `keel.slice` (`next` sem `NULL`, `partition` por passo) difere do da biblioteca | não é bug | documentação da base |
| 2026-11-15 | `keel_coll` | B6: comparador como parâmetro de instanciação (`instance coll.set i32 by cmp`) | funcionalidade, adiada | sem ação agora; é o caminho do comparador (Rota A) e substitui o molde para esse caso; só com caso real que `==`/`<` direto e a coluna-índice (struct como `extent`) não cubram |
| 2026-11-15 | `keel_coll` | D: `always_inline` emitido pelo cgen para o gcc devirtualizar `p->eq` | recusado | a linguagem fica flexível para alvos embarcados (RISC-V, LLVM); nada de heurística de um `cc` no C gerado. A biblioteca usa `==`/`<` direto e chave escalar |
| 2026-11-15 | `keel_coll` | B5: o gcc não propaga `p->eq` através de `init`/`put` no C gerado | não é bug do cgen: o gcc não inlina o `init` cedo, e o `init` deixa um caminho com `eq` nulo (vale para `u32`, não só struct) | repro e causa em `keel_coll/docs/achados/comparador_conhecido/` (`t_causa.c`); decisão de desenho em B6 |
| 2026-11-19 | `xprk` | `alias.Set.VALOR` de outro módulo resolvia só até o tipo do conjunto e deixava `.VALOR` para o `cc` recusar (`docs/achados/tags-qualificados-entre-modulos/`, §1) | bug | caso 075, 0.3.0 |
| 2026-11-19 | `xprk` | `import M;` sem alias não resolvia nenhuma forma do nome do módulo nos usos qualificados (`docs/achados/tags-qualificados-entre-modulos/`, §2) | bug + lacuna da spec | spec §4.1 regra 5, caso 077, 0.3.0 |
| 2026-11-19 | `xprk` | `types` não injetava o nome de um conjunto `tags` do módulo importado (`docs/achados/tags-qualificados-entre-modulos/`, §3) | bug + lacuna da spec (o rationale já afirmava a regra) | spec §4.1 regra 6, caso 076, 0.3.0 |
| 2026-11-19 | `keel` (a própria base) | `foreach` e `walk` sobre contêiner de módulo com `dim` (`bitbuffer(4) u8`) recusados: o emissor lia o tipo pelo último `(` do detalhe da ilha. Fecha o A7 de verdade, que a 0.3.0 só fizera dizer a posição | bug | caso 078 e `test/cgen/cli/emit_refusal_site.sh`, 0.4.0 |
| 2026-11-19 | `keel` (a própria base) | `x[a..b]` sobre modificador de módulo com `dim` não era reconhecido (passava ao `cc`: `too many decimal points`), embora o `range-index` use uma dimensão (spec §4.5) | bug | caso 078, 0.4.0 |
| 2026-11-19 | `keel` (a própria base) | função da parte fixa de um módulo genérico (que não cita parâmetro nem modificador, regra 20 da §4.3) não é alcançável: chamada qualificada de fora dá `from-without-target`, e de dentro da instância sai com o nome cru e sem o `include` | bug | caso 080, 0.4.0 |
| 2026-11-19 | `keel` (a própria base) | `static_assert` no topo de módulo genérico que cita `W` ou `T`: `implementation-limit` sem posição (o nó não é "carregado" na instância) | bug | caso 080, 0.4.0 |
| 2026-11-19 | `keel` (a própria base) | `buffer.from(v, n)` e `bitbuffer.from(v, n)` com `v` um `array`: o cgen acrescenta `&` e o `cc` avisa de tipo de ponteiro | comportamento da spec (o primeiro argumento de um verbo de construção é lido como contêiner) | escrever `&v[0]`; a spec §5.3 diz `buffer.of(v)` para `array` |
| 2026-11-20 | xprk | conjunto `tags` com 17 ou mais itens não parseia (`unexpected-token` no início da declaração) | bug | limite de 16 itens do parser, agora 128; acima disso, `tags-limit-exceeded` com o limite na mensagem; `test/cgen/cli/tags_many.sh`; 0.4.0 |
| 2026-11-20 | `keel` (revisão da distribuição de bibliotecas) | `.a`, `.so`, `-l`, `-L` e `-Wl,` na linha iam antes dos `.c` gerados: `undefined reference` na ligação | bug | os `.c` gerados vêm antes das entradas de ligação do usuário; `test/cgen/cli/link_order.sh`; manual §7.6; 0.4.0 |
| 2026-11-22 | xprk | declaração com 30 ou mais referências a `tags` (tabela `pub array`): `a source line needs more than MAX_EDITS = 32 edits` | bug (o limite vinha do tamanho da declaração, não da linha) | caso 081 do golden; `k_em_emit_extern` calcula as edições fora dos inicializadores e `MAX_EDITS` deriva de `LINE_CAP`; 0.4.0 |
| 2026-11-23 | auditoria de limites | o nome gerado tem três limites e três diagnósticos | estudo | [E01](E01-nome-gerado-tres-limites.md) |
| 2026-11-23 | auditoria de limites | o `implementation-limit` fala sempre a mesma frase, sem valor nem configuração | estudo | [E02](E02-implementation-limit-generico.md) |
| 2026-11-23 | auditoria de limites (medição) | `extent` com 5 grupos sai sem diagnóstico; mensagens enganosas; tempo quadrático; arena antes dos tetos | bug + estudo | [E03](E03-recusas-silenciosas-e-escala.md) |
| 2026-11-23 | `keel_libs/str` (`keel_achados.md` A1) | `outcome T`, com `T` um `typedef` de outro módulo, gera um `.type.h` sem o include do módulo de `T` (`unknown type name`) | bug reproduzido na 0.4.0-dev (`/tmp/ka2/r1`) | fechado: golden 083, 0.4.0 |
| 2026-11-23 | `keel_libs/str` (A2) | `typedef buffer u8 text;` passa pelo cgen como C cru; o nome não é contêiner | reproduzido; a spec §4.2 reserva `byref` em `typedef`, então pode ser intencional: o cgen deveria recusar com diagnóstico de keel | a decidir (spec ou cgen) |
| 2026-11-23 | `keel_libs/str` (A3) | verbo de módulo genérico com dois modificadores: `two.run(d, 0)` sai `two_run`, símbolo que não existe | bug reproduzido (`/tmp/ka2/r3`) | fechado: golden 084, 0.4.0 |

Os dois probes de `keel_maths` que esperavam falha (`symbolic_shape_use`,
`matrix_partition_protocol_call_gap`) compilam na 0.2.0: são os achados dos
casos 070 e da regra 8 fechados, e o `check_probes.py` da biblioteca deve
deixar de esperar o erro.
