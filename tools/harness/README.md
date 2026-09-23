# harness — geração de `cgen` por modelo local, verificada

`tools/cgen/` ainda não existe de verdade (§9/§10 de `design/cgen-tool.md`
são o plano). Este diretório é a ferramenta que gera esse código chamando um
modelo local (`ollama`) marco a marco, e não faz parte do `cgen` em si —
não entra em `tools/cgen/src/`, não é distribuído, é andaime.

A ideia central: o projeto já é o ambiente mais estrito que dá para pedir a
um modelo pequeno. `golden/run.sh` é o oráculo (compila e roda), a emissão
é byte a byte determinística (`cgen-tool-spec.md §6`), e `design/cgen-tool.md`
§9 já divide a implementação em marcos com critério de aceitação. O modelo
local não decide nada — segue contrato e é aceito ou rejeitado por comando.

## Dois níveis

**Nível 1, o moedor.** Um modelo local (`ollama`) recebe uma *tarefa* — um
arquivo em `tasks/`, autocontido — e produz um arquivo C. O laço (`loop.py`)
compila e roda o comando de aceitação da tarefa; se falhar, tenta de novo.

**Nível 2, o verificador.** Depois que o nível 1 aceita uma tarefa (ou
esgota as tentativas), uma sessão Claude — Opus, por pedido do André — olha
o diff contra a spec de verdade, não só contra o comando de aceitação (que
pode ser fraco demais para pegar um "compila e passa, mas está errado").
Decide: aceitar e escrever a próxima tarefa, devolver com uma nota de
correção pontual, ou escalar para o André. Isso roda por *marco* (M1, M2,
...) ou tarefa bloqueada, não por tentativa — chamar Opus por tentativa
anularia a razão de ter um modelo local.

## Por que o laço não acumula erro

Cada tentativa é uma chamada isolada ao modelo (`/api/generate`, sem estado
de conversa no `ollama`). O prompt da tentativa N é sempre: a tarefa
completa + o arquivo da tentativa N-1 + **só o erro da tentativa N-1**,
truncado. Nunca a pilha de N-1 tentativas anteriores. Um modelo pequeno sob
histórico crescente tende a girar em torno do próprio erro anterior em vez
de corrigi-lo; o oposto do "não acumula" do André não é "não guarda nada" —
é "guarda o mínimo que orienta a próxima tentativa a divergir do erro atual,
não convergir para ele".

Esgotado `MAX_ATTEMPTS` (padrão 4), a tarefa vira `blocked`: relatório com
todas as tentativas e erros em `runs/<tarefa>/blocked-report.md`, e o laço
segue para a próxima tarefa independente, se houver. Bloqueio não trava o
lote inteiro.

**Erro silencioso é pior que erro verboso.** Um `Segmentation fault` sem
linha não dá o que uma tentativa seguinte usaria para divergir do erro
atual — só dá para repeti-lo. Os oráculos de M0 compilam uma cópia
instrumentada com `-fsanitize=address,undefined` só para rodar os casos de
comportamento (o `make -C tools/cgen` da lista de aceitação continua sem
sanitizer, é o build de verdade); o relatório vira `arquivo:linha: causa`,
que a tentativa seguinte consegue mesmo ler.

**Tentativa 1 é `temperature=0`; a partir da 2, sobe.** A primeira tentativa
fica determinística — reprodutível, sem ruído. Mas uma tentativa 2 com o
mesmo prompt e a mesma temperatura zero **reproduz byte a byte a mesma
tentativa 1**, se o modelo não tiver o que mudar sozinho — foi observado de
verdade rodando `m0-args-partition`: as tentativas 2 a 5 saíram idênticas, e
4 das 5 tentativas do orçamento foram desperdiçadas num ponto fixo. Por
isso o laço compara o código gerado com o da tentativa anterior; se for
igual, `temperature` sobe (`0.3 + 0.15 × repetições`, até `0.9`) e o prompt
ganha uma nota explícita: "sua última tentativa não mudou nada, tente uma
causa genuinamente diferente".

