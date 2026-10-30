# E7 — marcar os blocos `reading` do README (mecânico)

## Objetivo
Os dois pares de `reading` do README (a abertura e o exemplo 2) passam a ser
trechos literais do golden `027-readme`, com o marcador que o
`test/docs.sh` confere. O modelo já feito é o exemplo 4 (`### 4. Resultado com
default…`): veja como o marcador precede a cerca.

## Arquivos
Editar somente `README.md`. Criar `tools/agents/reports/E7.md`. Não tocar
`golden/`, `test/`, `tools/cgen/`.

## Contrato
Fonte: `golden/cases/027-readme/reading.k` (linhas 1-9) e
`golden/cases/027-readme/expected/c23/reading.c` (linhas 7-15).

1. Abertura (README linhas ~7-27): bloco ```keel sem rótulo e bloco ```c sem
   rótulo. Preceda cada um da linha de marcador, sem linha em branco entre o
   marcador e a cerca:
   `<!-- golden: cases/027-readme/reading.k:1-9 -->` e
   `<!-- golden: cases/027-readme/expected/c23/reading.c:7-15 -->`.
   O corpo de cada bloco passa a ser exatamente essas linhas (copie com
   `sed -n 'A,Bp' arquivo`, não digite). Não acrescente rótulo `//keel` nem
   `//C gerado` na abertura (ela não tem).
2. Exemplo 2 (README linhas ~250-280): os blocos têm os rótulos `//keel` e
   `//C gerado` na primeira linha; mantenha o rótulo e troque o corpo pelas
   mesmas linhas, com os mesmos dois marcadores.
3. Logo depois do bloco C do exemplo 2, onde hoje há a frase "A expressão de
   retorno é avaliada antes da limpeza. …", acrescente ao fim dela:
   " O `defer` sai inline, e a expressão vai para o temporário `keel__rv0`; o
   `#line` leva a linha do `defer` no `.k`."
4. Em nenhum outro lugar do README mexa.

## Aceitação
- `sh test/docs.sh` imprime `docs: 6 ok, 0 fail`.
- `git diff --stat` lista só `README.md`.
- `grep -n "int result = fgetc" README.md` não imprime nada.

## Relatório
`tools/agents/reports/E7.md`: o diff e a saída dos comandos.
