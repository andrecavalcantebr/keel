# E6 — inventário dos blocos do README (só leitura)

## Objetivo
O André quer que todo bloco de C gerado do `README.md` seja um trecho literal
do `expected/` de um caso do golden, conferido por script. Antes de planejar,
faça o inventário.

## Arquivos
Ler: `README.md`, `golden/README.md`, `golden/cases/*` (`*.k`, `NOTES`,
`expected/`), `golden/run.sh`. Criar somente `tools/agents/reports/E6.md`.
Não editar mais nada.

## Contrato
Para cada bloco de código cercado do `README.md` (liste todos, com
`README.md:linha-inicial-final`):
1. Linguagem da cerca (`keel`, `c`, outra), e o rótulo da primeira linha
   (`//keel`, `//C gerado`, …).
2. Os pares: qual bloco `keel` gera qual bloco `c` (vizinhos no texto).
3. Para cada par, o caso do golden que cobre a mesma construção, se houver
   (`grep -rn` pelo verbo/tipo em `golden/cases/*/*.k`), com `caso:arquivo`.
   Marque: "coberto por X" / "parcial (diga o que falta)" / "sem caso".
4. Para blocos `c` cobertos: o intervalo de linhas em
   `expected/c23/<arq>` que corresponde ao trecho, e as diferenças entre o
   bloco do README e esse trecho (nomes, `u32` × `uint32_t`, `#line`,
   includes, ordem). Não precisa ser igual: relate as diferenças.
5. Blocos que não são "C gerado" (C escrito à mão, shell, EBNF): liste e
   diga que ficam fora.

## Aceitação
`git status --short` não mostra arquivo modificado além do relatório novo.

## Relatório
`tools/agents/reports/E6.md`: tabela com uma linha por bloco/par, mais um
resumo de contagens (cobertos, parciais, sem caso) e as dúvidas.
