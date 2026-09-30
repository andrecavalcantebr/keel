# protocolo nominal — estudo de impacto na documentação e no código

**Status: estudo para discussão, não normativo.** Acompanha a entrada
["Protocolo nominal"](possibilidades.md#protocolo-nominal) de `possibilidades.md`
e parte dela. Nada aqui altera um normativo: cada mudança nos documentos abaixo
só é escrita depois do "sim" do André, como pede o `CLAUDE.md`.

**O que foi verificado e o que não foi.** Li os arquivos e as linhas citados, e
rodei `golden/run.sh` e `make check` antes (verdes). **Não** medi o custo de
implementação: os tamanhos são relativos ("pequeno", "médio", "grande"), por
leitura da estrutura, e não por protótipo. Onde afirmo o que o código faz, cito o
ponto; onde é hipótese, digo.

## 1. Resumo

O protocolo nominal tem quatro partes, com dependências diferentes:

1. **A cláusula `protocol` na linha `module`** e a **declaração `protocol`** (com
   composição).
2. **A exaustividade no implementador**: uma varredura das funções do módulo
   contra os protótipos dos protocolos declarados.
3. **O parâmetro de função tipado por protocolo**, e a verificação no uso.
4. **A instância da função**, por tipo concreto do argumento.

Há ainda o efeito sobre as **construções do núcleo** (§7), que muda o que a
spec §5.1 diz. As partes 1 e 2 **não precisam de instâncias**. Só as partes 3 e 4 dependem da
análise de corpo por instância, que ainda não existe. Isso dá uma ordem natural:
1 e 2 podem ser feitas antes do M5, e 3 e 4 depois.

| Parte | Tamanho | Depende de |
| --- | --- | --- |
| 1. Cláusula e declaração, com composição | pequeno a médio | gramática e tabela de símbolos |
| 2. Exaustividade no implementador | pequeno | `find_verb` e a AST retida do módulo |
| 3. Parâmetro de protocolo e verificação no uso | médio | resolução de verbos pelo tipo do símbolo |
| 4. Instância da função | médio a grande | o M5 (instâncias e análise de corpo por instância) |
| Documentação | médio | o "sim" do André |
| Golden | pequeno a médio | casos novos, c11 e c23 à mão |

## 2. Documentação

Cada linha é uma mudança **proposta**; nenhuma está feita.

| Documento | Seção | Mudança |
| --- | --- | --- |
| `keel-spec.md` | §2.1, §2.2 (tabelas de palavras, l. 193, 212, 239) | `protocol` passa a ser palavra contextual; produção `decl-protocol`; a linha `module` ganha a cláusula `protocol` depois de `type` |
| | §2.3 | a chamada a uma função com parâmetro de protocolo é ilha |
| | §4.3 | a cláusula `protocol` **não é binder**: não entra na aridade dos modificadores (l. 758); a regra 7 (o `T` opaco, l. 773) continua para o módulo genérico |
| | §4.4 | a resolução do verbo pelo tipo do contêiner passa a considerar o tipo protocolo no corpo da função (só os verbos do protocolo) |
| | §5.1 | os sete protocolos (tabela "Verbos exigidos por construção") passam a ser declarados em `keel.protocols`, com a coluna "Declarado na base por" virando a cláusula das linhas `module`; a regra 1 ("Declarar os verbos basta... Não há registro, marcação nem permissão") muda ou não conforme a saída (§7); a lista "O que o núcleo conhece pelo nome" ganha `keel.protocols` |
| | §§4.5 a 4.9, 5.3 a 5.7 | as construções (`x[i]`, `x[a..b]`, `foreach`, `walk`, `parallel`, `match`, `else`) passam a citar o protocolo; os diagnósticos de conformidade escritos à mão viram `protocol-not-satisfied` |
| | §5.2 | os protocolos de alocação e hierarquia definidos por papéis de procedência |
| | §6.2 | catálogo: `protocol-not-satisfied`, `protocol-verb-missing`, `verb-not-in-protocol`, `protocol-verb-conflict`, `circular-protocol`, `instance-depth`; o `protocol-on-parameter` continua para o parâmetro de tipo do módulo genérico |
| `keel-rationale.md` | nova seção "Protocolos" | nominal, sem `bound`, regra da orientação a objetos, exaustividade sem tipos C, sem transitividade, por que não é o molde |
| `keel-c-backend.md` | §2.1 e §2.1.1 | mangling da instância de função (`<módulo>_<função>_<tipos>`), com o sufixo de aridade por último |
| | §4 | header da instância de função (regras 1 a 5); quem instancia é quem usa |
| | §5 | emissão do parâmetro (ponteiro ou valor pelo `byref`) e do `&` na chamada |
| `cgen-tool-spec.md`, `design/cgen-tool.md` | §5.2 | o despejo de declaração mostra a cláusula `protocol` e a declaração; a ilha `call` de função com protocolo |
| `design/parser-design.md` | §2.3, §3, §5 | a cláusula na tabela de símbolos; a exaustividade na passagem 2; a instância de função em §5 |
| `design/codegen-design.md`, `design/diag-design.md` | emissão; catálogo e testes | instância de função; casos de falha |
| `golden/README.md`, `NOTES` | casos | os casos novos |
| `base/keel/*.k` | linhas `module` de `buffer` e `slice` | `protocol Indexable, Traversable` |
| `base/keel/protocols.k` | (novo) | os protocolos da base |

**Regras de trabalho que isso toca.** O golden é escrito à mão nos dois perfis
(`CLAUDE.md`), e "a emissão segue o `.k` sempre". A linha `module` com a cláusula
**não muda o C emitido**: o que muda são os `.k` da base e os casos novos.

## 3. Código

### 3.1 Pontos de contato verificados

| Onde | O que existe hoje | O que muda |
| --- | --- | --- |
| `engine/lexer.c:24-28` | `k_contextual_words`, ordenada por `strcmp` | acrescentar `protocol` (na ordem) |
| `engine/parser.h:41-48` | `KModuleHeader` com `dims`, `tags`, `types` (8 cada) e `k_scan_module_decl` | `protocols[8]`, `protocol_count`; a cláusula lida depois de `type` |
| `engine/storage_types.h` | `KSymKind`, `KAstNode` (`dim_first`, `tags_first`, `type_first`), `KAstKind` | `K_SYM_PROTOCOL`; `K_AST_PROTOCOL`; `proto_first/proto_end` no nó do módulo |
| `engine/parser.h:32` | `k_scan_ident_list`, a lista de identificadores | reaproveitada para a lista entre colchetes do `protocol Nome [lista]` |
| `engine/parser_islands.c:347` (`params_of`) | reconhece os marcadores `type` e `array` no parâmetro (`Param.is_type`, `.is_array`) | classificar o parâmetro cujo tipo-base é um símbolo `K_SYM_PROTOCOL` |
| `engine/parser_islands.c:377` (`find_verb`) | acha o verbo no módulo pelo nome e a aridade, e diz se há várias aridades (`Sig`) | reaproveitada pela varredura de exaustividade |
| `engine/parser_islands.c:789`, `:836` (`ast_of`) | lê a AST do módulo de origem de um símbolo | lê a linha `module` do implementador para a cláusula |
| `engine/parser_islands.c:1693` (`walk`) | varredura do corpo, `find_local` dá o tipo declarado do símbolo | no corpo de função com parâmetro de protocolo, o tipo do `b` é o protocolo |
| `engine/instances.c:71` (`k_collect_instances`) | dedupe por nome canônico em `KInstanceUse.symbol`, limite de capacidade | um tipo de uso para função, com o nome da instância de função |
| `engine/diag_catalog.def`, `gen-diags.py` | gerado do catálogo da spec §6.2 | regenerar; as mensagens vão à mão em `diag.c` |
| `test/parse/*.parse`, `parse_dump.sh` | oráculo do despejo de parse | linhas novas de declaração e de ilha |

### 3.2 As quatro partes

**1. Cláusula e declaração.** Um nó `K_AST_PROTOCOL` com `decl-protocol ::=
'protocol' IDENT [ '[' IDENT { ',' IDENT } ']' ] ( ';' | '{' { prototype } '}' )`.
O `k_scan_ident_list` já lê a lista. O corpo entre chaves reaproveita a leitura
de protótipos que o `extern_c` e as funções já têm. A cláusula da linha `module`
é lida por `k_scan_module_decl`, e os nomes vão ao nó do módulo. O símbolo
`K_SYM_PROTOCOL` é exportado como os demais, e os protótipos se alcançam pela AST
retida do módulo (`KModule.ast`).

**2. Exaustividade.** Ao fim da passagem 2 do implementador (quando as funções do
módulo estão registradas), para cada protocolo da cláusula:

- calcular o fecho da composição (a união dos protótipos dos incluídos e dos
  próprios), com `circular-protocol` se houver ciclo e `protocol-verb-conflict`
  se um mesmo verbo vier com assinaturas ou papéis diferentes;
- para cada protótipo, `find_verb(home, nome, aridade)`; conferir também a
  posição do receptor e os papéis de procedência.

**Não compara tipos C nem tipos de retorno** (spec §1.3). Só contam os verbos
declarados no **próprio** módulo, sem transitividade. A varredura é sobre uma
lista curta, uma vez por módulo.

**3. Uso.** O tipo do argumento vem do **símbolo** (`find_local` dá o tipo
declarado de `b`), e não de inferência de expressão. Conferir que o módulo do tipo
declara o protocolo, direta ou por fecho (`protocol-not-satisfied`). O
`k_mangle_symbol` já dá o nome canônico do tipo concreto.

**4. Instância.** A chamada `fn(x, b)` cria a instância (função, tipos
concretos). O nome é `<módulo>_<função>_<tipos manglados>`, com o sufixo de
aridade por último. O corpo é analisado de novo com o parâmetro ligado ao tipo
concreto; aí a regra da orientação a objetos vale na declaração (só os verbos do
protocolo sobre `b`) e o verbo resolve pelo módulo concreto na instância. A
passagem (ponteiro ou valor) sai do `byref` da instância, na assinatura e na
chamada, como o `&1` que o despejo já mostra nas ilhas `call`. A instância tem
header próprio, como toda instância de modificador (backend §4, regras 4 e 5).

### 3.3 Ordem e dependências

As partes 1 e 2 não tocam em instâncias, e se verificam com o despejo de
declaração e com casos de falha. As partes 3 e 4 dependem do M5:
`islands.h` diz que o módulo genérico "não tem ilhas concretas" e
`storage_types.h`, que o fecho transitivo é "uma passagem posterior". A instância
de função se apoia nessa passagem.

## 4. Testes e golden

- **Oráculo de parse:** o despejo de declaração passa a mostrar a cláusula
  `protocol` e a declaração; casos para a composição (losango, ciclo, conflito).
- **Diagnósticos** (`tools/cgen/test/diag/`): um diretório `protocols/` com um
  caso de falha por identificador novo. A cobertura do catálogo (hoje 29 de 140)
  sobe.
- **Unidade:** a varredura de exaustividade com um `KLoader` de mentira, sem
  diretório temporário: verbo ausente, verbo em outra aridade, verbo só em módulo
  importado (recusado, sem transitividade), fecho com composição.
- **Golden:** um caso novo (por exemplo `026-protocol`) cobre `fn(x, b)` e
  `fn(x, s)`, com `buffer i32` (por referência) e `slice` (por valor), o protocolo
  composto e o nome das duas instâncias. Os dois perfis são escritos à mão. Os
  casos existentes **não mudam**.

## 5. Fases e aceitação

| Fase | Entrega | Aceitação |
| --- | --- | --- |
| 0 | palavra `protocol`; cláusula na linha `module`; despejo | `lex_dump` e parse de declaração; a base lexa sem diagnóstico |
| 1 | declaração `protocol`, composição, fecho | casos de falha de `circular-protocol` e `protocol-verb-conflict` |
| 2 | exaustividade no implementador; `base/keel/protocols.k`; `buffer` e `slice` declaram | `protocol-verb-missing` e o caso "sem transitividade" |
| 3 | parâmetro de protocolo e verificação no uso | `protocol-not-satisfied`; a regra OO (`verb-not-in-protocol`) |
| 4 | instância da função, nome, header, passagem pelo `byref` | depois do M5; o caso golden novo |

As fases 0 a 2 vêm antes do M5. As fases 3 e 4 vêm depois.

## 6. Riscos e pontos a decidir

- **Onde ficam os binders.** A versão atual põe os binders do protocolo no
  módulo (`module keel.protocols type T;`), e a regra da spec diz que a
  assinatura do módulo fixa a aridade de tudo o que ele declara. Isso obriga
  todos os protocolos de um módulo a dividir a mesma lista. A alternativa é cada
  protocolo declarar os seus (`protocol Indexable type T { ... }`,
  `protocol Keyed type K, type V { ... }`), como um verbo declara o seu
  `type T`. Eu prefiro a segunda: protocolos não têm instâncias nem nomes
  manglados, então a regra que justifica a lista única do módulo não se aplica.
  Custo: a lista de binders entra na gramática de `decl-protocol`, e a composição
  exige que os incluídos tenham a lista igual, por espécie e ordem. A decidir.
- **Binder que o implementador não fornece.** O desenho atual exige a lista do
  protocolo igual à do implementador, então o uso nunca fornece binders
  (`Indexable b`). Um protocolo com um parâmetro a mais, não determinado pelo
  implementador, exigiria fornecê-lo no uso (`Convertible f32 b`), e o nome da
  instância passaria a incluí-lo (`my_fn_keel_buffer_i32_f32`). Fica fora da
  primeira versão.
- **Granularidade da cláusula.** Fica na linha `module`, e vale para todos os
  modificadores do módulo. Um módulo que precise de conjuntos diferentes por
  modificador se divide em módulos; a base tem um por módulo.
- **Nome da instância.** O comprimento é limitado pelo `name-too-long` (§2.4),
  sem regra nova, e a colisão por concatenação cai no `symbol-collision`.
- **O módulo do protocolo no `import`.** O implementador precisa importar o módulo
  do protocolo só para ver o nome, e a cláusula não é um import.
- **Depende do M5.** As partes 3 e 4 só andam depois dele, e um atraso lá os
  atrasa.
- **O que a verificação não cobre:** tipos C e de retorno dos verbos, e a
  existência do corpo: isso é do compilador C.

## 7. As construções do núcleo consomem os protocolos

A spec §5.1 já define sete protocolos (Indexável, Fatiável, Percorrível,
Particionável, Contável, Etiquetado, Falível). Declará-los como protocolos
nominais, em `base/keel/protocols.k`, torna-os o contrato das construções. A
tabela de nomes e os verbos estão em `possibilidades.md`, na entrada "Protocolo
nominal".

**Por que entra no mesmo estudo.** A regra "só os verbos do protocolo sobre `b`"
só é coerente se os verbos de uma construção estão dentro do protocolo dela. Sem
isso, `foreach (T x : b)` sobre um parâmetro `Indexable` não teria como ser
verificado na declaração.

**Módulos da base que passam a declarar** (a coluna "Declarado na base por" da
spec §5.1 vira a cláusula `protocol` da linha `module`):

| Módulo | Cláusula proposta |
| --- | --- |
| `keel.buffer`, `keel.slice` | `protocol Indexable, Sliceable, Traversable, Partitionable` |
| `keel.range` | `protocol Partitionable, Countable` |
| `keel.array` | `protocol Sliceable` (e `Indexable` pelo núcleo) |
| `keel.tagged`, `keel.corot` | `protocol Taggable` |
| `keel.outcome` | `protocol Failable` |

**Dois efeitos que precisam de decisão.**

1. **A regra 1 da §5.1.** "Declarar os verbos basta; não há registro, marcação nem
   permissão." A cláusula `protocol` é um registro. Na saída **híbrida** (a que eu
   prefiro), as construções continuam resolvendo pelos verbos e a regra 1 não
   muda; a cláusula só é exigida para passar o tipo como argumento de protocolo. Na
   saída **nominal em tudo**, as construções também a exigem, e quebram quatro
   fontes do golden que implementam `begin`, `has_next` e `next` por estrutura:
   `019-walk/lst.k`, `022-linux-list-import-c/tasks.k`,
   `023-linux-list-extern-c/tasks.k`, `024-linux-list-module/tasks.k`. O esperado
   dessas não muda de nome nem de conteúdo: só o `.k` ganha a cláusula. Como o golden
   é normativo, isso é uma decisão do André.
