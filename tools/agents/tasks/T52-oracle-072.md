# T52 — oráculo do caso 072: função sobre protocolo sem `inline`

## Objetivo

Escrever **à mão** o caso `golden/cases/072-protocol-function-implicit-inline/`:
uma função sobre protocolo (spec §4.14) declarada **sem a palavra `inline`**,
demonstrando a regra 8 reescrita em 2026-11-13: ela é sempre `inline`, mesmo
sem escrever a palavra. Hoje o cgen aceita esse programa e falha só no link
(`undefined reference`); este caso é o alvo da correção que vem depois
(cartão T53), que ainda não existe quando você trabalha. Você é o **autor do
oráculo**: não implementa nada, e o esperado sai do raciocínio sobre a spec e
o backend, não de rodar a ferramenta.

## Proibido (captura da ferramenta)

- **Não rode `tools/cgen/cgen`**, nem leia `tools/cgen/src/`.
- Não edite normativos, código, nem outros casos. Não commite.

## Arquivos (só estes)

Cria `golden/cases/072-protocol-function-implicit-inline/` com:
- `pf2.k` (fonte abaixo, **exatamente assim**)
- `proof.c`, `NOTES`
- `expected/c23/` e `expected/c11/`, escritos **separadamente**

Edita `test/cgen/gen/golden-cases.txt` (linha nova, formato das vizinhas,
`|wip`). Cria `tools/agents/reports/T52-oracle-072.md`.

## Fonte

```keel
// pf2.k
module pf2;

/* Mesma forma do golden 065 (`box`, `first`/`limit`), mas `span` SEM
   `inline`: spec §4.14 regra 8 (2026-11-13) — é sempre inline, a palavra é
   redundante quando presente e dispensável quando ausente. */
pub typedef struct { i32 lo, hi; } box;

pub inline i32 first(box *x) { return x->lo; }
pub inline i32 limit(box *x) { return x->hi; }

pub i32 span(Countable items) { return limit(items) - first(items); }

pub i32 span_of(box b) { return span(b); }
```

## O que o esperado deve afirmar

O mesmo que o golden 065 (`golden/cases/065-protocol-function/`) afirma para
`pf.k` — ele é a referência direta, troque só `pf`/`pf2` nos nomes. A
diferença inteira entre os dois casos é a AUSÊNCIA da palavra `inline` antes
de `span` em `pf2.k`; o C gerado é **idêntico em forma** (mesmo
`static inline` no header da instância, backend §5.19) porque a regra 8 não
deixa a palavra mudar nada. Copie o raciocínio do 065 e troque os nomes
derivados de `pf`/`box`/`span`/`span_of` para `pf2`/os mesmos nomes de tipo e
verbo (o módulo muda, os nomes dos verbos não).

## Análogo a ler

`golden/cases/065-protocol-function/` (tudo: `.k`, `proof.c`, `NOTES`,
`expected/`). `keel-spec.md` §4.14 (sobretudo a regra 8, reescrita em
2026-11-13). `keel-rationale.md`, "Por que a função sobre protocolo é sempre
`inline`".

## `proof.c`

Igual ao do 065, trocando o include e os símbolos de `pf` para `pf2`.

## Aceitação

```sh
golden/run.sh 2>&1 | tail -15
```
O caso 072 deve compilar, executar e passar nos checks de layout e de
`#line`, nos dois perfis.

## Relatório

`tools/agents/reports/T52-oracle-072.md`, pt-BR: arquivos criados, a saída do
`golden/run.sh` para o 072, e toda dúvida (não adivinhe).
