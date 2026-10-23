# T8b — auditoria dos designs (metade B) frente aos normativos

Agente: Haiku 5.5, só leitura. Nenhum arquivo do repositório foi editado,
nenhum `make`, `verify.sh` ou `golden/run.sh` foi rodado. O único arquivo
criado é este relatório.

Convenção usada nos achados: nos designs, `spec §x` pode querer dizer a
spec da ferramenta (`cgen-tool-spec.md`) ou a da linguagem (`keel-spec.md`);
o cartão diz que em `cgen-tool.md` "spec" sozinho é a da ferramenta, e
"linguagem §x" é a `keel-spec.md`. Onde o texto citado diz "spec" mas o
conteúdo é da linguagem, isso aparece como achado (B5, B6).

## 1. Uma linha por design

| Design | Linhas lidas | Achados |
| --- | --- | --- |
| `design/cgen-tool.md` | 924 | 8 (B1 a B8) |
| `design/diag-design.md` | 309 | 5 (B9 a B13) |
| `design/requisitos-v0.md` | 31 | 2 (B14 e B15) |
| `design/analises.md` | 150 | 0 (ver nota ao fim) |

Normativos conferidos: `keel-spec.md` (catálogo §6.2, §1.3, §2.2, §4.1,
§4.3, §4.4, §4.8, §5.1, §5.2), `keel-c-backend.md` (§4.1 a §4.4.1, §5.5.1,
§5.8, §6, §8), `cgen-tool-spec.md` (§4.1 a §4.10, §5, §6, §7, §7.1, §8).
Também confiro o código e os arquivos citados quando a afirmação era de
fato sobre o repositório (`tools/cgen/src/tool/main.c`,
`tools/cgen/diag-messages.md`, `golden/cases/`).

## 2. Achados

### design/cgen-tool.md

**B1**
- **onde:** `design/cgen-tool.md:199-200` (§2.6), e `:437` (§5.1), que dizem o contrário.
- **o design diz:** "Em modo `--stop-after`, as opções repassadas são **lidas** (perfil, lowering, depfile) e o resto é ignorado — `-o` inclusive."
- **o normativo diz:** `cgen-tool-spec.md:160`: "`lex` e `parse` escrevem em `stdout`; com `-o <arquivo>`, escrevem no arquivo, e `-o -` é `stdout`." `cgen-tool-spec.md:161`: "`-o` com `--stop-after=gen` é recusado". E `cgen-tool-spec.md:608` (§7.1): `output-with-gen`, código 2.
- **tipo:** contradição.
- **correção proposta:**
  > Em modo `--stop-after`, as opções repassadas são **lidas** (perfil, lowering, depfile) e o resto é ignorado. `-o` não é ignorado: em `lex` e `parse` nomeia o arquivo da saída (`-o -` é `stdout`); em `gen` é recusado com `output-with-gen` (código 2), porque a fase escreve vários arquivos.
- **certeza:** alta.

**B2**
- **onde:** `design/cgen-tool.md:113` (linha `--parallel-lowering` da §2.2) e §2.6 (`:189-204`), onde a regra não aparece.
- **o design diz:** nada sobre a combinação `--parallel-lowering=openmp` sem `-fopenmp`; a tabela só diz "`auto` | `openmp` | `serie`" como valores aceitos.
- **o normativo diz:** `cgen-tool-spec.md:299`: "Pedir `openmp` sem `-fopenmp` na linha faz o cgen recusar a invocação"; `cgen-tool-spec.md:606` (§7.1): `lowering-unavailable`, código 2, "`--parallel-lowering=openmp` sem `-fopenmp` na linha".
- **tipo:** omissão. Sem essa regra, a implementação poderia cair em série em silêncio, que é justamente o que a spec proíbe ("entregar outra coisa em silêncio é pior do que parar", `cgen-tool-spec.md:299`).
- **correção proposta:** acrescentar à §2.2, logo abaixo da tabela:
  > `--parallel-lowering=openmp` sem `-fopenmp` entre as opções repassadas é `lowering-unavailable` (código 2), e o cgen para antes de compilar. `serie` está sempre disponível; `auto` não recusa.
- **certeza:** alta.

