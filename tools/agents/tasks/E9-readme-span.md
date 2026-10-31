# E9 — marcar o exemplo 3 (`span`) do README (mecânico)

## Objetivo
O exemplo 3 do README (`### 3. Trecho fixo, elementos mutáveis e intervalo`)
passa a ser trecho literal do golden `027-readme`. Modelo: os exemplos 1, 2
e 4, já marcados (o marcador precede a cerca, sem linha em branco).

## Arquivos
Editar somente `README.md`. Criar `tools/agents/reports/E9.md`. Não tocar
`golden/`, `test/`, `tools/cgen/`.

## Contrato
Fontes: `golden/cases/027-readme/span.k` (linhas 1-18) e
`golden/cases/027-readme/expected/c23/span.c` (linhas 12-24).

1. Bloco ```keel (rótulo `//keel`): mantenha o rótulo, troque o corpo pelas
   linhas 1-18 de `span.k` (o `.k` ganhou `import keel.array as array;`);
   marcador `<!-- golden: cases/027-readme/span.k:1-18 -->`.
2. Bloco ```c (rótulo `//C gerado`): mantenha o rótulo, troque o corpo pelas
   linhas 12-24 de `span.c`; marcador
   `<!-- golden: cases/027-readme/expected/c23/span.c:12-24 -->`.
3. Copie os corpos com `sed -n 'A,Bp' arquivo`, não digite.
4. Depois do bloco C, antes da frase "O resultado é `50`", acrescente um
   parágrafo: "Quatro coisas aparecem no C. `slice.of(dados, 1, 3)` vira a
   função da instância, `keel_slice_of2_keel_array_i16`, que recebe o vetor e
   o comprimento (`3`, vindo da dimensão declarada); `part[0]` é
   `*keel_slice_i16_ptr1(part, 0)`; o literal `0..2` fora de uma travessia é
   `keel_range_of(0, 2)`; e o `foreach` sobre o `range` conta de `first` a
   `limit`, com um bloco em volta que fecha no `}` do corpo."
5. Em nenhum outro lugar do README mexa.

## Aceitação
- `sh test/docs.sh` imprime `docs: 11 ok, 0 fail`.
- `git diff --stat` lista só `README.md`.
- `grep -n "typedef int16_t i16" README.md` não imprime nada.

## Relatório
`tools/agents/reports/E9.md`: o diff e a saída dos comandos.
