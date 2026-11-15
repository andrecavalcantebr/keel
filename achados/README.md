# Achados das bibliotecas

Este documento não é normativo. É o canal dos achados que vêm das bibliotecas
que usam keel (`../keel_libs`: `keel_coll`, `keel_maths`) e dos usos locais do
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
`keel_coll/docs/achados/` ou `keel_maths/BUG-CGEN-DIMENSOES.md`). Aqui só entra
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

| 2026-11-15 | `keel_coll` | A1: `slice const T` com `T` struct emite o tipo `const_<módulo>_<struct>`, inexistente (`docs/achados/slice_const_struct/`) | bug | aberto; caso do golden pendente |
| 2026-11-15 | `keel_coll` | A4: tipo de modificador de outro módulo genérico (`pair.pair K V`) como retorno não é emitido (`cgen_limites/src/r3/`) | bug | aberto; caso do golden pendente |
| 2026-11-15 | `keel_coll` | A6: parâmetro de ponteiro de função repassado como argumento recebe `&` (`cgen_limites/src/r5_fnptr_arg.k`) | bug | aberto; caso do golden pendente |
| 2026-11-15 | `keel_coll` | A8: modificador com `dim` usado sem a dimensão passa pelo cgen e falha no `cc` (`cgen_limites/src/r8/`) | bug (diagnóstico ausente) | aberto; teste em `test/cgen/` pendente |
| 2026-11-15 | `keel_coll` | A5, A7, A9: `implementation-limit` sem posição nem construção (`array` com expressão na dimensão passado a `arena.from_array`; `walk` sobre módulo com `dim`) | bug (contraria `cgen-tool-spec.md` §diagnósticos) | aberto; teste em `test/cgen/` pendente |
| 2026-11-15 | `keel_coll` | A2: o cgen emite toda função `inline` da instância, usada ou não | não é bug (decisão da spec: a instância sai por inteiro, `keel-c-backend.md`) | pedido de emissão sob demanda é funcionalidade, `ROADMAP` §4 |
| 2026-11-15 | `keel_coll` | A3: módulo genérico companheiro não infere o tipo do argumento (`from-without-target`) | não é bug (o tipo vem do alvo, spec §4.4) | possível lacuna de documentação |
| 2026-11-15 | `keel_coll` | A10: `extent` com coluna de tipo genérico em módulo genérico | sem repro | pista, não investigada |
| 2026-11-15 | `keel_coll` | A11: contrato de `keel.slice` (`next` sem `NULL`, `partition` por passo) difere do da biblioteca | não é bug | documentação da base |
| 2026-11-15 | `keel_coll` | B6: comparador como parâmetro de instanciação (`instance coll.set i32 by cmp`) | funcionalidade | `ROADMAP` §4 |
| 2026-11-15 | `keel_coll` | B5: o gcc não propaga `p->eq` através de `init`/`put` no C gerado | pergunta aberta sobre o emissor | a discutir |

Os dois probes de `keel_maths` que esperavam falha (`symbolic_shape_use`,
`matrix_partition_protocol_call_gap`) compilam na 0.2.0: são os achados dos
casos 070 e da regra 8 fechados, e o `check_probes.py` da biblioteca deve
deixar de esperar o erro.