**B3**
- **onde:** `design/cgen-tool.md:549-557` (§6, a chamada ao `cc`), e `:108-109` (`--checks`, `--line` na §2.2).
- **o design diz:** a chamada tem `<--cc> <repasse…> [main] -I D`, sem nada sobre `--checks` nem `--line`.
- **o normativo diz:** `cgen-tool-spec.md:105`: `--checks=off` "passa `-DKEEL_CHECKS=0`". `cgen-tool-spec.md:106`: `--line` controla a emissão de `#line` (`keel-c-backend.md` §6). O `golden/run.sh` já passa `-DKEEL_CHECKS=$chk` ao `cc`.
- **tipo:** omissão.
- **correção proposta:** acrescentar à §6, depois da lista de regras:
  > - `--checks=off` acrescenta `-DKEEL_CHECKS=0` à chamada; com `on` (o padrão), nada é acrescentado: o prelúdio já vale `KEEL_CHECKS 1` quando a macro não existe (backend §5.17).
  > - `--line=off` não emite `#line` (backend §6); não muda a chamada ao `cc`.
- **certeza:** alta.

**B4**
- **onde:** `design/cgen-tool.md:400` (§3.3, passo 10, `--main`).
- **o design diz:** "`--main`: gera `main_<símbolo>.c` (§5.3)", sem dizer o que acontece quando o módulo não tem função de entrada pública.
- **o normativo diz:** `cgen-tool-spec.md:282-283` (§4.7) e `:605` (§7.1): `missing-entry-point`, código 2, "`--main` sobre módulo sem função de entrada pública".
- **tipo:** omissão (menor: é diagnóstico de invocação, e a §4 do design remete à spec).
- **correção proposta:** no passo 10 de §3.3, acrescentar: "Se o módulo pedido não tem função de entrada pública, `missing-entry-point` (código 2), antes de qualquer escrita."
- **certeza:** alta.

**B5**
- **onde:** `design/cgen-tool.md:478, 492, 498, 500, 501, 503, 504, 509, 514` (§5.2, coluna "Conteúdo" da tabela de espécies e marcas).
- **o design diz:** por exemplo, `unavailable`: "(spec §4.3, regras 11 a 13)"; `defer`: "(spec §4.6)"; `range-index`: "(spec §5.1)"; `array-index`: "(spec §2.3)"; `parallel`: "(spec §4.8)"; `else`: "(spec §4.10)"; `extent`: "(spec §4.11)"; `&<k>`: "(spec §4.4)"; `slice.of`: "(spec §5.3)".
- **o normativo diz:** essas referências são de `keel-spec.md` (regras 11 a 13 em `keel-spec.md:815-817`; §4.6 é o cleanup; §5.1 os protocolos; §2.3 o reconhecimento; §4.8 `parallel`; §4.10 `else`; §4.11 `extent`; §4.4 resolução; §5.3 buffer/slice). Na spec da ferramenta, `cgen-tool-spec.md` tem só §§ 1–10 sem subseções em §2, §5 e §6 e não tem §2.3, §4.11 nem §5.3. Pela convenção do próprio documento, "spec §x" aponta para seções que não existem ou tratam de outro assunto.
- **tipo:** referência errada.
- **correção proposta:** trocar "spec §" por "linguagem §" nas nove linhas: `(linguagem §4.3, regras 11 a 13)`, `(linguagem §4.6)`, `(linguagem §5.1)`, `(linguagem §2.3)`, `(linguagem §4.8)`, `(linguagem §4.10)`, `(linguagem §4.11)`, `(linguagem §4.4)`, `(linguagem §5.3)`.
- **certeza:** alta.

**B6**
- **onde:** `design/cgen-tool.md:907` (P16: "Spec §4.1, backend §4.1"), `:912` (P15: "spec §4.3", "spec §4.4"), `:924` (P14: "spec §4.3").
- **o design diz:** "Spec §4.1" para o `extern_c` que segue `pub`/`priv`; "spec §4.3" para a instanciação degenerada; "spec §4.4" para o `verb-not-in-instance`; "spec §4.3" para o `protocol-on-parameter`.
- **o normativo diz:** o `extern_c` está em `keel-spec.md:636-637` (§4.1, regra 12) e `keel-c-backend.md` §4.1. A "declaração de módulo genérico que não menciona parâmetro" está em `keel-spec.md` §4.3, regra 20. `verb-not-in-instance` é `keel-spec.md:2784` (catálogo, citado como §4.4). `protocol-on-parameter` é `keel-spec.md:2731`, na §4.3. Em `cgen-tool-spec.md`, §4.1 é a tabela de opções, §4.3 é "Compilação separada" e §4.4 é o exemplo: nenhum desses trata desses assuntos.
- **tipo:** referência errada (ambígua pela convenção do documento).
- **correção proposta:** em §13, trocar cada "Spec §4.1", "spec §4.3", "spec §4.4" pelo "linguagem §" correspondente: "linguagem §4.1 (`keel-spec.md`, regra 12)", "linguagem §4.3, regra 20", "linguagem §4.4 (catálogo, `verb-not-in-instance`)", "linguagem §4.3 (`protocol-on-parameter`)".
- **certeza:** alta.

