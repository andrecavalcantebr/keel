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

## Lições de como escrever uma tarefa, tiradas de bloqueio real

Toda vez que uma tarefa travou 5 tentativas seguidas neste projeto, a
causa coube numa destas quatro — vale conferir as quatro **antes** de
escrever uma tarefa nova, porque cada uma já custou uma rodada inteira de
retrabalho pelo menos uma vez:

1. **Repetir os `#include` exatos, sempre, mesmo que outra tarefa já os
   tenha mostrado.** `m1-lexer-skip-trivia` travou 5x só porque a tarefa
   nunca escreveu `#include "keel/keel_slice_char.type.h"` explicitamente
   — só citava o tipo dentro de uma assinatura. O modelo não carrega
   contexto de uma tarefa pra outra; se o `#include` não está na tarefa
   atual, ele não existe.
2. **Reexplicar o contrato de retorno de qualquer função "já
   implementada" que a tarefa reutilize.** Mesmo bug, tarefa diferente:
   `skip_trivia` tratou `k_lexer_peek_at` como "0 = sucesso" (convenção
   comum em C) quando na verdade a função devolve o próprio byte lido
   (space é 32, não 0) ou `-1` no EOF. A assinatura sozinha não basta —
   sem reafirmar o contrato de retorno na tarefa que consome, o modelo
   assume a convenção mais comum, que aqui é a errada.
3. **Preferir varredura linear a busca binária em tabela pequena.** Três
   vezes seguidas (contando o benchmark que escolheu o modelo), o mesmo
   bug: comparador que ordena por tamanho antes de `strcmp`, quebrando a
   suposição de ordenação que a busca binária depende. Numa tabela de
   ~70 entradas não há ganho real nenhum em não usar `for` simples — é
   ponto cego específico deste modelo, então a tarefa evita a forma
   inteira em vez de pedir cuidado.
4. **Dar exemplo numérico concreto sempre que houver aritmética de
   posição/tamanho envolvida.** `m1-lexer-peek` travou 5x com só prosa
   ("`*width_out` recebe o número de bytes..."); a mesma tarefa, com um
   "Worked example" numérico (`pos=0` → `width_out=3`, passo a passo),
   passou de primeira. Prosa descreve a regra; exemplo mostra a conta
   feita — e é a conta que este modelo erra.

## O balanço de custo não fechou como esperado (2026-09-22)

A motivação de usar modelo local era gastar menos tokens de nuvem. Nas
primeiras ~10 tarefas de M0/M1, isso **não se confirmou** nas que
travaram: `m0-args-partition` sozinho levou 4 rodadas de 5 tentativas
(~21 chamadas ao modelo) até passar, e cada rodada bloqueada custou do
lado do Claude ler o código gerado, às vezes rodar `gdb`, diagnosticar a
causa exata, reescrever a tarefa e revalidar o oráculo contra uma
referência própria antes de tentar de novo — ciclo caro o bastante para,
nessas peças específicas, provavelmente superar o custo de só escrever a
função de 20-80 linhas à mão. Estimativa grosseira: ~150.000-250.000
tokens do lado do modelo local nessa janela, e o lado Claude
provavelmente maior — sem instrumentação para medir o Claude com
precisão, essa parte ficou só como estimativa registrada em conversa, não
em número auditável.

**O que compensou de verdade:** as tarefas pequenas, com escopo de uma
função só e as quatro lições acima já aplicadas, passaram de primeira ou
segunda tentativa (`m0-roots-check`, `m0-match-long-option`,
`m0-base-resolve`, `m1-read-source`, `m1-token-predicates-shape`). A
lição prática: delegar vale a pena quando a tarefa é pequena e mecânica
e o contrato já foi escrito sem ambiguidade conhecida; quando há suspeita
de ambiguidade, o retrabalho de diagnosticar supera o que se economizaria
delegando.

## Execução sem o Claude no laço

A partir de 2026-09-22, o padrão passou a ser: Claude escreve a tarefa +
o oráculo, valida o oráculo contra uma implementação de referência escrita
à mão (para confirmar que o oráculo pega bug de verdade e não acusa
implementação correta), e entrega os dois já prontos — sem chamar
`loop.py`. O André roda localmente e só reporta de volta o que não
conseguir diagnosticar rápido pelos próprios logs (`runs/<tarefa>/`). Isso
corta o custo Claude de acompanhar tentativa por tentativa — que era
justamente a parte mais cara do ciclo, pela nota acima.

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

