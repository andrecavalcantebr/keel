# T8 — auditoria dos designs frente aos normativos

## Objetivo

Os documentos de implementação em `design/` foram escritos antes de várias
decisões que hoje estão nos normativos. Alguns ainda descrevem o estado
antigo. Esta auditoria lista cada afirmação de um design que **contradiz**,
**omite** ou **referencia errado** o que os normativos decidiram, com a
correção proposta. **Ela não edita nada.** O André decide o que corrigir, e a
correção vira outra tarefa.

O cartão tem duas metades, para dois agentes em paralelo (os dois só leem):

| Metade | Designs auditados | Relatório |
| --- | --- | --- |
| **A** | `design/codegen-design.md`, `design/parser-design.md`, `design/lexer-design.md` | `tools/agents/reports/T8a-design-audit.md` |
| **B** | `design/cgen-tool.md`, `design/diag-design.md`, `design/requisitos-v0.md`, `design/analises.md` | `tools/agents/reports/T8b-design-audit.md` |

## Arquivos

- Criar: apenas o relatório da sua metade.
- Só leitura em todo o resto. Não rode `make`, `verify.sh` nem `golden/run.sh`.
- Fora do escopo: `design/possibilidades.md`, `design/proposta-*.md`,
  `design/*-impacto.md` (são propostas e histórico).

## Os normativos (a fonte da verdade)

| Documento | Como os designs o citam |
| --- | --- |
| `keel-spec.md` | "spec §x" ou "linguagem §x" (em `cgen-tool.md`, "spec" sozinho é a spec **da ferramenta**; "linguagem §x" é a `keel-spec.md`) |
| `keel-c-backend.md` | "backend §x" |
| `cgen-tool-spec.md` | "spec da ferramenta §x", ou "spec §x" dentro de `cgen-tool.md` |
| `keel-rationale.md` | "rationale" |

Seções da `keel-spec.md` (ela termina no §6):
§1 Escopo e princípios · §2 Léxico, vocabulário e gramática (2.1 léxico, 2.2
palavras e gramática, 2.3 reconhecimento, 2.4 grupos condicionais, 2.5 erros
léxicos) · §3 keel por exemplos · §4 Construções do núcleo (4.1 módulos e
interoperabilidade, 4.2 tipos/declarações/marcadores, 4.3 módulos genéricos,
4.4 resolução de operações e parâmetro `type`, 4.5 indexação e `range-index`,
4.6 cleanup léxico (`defer`), 4.7 travessia (`foreach`, `walk`), 4.8 execução
particionada (`parallel`), 4.9 tags e `match`, 4.10 tratamento de resultado
(`else`), 4.11 `extent`, 4.12 procedência e invalidação (papéis), 4.13 molde
(v1), 4.14 funções sobre protocolo) · §5 Protocolos e módulos da base (5.1
protocolos, 5.2 `keel.arena`, 5.3 buffer/slice/range/array, 5.4 tagged, 5.5
outcome/corot, 5.6 routine, 5.7 parallel, 5.8 biblioteca padrão) · §6
Diagnósticos (6.2 catálogo).

Seções do `keel-c-backend.md` §5: 5.1 declarações · 5.2 containers (5.2.1
verbos indisponíveis) · 5.3 açúcar de indexação · 5.4 `arena` · 5.5 `defer` ·
5.6 `tags`/`match` · 5.7 `foreach` · 5.8 ponto de entrada · 5.9 `parallel` ·
5.10 `keel.routine` · 5.11 cursor e partição · 5.12 `else` · 5.13 `at` · 5.14
`outcome`/`corot` · 5.15 `extent` · 5.16 parâmetro `type` · 5.17 verificações
de debug · 5.18 binder de dimensão · 5.19 função sobre protocolo.

Seções da `cgen-tool-spec.md`: §1 motivação · §2 o que é · §3 E/S · §4
invocação (4.1–4.10) · §5 critério de atualização · §6 escrita de arquivos ·
§7 diagnóstico e código de saída · §8 distribuição e base · §9 fora de escopo.