**B7**
- **onde:** `design/cgen-tool.md:693-694` (§8.3, o `gen/main_hello.c`).
- **o design diz:** "inclui o `.h` do módulo, como qualquer `.c` (regra 3 do backend §4.3.2)".
- **o normativo diz:** `keel-c-backend.md` §4.3.2, regra 3 trata das quatro seções do `.h`. Quem diz que o `.c` inclui o próprio `.h` é a regra 4 ("`.c` inclui o próprio `.h` e o `.h` de cada módulo ou instância que usa"). A forma do `main` está em `keel-c-backend.md` §5.8 (`:1254-1268`), que o design deveria citar.
- **tipo:** referência errada.
- **correção proposta:** trocar por "(backend §4.3.2, regra 4, e §5.8)".
- **certeza:** alta.

**B8**
- **onde:** `design/cgen-tool.md:868-870` (§10, Testes).
- **o design diz:** "Enquanto o golden tiver prosa explicativa nos cabeçalhos (**[P5]**), o diff ignora o comentário de abertura de cada arquivo."
- **o normativo diz:** `design/cgen-tool.md:919-920` (§13) resolve o P5 em 2026-09-19. Hoje, cada arquivo de `golden/cases/*/expected/` tem só uma linha de cabeçalho gerado, por exemplo `golden/cases/001-arena-buffer-defer/expected/c23/app/app_cfg.c:1`: `/* app/app_cfg.c — generated from app/cfg.k by cgen, C23 profile. */`. Não encontrei prosa nos cabeçalhos do golden.
- **tipo:** estado superado apresentado como atual. A condição está falsa, e o marcador [P5] foi resolvido.
- **correção proposta:** trocar o parágrafo por: "O cabeçalho de cada arquivo é a linha de geração (uma linha, sem versão nem data, `design/cgen-tool.md` §5.3), e o diff a compara junto com o resto."
- **certeza:** alta quanto ao estado do golden; a intenção do autor pode ser manter o diff tolerante a cabeçalhos, o que é dúvida de redação, não de fato.

### design/diag-design.md

**B9**
- **onde:** `design/diag-design.md:111` (§3), e também `tools/cgen/diag-messages.md:29`, que repete o mesmo termo.
- **o design diz:** "`Condition` ... é o que o `--cgen-help=<classe>` mostra".
- **o normativo diz:** `cgen-tool-spec.md:114`: `--cgen-help`, `--help` imprimem a sinopse, sem valor. `cgen-tool-spec.md:118-124`: "`--help=<classe>` atravessa verbatim" (vai ao `cc`). A implementação segue isso: `tools/cgen/src/tool/main.c:99` só reconhece `--cgen-help` e `--help`.
- **tipo:** contradição (a opção não existe na superfície congelada, e a spec diz que a ajuda por classe é do `cc`).
- **correção proposta:** trocar a frase por: "é o que a documentação de cada diagnóstico mostra. O cgen não tem ajuda por classe: `--help=<classe>` vai ao `cc` (ferramenta §4.1). Se a ajuda por condição for desejada, é uma opção nova e passa pela spec da ferramenta antes de existir." Aplicar a mesma troca em `tools/cgen/diag-messages.md:29`.
- **certeza:** alta.

**B10**
- **onde:** `design/diag-design.md:25` (§1).
- **o design diz:** "Nenhuma mensagem de diagnóstico do keel existe escrita em lugar nenhum do repositório."
- **o normativo diz:** `tools/cgen/diag-messages.md` existe (173 linhas de identificadores; 141 com mensagem), e `tools/cgen/src/engine/diag_messages.def` é gerado dele. O próprio design, na §3 (`:107-111`), descreve esse arquivo como existente.
- **tipo:** estado superado apresentado como atual; o §1 contradiz a §3 do mesmo documento.
- **correção proposta:** "As mensagens já existem em `tools/cgen/diag-messages.md` (uma linha por identificador; 141 das 173 linhas têm mensagem), e o `gen-diags.py` as gera para `src/engine/diag_messages.def`. O que falta é a mensagem das linhas ainda vazias, e os casos que as afirmam."
- **certeza:** alta.

