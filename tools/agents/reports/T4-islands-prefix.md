# T4 — relatório: prefixo `isl_` nos 103 nomes externos de `parser_islands*.c`

## Resumo

As 103 funções de ligação externa de `parser_islands*.c` (todas as que não
começam por `k_`) receberam o prefixo `isl_`. Só o nome mudou: nenhum corpo,
nenhuma linha foi realinhada, e a única variável que tinha o mesmo nome de uma
função (`binder`) foi preservada. As quatro aceitações passam. Nada foi
commitado.

## Método

1. Snapshot tirado antes de qualquer edição:
   `sh tools/agents/verify.sh snapshot /tmp/keel-snap-T4` (80 fontes).
2. Lista de nomes obtida do objeto compilado, com
   `nm tools/cgen/obj/engine/parser_islands*.o | awk '$2=="T"{print $3}' | grep -v '^k_'`
   (103 nomes; o único `k_` exportado é `k_collect_islands`, que ficou).
3. Renomeação por um tokenizador simples em Python (fora do repositório, em
   `/tmp/t4/rename.py`). Ele percorre cada arquivo ignorando comentários,
   literais de string e de caractere, e renomeia um token `NOME` só quando:
   - o token é um identificador exatamente igual a um dos 103 nomes;
   - não é precedido de `.` nem de `->` (acesso a campo);
   - é seguido de `(` (definição, protótipo ou chamada).
4. Referências que não são chamadas (ponteiro de função, argumento, `&nome`)
   e variáveis homônimas foram tratadas pela compilação e por uma varredura
   separada dos usos "soltos" (ver a seção seguinte). Não ficou nenhuma
   referência de função sem `(`.

Contagem de ocorrências renomeadas, por arquivo (cada uma é `NOME(` que
virou `isl_NOME(`):

| arquivo | renomeações |
|---|---|
| `parser_islands.c` | 284 |
| `parser_islands_checks.c` | 239 |
| `parser_islands_closure.c` | 217 |
| `parser_islands_flow.c` | 807 |
| `parser_islands_index.c` | 167 |
| `parser_islands_internal.h` | 103 (um protótipo por nome) |
| `parser_islands_types.c` | 537 |
| **total** | **2354** |

`git diff --stat`: 7 arquivos, 1590 linhas inseridas e 1590 removidas (as
linhas com mais de um nome renomeado contam uma vez só).

## Nomes que também são variável ou campo

- **`binder`** é o único clash real. Há 15 usos de `binder` sem `(` em
  `parser_islands_checks.c` (3), `parser_islands_flow.c` (7) e
  `parser_islands_types.c` (5). Todos são variáveis locais (`bool binder`,
  `Range binder`) usadas com `=`, `.`, `!`, `&&`, `||`, `if (…)`. Nenhum é
  referência à função `binder`, que virou `isl_binder` (definição no `.c`,
  protótipo no `.h`, chamadas em `types.c`). Ficaram como estavam.
- **Nomes de campo:** zero acessos `.nome` ou `->nome` que coincidam com um
  dos 103 nomes. Não houve caso de campo para separar.
- **Parâmetros e outras variáveis** (`tok`, `walk`, `generic`, `kt`, `emit`,
  `keep` e outros) aparecem no código sempre como variável ou parâmetro, e
  nunca seguidos de `(`; por isso nenhum deles foi tocado.
- **Usos como ponteiro de função:** nenhum. A varredura de usos sem `(`
  encontrou só o `binder` acima. Por isso a aceitação 3 deu saída vazia, sem
  justificativas a registrar.
- **Outros arquivos:** `tools/cgen/src/engine/islands.h` declara só
  `k_collect_islands`. Nenhum outro arquivo chama, declara ou define os 103
  nomes (grep em `tools/cgen/src` e `test/cgen`, sem achados fora de
  `parser_islands*`). `parser_islands_internal.h` é incluído só pelos seis
  `parser_islands*.c` e por `parser_islands.c`.
- **Macros:** nenhum `#define` usa um dos 103 nomes.

## Alinhamento não tocado (não realinhado, conforme o cartão)

Continuações de chamada que estavam alinhadas com a abertura do parêntese
ficaram 4 colunas fora do alinhamento, porque o nome cresceu 4 caracteres.
Contagem aproximada: para cada chamada renomeada que deixa o parêntese aberto
no fim da linha, conta-se a linha seguinte se o recuo dela era exatamente a
coluna do parêntese antigo mais 1.

| arquivo | continuações desalinhadas |
|---|---|
| `parser_islands.c` | 1 |
| `parser_islands_checks.c` | 1 |
| `parser_islands_closure.c` | 0 |
| `parser_islands_flow.c` | 15 |
| `parser_islands_index.c` | 4 |
| `parser_islands_types.c` | 5 |
| `parser_islands_internal.h` | 2 |
| **total** | **28** |

A contagem só olha a linha imediatamente seguinte à chamada; continuações de
mais de uma linha podem ter mais casos. Conforme o cartão, nada foi realinhado.

## Aceitações (saída final)

**1. Só `k_` e `isl_` saem dos objetos**

```
$ make -C tools/cgen -j8 >/dev/null
$ nm tools/cgen/obj/engine/parser_islands*.o | awk '$2=="T"{print $3}' | grep -v -E '^(k_|isl_)'
(nada)
$ nm tools/cgen/obj/engine/parser_islands*.o | awk '$2=="T"{print $3}' | grep -c '^isl_'
103
```

Os avisos de `src/tool/main.c` (quatro `set but not used`) são os do baseline
(`tools/agents/warnings.baseline` = 4) e não têm relação com a tarefa.

**2. Só o prefixo mudou (com bash)**

```
$ bash -c 'for f in tools/cgen/src/engine/parser_islands*.c tools/cgen/src/engine/parser/parser_islands_internal.h; do sed "s/\bisl_//g" "$f" | cmp -s - <(git show HEAD:"$f") || echo "DIFF $f"; done'
(nada)
```

**3. Nenhum nome de variável, parâmetro ou campo foi renomeado**

```
$ grep -n -E '\bisl_[a-z_0-9]+\b' tools/cgen/src/engine/parser_islands*.[ch] | grep -v -E '\bisl_[a-z_0-9]+\(' | head
(nada)
```

**4. Comportamento igual**

```
$ sh tools/agents/verify.sh compare /tmp/keel-snap-T4
ok   build (4 warnings, baseline 4)
ok   make check (41 lines ok)
ok   golden: 45 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ok   behaviour identical to /tmp/keel-snap-T4
ALL OK
```

## Dúvidas e observações

- O `make check` e o `golden/run.sh` estão verdes; `golden` tem 3 casos `WIP`,
  como antes da tarefa.
- A aceitação 3 passou com saída vazia, o que é mais forte que "só usos como
  ponteiro de função, cada um justificado": não houve nenhum uso desse tipo.
- A contagem de desalinhamento (28) é uma heurística e não entra em nenhuma
  aceitação; serve só de registro para uma eventual limpeza futura.
- Nenhum oráculo foi enfraquecido nem alterado. Os scripts de apoio ficaram em
  `/tmp/t4/` e não fazem parte do repositório.
- Escopo do `git status`: as sete alterações são os seis `parser_islands*.c`
  e `parser_islands_internal.h`. O cartão `tools/agents/tasks/T4-islands-prefix.md`
  já era não rastreado antes da tarefa, e este relatório é o único arquivo novo.
