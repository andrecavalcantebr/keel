# moldes (`keel_code`) — estudo de impacto na documentação e no código

**Status: estudo para discussão, não normativo.** Acompanha a proposta
["Moldes: a marca `keel_code`"](possibilidades.md#moldes-a-marca-keel_code-2026-09-30)
em `possibilidades.md` e parte dela. Nada aqui altera um normativo: cada mudança
nos documentos abaixo só é escrita depois do "sim" do André, como pede o
`CLAUDE.md`.

**O que foi verificado e o que não foi.** Li os arquivos citados e as linhas
indicadas, e rodei `golden/run.sh` e `make check` antes (verdes). **Não** medi
o custo de implementação: os tamanhos abaixo são relativos ("pequeno", "médio",
"grande"), por leitura da estrutura, e não por protótipo. Onde afirmo o que o
código faz, cito o ponto; onde é hipótese, digo.

## 1. Resumo

O molde é uma função marcada `keel_code` que o cgen **expande em fichas no
ponto de chamada**, com locais renomeados, resolução de nomes no módulo do molde
e prelúdio içado para o início do statement. Ele substitui o tratamento
especial do `arena.from_stack`, que hoje nomeia o módulo no parser
(`parser_islands.c:1036`).

**Timing.** A emissão (M3, `emit.c`) ainda **não existe**: o cgen para em
`--stop-after=parse`, e o `from_stack` especial existe só como ilha no despejo.
Fazer o molde antes da emissão evita escrever, na emissão, um caso especial que
seria descartado depois. Este é o argumento principal para não esperar.

| Área | Tamanho | Observação |
| --- | --- | --- |
| Documentação normativa | médio | muitas seções, mas mudanças curtas; depende do "sim" |
| Gramática e tabela de símbolos | pequeno | `params_of` já reconhece `type` e `array` |
| Expansão (substituição, renome, origem) | médio | módulo novo, sem depender do resto |
| Prelúdio e posição da chamada | médio a grande | o `walk` hoje não acompanha profundidade de parênteses |
| Armazenamento sem alocação | médio | o engine não aloca; a expansão precisa de buffers do `tool` |
| Diagnósticos, dump e testes | pequeno a médio | o catálogo e o `diag_catalog.def` são gerados |
| Golden | pequeno, mas à mão | c11 e c23 escritos à mão, nunca derivados |

## 2. Documentação

Cada linha é uma mudança **proposta**; nenhuma está feita.

| Documento | Seção | Mudança | Ligação |
| --- | --- | --- | --- |
| `keel-spec.md` | §1.2 e §1.3 | dizer que keel expande **moldes** (ficha a ficha), e continua sem expandir macros de C; o contrato de análise passa a incluir a análise do código expandido | rationale "Macros e sintaxe de keel" |
| | §2.1 e §2.2 | tabela de palavras (l. 193, 212, 239): `keel_code`, `child`, `parent`, `invalidates` e `dim` em parâmetro de função; produção de função com o qualificador; parâmetro `keel_code` | lexer `lexer.c:26` |
| | §2.3 | uma chamada a molde é ilha; o símbolo conhecido faz a ilha, como qualquer verbo | ilha nova |
| | §4.4 | o molde entra na resolução (passo 1: verbo do tipo do contêiner); a expansão é depois da resolução | |
| | nova subseção (§4.12, ou dentro de §4.4) | o contrato do molde: parâmetros, expansão, `return`, posição, limites, higiene | proposta em `possibilidades.md` |
| | §5.1 | a lista do que o núcleo conhece pelo nome perde o `from_stack` e, com os papéis, o resto da arena; papéis de procedência na assinatura | proposta dos papéis |
| | §5.2 | `from_stack` passa a ser um molde da biblioteca; a regra 10 (procedência) passa a vir dos papéis | |
| | §6.2 | catálogo: novos `circular-mold`, `mold-depth`, `mold-size`, `mold-position`, `mold-return`, `mold-argument`; sai `nonconstant-arena-stack` (o `nonconstant-dim` cobre o `dim`) | `gen-diags.py` gera o `.def` a partir daqui |
| | §6.3 a §6.5 | garantias e limites: a expansão é ficha a ficha; o que duplica código; o que o molde não garante | |
| `keel-rationale.md` | "Macros e sintaxe de keel" (l. 1412) | reconciliar: a rationale diz que keel evita colagem de fichas e avaliação duplicada, e o molde respeita isso (sem `##`, valor comum avaliado uma vez, higiene) | |
| | nova seção "Moldes" | o motivo: macros escapam da análise; `template` e `macro` descartados; o `inline` de C não move código para o escopo do chamador | |
| `keel-c-backend.md` | §2.3 | nomes reservados: o esquema de nome dos locais de molde (`keel__<molde><n>_<local>`) | |
| | §5.4 | `arena.from_stack` deixa de ser emissão especial | |
| | §6 | mapeamento de linhas do código expandido para o ponto de uso | |
| | nova subseção | emissão de molde: prelúdio no início do statement, `(void)(E);` em statement, sem função no C | |
| `cgen-tool-spec.md`, `design/cgen-tool.md` | §5.2 e o plano (§9) | formato do despejo para a ilha `mold` e suas ilhas aninhadas; um marco de molde antes do M5 | |
| `design/parser-design.md` | §2.3 e §3 | a passagem 3 percorre também o trecho expandido; a estrutura de `KIsland` e de `KLexeme` | |
| `design/codegen-design.md` | emissão | inserção de prelúdio e substituição da chamada | |
| `design/diag-design.md` | catálogo e testes | novos casos de falha | |
| `golden/README.md`, `NOTES` | casos | o caso novo e a mudança do 001 | |
| `base/keel/arena.k` | `from_stack` | vira molde; o comentário do cabeçalho (l. 15) muda | |

**Regras de trabalho que isso toca.** Os perfis c11 e c23 do golden são
escritos à mão (`CLAUDE.md`), e "a emissão segue o `.k` sempre". O molde muda o
`.k` da base, então o esperado é reescrito à mão nos dois perfis.

## 3. Código

### 3.1 Pontos de contato verificados

| Onde | O que existe hoje | O que muda |
| --- | --- | --- |
| `engine/storage_types.h` | `KSymKind`, `KAstNode`, `KIslandKind` (`K_ISLAND_FROM_STACK`), `KLexeme` (`token`, `pp_kind`, `directive`) | `K_SYM_MOLD`; `KAstNode.is_mold` (o `is_inline` existente é o `inline` de C, `ast.c:167`); `K_ISLAND_MOLD` no lugar de `K_ISLAND_FROM_STACK`; em `KLexeme`, a origem (`fonte`, `corpo`, `argumento`), o módulo de origem e o `site` (índice da ficha de chamada) |
| `engine/ast.h` | `KAst` com `tokens`, `nodes`, `islands`, `island_text`; "Source and token storage outlive the tree" | região de fichas expandidas anexada a `tokens`, e um buffer de texto para as fichas sintetizadas |
| `engine/loader.h` | `KModule` retém `ast` ("retained source-backed tree") | nada: o corpo do molde é lido de `KModule.ast`, pelos `body_first/body_end` do nó da função. Já há precedente de ler a AST do módulo importado para assinaturas (`parser_islands.c:377`, `:844`) |
| `engine/parser_islands.c:347` | `params_of` reconhece o marcador `type` e `array` do parâmetro (`Param.is_type`, `.is_array`) | `Param.is_dim` e `.is_code`; os papéis (`child`, `parent`, `invalidates`) |
| `engine/parser_islands.c:1036-1090` | o `from_stack` é tratado pelo nome (`module_named(home, "keel.arena")`), redirecionado a `from_array`, com `decimal_of` | sai; entra o ramo geral de molde |
| `engine/parser_islands.c:1693` (`walk`) | uma varredura linear sobre `[n->first, n->end)`, com `depth` de chaves, `has_target`, e a pilha `locals` | refatorar em varredura de intervalo com pilha de expansões; acompanhar profundidade de parênteses para classificar a posição |
| `engine/parser_islands.c:94` (`live`) | uma ficha é viva se não é diretiva | igual, também para fichas expandidas |
| `engine/diag.h` | `KDiagnostic.at` é "a view of the source: position and extent" | diagnóstico dentro de expansão usa a ficha de chamada como `at` (as fichas de outro módulo, ou do buffer de texto, não têm posição neste fonte) |
| `engine/diag_catalog.def` | gerado por `tools/cgen/gen-diags.py` a partir do catálogo da spec §6.2 | regenerar depois de a spec mudar; as mensagens vão à mão em `diag.c` |
| `engine/lexer.c:26` | lista de palavras keel | `keel_code` e as palavras de papel |
| `engine/parser_dump.c` e o oráculo | linha `ilha	from-stack	arena.from_stack/2 → keel_arena_from_array &1` (`test/parse/001-...parse:32`) | linha `ilha	mold ...` e ilhas aninhadas |
| `tool/tool.c:84`, `names.c`, `instances.c` | constrói `KSymbol` das declarações e exporta pelos `KModule.symbols` | símbolo de molde exportado, com o corpo alcançável pela AST retida |

### 3.2 A expansão

**Onde entra.** As fichas expandidas são **anexadas** ao buffer `tokens`, depois
das fichas do fonte, e uma ilha `K_ISLAND_MOLD` guarda o intervalo `[exp_first,
exp_end)`. O `walk` percorre esse intervalo com o mesmo laço. Como tudo é
índice de ficha em intervalo semiaberto, isso é coerente com o resto da árvore.
Uma ficha do corpo é uma fatia do fonte do módulo do molde (mantido vivo pelo
`KModule.ast`); uma ficha renomeada (`keel__greet0_buf`) não existe em nenhum
fonte, e precisa de um buffer de texto próprio.

**Sem alocação.** O engine não aloca (`lexer.h`, `loader.h`) e segue o
padrão "duas passadas, contar e preencher" (`ast.h`). A expansão não sabe o
tamanho de antemão, então a `tool` reserva a capacidade das fichas e do texto,
e o limite `mold-size` **é** o contrato de capacidade: ele impede a explosão
exponencial e garante que o buffer não estoure. A constante mora junto dos
limites da `tool` (`cli_limits.h`) ou de `memory.h`, e o teste de unidade
exercita o estouro.

**Origem das fichas.** É um campo em `KLexeme`. Com ele:

- as fichas de `corpo` resolvem os nomes no módulo do molde, e as de
  `argumento`, no chamador;
- a detecção de ciclo considera só a cadeia de fichas de corpo;
- o `site` dá a posição para diagnósticos e para o `#line`.

**Posição da chamada.** É a parte mais custosa. O `walk` de hoje conta chaves e
`;`, mas não parênteses nem palavras de contexto. Classificar "condição de
`if`", "operando direito de `&&`", "laço" e "sub-statement sem chaves" exige
rastrear o início do statement, a profundidade de parênteses e as palavras
`if`, `else`, `for`, `while`, `switch`, `&&`, `||`, `?`. Estimo trabalho médio a
grande, e é onde eu esperaria mais casos de falha.

### 3.3 Módulos novos e alterados

- **`engine/mold.c`** (novo): coleta o molde na declaração (parâmetros por
  tipo, corpo como fichas, locais declarados, `return` final, chamadas a outros
  moldes).
- **`engine/mold_expand.c`** (novo): substituição, renome, origem, prelúdio e
  `return`, com limites.
- **`engine/parser_islands.c`** (alterado): o ramo de molde na chamada, a
  varredura de intervalo, e a classificação de posição.
- **`engine/storage_types.h`, `ast.h`, `lexer.c`, `diag.c`, `parser_dump.c`,
  `tool/tool.c`** (alterados, pequenos).

Isso é estimativa da forma, e não um projeto de função a função.

## 4. Testes e golden

- **Unidade:** o expansor com um `KLoader` de mentira, como pede o desenho do
  loader ("um teste entrega bytes"), sem diretório temporário: substituição
  (`type`, `dim`, `keel_code`, valor), renome, origem, `return`, ciclo,
  profundidade e tamanho.
- **Oráculo de parse** (`tools/cgen/test/parse/`): o `001` muda a linha da ilha
  do `from_stack`; casos novos para `greet` e para o `from_stack` duas vezes no
  mesmo bloco.
- **Diagnósticos** (`tools/cgen/test/diag/`): um diretório `molds/` com um caso
  de falha por identificador novo. A cobertura do catálogo (hoje 29 de 140) sobe.
- **Golden:** o caso `001-arena-buffer-defer` usa
  `if (!arena.from_stack(t, 4096)) return ...` (`golden/cases/001-.../app/cfg.k:33`);
  o esperado dos dois perfis muda o nome `keel__st0` para o esquema do molde.
  Um caso novo (por exemplo `026-mold`) cobre parâmetro de valor, dois moldes no
  mesmo bloco e o prelúdio em posição de statement. Os dois perfis são escritos à
  mão; a skill `cgen-harness-task` cobre o esperado do parser.

## 5. Fases e aceitação

| Fase | Entrega | Aceitação |
| --- | --- | --- |
| 0 | papéis de procedência na assinatura (`child`, `parent`, `invalidates`) | parse e diagnósticos de procedência sobre papéis |
| 1 | declaração do molde: qualificador, parâmetros `type` e `dim`, coleta do corpo | despejo de declaração; unidade do coletor |
| 2 | expansão sem prelúdio (só expressão) | unidade do expansor; ilha `mold` no despejo |
| 3 | prelúdio e classificação de posição | `from_stack` em `if`, atribuição, `&&` (erro) e sub-statement sem chaves (erro) |
| 4 | limites e diagnósticos | os seis casos de falha novos |
| 5 | `from_stack` da base vira molde; golden 001 e caso novo | `golden/run.sh` e `make check` verdes |
| 6 | parâmetro `keel_code` e a forma com bloco | depois do `construct`; fora deste estudo |

A fase 5 é a que troca o tratamento especial pelo molde; até lá o `from_stack`
continua como está. As fases 0 a 5 vêm antes da emissão (M3), e o molde chega a
ela como uma ilha a mais.

## 6. Riscos e pontos a decidir

- **Custo do prelúdio e da posição.** O maior risco de implementação, e o que
  eu mediria primeiro num protótipo pequeno.
- **Duplicação de código.** Um parâmetro `keel_code` usado duas vezes duplica o
  fragmento. É aceitável, mas precisa constar nos documentos.
- **Golden mudando de nome.** `keel__st0` deixa de ser o nome. Manter o nome
  antigo seria possível com um esquema de nome próprio por molde, ao custo de uma
  regra a mais. Decisão do André.
- **Ordem no plano.** Este estudo propõe as fases 0 a 5 antes do M5 (base). Se o
  plano prefere o M5 primeiro, o `from_stack` especial precisa ser emitido antes
  e depois removido.
- **O que a expansão não cobre:** alias, retorno indireto e efeito
  interprocedural continuam fora do alcance da análise, com ou sem molde.