**B11**
- **onde:** `design/diag-design.md:23` ("149 identificadores"), `:175` e `:245` e `:284` ("135"), `:286` ("os 7 `debug`").
- **o design diz:** o catálogo tem 149 identificadores; faltam cobrir 135; são 7 os `debug`.
- **o normativo diz:** `keel-spec.md:2667-2829` (§6.2) tem 157 linhas de identificador, sem repetição: 139 `error`, 7 `warning`, 2 `info`, 9 `debug`. Os 9 `debug` incluem `par-target-above-total` e `mask-above-64-slots` (`keel-spec.md`, §4.8, "Do backend, em execução debug"), que o design não conta.
- **tipo:** contradição numérica com o normativo.
- **correção proposta:** trocar "149" por "157 (139 `error`, 7 `warning`, 2 `info`, 9 `debug`)". Trocar "os 7 `debug`" por "os 9 `debug`". Para os "135": ver a dúvida Dú3.
- **certeza:** alta para 157 e 9; dúvida para o 135 (ver Dú3).

**B12**
- **onde:** `design/diag-design.md:103-116` (§3, "Gerada, não escrita à mão").
- **o design diz:** "um script (`tools/cgen/gen-diags.py`) extrai as duas colunas e gera o `diag_catalog.def`", e a manutenção da mensagem é descrita só como texto escrito à mão em `diag-messages.md`.
- **o normativo diz:** `tools/cgen/diag-messages.md:31-37` (seção "A manutenção", que o design não cita): `tools/cgen/diag-sync.py` pede ao Claude (Haiku 5.5) a condição e a mensagem das linhas novas ou alteradas, usa `diag-messages.lock.json` para saber o que mudou, e roda `gen-diags.py`. Commit `7787ac0` introduziu essa ferramenta.
- **tipo:** omissão (o design descreve um fluxo que não é mais o único).
- **correção proposta:** acrescentar ao fim do parágrafo "Gerada, não escrita à mão": "A condição e a mensagem das linhas novas ou alteradas passam por `tools/cgen/diag-sync.py`, que pede a tradução ao modelo e guarda o hash da condição em `diag-messages.lock.json`; o `gen-diags.py` gera o `.def` em seguida. O resultado se revisa com `git diff`."
- **certeza:** alta quanto ao fato; a escolha de descrever ou não o script no design é de André.

**B13** (achado no normativo; o design está certo)
- **onde:** `keel-spec.md:2682` (§6.2, `import-clause-order`).
- **o design diz:** `design/diag-design.md:143` (§4): "`import-clause-order`: write `import M as m types;` — `as` comes before `types`".
- **o normativo diz:** `keel-spec.md:2682` diz "`import M types as m;` — a mensagem dá a forma correta". Mas a gramática, `keel-spec.md:301` (`import ::= 'import' module-name [ 'as' IDENT ] [ 'types' ] ';'`), e os exemplos (`keel-spec.md:604`, `:839-841`, `:1356`) põem `as` antes de `types`.
- **tipo:** contradição dentro do normativo (o catálogo contradiz a gramática). O design está certo.
- **correção proposta:** em `keel-spec.md:2682`, trocar o exemplo por "`import M as m types;` — a mensagem dá a forma correta". Como a spec é normativa, a mudança passa pelo André (regra de discussão antes de editar).
- **certeza:** alta.

### design/requisitos-v0.md

**B14**
- **onde:** `design/requisitos-v0.md:9` (item 3).
- **o design diz:** "Passes em ordem fixa: coleta, resolução, expansão (vazia na v0), papéis, `defer`, emissão."
- **o normativo diz:** `keel-spec.md:135-136` (§1.3): "Na v1, expandir moldes com origem e vínculo de nomes preservados e **reanalisar a AST resultante antes de papéis e `defer`** (§4.13)." A reanálise é um passe próprio, depois da expansão e antes dos papéis.
- **tipo:** omissão. O documento diz que lista os requisitos para que as construções v1 não exijam reescrita, e um passe de v1 está fora da lista.
- **correção proposta:** "3. **Passes em ordem fixa:** coleta, resolução, expansão (vazia na v0), reanálise da AST expandida (vazia na v0), papéis, `defer`, emissão."
- **certeza:** alta.

