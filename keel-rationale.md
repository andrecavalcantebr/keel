# keel — Justificativas de projeto

Este documento reúne as motivações e as decisões de projeto do PPC keel. Os
contratos pertencem à [especificação](keel-spec.md); a representação e a emissão
de C, ao [backend](keel-c-backend.md); a linha de comando, à
[ferramenta](cgen-tool-spec.md). Cada seção responde por uma decisão, e a
especificação aponta para ela no lugar em que a decisão aparece como regra.

## A motivação, em uma frase

> **keel é o que já se faz com truque de macro, escrito como sintaxe.**

Todo programa C sério tem um contêiner genérico, e há três formas de escrevê-lo:
`#define T int` com colagem de token, X-macros, ou `void *` com `sizeof` no ponto
de chamada. As três funcionam, as três compilam, e as três cobram a mesma coisa —
o tipo some do diagnóstico, o depurador mostra `void *`, o erro do compilador
aparece expandido numa linha que ninguém escreveu, e duas bibliotecas nunca
concordam sobre o que é um vetor de `Point`.

O PPC gera o que a macro geraria — struct nomeada, funções inline, sem indireção
e sem overhead — com o nome do tipo intacto do fonte até a mensagem do compilador
C. Nenhuma das três formas deixa de compilar por causa de keel; o que ele oferece
é não precisar mais delas.

E o mesmo vale para o resto do vocabulário: cada construção responde a uma coisa
que **o C não expressa**, ou expressa mal. Unidade, limites que viajam com o
dado, memória por região, saída de escopo com limpeza, ponteiro que diz
um-ou-muitos, conjunto fechado de etiquetas. É esse o teste de admissão, e ele é
mais estreito do que "serve à orientação a dados": inverter layout, por exemplo,
o C expressa perfeitamente — `struct { f32 *x; f32 *y; }` —, e por isso não há
`soa` (ver a tabela de recusas adiante).