2. **Tipo associado.** O `walk` exige que o usuário escreva o tipo do cursor, "o
   produto declarado de `begin`" (spec §4.7), e o binder de `parallel` escreve o
   tipo da partição. Num corpo genérico sobre `Traversable` ou `Partitionable`,
   esse tipo depende do argumento e não pode ser escrito. `foreach` sobre
   `Indexable` não tem o problema. Saídas: recusar `walk` e `parallel` sobre
   parâmetro de protocolo na primeira versão; declarar o tipo no protocolo e
   nomeá-lo no corpo; ou permitir omitir o tipo do cursor quando o contêiner é de
   protocolo.

**Código.** Nada novo em relação às partes 1 a 4: as construções já procuram os
verbos por nome e aridade (`find_verb`, `parser_islands.c:377`). Muda de onde vem a
lista de verbos exigidos, que deixa de estar escrita no código de cada
construção e passa a ser lida do protocolo. A base ganha `keel/protocols.k`, e as
linhas `module` de `buffer`, `slice`, `range`, `array`, `tagged`, `corot` e
`outcome` ganham a cláusula. O `lex_dump` e o parse de toda a `/base` têm de
continuar limpos.

**`array`.** Participa de `Indexable` pelo núcleo, sem módulo de verbos, e não tem
descritor: a extensão é um `dim` em tempo de tradução. Não deve ser argumento de
parâmetro de protocolo na primeira versão.

**Fases.** A declaração dos sete protocolos e as cláusulas da base entram na
fase 2 (antes do M5), no modo híbrido. A migração das construções para a leitura
dos verbos a partir do protocolo, e o tratamento de `walk` e `parallel` sobre
parâmetro de protocolo, ficam com as fases 3 e 4 (depois do M5).

