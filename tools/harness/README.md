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

## Um oráculo tem que sobreviver a um travamento, não só a uma resposta errada

Achado validando `m2-parser-modifier-decl` (2026-09-26): a versão errada que
eu montei de propósito para testar o oráculo (esquecer de consumir `byref`)
não deu erro — deu **loop infinito**, porque o token sobrando foi lido como
se fosse o `'{'` de abertura, e o balanceador nunca via o `'}'` que fecharia
de verdade. `run_acceptance` do `loop.py` chamava `subprocess.run(...,
timeout=300)` sem capturar `subprocess.TimeoutExpired` — um travamento assim
numa tentativa de verdade derrubaria o laço inteiro, não só reprovaria a
tentativa. Duas correções, as duas aplicadas: **(1)** todo `test/unit/*.sh`
de oráculo do parser embrulha o binário em `timeout 10 "$BIN"`; **(2)**
`loop.py` agora captura `TimeoutExpired` e trata como reprovação comum. A
lição para quem valida um oráculo novo: testar não só uma versão *errada*
contra ele, mas também uma versão plausível que poderia travar — a mesma
classe de omissão (não consumir um token esperado) costuma causar as duas.

## O oráculo não pode ser resolvido escrevendo a resposta (2026-09-26)

Correção do André, mais fundamental que a de cima: até
`m2-parser-modifier-decl`, cada oráculo era validado escrevendo uma
implementação de referência correta no caminho real, rodando o oráculo
contra ela, e só então apagando-a. **Isso não faz sentido** — se já existe
uma implementação correta, escrevê-la para "validar" é resolver o problema;
delegar ao modelo local a partir daí não economiza nada, e o ciclo de
tarefa + laço só *soma* custo por cima do que já foi gasto escrevendo a
referência.

O padrão certo já estava no próprio projeto, só não foi seguido: o
comentário de `test/parse_dump.sh` já diz que os `.parse` "are NOT
regenerated from cgen" — são derivados à mão do `.k` e da spec. A mesma
regra vale para um oráculo de unidade: a entrada e a saída esperada são
**derivadas por raciocínio sobre a gramática/contrato**, escritas direto
nas asserções do `main.c`, nunca obtidas rodando uma implementação que
resolve a função. O "worked example" que já vai em toda tarefa (para o
modelo) é exatamente essa derivação — é só copiar os números dali para o
oráculo, não refazer o trabalho em código.

**O que ainda se pode compilar, sem violar a regra:** o `TESTMAIN` sozinho
contra um stub deliberadamente errado (zera a saída e retorna) — não para
validar a resposta, só para pegar erro de sintaxe/`#include`/dependência
faltando no próprio `.sh`. Ele tem que falhar em toda asserção; se não
falhar, o oráculo é que está quebrado. Isso não é "escrever a resposta": é
testar o cabo, não o problema.