## Isolamento

Cada tentativa escreve em `runs/<tarefa>/attempt-N/`, nunca direto na árvore
real. Só uma tentativa aceita é copiada para o destino real (`tools/cgen/src/...`),
e só depois disso o `git status` do repositório muda. Uma tentativa ruim
nunca suja a árvore de trabalho.

## Sonda de velocidade

`ollama` é uma instância só, compartilhada com qualquer outro uso local do
mesmo modelo (o chat do editor, por exemplo). Antes do lote, `loop.py` manda
um prompt fixo e curto e mede tok/s — não para decidir nada, só para o
André ver, no início do log, se a velocidade de agora bate com a de
sempre. Se a média das tentativas do lote cair muito abaixo dessa sonda, o
resumo final avisa. Cada tentativa também registra o próprio tok/s
(`decode`/`prefill`, tirados de `eval_count`/`eval_duration` da resposta do
`ollama`) em `runs/<tarefa>/log.json`.

## Por que o laço é sequencial, uma tarefa por vez

Testado (`concurrency_test.py`, descartável, não faz parte do laço): duas
chamadas `/api/generate` concorrentes contra o mesmo modelo carregado, num
contexto menor (16k) para caber, deram **0,81× o throughput agregado** de
rodar as duas em sequência — pior, não melhor. A GPU integrada desta
máquina não tem folga de computação sobrando; duas gerações concorrentes só
dividem o mesmo teto, com overhead de contenção por cima. Não é questão de
`OLLAMA_NUM_PARALLEL` nem de memória — é *compute-bound*. Por isso o
`loop.py` não tenta paralelizar tentativas nem tarefas independentes nesta
máquina; se isso mudar (GPU discreta, outra instância de `ollama`), vale
retestar antes de reabrir a ideia.

## Layout

    tasks/     uma tarefa por arquivo .md — frontmatter YAML (id, output,
               acceptance, max_attempts) + corpo em Markdown: contrato
               autocontido, trecho já curado da spec/backend/design, e o
               que fica fora de escopo
    oracles/   os comandos/scripts de aceitação, escritos à mão — nunca
               gerados pelo modelo, no mesmo espírito do golden/run.sh
    runs/      log por tentativa (não versionado — ver .gitignore)
    loop.py    o laço de nível 1

## Formato de uma tarefa

Ver `tasks/m0-args-partition.md` ou `tasks/m0-base-resolve.md`. Frontmatter:

```yaml
---
id: m0-args-partition
output: tools/cgen/src/tool/main.c
acceptance:
  - "make -C tools/cgen"
  - "sh tools/harness/oracles/m0-args-partition.sh"
max_attempts: 5
---
```

`{output}` num comando de aceitação é substituído pelo caminho real do
arquivo. O corpo do arquivo, abaixo do frontmatter, é o prompt inteiro
mandado ao modelo. Campos do corpo, por convenção (não impostos pelo
`loop.py`, que só lê o texto):

- **Contrato** — o trecho normativo relevante, colado, não referenciado. O
  modelo local não lê `keel-spec.md` nem os `design/*.md`; quem cura o
  trecho é quem escreve a tarefa (hoje, o André com o Claude).
- **O que "correto" significa aqui** — invariantes que o comando de
  aceitação sozinho não prova (ex.: "nenhum estado global mutável").
- **Fora de escopo** — o que a tarefa não autoriza tocar; é o que evita o
  modelo inventar chamada para código que ainda não existe.

## Estado atual

`qwen3-coder:30b-a3b-q4_K_M` foi escolhido depois de comparar com
`qwen2.5-coder:14b` numa tarefa de M1 real (decodifica ~3,5× mais rápido;
erra mais de primeira, mas o laço de retry paga essa diferença de sobra).
M0 em andamento: `m0-base-resolve` passou de primeira; `m0-args-partition`
está sendo refeito depois de dois ajustes no próprio laço, achados ao vivo
rodando contra ele — ver as duas últimas notas de "por que o laço não
acumula erro".
