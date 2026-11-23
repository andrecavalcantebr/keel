# limites-medicao — medir os limites do cgen que ficaram sem medição

## Objetivo

A auditoria `tools/agents/reports/limites-auditoria.md` listou limites do cgen
lidos do código e **não medidos**. Meça cada um com um programa `.k` mínimo e
registre valor exato, diagnóstico e mensagem. O resultado entra numa tabela do
manual (e serve a uma IA que ajusta o programa aos limites), então o valor tem de
ser o **observado**: o maior que passa e o menor que é recusado.

## Arquivos

- Cria: `tools/agents/reports/limites-medicao.md`. Os programas de medição ficam
  em `/tmp/limites2/` (um subdiretório por limite; o caminho do `.k` é o do
  `module`, `module-path-mismatch` senão). Nada mais: não edite código,
  normativos, golden nem testes; não faça commit.

## Contrato

Ferramenta: `tools/cgen/cgen -std=c23 --base-dir base -I <dir> --stop-after=gen
--dest-dir <tmp> <arq>.k` (a partir da raiz do repositório; `make -C tools/cgen
all` antes, se `tools/cgen/cgen` não existir). Use `--stop-after=parse` quando o
limite é do parser. Estude a gramática no `keel-llm-context.md` (seção 1) e os
exemplos em `golden/cases/` para escrever programas **válidos** que cheguem ao
limite (um programa inválido não mede nada).

Limites a medir (valor do código entre parênteses; meça o real):

1. `extent`: dimensões (4), grupos (4) e colunas (8) — `golden/cases/020-extent`,
   `035-match-column` mostram a sintaxe.
2. `match`: rótulos de um `match` (32).
3. `defer [now …]`: entradas (8).
4. `routine`: campos (16).
5. `parallel`: capturas (16).
6. `protocol`: itens (16) e parâmetros (8).
7. `#include` gerados por arquivo (`MAX_INCLUDES` 32): faça um módulo cujo `.h`
   gerado precise de mais de 32 includes (importando muitos módulos e usando
   seus tipos), não só 33 `import_c`.
8. Parâmetros de função num módulo **genérico** e numa função chamada pelo
   fechamento (`K_PARAMS_MAX` 16): 17 parâmetros trunca em silêncio? Compare o
   `.h` gerado com o esperado.
9. Declaradores numa declaração `struct`/`typedef`/`array` (8): `typedef int a, b, …`
   com 9 nomes; `array i32 a[2], b[2], …`.
10. Dimensões de `array` (8) e índices de `x[i, j, …]` com 9 índices.
11. Profundidade de declarador (16): `int ****…` e sufixos aninhados.
12. Locais por arquivo (`K_LOCALS_MAX` 128): 129 variáveis locais/globais
    declaradas, e se isso dá `implementation-limit`.
13. Módulos importados direta e transitivamente (`WRITTEN_CAP`, 1024) e fontes
    `.k` (`CGEN_SOURCES_CAP` 1024): só se for viável gerar os arquivos por
    script; senão, "não medido" com o motivo.
14. Linhas de um fonte (há limite?), bytes de um fonte, e comprimento de uma
    constante de texto: 1.000.000 linhas passa? Meça um fonte de 10 mil, 100 mil e
    1 milhão de linhas de `pub i32 fN(void) { return N; }` e registre tempo e
    pico de memória (`/usr/bin/time -f '%e s %M KB'`).
15. Funções por módulo e módulos por programa: sem limite conhecido; meça 10 mil
    funções num módulo.

Para cada limite: o programa (caminho em `/tmp/limites2/`), o comando, o maior
valor que passa, o menor que falha, o diagnóstico (id e mensagem completa) e se
a recusa é silenciosa (saída errada sem diagnóstico: **destaque**, é bug).

## Passos sugeridos

1. Gere os programas por script (`python3 -I`), variando N; bisseque.
2. Confirme os valores com o programa mínimo, sem tentativa no escuro.
3. Escreva o relatório.

## Aceitação

- `git status --short` lista só `tools/agents/reports/limites-medicao.md` como
  novo (mais o que já estava sujo: `gdb/__pycache__/`, `gen/`, `skills/`).
- O relatório tem uma linha por limite, com "passa até" e "falha em", e a seção
  "Dúvidas" com qualquer recusa silenciosa ou mensagem enganosa.

## Relatório

O arquivo acima, em pt-BR (código e ids em inglês).