As cinco funções já aceitas (`k_scan_qualified_name` até
`k_scan_modifier_decl`) não precisam ser refeitas — o código gerado está
correto e a suíte prova isso independente de como o oráculo foi validado.
A correção vale só daqui para frente: registrada em
`.claude/skills/cgen-harness-task/SKILL.md`, seção "Preparing the oracle (a
single recognizer)".

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
causa coube numa destas cinco — vale conferir todas **antes** de
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
5. **Citar a assinatura de uma função reutilizada colando código real do
   repositório, nunca reescrevendo-a em prosa ou pseudocódigo próprio.**
   `m2-parser-header` travou 5x (2026-09-26), as cinco pela mesma causa:
   a tarefa descrevia `k_lexer_next(lexer, &pp_kind)` em prosa ("returns
   the next token, advancing lexer"), e o modelo inventou uma convenção
   diferente da real — tratou o segundo parâmetro como um "out" que
   recebe o próprio token, e um campo `.slice` em `KToken` que não
   existe. A tarefa anterior (`m2-parser-scan-qualified-name`, mesmo
   dia) tinha passado de primeira citando a mesma função como
   pseudocódigo — `tok = k_lexer_next(lexer, next_pp_kind_out)` — o que
   já bastava. A lição, mais forte que "reexplicar o contrato" (item 2):
   **não reescrever, colar.** `grep` por uma chamada real já aceita no
   repositório (`engine/parser_keel.c`, ou qualquer `.c` que já passou
   pelo laço) e copiar esse trecho, com atribuição de arquivo:linha, é
   mais barato de escrever e impossível de errar a sintaxe — ao
   contrário de uma paráfrase nova, que carrega o mesmo risco de erro do
   lado de quem escreve a tarefa que o modelo tem do lado de quem a lê.

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
  - "sh tools/cgen/test/cli/args_partition.sh"
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
`tools/cgen/test/lex_dump.sh`: despejos fixos (`lex/*.tokens`), o lexer de
referência independente (`lex/reference_lexer.py`) contra todo `.k` do golden
e da `/base`, e os diagnósticos (`lex/diag.k` → `lex/diag.stderr`). Montar
tarefa para o modelo local custava mais que escrever direto; o modelo local
volta a valer a partir do parser, quando houver despejos esperados de
`--stop-after=parse` para os casos do golden.

Os oráculos por reconhecedor (`m1-lexer-*`, `m1-token-predicates-*`) e as
tarefas deles saíram em 2026-09-24: o despejo testa cada reconhecedor pelo
lexer inteiro, e os casos que só eles tinham (fim de arquivo no meio de
emenda, comentário ou literal; CR sozinho; NUL; arquivo vazio) viraram
arquivos pequenos em `tools/cgen/test/lex/`, um EOF por arquivo. Ficam no git.

**O oráculo do parser (M2 em diante) já existe:** `tools/cgen/test/parse_dump.sh
[header|decl|inst|ilha]` compara o `--stop-after=parse` dos casos 001, 009 e
013 com os despejos escritos à mão em `tools/cgen/test/parse/*.parse` (formato no
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

**Os oráculos viraram a suíte do cgen (2026-09-25).** O que era `oracles/` —
os testes de `tool/` das tarefas M0 e M1, o despejo do lexer e o do parser —
mora em `tools/cgen/test/` e roda com `make -C tools/cgen check`
(`PARSE_LEVEL=header|decl|inst|ilha` inclui o parser). As tarefas `tasks/m0-*`
e `m1-read-source` ficam como histórico: os comandos de aceitação delas apontam
para os caminhos antigos. Tarefa nova para o modelo local usa os testes de lá
como aceitação.

**M2 começou (2026-09-26), primeira tarefa via modelo local: `k_scan_qualified_name`.**
`engine/parser.h` (novo, escrito à mão — a primeira peça do parser, no mesmo
papel que `lexer.h` teve para M1) declara `qualified-name ::= IDENT { '.'
IDENT }` (spec §2.2), a produção de `module-name` e de todo nome pontuado.
Oráculo próprio (`tools/cgen/test/unit/parser_scan_qualified_name.{sh,c}`,
molde M0/M1: `SRC` + `TESTMAIN` sob sanitizer), validado antes da tarefa contra
uma implementação de referência escrita à mão — e contra uma versão
deliberadamente errada (ignora os pontos), para confirmar que o oráculo pega o
bug. Tarefa (`tasks/m2-parser-scan-qualified-name.md`) **passou de primeira
tentativa** (26,1s, 17,6 tok/s decode, igual à sonda — sem sinal de contenção).
`make -C tools/cgen check` continua verde com o arquivo gerado.

**Tentativa de agrupar produções (`m2-parser-header`), duas rodadas no mesmo
dia.** André pediu uma tarefa mais ampla — três reconhecedores
(`k_scan_module_decl`, `k_scan_import`, `k_scan_import_c`) num arquivo só —
para testar se agrupar amortiza o custo de escrever a tarefa. Primeira
versão, descrevendo `k_lexer_next` em prosa: **bloqueou nas 5 tentativas**,
as cinco pela mesma causa (lição 5, acima). Segunda versão, reescrita citando
código real (a implementação inteira, já aceita, de
`k_scan_qualified_name`, colada verbatim) em vez de prosa: **passou de
primeira tentativa** (53,7s). A própria reescrita revelou um bug na
referência que eu tinha validado à mão (`k_scan_import_c` devolvia `;` em
vez do token depois dele) — só apareceu porque o oráculo da segunda rodada
passou a afirmar `next_out` contra um marcador, o que o oráculo da primeira
rodada não fazia. Conclusão registrada para a avaliação com o André: o
agrupamento por si só não travou nada; o que travou foi a mesma omissão da
lição 2/4 (contrato reescrito em prosa em vez de citado), e o agrupamento só
multiplicou o efeito por três.

**Terceira rodada, mais ambiciosa (`m2-parser-extern-c`), já com a lição 5
aplicada desde o início.** `k_scan_braced_opaque` (balanceamento genérico de
`{ }`, com aninhamento) e `k_scan_extern_c` (que **chama** a primeira) — duas
funções novas, uma delas dependente da outra, geradas juntas. **Passou de
primeira tentativa** (45,5s), com aninhamento correto e `next_out` certo em
todos os cinco casos do oráculo (corpo vazio, plano, aninhado,
`extern_c` com e sem `[type_h]`). Terceira confirmação seguida da hipótese
do André: agrupar produções relacionadas funciona bem quando a tarefa cita
código real em vez de reescrevê-lo em prosa.

**`k_scan_ident_list` fecha a etapa 1 (header) por inteiro.** Tarefa mínima
de propósito (uma função só, `IDENT { ',' IDENT }` — o binder `dim`/`tags`/
`type` de `module`), pedido do André depois de notar que as ~107 linhas
finais do modelo local custaram um preparo (tarefa + oráculo + validação)
bem maior do lado do Claude — quarto PASS de primeira tentativa seguido, e o
mais rápido (19,3s). A fiação em `k_scan_module_decl` (três chamadas
sequenciais à função nova, uma por binder) foi feita direto por Claude, sem
tarefa — é código de baixo risco, três linhas repetidas, o mesmo padrão já
comprovado três vezes; delegar aqui só teria custo, sem reduzir risco.
`module`, `import`, `import_c`, `extern_c` — os quatro `top-item` de
`unit` (spec §2.2) — e os binders de `module` estão todos reconhecidos.

**Etapa 2 (coleta) começou: `engine/symtab.h`/`symtab.c` escritos direto por
Claude** (parser-design §2.1 — mapa ordenado por grafia, iteração por
inserção, `KSymKind`), e `k_scan_modifier_decl`, o primeiro reconhecedor que
**registra** símbolo em vez de só reconhecer sintaxe, delegado com sucesso:
**quinto PASS de primeira tentativa seguido** (26,0s). Achado de spec ao
projetar a tarefa (§4.3: "a assinatura do módulo fixa a quantidade... de
todos os seus modificadores") — a aridade de um `modifier` é a do próprio
`module` que o declara (`KModuleHeader.dim_count + tag_count + type_count`),
não algo que se descubra olhando dentro do corpo do modificador; isso
manteve a tarefa pequena, sem precisar interpretar `{ }` além de
delimitá-lo com `k_scan_braced_opaque`, já pronto. Este é também o marco em
que André corrigiu o método de validar oráculo (seção acima) — as cinco
funções desta etapa e da etapa 1 continuam válidas, só o processo mudou daqui
para frente.

**`k_scan_tags_decl`, primeira tarefa sob o processo corrigido: sexto PASS
de primeira tentativa seguido** (27,3s). Sem implementação de referência —
a entrada e a saída esperada do oráculo (`test/unit/parser_tags_decl_main.c`)
foram derivadas por raciocínio sobre a gramática, e a única compilação de
checagem foi contra um stub deliberadamente errado (zera a saída, retorna
`false`), só para confirmar que o `.sh`/`.c` do teste em si não tinham erro
de sintaxe ou dependência esquecida — o stub falhou nas cinco asserções,
como devia. `tags-list` sem valor de tag por agora (mesmo padrão dos
binders de `module`: registra sem valor primeiro).

**`k_scan_struct_decl`: sétimo PASS de primeira tentativa seguido** (29,1s).
Terceiro reaproveitamento de `k_scan_braced_opaque` (depois de `extern_c` e
`modifier`). Achado de bordas ao projetar: o corpo `{ }` de `struct-spec`
**é seguido de `';'`** — diferente de `extern_c` e `modifier`, cujas
gramáticas não têm `';'` depois do `'}'` — então o `next_out` que
`k_scan_braced_opaque` devolve já está sentado nesse `';'` (lido, mas não
passado), e sobra uma leitura extra depois da chamada. Anônimo (`struct {
...};`) não registra nada e não é falha — o oráculo cobre os dois casos.
Campos são opacos por agora (mesmo padrão de sempre: primeiro a casca,
depois o conteúdo).

**Balanço da etapa 2 até aqui:** `modifier`, `tags` (sem valor) e `struct`
(campos opacos) registram símbolo. **O que falta é de outra categoria de
dificuldade** — `decl-typedef`, `decl-function` e `decl-keel` (variável/
`constexpr` de arquivo) todos passam pelo que `parser-design.md` §4 chama
"o ponto mais delicado do parser": decidir se uma sequência de tokens é
mesmo uma declaração keel, o que exige achar o *declarador* em geral
(inclusive ponteiro a função) e, para `decl-keel`, resolver se o
especificador é um tipo nomeado, uma aplicação de modificador (o que exige
consultar a aridade já registrada) ou C opaco. Isso não é mais "uma função
pequena e mecânica" — é a peça que os designs já avisam ser a mais fácil de
errar. Fica para quando o André decidir se quer entrar nesse desenho junto,
ou se prefiro escrever essa parte direto em vez de montar tarefa (mesmo
critério do "balanço de custo" registrado acima).

**André decidiu: primeira tentativa nessa parte mais difícil, delegada
mesmo assim.** `k_scan_known_type` — o núcleo dos passos 2-3 do despacho de
`parser-design.md` §4 ("IDENT registrado como modificador? IDENT
registrado como tipo?"), com argumento de um token só, sem aninhamento.
André apontou que a "ambiguidade" aqui se resolve com uma consulta à
`symtab` já feita, não com lookahead de verdade — e a tarefa foi escrita
assim, sem esconder isso. **Oitavo PASS de primeira tentativa seguido**, e
o primeiro que o próprio André rodou sozinho (`loop.py` direto, sem Claude
acompanhando) — tarefa e oráculo entregues numa sessão, executados do outro
lado do limite de uso, na sessão seguinte. Único achado: um warning de
sinal (`size_t` vs `int` no laço de aridade), cosmético, mesma tolerância
já aplicada ao resto do projeto (sem `-Werror`).

Isso não fecha decl-typedef/decl-function/decl-keel — só prova que o passo
mais delicado do despacho (a consulta à symtab) é delegável quando isolado
do resto (achar o declarador em geral, que continua de fora).