**B15**
- **onde:** `design/requisitos-v0.md:13-14` (item 5).
- **o design diz:** "As construções resolvem verbos por protocolo → verbos, não por nomes fixos em cada construção."
- **o normativo diz:** `keel-spec.md:1958-1968` (§5.1, "O que o núcleo conhece pelo nome"): "Fora dos protocolos, os pontos abaixo ligam uma construção a um módulo determinado. A lista é fechada": `arena`, protocolos, `array`, `a..b`, `parallel`.
- **tipo:** omissão. O requisito é verdadeiro como regra geral, mas a exceção fechada é o que a implementação precisa saber para não generalizar demais.
- **correção proposta:** acrescentar ao item 5: "As exceções são a lista fechada da linguagem §5.1 ('O que o núcleo conhece pelo nome'): `arena`, o `= {0}` e a recusa de parâmetro por valor; `array`; `a..b`; `parallel`."
- **certeza:** média: é uma escolha de redação sobre se o requisito deve citar a exceção.

### design/analises.md

Nenhum achado. O documento é não normativo e datado (2026-09-29), e o próprio
cabeçalho diz que serve para "comparar o que se pensava com o que aconteceu".
Três afirmações ficaram para trás, sem serem incoerências:
- "o golden o verifica com `-Werror=vla`": verdadeiro (`golden/run.sh:16`).
- "140 diagnósticos" (hoje 157, `keel-spec.md` §6.2): era a contagem da data.
- "Os genéricos não servem para escrever algoritmos" e o item "Protocolo nominal com `bound` primeiro": depois da data, o commit `e37d6bf` (2026-10-01) incluiu "protocolos estruturais" e "funções sobre protocolo" (`keel-spec.md:1742`, §4.14). Se André quiser, uma nota de atualização no fim do documento seria o lugar.

## 3. Dúvidas

- **Dú1 (requisitos, passes).** Onde cai a baixa das construções que terminam escopo (`win`/`fail`, `break` no braço de `match`, `else`), que `keel-c-backend.md` §5.5.1 regra 1 (`:1120`) põe antes de `defer`? A lista de passes do requisito não tem lugar para ela. Pode ser "expansão" (que é vazia na v0) ou um passe próprio. Não consegui decidir pelo normativo.
- **Dú2 (ferramenta, depfile).** `design/cgen-tool.md:140` e `:571-573` leem `-MQ` como opção de alvo. `cgen-tool-spec.md:150` lista só `-MMD`, `-MD`, `-MF`, `-MT` como opções de dependência lidas pelos dois lados. Se `-MQ` deve ser lida, a spec precisa dizer; se não, o design tem de deixar de lê-la.
- **Dú3 (diag, o 135).** O número 135 aparece três vezes em `diag-design.md` e não bate com nenhuma contagem simples do catálogo (139 `error`; 148 fora dos `debug`). Pode ser uma conta de outra época. André tem de dizer se o alvo é "todos os não-debug" (148) ou "os de erro" (139).
- **Dú4 (diag, `info`).** A decisão "`info` sai por padrão" (`diag-design.md:308`, R1, resolvida em 2026-10-02) está só no design. `cgen-tool-spec.md` §7 diz "Severidades `error`, `warning`, `info`" e não diz o padrão, e `keel-spec.md` §6.1 também não. Se a decisão vale para o usuário, deve ir para a spec da ferramenta; se é só do motor, pode ficar no design.
- **Dú5 (ferramenta, `--instance`).** `design/cgen-tool.md:244` usa `module-not-found` quando o módulo de um `--instance` não é achado. `cgen-tool-spec.md:601-605` (§7.1) define `module-not-found` como "um `import` que nenhuma raiz resolve". Ou se estende a definição, ou o caso vai para `invalid-instance`.
- **Dú6 (ferramenta, modo transparente).** `design/cgen-tool.md:192` e `:382` falam em `execvp` do `cc`; `:281` (`cc.c`) e `:562` falam em `posix_spawnp`. Não é conflito normativo, mas são duas formas de chamar o `cc` no mesmo documento. Se o modo transparente for `execvp`, a §6 deve dizer isso.
- **Dú7 (ferramenta, plano de arquivos).** A tabela de `tools/` da §3.1 (`args.c`, `writer.c`, `depfile.c`, `cc.c`) não bate com `tools/cgen/src/tool/` hoje (`match_option.c`, `output.c`, `roots.c`, `read_source.c`, `stop_lex.c`, `stop_parse.c`, `base_resolve.c`). A tabela é plano, e não normativo; vale atualizar quando M3 e M4 fecharem.

## 4. Colateral, fora da metade

- `keel-c-backend.md` §8 (`:1953-1964`) lista 7 identificadores `debug`. O catálogo tem 9: faltam `par-target-above-total` e `mask-above-64-slots`. É a mesma contagem de B11, vista do lado do backend.