**M0 fechado**: partição de `argv`, `--cgen-version`, link transparente,
os três erros de invocação, `--base-dir` resolvendo de verdade. `main.c`
mais quatro peças extraídas (`roots.c`, `match_option.c`,
`base_resolve.c`) — ver "Lições" acima para o porquê de cada extração.

**M1, os sete reconhecedores prontos**: `read_source`, predicados de token
(palavra + forma), `lexer_peek` (emenda), `skip_trivia`, `scan_identifier`,
`scan_number`, `scan_punct`, `scan_quoted` e `scan_directive` — todos
passaram rodando localmente, sem o Claude no laço (ver "Execução sem o
Claude no laço"). `scan_directive` travou as 5 tentativas na primeira
versão da tarefa (comparação de palavra-chave escrita como cadeia de `if`
caractere a caractere, à mão — errou comprimento/ordem de letras em
`ifdef`/`ifndef`, mesma classe da lição 3); reescrita para exigir a mesma
forma já usada em `token_predicates_words.c` (tabela `{palavra, TKPpKind}`
percorrida com `strlen`+`memcmp`), passou de primeira.

**M1 fechado (2026-09-24), escrito pelo Claude, fora do laço:**
`k_lexer_next`, o ponto de entrada `k_parser_keel`, `--stop-after=lex`, os
diagnósticos léxicos (`engine/diag.c`, `tool/report.c`) e as correções de
borda (emenda dentro de delimitador de comentário, predicados sobre a grafia
lógica, nome universal de caractere, `u8'x'`). O oráculo de integração é
`oracles/m1-lex-dump.sh`: despejos fixos (`lex/*.tokens`), o lexer de
referência independente (`lex/reference_lexer.py`) contra todo `.k` do golden
e da `/base`, e os diagnósticos (`lex/diag.k` → `lex/diag.stderr`). Montar
tarefa para o modelo local custava mais que escrever direto; o modelo local
volta a valer a partir do parser, quando houver despejos esperados de
`--stop-after=parse` para os casos do golden.

Os oráculos por reconhecedor (`m1-lexer-*`, `m1-token-predicates-*`) e as
tarefas deles saíram em 2026-09-24: o despejo testa cada reconhecedor pelo
lexer inteiro, e os casos que só eles tinham (fim de arquivo no meio de
emenda, comentário ou literal; CR sozinho; NUL; arquivo vazio) viraram
arquivos pequenos em `oracles/lex/`, um EOF por arquivo. Ficam no git.

**O oráculo do parser (M2 em diante) já existe:** `oracles/m2-parse-dump.sh
[header|decl|inst|ilha]` compara o `--stop-after=parse` dos casos 001, 009 e
013 com os despejos escritos à mão em `oracles/parse/*.parse` (formato no
desenho do cgen §5.2). O nível diz quais linhas contam, então o parser pode
ser construído por passagem: `header` e `decl` são o M2; `inst` e `ilha`, a
passagem 3.

**Ideia em discussão, não decidida (2026-09-22):** os reconhecedores são
todos FSMs — alguns puramente sequenciais (`scan_punct`), outros com ramos
que se decidem cedo e seguem lineares dali (`scan_number`: o primeiro byte,
ou o par `0x`, decide dígito decimal vs. hex vs. fração/expoente, mas é uma
tabela de transição só, maior, não um "híbrido"). A hipótese é que escrever
a próxima leva de tarefas (a partir de `k_lexer_next`, que é o despachante
mais irregular de todos) como uma FSM explícita — estados nomeados e tabela
de transição por classe de byte, em vez de prosa de regras — produziria
código mais uniforme e evitaria bugs de "ramo esquecido" como o de
`scan_directive` acima. Custo: desenhar a tabela de estados é mais trabalho
de preparo por tarefa do que escrever regras em prosa. Ainda não aplicada a
nenhuma tarefa; avaliar ao escrever a tarefa de `k_lexer_next`.
