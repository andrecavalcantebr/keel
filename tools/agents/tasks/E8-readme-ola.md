# E8 — marcar o exemplo 1 (`ola`) do README (mecânico)

## Objetivo
O exemplo 1 do README (`### 1. Módulo completo e chamada C`) passa a ser
trecho literal do golden `028-readme-main`. Modelo: o exemplo 4
(`### 4. Resultado…`) e o exemplo 2 já marcados; veja como o marcador precede
a cerca, sem linha em branco entre os dois.

## Arquivos
Editar somente `README.md`. Criar `tools/agents/reports/E8.md`. Não tocar
`golden/`, `test/`, `tools/cgen/`.

## Contrato
Fontes: `golden/cases/028-readme-main/ola.k` (linhas 1-8),
`golden/cases/028-readme-main/expected/c23/ola.c` (linhas 7-11) e
`golden/cases/028-readme-main/expected/c23/main_ola.c` (linha 3).

1. Bloco ```keel (rótulo `//keel`): mantenha o rótulo, troque o corpo pelas
   linhas 1-8 de `ola.k`; marcador
   `<!-- golden: cases/028-readme-main/ola.k:1-8 -->`.
2. Bloco ```c (rótulo `//C gerado`): mantenha o rótulo, troque o corpo pelas
   linhas 7-11 de `ola.c`; marcador
   `<!-- golden: cases/028-readme-main/expected/c23/ola.c:7-11 -->`.
3. Logo depois desse bloco C, abra um segundo bloco ```c (rótulo
   `//C gerado, unidade de entrada`) com o corpo da linha 3 de `main_ola.c`;
   marcador `<!-- golden: cases/028-readme-main/expected/c23/main_ola.c:3-3 -->`.
   Atenção: `test/docs.py` ignora a primeira linha do bloco só se for
   exatamente `//keel` ou `//C gerado`. Então use o rótulo `//C gerado` também
   neste bloco (e escreva a explicação de que é a unidade de entrada na prosa).
4. Copie os corpos com `sed -n 'A,Bp' arquivo`, não digite.
5. Na prosa depois dos blocos (a frase que começa com "`ola_main` é a função
   do módulo"), troque o trecho "o wrapper `main` fornece a entrada do
   programa" por "o wrapper `main`, numa unidade à parte (`main_ola.c`, gerada
   por `--main ola`), fornece a entrada do programa; ele repassa `argc` e
   `argv`, por isso `main` de módulo os recebe". Não mude o resto da frase.
6. Em nenhum outro lugar do README mexa.

## Aceitação
- `sh test/docs.sh` imprime `docs: 9 ok, 0 fail`.
- `git diff --stat` lista só `README.md`.
- `grep -n "int main(void)" README.md` não imprime nada.

## Relatório
`tools/agents/reports/E8.md`: o diff e a saída dos comandos.