Confira sempre lendo o normativo; a lista acima é só o mapa.

## O que é incoerência (e o que não é)

É incoerência:

1. **Contradição:** o design afirma um comportamento que o normativo nega ou
   decide de outro jeito.
2. **Conceito superado:** o design trata como especial algo que o normativo
   tornou geral, ou cita mecanismo que deixou de existir.
3. **Omissão em lista que se diz completa:** uma tabela que mapeia "cada
   construção" ou "cada seção" e deixa de fora o que o normativo tem hoje.
4. **Referência errada:** "§x" que aponta para seção inexistente ou de outro
   assunto.

Não é incoerência: estilo, redação, ou um registro histórico datado ("Resolvida
em 2026-09-20: …", "Estado em …") que descreve o passado como passado. Só conta
se o texto apresenta o estado antigo como atual.

## Achados já confirmados pelo orquestrador (exemplos do tipo procurado)

Inclua-os no relatório da metade A, conferidos, com a correção proposta:

- `design/codegen-design.md:66` e `:452` dão ao `arena.c` "a reescrita de
  `alloc(a,T,n)`". Na spec §5.2 (regra 4) e no backend §5.4 (regra 2) e §5.16,
  `alloc(a, T, n)` é o mecanismo **geral** do parâmetro `type` apagado: a
  `arena` é um tipo comum do módulo `keel.arena`. O que resta de específico à
  arena no núcleo está na tabela "O que o núcleo conhece pelo nome" da spec
  (perto da linha 1960): o `= {0}` implícito e a recusa de parâmetro por valor.
- `design/codegen-design.md:131-141`, a tabela de "quem vai onde": a linha
  `extern_c` manda o conteúdo para o `.c`. O backend §4.1 (perto da linha 340)
  manda `extern_c`/`pub extern_c` para o `.h`, `[type_h]` para o `.type.h` e
  só `priv extern_c` para o `.c`, e tem linhas para diretiva de topo e C de
  topo que o design não tem.
- `design/codegen-design.md:61-72` e `:447-462`: as tabelas por construção vão
  até o backend §5.14, e faltam §5.15 a §5.19.
- `design/lexer-design.md:10` cita "linguagem §4.5" para a visão do fonte;
  a §4.5 da spec é indexação e `range-index`. Descubra a seção certa ou diga
  que não há.

## Método

1. Leia o design inteiro, seção por seção.
2. Para cada afirmação sobre o comportamento da linguagem, do C gerado ou da
   ferramenta, encontre o trecho normativo (use `grep -n` nos normativos) e
   compare.
3. Confira toda referência "§x" contra o título da seção citada.
4. Procure, em especial, os temas em que as decisões mudaram: papéis
   (`parent`, `child`, `invalidates`, `consumes`, §4.12) no lugar de regras
   próprias da arena; parâmetro `type` (§4.4); protocolos (§5.1, §4.14);
   `extent` (§4.11); visibilidade do `extern_c` (P16, backend §4.1);
   `#line` "ao pé da letra" (backend §6: só quando os contadores divergem);
   o fim do `from_stack`; `const` como qualificador, e não espécie de símbolo;
   `byref`.

## Aceitação

O relatório existe e cada achado tem as seis partes abaixo. Achado sem trecho
normativo citado não conta.

## Relatório

`tools/agents/reports/T8a-design-audit.md` ou `T8b-design-audit.md`, em pt-BR:

1. Uma linha por design auditado: quantas linhas leu e quantos achados.
2. Os achados, numerados (`A1`, `A2`, … ou `B1`, `B2`, …), cada um com:
   - **onde:** `arquivo:linha` do design;
   - **o design diz:** a citação literal, curta;
   - **o normativo diz:** `arquivo:linha` e a citação literal, curta;
   - **tipo:** contradição, conceito superado, omissão ou referência errada;
   - **correção proposta:** o texto novo, pronto para colar;
   - **certeza:** alta, ou "dúvida", com o motivo.
3. As dúvidas: o que você não conseguiu decidir, e por quê.

Sua mensagem final é um resumo curto: quantos achados, e os três mais graves.