Referência: [spec v3 §1](keel-spec.md#1-escopo-e-princípios), princípio 9.

## C usado como o assembly portável

Existe uma camada de software que todo o resto vai por cima: quase todo sistema
operacional, driver, runtime ou interpretador; uma linguagem nova, quando precisa
falar com outra linguagem, fala através dela; um alvo de hardware ganha um
compilador para ela antes de ganhar qualquer outra coisa. Essa camada é o C.

O C ocupa hoje o lugar que o assembly ocupou até os anos 70, mas de forma ainda
mais premente: é o denominador comum da máquina, e de qualquer máquina — ele é
**portável**. Um `.c` pode correr num microcontrolador de oito bits e num cluster
de GPU, com pouca ou nenhuma alteração.

Mas, ao contrário do assembly, continuamos escrevendo à mão a camada de baixo. As
duas saídas que apareceram foram:

- **Sair do C.** C++, Rust, Zig, Go. Cada uma resolve problemas reais, e cada uma
  troca o denominador comum por um seu — outro compilador, outro runtime, outra
  ABI, outro conjunto de alvos.
- **Ficar no C e improvisar.** Macro com colagem de token, X-macro, `void *` com
  `sizeof` no ponto de chamada. Funciona, compila hoje, e some do diagnóstico.

keel é a terceira, e dela saem três fatos — e é deles que os princípios saem:

> **1. O gerado é código que um humano assinaria.** Se você não consegue prever o
> C que uma construção produz, a construção falhou. É por isso que a spec é
> escrita em pares keel/C.

> **2. O PPC não precisa entender C.** Um compilador não entende as macros do
> assembler: ele emite dentro delas. keel faz o mesmo — reconhece as próprias
> construções e copia o resto sem examinar.

> **3. O compilador C é o verificador final.** keel não reimplementa o sistema de
> tipos do C. Erro de tipo aparece no compilador C, mas com os nomes que você
> escreveu no fonte keel.

Os três reaparecem como regra na spec, e nenhum como texto repetido: o primeiro é
o princípio 2, mais a convenção de redação em pares; o segundo é o princípio 7 e
o contrato de análise da §1.3; o terceiro é o princípio 3.

Referência: [spec v3 §1.1](keel-spec.md#11-princípios-de-projeto) e
[§1.3](keel-spec.md#13-contrato-de-análise).

## Quem escreve C pensa em máquina; quem escreve keel pensa em C

O programador C mantém na cabeça, o tempo todo, uma imagem aproximada das
instruções que sua linha vira. Não é a forma exata — não precisa ser —, mas é
fiel o bastante para decidir.

keel ocupa a mesma posição um andar acima. A imagem que ele pede que você
mantenha é a do **C gerado**, mais abstrata justamente porque C é linguagem de
alto nível. **Uma construção cujo C você não consiga prever é uma construção que
falhou** — e é essa a régua do princípio 2. É por isso que a spec é escrita em
pares, e por isso os pares dos casos golden são compilados e executados: o que se
promete prever tem de estar escrito em algum lugar que não mente.

Referência: [spec v3 §1.1](keel-spec.md#11-princípios-de-projeto), princípio 2.

## Critério de admissão, em exercício

Uma construção entra no PPC quando passa por três perguntas, nesta ordem:

1. **O PPC consegue?** Se reconhecer ou traduzir a construção exigir análise
   semântica de C, ela está fora pelo contrato de análise, e nenhuma outra
   qualidade a recupera.
2. **Um módulo consegue?** O que uma biblioteca keel escreve, ela escreve. O
   núcleo recebe o que nenhum módulo alcança.
3. **A construção carrega uma garantia?** Redução de escrita não é critério.
   Entra a forma que passa a sustentar uma verificação, uma ordem de avaliação
   ou uma identidade que o C não expressa.

As duas primeiras dão o teto e o piso; a terceira decide. Passar nas duas
primeiras não admite nada.

**`match` e a terceira pergunta.** Lido como abreviação de `switch`, `match`
não passa: as duas formas escrevem o mesmo despacho e a diferença seria de
escrita. Não é essa a construção admitida. A unidade de admissão é `tags`, o
conjunto fechado declarado em keel; `match` é o leitor desse conjunto, e o que
ele carrega são três garantias que o `switch` não oferece: exaustividade
verificada nos dois sentidos, um escopo por braço sem passagem implícita ao
braço seguinte, e um operando resolvido pelo verbo `tag` em vez de por um
inteiro qualquer. Nenhuma delas é obtida a partir de um `enum` C, porque um
`enum` vindo de header não é uma lista que o PPC conheça.

**`walk` e a terceira pergunta.** É a mesma admissão de `foreach`, aplicada ao
outro protocolo. Sem `walk`, um contêiner que não indexa perde a travessia
reconhecida — e com ela o cleanup nas saídas, a recusa de mutação estrutural e
a forma declarada do binder — e cai num `while` opaco. `walk` fixa a ordem das
chamadas do protocolo: `begin` uma vez, `has_next` antes de cada elemento,
`next` uma vez por iteração e depois do teste.

**`partition`, `routine.seq` e `routine.par` e a segunda pergunta.** Não são
admissões: são devoluções. A divisão contígua de um `buffer` é escrevível por
um módulo, e passa a ser contrato da base. A composição de chamadas
cooperativas sobre uma tabela é uma função, e passa a ser função. O que fica no
núcleo é o que sobra: o cabeçalho de `parallel`, que declara quantos workers,
qual política e o que é capturado.

Referência: [spec v3 §1.1](keel-spec.md#11-princípios-de-projeto) e
[§1.3](keel-spec.md#13-contrato-de-análise).

## O que fica fora do PPC, e por quê

A lista não é registro de trabalho futuro: é o limite do mecanismo, escrito
junto com ele. Quando aparecer a próxima proposta, o teste é o da seção
anterior, e não a presença nesta tabela.

São duas recusas de espécie diferente, e misturá-las esconde as duas.

**Recusado pelo limite de análise.** O PPC não conseguiria reconhecer ou
traduzir estas formas sem análise semântica de C:

| Fica de fora | O que exigiria |
| --- | --- |
| Operador de propagação de erro | Localizar o operador dentro de expressão C arbitrária e içar seu operando, o que muda a ordem de avaliação do que estava escrito |
| Tupla anônima e desestruturação de retorno | O tipo de uma chamada |
| Extração ou conversão implícita de resultado | O tipo de uma chamada, e a correspondência entre dois catálogos de código |
| Função genérica livre | Deduzir o argumento de tipo a partir do argumento escrito na chamada |
| Corrotina com retomada de posição | Interpretar o corpo para içar variáveis locais através da suspensão |
| Lambda e closure | O mesmo, mais captura implícita |
| Fatiamento multidimensional como sintaxe | Despachar pela forma dos argumentos, e não pela contagem |
| Inversão de layout, como um marcador `soa` | Conhecer o tipo de cada campo para decidir o que é invertível |
| Constraints, concepts ou interfaces sobre o parâmetro de tipo | Um predicado sobre o argumento de tipo; o único bit existente, `byref`, pertence ao modificador |
| Especialização de modificador, total ou parcial | Casar padrão sobre argumento de tipo, o que transforma substituição em avaliação |
| Sobrecarga por tipo de argumento | Escolher entre assinaturas pelo tipo do que foi escrito; a resolução conta argumentos |
| Aritmética no argumento de `dim`, como `view(N-1)` | Avaliar expressão onde há substituição de um número conhecido |
| Família de declarações cuja quantidade dependa de `dim` | Gerar declarações que ninguém escreveu, a partir de um `N` que a biblioteca não conhece |

**Recusado por contrato.** Estas seriam analisáveis, e mesmo assim não entram:
o custo é de significado, não de análise.

| Fica de fora | O que custaria |
| --- | --- |
| Braço padrão em `match` | A exaustividade deixa de ser verificável como propriedade do programa: um `default` absorve a tag que o programa esqueceu, que é exatamente o caso que a verificação existe para mostrar |
| Valor associado por tag | `tagged` tem um `T` só. Um tipo por braço faria o tipo do valor depender do braço, e a leitura do valor deixaria de ser verificável fora dele |
| `corot` no protocolo de `else` | O terceiro estado passaria calado por um tratamento binário: `else` teria de decidir o que fazer com `ONGOING`, e qualquer decisão estaria errada em metade dos usos |
| Cancelamento de participante cooperativa | Não há ativação suspensa a cancelar. Uma chamada de encerramento imporia às participantes uma assinatura que o protocolo não pede |
| Timeout implícito nas composições | Introduziria relógio e política de tempo num mecanismo que só conta estados |
| Transporte automático de valor da participante para a composição | Escolher uma vencedora resolveria apenas parte dos casos de alvo um, e não definiria valor para alvo `N` nem para uma sequência |
| Sincronização inserida por `parallel` | O PPC não sabe o que as partes alcançam por ponteiro; inserir sincronização seria prometer uma segurança que ele não verificou |
| `defer` inserido automaticamente sobre memória externa | Aquisição e liberação são do contrato da origem, não do reconhecimento de uma arena |
| Realocação implícita de `buffer` | Invalidaria silenciosamente vistas e ponteiros derivados, que é a decisão que o programa precisa manter à vista |

Referência: [spec v3 §5.4](keel-spec.md#64-programa-conforme-e-limites) e o
item 6 de cada contrato do capítulo 4.

## Fronteira com C e conflitos léxicos

keel precisa reconhecer suas construções dentro de código C, inclusive em
expressões. A escolha é usar varredura léxica, estrutura de delimitadores e
informações de símbolos keel declarados, sem implementar um analisador
semântico de expressões e tipos C.

Palavras contextuais reduzem conflitos com identificadores existentes, mas
não eliminam todos os conflitos de reconhecimento. Algumas posições, nomes e
estruturas de pré-processamento precisam de restrições explícitas. A recusa
nesses casos preserva um reconhecimento definido sem exigir conhecimento
semântico do C que o PPC não possui.

A promessa central é traduzir keel integralmente para C e preservar o texto
das expressões fora das substituições necessárias. Ela não equivale a aceitar
qualquer fonte C sem adaptação.

Referência: spec v3 §§1.1, 1.3 e 1.4.

## Por que `<opaco>` é o terminal central

Uma gramática de ilha não é uma gramática de C com buracos: é o inverso — uma
gramática pequena com um terminal enorme. `<opaco>` é o que torna o contrato de
análise da §1.3 verificável em vez de aspiracional, porque toda produção que não
sabe o que fazer com um trecho **tem para onde reduzi-lo**.

A consequência é medível: a gramática tem setenta e duas produções, e nenhuma
delas descreve expressão, precedência ou a gramática completa de declaradores do
C. A ambiguidade clássica — `(a)(b)` é chamada ou conversão? — não aparece,
porque as duas leituras reduzem ao mesmo `<opaco>`.

Referência: [spec v3 §2.2](keel-spec.md#22-sintaxe).

## Por que `..` não é um token

Fundir `2..7` num token de intervalo exigiria **retrair o número já emitido**, e
faria um token conter sequência balanceada arbitrária: `xs[(a+1)..(b*2)]` é legal,
e nada impede que os lados contenham chamadas, casts e vírgulas dentro de
parênteses. Um lexer que carrega estrutura balanceada dentro de um token deixou de
ser lexer.

Deixando `..` como pontuação, a forma do intervalo vira problema do parser, que é
onde o balanceamento já existe. O léxico continua não decidindo nada — que é a
regra da §2.4 e o que mantém as duas camadas separáveis.

Referência: [spec v3 §2.2](keel-spec.md#22-sintaxe).

## A posição de contêiner não é um sistema de tipos

`contentor` parece tipagem e não é: é uma **tabela fixa de tipos de retorno**
avaliada sobre símbolos que keel mesmo declarou. Cada nó tem tipo em função apenas
do primeiro argumento, e a avaliação nunca entra em `<opaco>`. É o preço mínimo da
resolução de verbo — sem ele, `buffer.push(x, v)` não teria como escolher a função
—, e é o único lugar em que ele é pago.

É também onde a camada foi raspada de propósito. `f(y)[i]` e `(cast)x[i]` são
recusados não porque sejam difíceis, mas porque aceitá-los significaria tipar o
retorno de uma chamada e o resultado de uma conversão — e a partir daí não há onde
parar: o próximo pedido é o operador de propagação, e depois a desestruturação.
As duas estão na tabela de recusas por limite de análise, e a gramática é o lugar
onde esse limite fica escrito em vez de prometido.

Referência: [spec v3 §2.2](keel-spec.md#22-sintaxe) e
[§4.4](keel-spec.md#44-resolução-de-operações).

## Por que a contagem é por alternativa, e não sobre o texto

keel precisa achar o fim de cada corpo de função sem expandir o pré-processador, e
para isso conta delimitadores. Havia dois desenhos, e a diferença entre eles não é
de implementação — é de **o que o diagnóstico mede**.

O primeiro é o óbvio: diretiva é um token opaco, conta-se `{}` sobre o fluxo, e
pronto. Zero consciência de pré-processador. Ele quebra no idioma mais comum que
existe em código de plataforma:

```c
#ifdef MIPS
    if ((v = read(port))) {
#else
    if ((v = digitalRead(port))) {
#endif
    /* muito código */
    }
```

Duas aberturas, um fechamento, arquivo recusado — e o programa é C perfeito, com
estrutura idêntica nas duas compilações. **A contagem plana não mede uma
propriedade do programa, mede um artefato da varredura:** ela diz que o texto tem
mais chaves do que qualquer compilação dele teria. Diagnóstico que reprova o
scanner em vez do código está errado, por mais barato que seja.

A contagem por alternativa mede o que interessa: **a estrutura de blocos do
arquivo depende da condição?** Se todas as alternativas produzem o mesmo delta,
toda compilação possível tem a mesma estrutura, e keel delimita os corpos sem
saber qual delas acontece. Se discordam, há tantas estruturas quanto ramos, e
escolher uma significa avaliar a condição — que é onde a linha está.

**E é só aí que ela está.** O custo do desenho é reconhecer oito palavras; não é
ler o que vem depois delas. Testar `#ifdef MIPS` seria keel decidir por
plataforma, e mataria cross-compilation. Ler a palavra `#ifdef` não decide nada: é
reconhecer onde uma alternativa começa, e as duas coisas só parecem vizinhas
porque moram na mesma linha.

**Por que todas as alternativas têm de concordar, e não cada uma fechar o que
abriu.** A segunda regra é mais simples de enunciar e recusa exatamente o caso
acima: as duas alternativas do `#ifdef MIPS` abrem uma chave e não a fecham. Ela
exigiria delta zero em cada ramo, o que proíbe o idioma que a regra existe para
admitir. Concordância é a condição certa porque é a que o consumidor precisa: ele
não quer que o ramo seja neutro, quer saber com que profundidade continuar depois
do `#endif`.

Referência: [spec v3 §2.4](keel-spec.md#24-semântica).

## Por que a redeclaração é recusada em vez de classificada

keel poderia tentar decidir se `FILE *xs;` é declaração — bastaria classificar o
primeiro identificador. Só que classificar exige saber que `FILE` é um nome de
tipo, e o contrato da §1.3 diz que keel não sabe isso sobre nome nenhum do C.

Então sobra recusar o padrão em que ele erraria, que é o princípio 5 na letra. O
custo é pequeno e localizado: um `IDENT IDENT` em início de statement cujo segundo
nome é símbolo de keel. Renomear a variável local resolve, e a mensagem diz isso.

É o único ponto do documento em que o limite de análise aparece como **restrição
ao usuário** em vez de propriedade da implementação — e vale registrar exatamente
por isso, porque é a fatura da decisão de não entender C.

Referência: [spec v3 §2.5](keel-spec.md#25-restrições-e-diagnósticos).

## Por que `instance` é a única ressalva do documento

Quase toda ambiguidade com C se resolve pela posição e pelos símbolos
registrados. Uma não: `instance G A;` tem exatamente a forma de `buffer i32 xs;`,
e o que as separa é `instance` estar na primeira posição. Se um modificador se
chamasse `instance`, as duas leituras seriam `instance(G(A))` declarando uma
variável e a declaração de instanciação — e nenhum token adiante as separa.

A saída poderia ser uma regra de precedência ("instance na primeira posição sempre
ganha"), e ela funcionaria. Foi recusada porque tornaria **silenciosamente
inalcançável** um modificador legalmente declarado: quem escrevesse
`pub modifier instance {…}` teria um tipo que nunca poderia ser usado, sem nenhuma
mensagem dizendo por quê. Error na declaração custa uma linha e diz o motivo no
lugar certo.

Referência: [spec v3 §2.3](keel-spec.md#23-reconhecimento).

## Reconhecimento de funções e declaradores C

keel precisa localizar declarações e corpos de funções para registrar o nome,
separar interface e implementação, traduzir parâmetros keel e reconhecer as
saídas que participam de `defer`. Para isso, precisa distinguir uma função de
um objeto cujo declarador ou inicializador também contenha parênteses.

Os três casos mostram por que a presença de `(` não basta:

```c
int f(void);       /* declaração de função */
int (*f)(void);    /* declaração de ponteiro para função */
int x = f(1);      /* declaração de objeto com chamada no inicializador */
```

No primeiro, o grupo final `(void)` vem imediatamente depois do identificador
`f`. No segundo, esse grupo vem depois de `(*f)`: o token anterior é `)`, e o
nome está dentro de um declarador aninhado. No terceiro, o `=` externo indica
que os parênteses pertencem ao inicializador, não ao cabeçalho da declaração.
Essas diferenças podem ser verificadas pela sequência de tokens e pelo
balanceamento, sem consultar o significado de `int` ou o tipo retornado por `f`.

Uma gramática completa de declaradores C permitiria classificar formas mais
gerais, mas ampliaria a análise que keel se propõe a realizar. A regra adotada
reconhece a forma de função necessária às suas construções e deixa a validação
dos tipos C com o compilador C. Não reconhecer uma declaração como função keel
não equivale a diagnosticá-la como C inválido; seu tratamento depende das demais
regras de declaração e das informações de que a tradução necessita.

A busca pelo grupo final exige percorrer uma região de comprimento variável.
Por isso, a justificativa é a suficiência da informação lexical e estrutural,
não uma promessa de lookahead de tamanho fixo.

Referência: [spec v3 §2.3](keel-spec.md#23-reconhecimento),
“Declarações e palavras contextuais”.

## Perfis e exemplos de tradução

O alvo original era exclusivamente C23. A introdução do perfil C11 ampliou as
opções de geração; referências exclusivas a C23 em definições gerais são
resquícios da versão anterior.

Os pares keel/C mostram a operação essencial que o programador precisa
compreender. Reproduzir todos os detalhes de emissão tornaria a spec dependente
da apresentação do backend e duplicaria regras como mapeamento de linhas e
atributos gerados.

Referência: convenções da spec v3 e §1.2.

## Prelúdio e base mínima

O prelúdio implícito limita-se a `import keel types;`. Importar os demais
módulos explicitamente torna suas dependências visíveis, inclusive quando
pertencem à base distribuída com keel.

`keel.tagged` entra pela mesma regra: o modificador que associa etiqueta a
valor é módulo comum, importado por quem o usa, e não palavra do núcleo.
`match` não depende desse import — despacha sobre qualquer tipo que declare
`tag` —, e é o que mantém o núcleo independente da base.

A prioridade da revisão é fechar o núcleo, as verificações do parser e a base
mínima. Bibliotecas adicionais serão desenvolvidas sobre esses contratos,
com apoio da experiência de implementação do compilador.

Referência: spec v3 §4.1.

## Constantes nomeadas

`constexpr` foi introduzido depois das primeiras regras que exigiam literais.
Essas regras devem incorporar constantes nomeadas sem introduzir avaliação de
expressões no PPC. Ler um literal de uma declaração conhecida permite usar um
nome sem mudar o limite de análise.

O mesmo limite governa os valores escritos de um conjunto de tags: aceita-se um
literal decimal, com sinal opcional, ou uma constante conhecida. Um valor
calculado exigiria avaliação de expressão, e um conjunto sem valor conhecido
não sustentaria a verificação de exaustividade sobre nomes.

Referência: spec v3 §4.2.

## O que o VLA errou

O VLA é o exemplo canônico da confusão que keel desfaz, e vale enunciar cedo
porque ele decide o desenho da base inteira.

> **O VLA solda duas decisões numa sintaxe só: *quanto*, e *onde e por quanto
> tempo*.**

É a solda que o inutiliza. Não dá para retornar, não dá para exceder o frame, não
dá para saber se falhou, e metade dos alvos não o implementa. keel separa as duas
decisões e dá um nome a cada uma: *quanto* é a capacidade do `buffer`, ou o `n` de
`arena.from_parent`; *onde e por quanto tempo* é a arena — ou o `array`, ou a
região crua.

Por isso `buffer` não é o VLA de verdade: ele é um descritor, retorna por valor, e
a vida do dado é a da arena, não dele. E a `arena` também não é:
`arena.from_stack` tem a forma de vida do VLA mas exige tamanho constante, e
`arena.from_parent` tem o tamanho de execução mas o armazenamento não volta
sozinho. O que chega perto é a composição das duas, com a disciplina de pilha
**explícita** em vez de implícita — e escrito assim funciona onde VLA não existe,
o modo de falha é testável, e quem lê sabe de onde saiu a memória.

Referência: [spec v3 §5.2](keel-spec.md#52-keelarena) e
[§5.3](keel-spec.md#53-keelbuffer-keelslice-e-keelrange).

## Memória por região

“Dono” não descreve adequadamente a diferença entre `buffer` e `slice`. A
diferença é a expansibilidade: `buffer` gerencia uma sequência que pode crescer;
`slice` descreve um único trecho de extensão fixa, com `len` e `ptr`. Os dados
do trecho podem ser alterados por índice ou ponteiro, respeitados os
qualificadores do tipo e a validade do armazenamento. A responsabilidade pelo armazenamento é outra
questão, determinada pela origem da região.

`from_memory` permite incorporar regiões externas sem assumir sua aquisição
ou liberação. `defer` fornece ao programa uma forma de organizar a limpeza,
mas sua disponibilidade não implica inserção automática nem garantia de uso.

A verificação de relações conhecidas entre arenas cabe na varredura de
símbolos keel. Uma prova geral de escape por parâmetros de saída exigiria
interpretar expressões e efeitos de código C além do contrato do PPC. Essa
limitação deve constar da especificação junto à garantia efetivamente oferecida.

Referência: spec v3 §§4.5–4.6.

## Substituição e aridade fixa

`dim` fornece substituição numérica; `type` fornece a substituição do
parâmetro pelo tipo escrito, inclusive uma forma como `struct Person`;
`tags` fornece a substituição do parâmetro pelo `enum` de um conjunto
declarado. O significado de `N` é dado pelo modificador: pode
representar um rank, mas o PPC não impõe essa interpretação.

Gerar `N` parâmetros para um verbo exigiria produzir uma família de declarações.
Receber um vetor `T valores[static N]` mantém a assinatura com aridade fixa e
transfere o conjunto de valores por um único argumento. Depois da substituição,
o compilador C recebe uma extensão constante no parâmetro.

O argumento de um parâmetro `tags` é o **nome** do conjunto, não sua lista. A
identidade canônica cita esse nome, e o nome gerado não cresce com a quantidade
de tags; a lista continua disponível para as declarações da instância, que é o
que a expansão precisa. Passar a lista faria a identidade depender de uma
sequência de nomes, e duas ordens da mesma lista produziriam instâncias
distintas de representação idêntica.

Essa escolha limita as formas de interface de um tensor, mas o tensor é um
exemplo de modificador, não o modelo de todo parâmetro `dim`. A orientação a
dados de keel permanece dentro dos limites de substituição e reconhecimento,
sem metalinguagem de templates ou análise semântica de tipos C.

Referência: [spec v3 §4.3](keel-spec.md#43-módulos-genéricos).

## Particionável e percorrível

A revisão anterior deixava `parallel` conhecer a divisão: a fórmula das faixas
era do PPC, e a construção exigia do contêiner os mesmos verbos de `foreach`.
Isso obrigava dois compromissos que não eram necessários. O primeiro é que só
podia ser distribuído o que fosse indexável, ainda que a divisão de outra
estrutura seja escrevível por seu próprio módulo. O segundo é que o PPC
afirmava a disjunção das faixas — uma propriedade que ele não verifica em
estrutura alguma, e que só o autor do módulo pode sustentar.

`partition(x, k, w)` resolve os dois. O contêiner declara como se divide; o PPC
avalia o verbo uma vez por worker e liga o resultado ao binder. A divisão
contígua de `buffer T`, `slice T` e `range` continua definida, mas agora como
contrato da base, no mesmo lugar em que se declaram `length` e `ptr`. Para
quem escreve o programa nada muda no caso comum; para quem escreve um módulo,
distribuir passa a ser possível.

A separação também é entre capacidades: particionável, indexável e percorrível
por cursor são independentes. Um contêiner pode declarar uma, duas ou as três,
e cada construção pede a sua. `parallel` deixa de percorrer: entrega a parte, e
percorrê-la é escolha escrita no corpo do worker, por `foreach`, por `walk` ou
por qualquer forma C.

O binder de partição recebe o produto declarado de `partition`, que para a base
é um `slice T` por valor. É o uso previsto da construção, e por isso não incide
o aviso de cópia de contêiner: o que se copia é o descritor de uma vista, que é
a forma normal de passá-la.

Referência: [spec v3 §4.10](keel-spec.md#48-execução-particionada).

## Onde keel concorda com o OpenMP

O critério do núcleo mínimo não foi traçado contra o pragma; foi traçado pelo
teste de admissão, e a maior parte do que passa nesse teste **já faz como o
pragma faz**. `constexpr`, `array`, `ref`, `import_c` e `extern_c` anotam e somem
no lowering: é a filosofia do pragma, aplicada onde ela cabe.

E `parallel` é onde keel **consome** a abordagem em vez de recusá-la. Ele não traz
agendador, não traz pool de threads e não linka runtime próprio: um dos dois
lowerings especificados é uma diretiva, que o compilador C atende quando a flag
está lá. A diferença em relação à v1 é que a linguagem deixou de nomear o
mecanismo — hoje ela descreve as execuções permitidas, e qual delas sai é do
backend e da invocação. O que não mudou é a razão de não haver runtime de keel:
um lowering por threads da libc precisaria do tipo de cada captura, e o contrato
de análise proíbe conhecê-lo.

> **Núcleo é o que nenhum módulo keel poderia escrever. Pragma é o que nenhuma
> declaração precisa nascer.**

Referência: [spec v3 §1.1](keel-spec.md#11-princípios-de-projeto), princípio 8,
e [§4.8](keel-spec.md#48-execução-particionada).

## Políticas e sinalização de interrupção

A quantidade de workers expressa a divisão solicitada pelo programa. Não fixar
um teto no PPC permite que a execução use os recursos disponíveis sem
prometer simultaneidade. Uma parte vazia não exige acesso a dados: o corpo do
worker executa sobre um recorte de comprimento zero.

`ALL`, `ANY` e `N` expressam as políticas de conclusão. O flag de interrupção
comunica um pedido que os workers podem observar e tratar. Fazer o predicado
`interrupted` depender de alguém já ter parado impediria usá-lo como consulta
ao próprio pedido; por isso, o predicado reflete o flag ativado.

As consultas do bloco são verbos, e não palavras. O nome do `parallel` declara
um símbolo de tipo `parallel.control`, e `interrupted`, `ok`, `failed` e `wins`
são operações de `keel.parallel` sobre ele. Um pedido de interrupção e um
resultado são coisas que se leem, e leitura é verbo; fazer disso palavra do
núcleo acrescentaria vocabulário para não acrescentar garantia nenhuma. Também
é o que dá razão ao nome ser obrigatório: ele não nomeia apenas a execução,
declara o símbolo por onde ela é consultada.

O lowering deixou de ser matéria da spec. Série, OpenMP, pool de threads ou
outro mecanismo produzem execuções que a semântica já permitia, porque a
construção nunca prometeu simultaneidade: a execução serial das partes em
ordem crescente é uma delas. Um aviso de indisponibilidade de OpenMP relataria
um desvio que não existe, e por isso saiu do catálogo. Em compensação, a
responsabilidade fica escrita do outro lado: quando o lowering escolhido de
fato executa em paralelo, os acessos que as partes compartilham são do
programa, e o PPC não insere sincronização.

Referência: [spec v3 §4.10](keel-spec.md#48-execução-particionada).

## Conjuntos fechados e exaustividade

Um `enum` C é aberto para quem o lê: o compilador aceita qualquer inteiro no
lugar de uma constante, e um `switch` sem `default` não é erro. Nada disso é
corrigível por análise, porque o PPC não abre headers e não conhece as
constantes que não declarou.

`tags Nome [ … ];` declara em keel a lista que faltava. Da declaração vêm três
coisas que o `enum` sozinho não dá: os nomes são conhecidos na tradução, o que
sustenta a verificação de exaustividade sem análise de fluxo; a lista é a
mesma para todos os usos, o que torna a verificação estável sob edição; e o
conjunto tem identidade de módulo, o que permite usá-lo como argumento de tipo.

A verificação é sobre nomes, e não sobre valores. Duas tags com o mesmo valor
escrito continuam sendo dois nomes, e cada uma exige seu rótulo — a alternativa
seria comparar valores, o que exigiria avaliação, e faria a exaustividade
depender de uma aritmética que o programa não escreveu.

**De onde vem o conjunto, quando o operando não é `tagged`.** A resposta natural
— "do módulo do operando" — só funciona enquanto o módulo declarar um conjunto
só, e nada obriga isso. Por isso a regra tem duas etapas: primeiro o argumento
escrito na instância, quando o módulo tem parâmetro `tags`; depois, o conjunto
único do módulo. Com dois conjuntos e sem parâmetro que decida, não existe
critério, e inventar um — o primeiro declarado, o de mesmo nome do tipo —
esconderia a ambiguidade num lugar em que o programa não a veria. A recusa é
`match-conjunto-ambiguo`, e a saída é escrever o parâmetro.

**A escrita da etiqueta é verificada; a leitura, não.** Parece assimétrico e é
deliberado. A etiqueta armazenada pode não pertencer ao conjunto — veio de
`memset`, de arquivo, de rede —, então `tag` devolve o que está lá e a
verificação de debug observa. Escrever é o outro lado: ali o programa nomeia uma
constante, keel a reconhece, e recusar a que é de outro conjunto custa uma
consulta à tabela. O que torna isso protocolo, e não privilégio de `keel.tagged`,
é o parâmetro tipado: quem declara `mark(m *t, E e)` com `E` do módulo ganha a
verificação, seja ele da base ou do programa. O C não a faria — `enum` converte
para `int` em silêncio.

Não há braço padrão, e a ausência é a razão de a construção existir. Um
`default` absorveria a tag esquecida, que é precisamente o caso que a
verificação existe para mostrar. O `default` presente no C emitido não é braço:
é o ponto onde a verificação de debug observa uma etiqueta fora da lista.

`tagged E T` associa a etiqueta ao valor. O modificador é comum: um `i32` de
etiqueta e um campo do tipo associado, com `void` omitindo o campo pela regra
geral de omissão. Isso permite que a mesma construção sirva a uma variável de
estado sem valor, `tagged Ciclo void`, e a um valor etiquetado de verdade,
`tagged Kind struct Node`.

O despacho não exige `tagged`. `match` pede o verbo `tag` pela resolução
comum, e o conjunto vem da declaração `tags` associada ao módulo do operando.
É o que faz `corot` participar de `match` sem mudança de representação: o
código continua sendo um `i32`, e `corot.tag` o traduz para uma das três tags
declaradas pelo próprio módulo. Um operando cujo módulo não declara conjunto
não admite `match`, porque não haveria lista contra a qual verificar; o
programa continua podendo escrever `switch`.

Referência: [spec v3 §4.11](keel-spec.md#49-conjuntos-de-tags-e-despacho).

## Estrutura de controle e máquina completa

O despacho por etiqueta é uma estrutura de controle, comparável a um `switch`.
Seu laço pertence ao programa. Isso permite usar as funções e os dados da
aplicação sem impor uma política de avanço, e sem exigir das funções chamadas
uma assinatura de corrotina além do retorno `corot` quando o protocolo é
utilizado.

A máquina é a função que contém o despacho, e o estado é dado do programa: uma
variável `tagged`, um campo de struct, um valor lido de memória. Nada é
preservado entre chamadas pela construção, e por isso não há frame suspenso,
pilha secundária nem contexto oculto. Trocar de etiqueta é uma escrita comum, e
só é observada no próximo despacho que o programa executar.

Cada braço abre seu próprio escopo, e chegar ao seu fim encerra o despacho. A
alternativa — passagem implícita ao braço seguinte — é a fonte de erro conhecida
do `switch`, e não há razão para reproduzi-la numa construção nova. Rótulos
consecutivos compartilham um braço, o que cobre o caso legítimo em que duas
tags têm o mesmo tratamento, sem reabrir a passagem entre corpos distintos.

Os corpos ficam fora do `switch` emitido, que serve apenas para escolher um
rótulo. Assim, um laço ou um `switch` escrito pelo programa dentro de um braço
mantém seus próprios `break` e `continue`; o `break` escrito no nível do braço é
que pertence ao despacho, e é reescrito como salto para sua saída, com o
cleanup dos escopos que deixa.

`match` não produz valor. Valor pertence a função, e uma estrutura de controle
que produzisse resultado exigiria decidir o que vale quando nenhum braço
escreve — decisão que o programa toma melhor com uma variável à vista.

Referência: [spec v3 §4.11](keel-spec.md#49-conjuntos-de-tags-e-despacho).

## Resultados finais e estados cooperativos

`outcome` representa uma operação terminada: código zero é sucesso e qualquer
outro código é falha. `corot` também precisa representar uma operação em
andamento, para a qual reserva zero; sucesso usa valores negativos e falha,
valores positivos.

Em `outcome`, sucesso e falha são complementares: há resultado válido ou
é necessário tratar sua ausência ou erro. Essa partição também serve a um
optional: `has_value` é verdadeiro para código zero e falso nos demais casos.
Códigos de erro podem ser positivos ou negativos, sem mudar o protocolo.

Em `corot`, ausência de falha não implica sucesso: também pode indicar
`ONGOING`. O problema de usar `failed` não é a possibilidade de implementar
um teste `code > 0`, mas o significado de deixar passar esse terceiro estado
pelo tratamento de resultado final de `else`.

Os três estados de `corot` são um conjunto fechado, e a v3 os declara como tal:
`tags Status [SUCCESS = -1, ONGOING = 0, FAILED = 1]`, no próprio módulo. A
declaração não muda a representação — o código continua sendo um único `i32`, e
o código de falha continua sendo o inteiro que a participante escreveu — e dá
duas coisas de graça: `corot.tag` como verbo de leitura e `corot` como
operando de `match`, com exaustividade sobre os três estados. Tratar os três
casos deixa de ser uma cadeia de predicados que se pode esquecer de fechar.

A composição de chamadas cooperativas produz `outcome`, e não `corot`: ela
resolve antes de devolver o controle, então não existe o estado intermediário.
O sucesso negativo de uma participante não é copiado como código externo — a
política produz sucesso zero ou falha não zero, conforme o protocolo do
resultado final.

A decisão é usar `corot.faulted` para consultar falha cooperativa e
`corot.fault` para produzi-la. `failed` preserva o significado do protocolo de
resultados finais, sem uma exclusão adicional baseada na presença de `ongoing`.

Referência: [spec v3 §5.5](keel-spec.md#55-keeloutcome-e-keelcorot).

## Produção e consulta da falha cooperativa

O par `corot.fault` / `corot.faulted` distingue a escrita de um estado e a
consulta sobre ele, sem reutilizar o vocabulário de falha de resultado final
`outcome.fail` / `outcome.failed`.

`fault` designa aqui conclusão cooperativa sem sucesso. Não introduz exceção,
trap ou categoria especial de defeito. O código positivo identifica essa
conclusão; zero e negativos pertencem aos outros dois estados de `corot`.
Normalizar silenciosamente um argumento inválido mascararia o uso incorreto
desse contrato, por isso `corot.fault` exige positividade.

A grafia antiga de consulta não é mantida como alias: a mera presença de
`failed` voltaria a incluir `corot` no protocolo de `else`. Já `outcome`
continua aceitando códigos positivos ou negativos para representar falha ou
ausência, com zero reservado para o resultado válido.

A mudança da v3 é a categoria dos produtores. `cowin`, `coagain` e `cofault`
eram statements que retornavam da função; `corot.win`, `corot.again` e
`corot.fault` são verbos do módulo, que ajustam o objeto recebido e devolvem
sua cópia ajustada. A saída passa a ser escrita: `return corot.win(r);`.

Três consequências justificam a troca. A primeira é uniformidade: os
produtores de `corot` passam a ter a mesma forma dos de `outcome`, que já
recebiam o resultado no primeiro argumento. A segunda é que uma palavra do
núcleo que retorna da função precisa de regras próprias sobre onde pode
aparecer e sobre o que faz com o cleanup; um verbo não precisa de nenhuma,
porque o `return` que o acompanha já é um ponto de saída conhecido. A terceira
é que o valor deixa de ser um caso especial: escrever o estado sem sair da
função — para registrá-lo, para devolvê-lo mais adiante ou para armazená-lo em
um campo — passa a ser possível sem construção nova.

Nenhuma das três formas carrega valor: `corot` não tem campo de valor, e o que
uma passagem produz pertence ao contexto que o programa passou. O que elas
escrevem é o código, e é dele que os três estados são lidos.

Referência: [spec v3 §5.5](keel-spec.md#55-keeloutcome-e-keelcorot).

## Composição como biblioteca

`coseq` e `copar` eram construções do núcleo: uma lista de chamadas escrita
entre chaves, com uma região de finalização delimitada por rótulo. A forma era
legível, e o custo estava fora dela. As chamadas eram fixadas no texto, então
uma composição montada em tempo de execução não era expressável; a região de
finalização precisava de quatro diagnósticos próprios de salto e posição, só
para impedir que uma saída escapasse da atribuição final do código; e a lista de
participantes, de dois; e `coexit` ocupava uma reserva léxica
condicional, válida apenas dentro daquelas duas construções.

A pergunta 2 do critério de admissão responde por si: uma tabela de entradas e
um laço sobre ela são escrevíveis por um módulo. `routine.seq` e `routine.par` são
funções da base, e a lista de participantes passa a ser um dado — um vetor
estático, ou um `buffer` montado em execução, entregue por `slice.of`.

O que se ganha é composição sobre dados: entradas escolhidas em tempo de
execução, o mesmo recorte reaproveitado, o código de cada participante legível
na própria entrada depois da chamada. O que se perde é a inserção em linha das
participantes, porque a chamada passa a ser indireta. É o preço declarado da
troca, e ele é aceitável na escala em que essas composições operam: uma chamada
indireta por participante por ciclo, contra uma construção do núcleo com região
própria e quatro verificações de salto.

A finalização deixa de existir como região, e por isso não precisa de proteção:
o que era escrito depois de `coexit:` é agora o statement seguinte à declaração
do resultado, sujeito às regras comuns de `return`, `goto`, `defer` e `else`.
A reafirmação do código pela máquina também deixa de ser necessária, porque não
há mais um corpo do usuário entre a resolução da política e a entrega do
resultado.

O resultado é `outcome u32`: o código diz se a política foi satisfeita, e o
valor associado é a quantidade de entradas que terminaram em `SUCCESS`. Não há
transporte automático do valor de uma participante, pela razão registrada na
tabela de recusas por contrato. `mask` é conveniência derivada da tabela, e não
a representação do resultado — por isso é um verbo à parte, com o limite de 64
slots verificado em debug.

A assinatura da participante é fixa: retorno `corot` e um parâmetro `C *`.
Fixá-la é o que torna a tabela um tipo. Funções de outra forma pedem outra
tabela ou um adaptador escrito pelo programa, que é a mesma resposta que o C dá
a qualquer vetor de ponteiros para função.

Referência: [spec v3 §4.13](keel-spec.md#56-keelroutine).

## Rotina, tabela e contexto

A primeira redação pôs a composição no mesmo módulo do resultado: `keel.corot`
declarava o modificador `corot`, o tipo da participante — chamado `cofunc` — e a
tabela. O parâmetro do módulo passava então a ter duas leituras: em `corot T`,
`T` era o tipo do valor associado; em `cofunc T` e `entry T`, o tipo do
contexto. Duas leituras do mesmo parâmetro no mesmo módulo é defeito, não
economia.

Acrescentar um segundo parâmetro não resolve, e o motivo é a regra de aridade:
a assinatura do módulo vale para **todos** os seus modificadores, e a identidade
inclui **todos** os argumentos canônicos. Com `type T, C`, `corot void` deixaria
de ser escrevível — todo uso passaria a exigir dois argumentos — e se
fragmentaria em um tipo por contexto: duas participantes de contextos diferentes
não compartilhariam mais o tipo de retorno, e os verbos de `corot` seriam
instanciados por contexto para operar sobre um campo `i32` que nada tem com o
contexto.

A divisão em dois módulos resolve os três problemas de uma vez. `keel.corot`
fica sendo o estado de saída; `keel.routine`, a composição, parametrizada pelo
contexto. Some o duplo sentido, some a fragmentação, e some também a ressalva de
auto-instanciação: a participante devolve `corot`, que vem de um import comum, e
não de uma aplicação do próprio modificador do módulo a outro argumento. O
qualificador passa a dizer o que a chamada faz — `routine.par(...)` roda uma
tabela de rotinas, enquanto `corot.par(...)` sugeria um par de resultados.

Feita a divisão, a pergunta do parâmetro voltou pelo outro lado e se resolveu
sozinha: `keel.corot` não precisava de parâmetro nenhum. É o assunto dos três
parágrafos adiante.

O nome não é `cofunc` porque o prefixo `co` perdeu referente: depois que
`cowin`, `cofault`, `coagain`, `cobreak`, `coseq` e `copar` saíram, ele deixou
de marcar uma família e passou a marcar apenas a origem histórica.
Também não é `kroutine` nem `coroutine`: os dois prometem pilha própria,
suspensão e escalonador, que são exatamente as três coisas que a composição não
tem. `routine` descreve o que existe — uma rotina reentrante, chamada de novo a
cada ciclo até deixar de responder `ONGOING`.

**O estado cooperativo deixou de ser modificador.** Ele nasceu como `corot T`,
por simetria com `outcome T`, e a simetria não se sustentou: nenhuma construção
lê esse valor. O `else` exclui `corot` por construção, o `match` lê só a tag, e
a composição exige a forma sem valor. Um parâmetro de tipo que nenhum
consumidor usa é peso morto, e tirá-lo fecha três pontas de uma vez — o header
por instância desaparece, `keel_corot_void` vira `keel_corot`, e some a
pergunta de por que um módulo importava *uma instância fixa* de outro.

O que se perde é a corrotina escrita à mão que devolveria valor junto com
`SUCCESS`. O escape é o mesmo que a tabela já impunha: o valor viaja pelo
contexto. Uniformizar isso é ganho — o protocolo cooperativo passa a ter uma
história só, e não duas.

**`corot` é struct de um campo, e não `typedef i32`.** A comparação natural é
com `thrd_success` e companhia, que são `int` porque o C não tem alternativa e
porque os valores são um enum plano. Aqui não são: **o sinal carrega a tag e a
magnitude carrega o código**, e é essa codificação que um inteiro nu convida a
confundir. Com `typedef i32 corot;` o C não cria tipo, e passam em silêncio
`if (r)` — que lê como "deu certo" quando sucesso é negativo —, `r == 0`,
`total += r` e a mistura com o código de um `outcome`. Com a struct, os quatro
são erro do compilador C no ponto certo. Como keel não tipa expressão, o
sistema de tipos do C é a única barreira disponível, e a struct é o que a
compra; o typedef compra zero. Em execução não custa nada: struct de um `i32`
volta em registrador em toda ABI corrente.

**A linha da tabela chama-se `slot`.** `entry` lia como entrada — em português,
*input* —, e um campo de saída ali dentro range. O que a linha é, de fato, é um
lugar que guarda a participante, o contexto e o estado com que ela terminou.
Mover o estado para fora exigiria um segundo recorte paralelo, com dois
comprimentos a manter em acordo e um erro novo a diagnosticar: o que a
composição escreve fica onde a composição já está. O campo continua invisível
ao programa, que lê por `routine.state(s)` — e o verbo não se chama `outcome`,
porque o tipo tem três estados e `outcome` é o nome de outro protocolo no mesmo
programa.

O zero continua fazendo o trabalho certo: uma tabela inicializada com zeros já
está em `ONGOING`.

**E `corot` fica em módulo próprio, e não dentro de `keel.routine`.** Mecanicamente
caberia — uma declaração que não menciona o parâmetro é emitida uma vez. Mas o
qualificador passaria a mentir (`routine.ok(r)`, para ler um estado), quem
escreve uma participante sem usar tabela nenhuma importaria a composição, o
conjunto `Status` deixaria de estar sozinho no seu módulo — que é o que torna
trivial a regra de qual conjunto um `match` verifica —, e o par com
`keel.outcome` se perderia. O custo de manter separado é uma linha de import,
o mesmo que `keel.slice` já cobra.

Referência: [spec v3 §4.13](keel-spec.md#56-keelroutine).

## Por que o nome canônico é fixado na declaração

A alternativa seria fixá-lo no uso — cada módulo nomearia a instância à sua
maneira, e o backend reconciliaria. Isso não fecha: `buffer geom.Point` escrito em
dois módulos tem de dar o **mesmo tipo C**, senão a atribuição entre eles falha no
compilador. Fixar na declaração é o que faz o nome ser função apenas do tipo, e
não de quem o escreveu.

Daí também o argumento carregar a própria qualificação. `buffer Point` com `Point`
injetado por `types` e `buffer geom.Point` são a mesma instância, porque o nome
canônico do argumento é `geom.Point` nos dois casos — `types` é puramente de
fonte, e a identidade não pode depender de uma escolha de escrita.

Referência: [spec v3 §1.1](keel-spec.md#11-princípios-de-projeto), princípio 4,
e [§4.1](keel-spec.md#41-módulos-e-interoperabilidade).

## Módulos e identidade

A identidade fixada na declaração permite que aliases e imports mudem a escrita
local sem mudar o tipo compartilhado entre módulos. A comparação de nomes no
fecho de imports usa a informação disponível à tradução; prometer detectar
colisões entre unidades sem relação exigiria uma análise global do build.

O ponto de entrada também segue a identidade do módulo. Prefixar sua função
`main` e gerar um wrapper para o módulo selecionado permite manter entradas de
teste ou aplicações alternativas no mesmo conjunto de fontes, sem reservar um
único nome C entre todas as implementações.

`import_c` e `extern_c` atendem necessidades distintas: o primeiro disponibiliza
um header ao compilador C; o segundo permite escrever declarações externas no
próprio fonte. Conhecer seus nomes explícitos ajuda a evitar conflitos, mas não
fornece informação suficiente para inferir tipos C.

Referência: [spec v3 §4.1](keel-spec.md#41-módulos-e-interoperabilidade).

## Marcadores e declarações

`array` registra um fato que o C já representa: o símbolo é um vetor. Com esse
registro, keel pode reescrever índices múltiplos e pedir dimensões por `sizeof`,
sem contar inicializadores nem converter o armazenamento para um formato próprio.

`ref` registra uma restrição sobre uma declaração. A varredura dos operadores
aplicados ao símbolo cabe no limite de análise; seguir todas as cópias do
endereço mudaria esse contrato. A aceitação de `NULL` permite receber o resultado
de uma alocação e verificar sua falha antes de acessar o elemento.

`constexpr` separa uma constante nomeada de um objeto constante com endereço.
Essa separação fornece um contrato comum aos perfis C11 e C23. Quando o programa
precisa de endereço, a declaração C `static const` já expressa a necessidade.
Quando keel precisa do número para substituir tokens, ler um literal conhecido
preserva esse limite sem introduzir avaliação de expressões C.

Referência: [spec v3 §4.2](keel-spec.md#42-tipos-declarações-e-marcadores).

## Resolução por declaração

A assinatura escrita fornece a aridade e a forma de valor ou ponteiro; a
declaração do argumento fornece sua identidade conhecida. A adaptação usa essas
informações explícitas. Examinar o tipo resultante de uma expressão C arbitrária
exigiria outro modelo de análise.

Operações como `buffer.from`, que não carregam a instância nos argumentos,
usam um alvo declarado. Os verbos de `outcome` recebem o objeto explicitamente
e seguem o despacho pelo primeiro argumento.
Restringir esses usos a inicialização, atribuição a símbolo conhecido e retorno
permite localizar o tipo necessário. O qualificador de um produtor identifica
o contêiner obtido: o envelope `outcome` descreve a possibilidade de falha,
sem mudar a leitura de `buffer.clone` como criação de um buffer.

A mesma resolução serve às construções. `foreach`, `walk`, `parallel` e `match`
procuram os verbos de que precisam — `length` e `get` ou `ptr`; `begin`,
`has_next` e `next`; `partition`; `tag` — pelo protocolo comum, e não por uma
lista de tipos privilegiados. É o que permite a um módulo do usuário participar
de qualquer uma delas sem que o PPC saiba que ele existe, e é o que impede que
a base receba tratamento que a spec não escreveu.

Referência: [spec v3 §4.4](keel-spec.md#44-resolução-de-operações).

## Acesso e travessia

Indexação por `ptr` produz um lvalue para o elemento original. Isso permite
alterar um contêiner aninhado sem copiar seu descritor. `get` tem uma finalidade
diferente: obter um valor. A recusa de cópias de elementos que sejam instâncias
de modificadores torna explícita essa diferença no ponto de uso.

O acesso `at` oferece resultado falível com verificação também em release.
Sua garantia depende da chamada escrita, enquanto o açúcar de indexação conserva
um contrato de índice válido, com instrumentação de debug.

`foreach` obtém contêiner e comprimento na entrada para definir o conjunto
percorrido. A informação dos binders escolhe cópia ou endereço; `apply` usa a
mesma escolha e uma chamada fixa. Um parâmetro `dim` não precisa representar um
rank para participar desse modelo: o módulo declara suas operações e suas
aridades, e a travessia consulta o protocolo disponível.

Referência: [spec v3 §§4.6–4.9](keel-spec.md#53-keelbuffer-keelslice-e-keelrange).

## Cursor explícito

Nem toda sequência responde por índice. Uma lista encadeada, um mapa, um fluxo
lido por demanda e um recorte cujo comprimento só se conhece ao percorrer não
declaram `length` nem `ptr`, e exigi-los como condição de travessia obrigaria
esses módulos a inventar um índice que sua representação não tem — ou a deixar
a travessia fora do reconhecimento de keel.

`walk` pede outro protocolo: `begin`, `has_next` e `next`. As três operações
existem em qualquer estrutura percorrível, e nenhuma pressupõe posição
calculável. A construção fixa a ordem em que são chamadas, que é a parte que um
laço escrito à mão erra: `begin` uma vez na entrada, `has_next` antes de cada
elemento, `next` uma vez por iteração e depois do teste, nunca para descartar
resultado.

O cursor é escrito, com seu tipo qualificado pelo módulo do contêiner. Deduzi-lo
exigiria conhecer o retorno de `begin` como tipo de expressão, e escondê-lo
criaria um objeto sem nome no escopo do corpo. Escrevê-lo tem uma vantagem
adicional: o cursor é um objeto do programa, legível dentro do corpo, e um
módulo que exponha posição, chave ou profundidade a entrega por ele sem que a
construção precise de uma segunda vaga de binder.

O tipo do elemento é o que `next` declara: um módulo que devolve `T` atende o
binder por valor, e um que devolve `T *`, o binder por ponteiro. Não há
conversão nem seleção por sobrecarga, pela mesma razão que vale no resto do
PPC — escolher entre formas pelo tipo do que foi escrito é análise que ele não
faz.

Na base, `buffer` e `slice` declaram `next` devolvendo o endereço do elemento,
e não uma cópia: o binder por ponteiro serve à escrita e nunca perde um
descritor por cópia. O cursor deles é uma posição em `size_t`, declarada uma vez
por módulo e não por instância, porque não depende do tipo do elemento — por
isso se escreve `buffer.cursor`, sem argumento.

A terminação não é provada. Um `next` que não avance produz laço infinito,
exatamente como o `while` equivalente escrito à mão; a construção não promete
mais do que a forma que substitui.

Referência: [spec v3 §4.9](keel-spec.md#47-travessia-sequencial).

## Limpeza e saídas

O cleanup léxico associa a liberação ao escopo escrito. Avaliar o retorno antes
da limpeza permite devolver um valor calculado a partir de um recurso que será
liberado na saída, sem manter o recurso vivo depois do retorno.

A captura tardia observa os valores na saída; a captura explícita copia no
registro. Tipos escritos permitem materializar essas cópias sem inferência.
O diagnóstico de sombreamento evita que a emissão da limpeza encontre outra
variável com o mesmo nome em um ponto de saída interno.

Entradas externas por cima de registros e saídas não locais são problemas de
controle, não de aquisição de recursos. As verificações léxicas cobrem as formas
especificadas; não permitem concluir que todo salto C ou efeito de macro terá
limpeza automática.

As saídas introduzidas por construções keel seguem a mesma regra das saídas C.
O `break` de um braço de `match` e os verbos `win` e `fail` de um corpo de
worker deixam escopos, inclusive os de travessias aninhadas, e executam os
registros correspondentes. Uma consulta que não sai de escopo, como
`interrupted`, não dispara limpeza. A v3 não tem mais nenhuma região com
regras de salto próprias: com o fim da finalização das composições, as
restrições de entrada e saída voltam a ser as do cleanup léxico.

Referência: [spec v3 §4.8](keel-spec.md#46-cleanup-léxico).

## Macros e sintaxe do PPC

keel organiza, por uma sintaxe própria, operações que seriam expressas com
macros em C. A intenção é permitir escrever essas operações como construções
legíveis, com reconhecimento e expansão definidos, sem depender de colagem de
tokens, avaliação duplicada de argumentos ou outras técnicas de macro.

Chamar essa sintaxe de linguagem de uso não atribui ao PPC um sistema de tipos
independente. Os modificadores descrevem instanciações e transformações que
produzem declarações C. A tabela de símbolos guarda informações explícitas
necessárias à expansão; compatibilidade e validação semântica de tipos continuam
com o compilador C. Por isso a spec fala em tipos e declarações reconhecidos
pelo PPC, em vez de uma categoria de “tipos keel”.

Referência: [spec v3 §1](keel-spec.md#1-escopo-e-princípios).

## Resultado recebido pelo verbo

O primeiro argumento identifica o objeto cujo tipo foi modificado por
`outcome.outcome`. Na chamada `outcome.win(r,v)`, `outcome` qualifica o módulo
do verbo. A declaração de `r` fornece a aplicação do modificador necessária
ao despacho; procurar essa informação no destino de uma atribuição acrescentaria
uma dependência que a operação não precisa ter.

`win(r)` marca sucesso preservando o valor existente. `win(r,v)` também escreve
o valor. `fail(r,c)` e `none(r)` ajustam o código preservando o valor. Os verbos
modificam o receptor e devolvem seu conteúdo ajustado, permitindo tanto uma
chamada isolada quanto `return outcome.win(r,v);`. O retorno da função chamadora
continua explícito no C escrito pelo programa.

Os produtores de `corot` seguem a mesma forma desde a v3, e é o que torna as
duas famílias legíveis pela mesma regra: verbo qualificado, receptor no
primeiro argumento, saída escrita pelo programa.

Essa separação também define o default: `else v` consulta o resultado declarado
e chama `win(resultado,v)` quando ele falha. O setter `value(resultado,v)`
escreve somente o valor, e não serviria sozinho para estabelecer sucesso.

Referências: [spec v3 §5.5](keel-spec.md#55-keeloutcome-e-keelcorot) e
[§4.10](keel-spec.md#410-tratamento-de-resultado).

## Modificador e tipo modificado

O módulo reúne as declarações; o modificador dá sentido à aplicação ao tipo.
Com o alias `outcome`, `outcome.outcome` nomeia o modificador declarado pelo
módulo, enquanto `outcome T` usa seu nome abreviado. Essa diferença existe mesmo
quando as duas partes do nome têm a mesma grafia.

`buffer` é o modificador; `T` é o tipo sobre o qual ele atua; `buffer T` é o
tipo resultante. A instanciação materializa esse resultado em C, por exemplo
como `buffer_T` em uma representação abreviada dos nomes de emissão.
Os parâmetros pertencem às declarações do módulo genérico que descrevem
essa substituição. Chamar um tipo comum de “tipo sem parâmetros” mistura
o tipo com o mecanismo usado para produzir outras declarações.

A leitura de `long int` fornece a analogia: um especificador modifica o sentido
do tipo com que se combina. Em `buffer struct Person pessoas;`, `buffer`
modifica `struct Person` para produzir a representação de uma sequência de
pessoas, com ponteiro, comprimento e capacidade. O modificador define também
as operações que consultam e ajustam esses metadados.

`outcome T` associa o valor de `T` a um código em uma tagged struct. A leitura
desse código permite tratar resultado válido, erro ou ausência dentro do
protocolo de dois estados. `corot` é o estado cooperativo de três
estados, sem valor associado, e `tagged E T` generaliza a forma do par: uma etiqueta de um conjunto
declarado, mais o valor associado. Os três compartilham a representação — um
código ou etiqueta, mais um valor —, e o que os separa é a leitura desse campo.
`corot` e `tagged` nomeiam conjuntos declarados, e por isso são operandos de
`match`; `outcome` não nomeia conjunto, porque seus dois estados são uma
partição por zero, lida pelos predicados. O contrato pertence a essa
combinação de representação,
metadados e operações. A designação de optional ou error descreve usos do
resultado; o mecanismo do PPC é aplicar o modificador ao tipo fornecido.

“Genérico” descreve aqui o mecanismo de substituição que permite escrever as
declarações uma vez. O C resultante pode ser obtido com técnicas como X-Macros
ou a inclusão repetida de um header sob uma definição temporária de `T`:

```c
#define T int
#include <buffer.h>
#undef T
```

O fragmento ilustra a técnica de expansão, não um header da base keel. Um header
construído para esse uso materializaria as declarações para `int`. O PPC fornece
sintaxe e regras de nomes para realizar essa geração diretamente, mantendo o
compilador C como verificador do resultado.

O nome `struct Person` é um argumento de tipo completo para essa substituição.
Reconhecer a palavra `struct` seguida do nome basta para preservar essa forma
na expansão; não exige analisar semanticamente seus campos. A mesma forma de
referência por tag existe para `union` e `enum`.

Referências: [spec v3 §1](keel-spec.md#1-escopo-e-princípios),
[módulos e modificadores](keel-spec.md#43-módulos-genéricos) e
[resultados](keel-spec.md#55-keeloutcome-e-keelcorot).

## O nome do arquivo gerado é o símbolo

O header gerado de `keel.arena` chama-se `keel/keel_arena.h`, e não
`keel/arena.h`. A repetição incomoda à primeira vista, e o mesmo desconforto
aparece em `net/net_http.h`. Ela é o preço de uma propriedade que se quis:

> O header de um tipo é função do nome do tipo, e de mais nada.

Sem ela, há duas convenções convivendo no mesmo diretório. `keel/arena.h`
declara `keel_arena` e `keel/keel_buffer_i32.h` declara `keel_buffer_i32` — o
primeiro nome vem do módulo, o segundo do símbolo. Quem lê `keel_arena` num
diagnóstico do compilador C, ou quem precisa emitir o `#include` que traz esse
tipo, tem de saber antes se ele nasceu de um módulo ou de uma instância, porque
a resposta muda a regra. É uma pergunta que o nome já podia ter respondido.

E é uma pergunta que o **cgen** faz o tempo todo. A regra 5 do backend §4.3.2
manda cada arquivo com corpo incluir o `.impl.h` de quem ele chama; o gerador
descobre quem é chamado resolvendo o nome manglado, e o que ele tem em mãos,
naquele ponto, é exatamente um símbolo. Com a regra uniforme, o include é
concatenação; sem ela, é consulta a uma tabela de origem que precisaria existir
só para isso.

**A assimetria com o fonte é deliberada.** O `.k` continua morando no caminho do
módulo — `module app.cfg;` em `app/cfg.k` —, e o backend não o segue. A razão é
que os dois arquivos têm quantidades diferentes de informação: o `.k` **declara o
próprio nome na primeira linha**, então o caminho é redundante e pode servir de
conferência barata, que é o que `module-fora-do-caminho` faz. O header gerado não
declara nada sobre si; o nome do arquivo é o único identificador que ele tem, e
gastá-lo com meia informação seria desperdício.

Instância nunca teve escolha: `buffer i32` não é módulo e não tem caminho de
módulo para herdar, então o nome dela sempre foi o símbolo. A decisão aqui não
foi inventar uma convenção — foi **parar de manter duas**, estendendo ao módulo
a que a instância já obrigava.

## Arena é um tipo

`arena` já nomeia uma estrutura C que controla uma região de memória por um
topo de alocação. Não há um tipo `T` a modificar para declarar uma arena:
`typedef struct { ... } arena;` fornece o tipo, e `arena a;` declara um objeto
desse tipo. A declaração conserva essa forma no C emitido.

A passagem do descritor por referência pertence ao contrato das operações;
ela não caracteriza um modificador. Da mesma forma, o `T` em
`arena.alloc(a, T, n)` indica o tipo dos objetos a alocar, para obter tamanho
e alinhamento, sem alterar o tipo de `a`.

Preservar a declaração também preserva as regras C de inicialização. O
programa inicializa o descritor por um construtor ou escreve um inicializador
explícito. Tratar a declaração como uma instanciação com inicialização
implícita acrescentaria um efeito que `arena a;` não expressa.

Referências: [spec v3: memória por região](keel-spec.md#52-keelarena)
e [backend: arena](keel-c-backend.md#54-arena).

## O que a revisão v3 removeu

O registro existe para que a remoção seja legível como decisão, e não como
esquecimento. Cada linha aponta a seção deste documento que responde por ela.

| Saiu | Entrou no lugar | Onde está a razão |
| --- | --- | --- |
| `cofsm`, com sua declaração de estados e `cobreak` | `tags` mais `match`, sobre qualquer tipo que declare `tag` | Conjuntos fechados e exaustividade; Estrutura de controle e máquina completa |
| `coseq` e `copar` como construções | `routine.seq` e `routine.par`, funções da base sobre uma tabela de `slot` | Composição como biblioteca; Rotina, tabela e contexto |
| A região `coexit:` e a reafirmação do código | O statement seguinte à declaração do resultado | Composição como biblioteca; Limpeza e saídas |
| `cowin`, `coagain` e `cofault` como statements de retorno | `corot.win`, `corot.again` e `corot.fault` como verbos, com `return` escrito | Produção e consulta da falha cooperativa |
| A fórmula de faixas dentro de `parallel`, e a exigência de contêiner indexável | O verbo `partition`, declarado pelo módulo do contêiner | Particionável e percorrível |
| O aviso de indisponibilidade de OpenMP | Nada: o lowering é do backend, e a série sempre foi execução permitida | Políticas e sinalização de interrupção |
| A exigência de `length` e `get`/`ptr` para toda travessia | `walk`, com `begin`, `has_next` e `next` | Cursor explícito |

Os diagnósticos acompanham as construções, e de duas maneiras diferentes.

Os de `cofsm` **mudaram de contrato**: a condição que observavam continua sendo
observada, agora sobre o conjunto de tags e o despacho.

| Diagnóstico da v2 | Correspondente na v3 |
| --- | --- |
| `cofsm-sem-nome` | `tags-sem-nome` |
| `cofsm-nome-repetido` | `tags-nome-repetido` |
| `estado-repetido` | `tag-repetida` |
| `estado-de-outro-tipo` | `tag-de-outro-conjunto` |
| `estado-fora-da-lista` | `tag-fora-da-lista` |
| `estado-sem-rotulo` | `tag-sem-rotulo` |
| `estado-fora-de-faixa` | `tag-fora-de-faixa` |
| `cobreak-fora-de-cofsm` | nenhum: o `break` do braço é a saída, e um `break` fora de braço é C comum |

Os demais **deixaram de existir**, porque a condição que observavam deixou de
ser expressável: `return-em-coexit`, `goto-cruza-coexit`, `salto-cruza-coexit`,
`coexit-posicao-invalida`, `estagio-invalido` e `participante-sem-corot`, com a
região de finalização e a lista de participantes; `co-verbo-fora-de-corot` e
`cowin-sem-valor`, com os statements de retorno cooperativo;
`copar-politica-nao-constante`, com a política escrita entre colchetes, que
passou a ser argumento de execução; e `openmp-indisponivel`, pela razão
registrada em Políticas e sinalização de interrupção.

Na direção oposta, os diagnósticos novos são de protocolo, e não de forma
escrita: `nao-particionavel`, `nao-percorrivel-por-cursor`,
`particao-de-outro-tipo`, `cursor-de-outro-tipo`, `match-sem-tag` e
`match-sem-conjunto` recusam um tipo que não declara a operação exigida.

Referência: [spec v3 §5.2](keel-spec.md#62-catálogo).
