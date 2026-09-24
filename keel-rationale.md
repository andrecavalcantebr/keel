# keel — Justificativas de projeto

> © 2026 Faculdade de Engenharia Elétrica e de Computação, Universidade
> Federal do Amazonas (FEEC/UFAM). Licenciado sob
> [CC BY-SA 4.0](LICENSE-DOCS.md) ([tradução](LICENSE-DOCS.pt.md)) — este
> documento é prosa sobre a linguagem, não código; o `cgen` e a Base keel têm
> licença própria, ver [`LICENSE.md`](LICENSE.md). Escrita e revisão tiveram
> auxílio de Claude Opus e Claude Sonnet (Anthropic), sob direção humana.

Este documento reúne as motivações e as decisões de projeto de keel. Os
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

keel gera o que a macro geraria — struct nomeada, funções inline, sem indireção
e sem overhead — com o nome do tipo intacto do fonte até a mensagem do compilador
C. Nenhuma das três formas deixa de compilar por causa de keel; o que ele oferece
é não precisar mais delas.

E o mesmo vale para o resto do vocabulário: cada construção responde a uma coisa
que **o C não expressa**, ou expressa mal. Unidade, limites que viajam com o
dado, memória por região, saída de escopo com limpeza, ponteiro que diz
um-ou-muitos, conjunto fechado de etiquetas. É esse o teste de admissão, e ele é
mais estreito do que "serve à orientação a dados": inverter layout, por exemplo,
o C expressa perfeitamente — `struct { f32 *x; f32 *y; }` —, e por isso não há
marcador de inversão (ver a tabela de recusas adiante).

O que o C não expressa nesse struct é que `x` e `y` têm a mesma extensão, e
qual é ela. É isso, e não o layout, que `extent` declara: ver [`extent`: da
recusa à admissão](#extent-da-recusa-à-admissão).

Referência: [spec §1](keel-spec.md#1-escopo-e-princípios), princípio 9.

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

> **2. keel não precisa entender C.** Um compilador não entende as macros do
> assembler: ele emite dentro delas. keel faz o mesmo — reconhece as próprias
> construções e copia o resto sem examinar.

> **3. O compilador C é o verificador final.** keel não reimplementa o sistema de
> tipos do C. Erro de tipo aparece no compilador C, mas com os nomes que você
> escreveu no fonte keel.

Os três reaparecem como regra na spec, e nenhum como texto repetido: o primeiro é
o princípio 2, mais a convenção de redação em pares; o segundo é o princípio 7 e
o contrato de análise da §1.3; o terceiro é o princípio 3.

Referência: [spec §1.1](keel-spec.md#11-princípios-de-projeto) e
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

Referência: [spec §1.1](keel-spec.md#11-princípios-de-projeto), princípio 2.

## Critério de admissão, em exercício

Uma construção entra no núcleo quando passa por três perguntas, nesta ordem:

1. **keel consegue?** Se reconhecer ou traduzir a construção exigir análise
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
`enum` vindo de header não é uma lista que keel conheça.

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

Referência: [spec §1.1](keel-spec.md#11-princípios-de-projeto) e
[§1.3](keel-spec.md#13-contrato-de-análise).

## O que fica fora do núcleo, e por quê

A lista não é registro de trabalho futuro: é o limite do mecanismo, escrito
junto com ele. Quando aparecer a próxima proposta, o teste é o da seção
anterior, e não a presença nesta tabela.

São duas recusas de espécie diferente, e misturá-las esconde as duas.

**Recusado pelo limite de análise.** keel não conseguiria reconhecer ou
traduzir estas formas sem análise semântica de C:

| Fica de fora | O que exigiria |
| --- | --- |
| Operador de propagação de erro | Localizar o operador dentro de expressão C arbitrária e içar seu operando, o que muda a ordem de avaliação do que estava escrito |
| Tupla anônima e desestruturação de retorno | O tipo de uma chamada |
| Extração ou conversão implícita de resultado | O tipo de uma chamada, e a correspondência entre dois catálogos de código |
| Função genérica livre, com o tipo deduzido | Deduzir o argumento de tipo a partir do argumento escrito na chamada. O tipo escrito como argumento é admitido: ver [parâmetro de tipo em função](#parâmetro-de-tipo-em-função) |
| Corrotina com retomada de posição | Interpretar o corpo para içar variáveis locais através da suspensão |
| Lambda e closure | O mesmo, mais captura implícita |
| Fatiamento multidimensional como sintaxe | Despachar pela forma dos argumentos, e não pela contagem |
| Inversão de layout, como um marcador `soa` **automático** | Conhecer o tipo de cada campo para decidir o que é invertível |
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
| Sincronização inserida por `parallel` | keel não sabe o que as partes alcançam por ponteiro; inserir sincronização seria prometer uma segurança que ele não verificou |
| `defer` inserido automaticamente sobre memória externa | Aquisição e liberação são do contrato da origem, não do reconhecimento de uma arena |
| Realocação implícita de `buffer` | Invalidaria silenciosamente vistas e ponteiros derivados, que é a decisão que o programa precisa manter à vista |

Referência: [spec §6.4](keel-spec.md#64-programa-conforme-e-limites) e o
item 5 (casos especiais) de cada contrato do capítulo 4.

## `extent`: da recusa à admissão

A recusa original mirava um marcador que inverte o layout de um struct **já
existente**, inferindo dele o que é invertível. Essa forma continua fora: ela
exige exatamente a análise de tipo C que o critério de admissão recusa na
primeira pergunta.

A primeira admissão, com o nome `soa`, trocava a inferência por marcação
explícita: em `soa struct NOME { array T campo; … }`, cada campo marcado virava
ponteiro, e keel reescrevia `var[i].campo` como `var.campo[i]`. A forma foi
abandonada por quatro defeitos, e os quatro vinham da mesma tentativa de fingir
que havia uma linha:

- `p[i]` prometia uma linha que não existe. Não havia gather, e `p[i]` sozinho
  precisava de uma recusa própria.
- `p[i]` sobre `soa position *p` é C válido: indexação de ponteiro. A reescrita
  sequestrava essa forma e impedia vetor de tabelas (AoSoA).
- A marca `soa` no ponto de uso (`soa position p;`, repetida em todo
  parâmetro) era redundante: a informação pertence ao tipo.
- A inserção de um nível de ponteiro travava `array f32 x[MAX]` e fazia de
  `array f32 *x` um `f32 **x`.

Sem esses quatro, o que a construção acrescenta ao C é uma coisa só: **um
conjunto de vetores que compartilham um único controle de extensão.** O layout,
o C já expressa — `struct { f32 *x; f32 *y; }` —, e por isso não há inversão. O
que o C não expressa é que `x` e `y` têm a mesma extensão, e qual é ela. É esse
o fato que `extent` declara, e o nome diz isso: `soa` descreve o layout que
resulta quando há várias colunas; `extent` descreve o que foi declarado. O
precedente de nomenclatura é o `std::mdspan` do C++23, cujos *extents* também
misturam extensões estáticas e dinâmicas.

**A declaração carrega tudo; o uso, nada.** Os descritores ficam antes da `{`,
como lista de opções, no mesmo lugar em que `extern_c [type_h]` e
`defer [now …]` põem as suas. Variáveis e parâmetros usam `struct NOME`, e o
acesso é `p.x[i]` ou `p->x[i]`, como em C.

**`array` é o marcador de coluna, e mantém o seu sentido.** `array f32 x[MAX]`
é vetor embutido, como na spec §4.2; `array f32 *x` é ponteiro, exatamente como
escrito. Um vetor C sem marcador, como `char name[32]`, é campo comum. O que é
coluna vem do programador escrevendo `array`, e não de keel examinando o tipo C
do campo; é essa troca que tira a construção de trás da primeira pergunta do
teste de admissão. (A versão `soa` deste texto dizia que `array` "desaparecia
no lowering como `ref`". Não desaparece: `array` vira `T v[..]`.)

**A capacidade é obrigatória.** Cada grupo é `[contagem, capacidade]`, como em
`buffer` e `arena`: nada em keel cresce sem limite conhecido, e é a capacidade
que torna o acesso verificável. Ela é campo, quando dinâmica, ou constante
(`constexpr` ou literal), quando fixa; `#define` fica fora pela razão de
sempre, que é ser invisível a keel (spec §1.3). A resolução é por nome, não por
valor: primeiro campo, porque C não admite dois membros com o mesmo nome;
depois constante. A dimensão de uma coluna embutida confere com o grupo pelo
mesmo nome — `x[MAX]` sob `[len, MAX]` —, sem que keel avalie coisa alguma.
Coluna embutida sob capacidade que é campo teria duas fontes de capacidade, e é
recusada.

**A manutenção é do programa.** keel não insere campo nem gera `push` ou `pop`.
A versão com verbos sintetizados chegou a ser escrita, ainda como `soa`, e foi
revertida: verbos que não se deixam escrever como um `module` comum são campo
escondido e corpo gerado a partir de uma lista de campos que não cabe em `dim`,
`type` ou `tags` sem reabrir a metalinguagem de template que a spec §4.3 fecha.
O ganho de `extent` não é manter a extensão; é keel **saber** a extensão de
cada coluna.

**Com esse saber, keel verifica o acesso**, e essa é a garantia que responde à
terceira pergunta do critério. O modelo é o de `array` (spec §4.5) e o de
`get`/`set` (spec §5.3): recusa na tradução quando índice e capacidade são
decimais conhecidos, `assert` por índice escrito em debug, nada em release. O
`assert` amarra o invariante ao mesmo termo — `i < len && len <= cap` — e fica
no acesso, não nas escritas em `len`: alias e código C escapam a qualquer
varredura das escritas, e o acesso é o ponto por onde todo uso passa. Em várias
dimensões, verificar cada dimensão é essencial: um índice interno além da
capacidade lê a linha seguinte dentro do mesmo bloco, e o ASan não o vê.

**A função de acesso é do núcleo.** Um `assert` escrito antes da expressão
avaliaria o índice duas vezes, e `p.x[i++]` quebraria a avaliação única da spec
§4.5. A saída é a mesma do açúcar de indexação sobre modificador, `*ptr(x,i)`:
uma função `static inline` por coluna, com a verificação dentro, que recebe o
struct por ponteiro e um índice por dimensão. Quem a emite é o núcleo, porque só
ele tem a declaração, e nenhum módulo de aridade fixa a escreveria: o rank e o
tipo de cada coluna mudam de uma declaração para outra. É esse o privilégio que
o princípio 8 manda enumerar, e a enumeração é o próprio contrato da spec §4.11.

**O passo é a capacidade, não a contagem.** Na coluna por ponteiro, o endereço é
Horner sobre as capacidades internas, `(i·c₁ + j)·c₂ + k`. Com a capacidade
como passo, crescer a contagem até ela não move dado, e o layout por ponteiro é
o mesmo do embutido. A capacidade externa não entra no endereço, só na
verificação. A forma é retangular; tabelas irregulares (CSR) são outra
estrutura, que o programa monta com uma coluna de deslocamentos.

**O índice é a identidade da linha, e só enquanto não há remoção.** Remover por
troca com a última linha, a forma barata, muda o índice da linha trocada. Quem
precisa de identidade estável usa o que os sistemas de entidades usam: índice
mais geração, ou uma coluna de ids com um mapeamento de id para índice (slot
map, sparse set). Isso é biblioteca sobre `extent`, e não núcleo.

**A coluna de etiqueta é um `tagged` decomposto.** Quando as linhas são
heterogêneas, como os nós de uma AST, uma coluna cujo tipo é um conjunto `tags`
diz o que cada linha é, e o "valor" fica espalhado nas outras colunas.
`match (t.kind[i])` funciona porque o tipo declarado do operando é o conjunto
(spec §4.9). Ids e gerações, que dizem **quem** a linha é, são inteiros comuns,
sem conjunto fechado, e nunca operandos de `match`.

**Fora desta versão: descritores vetoriais e `tensor`.** Um grupo cujos
descritores fossem campos vetor (`size_t shape[N], cap[N]`) permitiria escrever
`tensor(N)` como biblioteca sobre `extent`. O tensor é biblioteca de um jeito ou
de outro — o açúcar com `dim` da spec §4.3 já o alcança —, e a escolha fica para
quando ele for desenhado.

Referência: [spec §4.11](keel-spec.md#411-extent).

## Fronteira com C e conflitos léxicos

keel precisa reconhecer suas construções dentro de código C, inclusive em
expressões. A escolha é usar varredura léxica, estrutura de delimitadores e
informações de símbolos keel declarados, sem implementar um analisador
semântico de expressões e tipos C.

Palavras contextuais reduzem conflitos com identificadores existentes, mas
não eliminam todos os conflitos de reconhecimento. Algumas posições, nomes e
estruturas de pré-processamento precisam de restrições explícitas. A recusa
nesses casos preserva um reconhecimento definido sem exigir conhecimento
semântico do C que keel não possui.

A promessa central é traduzir keel integralmente para C e preservar o texto
das expressões fora das substituições necessárias. Ela não equivale a aceitar
qualquer fonte C sem adaptação.

Referência: spec §§1.1, 1.3 e 1.4.

## Por que `<opaque>` é o terminal central

Uma gramática de ilha não é uma gramática de C com buracos: é o inverso — uma
gramática pequena com um terminal enorme. `<opaque>` é o que torna o contrato de
análise da §1.3 verificável em vez de aspiracional, porque toda produção que não
sabe o que fazer com um trecho **tem para onde reduzi-lo**.

A consequência é medível: a gramática tem setenta e duas produções, e nenhuma
delas descreve expressão, precedência ou a gramática completa de declaradores do
C. A ambiguidade clássica — `(a)(b)` é chamada ou conversão? — não aparece,
porque as duas leituras reduzem ao mesmo `<opaque>`.

Referência: [spec §2.2](keel-spec.md#22-sintaxe).

## Por que `..` não é um token

Fundir `2..7` num token de intervalo exigiria **retrair o número já emitido**, e
faria um token conter sequência balanceada arbitrária: `xs[(a+1)..(b*2)]` é legal,
e nada impede que os lados contenham chamadas, casts e vírgulas dentro de
parênteses. Um lexer que carrega estrutura balanceada dentro de um token deixou de
ser lexer.

Deixando `..` como pontuação, a forma do intervalo vira problema do parser, que é
onde o balanceamento já existe. O léxico continua não decidindo nada — que é a
regra da §2.4 e o que mantém as duas camadas separáveis.

Referência: [spec §2.2](keel-spec.md#22-sintaxe).

## A posição de contêiner não é um sistema de tipos

`container` parece tipagem e não é: é uma **tabela fixa de tipos de retorno**
avaliada sobre símbolos que keel mesmo declarou. Cada nó tem tipo em função apenas
do primeiro argumento, e a avaliação nunca entra em `<opaque>`. É o preço mínimo da
resolução de verbo — sem ele, `buffer.push(x, v)` não teria como escolher a função
—, e é o único lugar em que ele é pago.

É também onde a camada foi raspada de propósito. `f(y)[i]` e `(cast)x[i]` são
recusados não porque sejam difíceis, mas porque aceitá-los significaria tipar o
retorno de uma chamada e o resultado de uma conversão — e a partir daí não há onde
parar: o próximo pedido é o operador de propagação, e depois a desestruturação.
As duas estão na tabela de recusas por limite de análise, e a gramática é o lugar
onde esse limite fica escrito em vez de prometido.

Referência: [spec §2.2](keel-spec.md#22-sintaxe) e
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
    /* lots of code */
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

Referência: [spec §2.4](keel-spec.md#24-semântica).

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

Referência: [spec §2.5](keel-spec.md#25-restrições-e-diagnósticos).

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

Referência: [spec §2.3](keel-spec.md#23-reconhecimento).

## Reconhecimento de funções e declaradores C

keel precisa localizar declarações e corpos de funções para registrar o nome,
separar interface e implementação, traduzir parâmetros keel e reconhecer as
saídas que participam de `defer`. Para isso, precisa distinguir uma função de
um objeto cujo declarador ou inicializador também contenha parênteses.

Os três casos mostram por que a presença de `(` não basta:

```c
int f(void);       /* function declaration */
int (*f)(void);    /* declaration of a pointer to function */
int x = f(1);      /* object declaration with a call in the initializer */
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

Referência: [spec §2.3](keel-spec.md#23-reconhecimento),
“Declarações e palavras contextuais”.

## Perfis e exemplos de tradução

O alvo original era exclusivamente C23; o perfil C11 veio depois. Por isso as
definições gerais não citam dialeto: onde um perfil aparece nomeado, a regra é
daquele perfil, e a emissão de cada um pertence ao backend.

Os pares keel/C mostram a operação essencial que o programador precisa
compreender. Reproduzir todos os detalhes de emissão tornaria a spec dependente
da apresentação do backend e duplicaria regras como mapeamento de linhas e
atributos gerados.

Referência: convenções da spec e §1.2.

## Prelúdio e base mínima

O prelúdio implícito limita-se a `import keel types;`. Importar os demais
módulos explicitamente torna suas dependências visíveis, inclusive quando
pertencem à base distribuída com keel.

`keel.tagged` entra pela mesma regra: o modificador que associa etiqueta a
valor é módulo comum, importado por quem o usa, e não palavra do núcleo.
`match` não depende desse import — despacha sobre qualquer tipo que declare
`tag` —, e é o que mantém o núcleo independente da base.

A base é toda `pub inline`, e isso é decisão de linguagem, não de emissão. O
que ela oferece — `push`, `get`, `partition`, o cursor — é miolo de laço
quente, e ali o inline é o que mantém o código junto do dado que ele toca.
Distribuir corpos fora de linha, pré-compilados na instalação, trocaria isso
por uma chamada por acesso, salvo LTO, e ainda exigiria uma variante por perfil
e por `--checks`. A consequência é que `instance` sobre a base nunca tem corpo
a colocar em lugar nenhum, e o `redundant-instance` diz exatamente isso: o
aviso é o preço anunciado da escolha, e não defeito a corrigir.

A prioridade da revisão é fechar o núcleo, as verificações do parser e a base
mínima. Bibliotecas adicionais serão desenvolvidas sobre esses contratos,
com apoio da experiência de implementação do compilador.

A base segue uma convenção de grafia (spec §5.1), e a maiúscula do conjunto de
tags não é ornamento: nome de conjunto é nome de tipo, e `types` o injeta nu no
arquivo de quem importa. Uma palavra comum em minúscula ali colidiria com
identificadores do programa a cada import.

Referência: spec §4.1.

## Constantes nomeadas

`constexpr` foi introduzido depois das primeiras regras que exigiam literais.
Essas regras devem incorporar constantes nomeadas sem introduzir avaliação de
expressões em keel. Ler um literal de uma declaração conhecida permite usar um
nome sem mudar o limite de análise.

O mesmo limite governa os valores escritos de um conjunto de tags: aceita-se um
literal decimal, com sinal opcional, ou uma constante conhecida. Um valor
calculado exigiria avaliação de expressão, e um conjunto sem valor conhecido
não sustentaria a verificação de exaustividade sobre nomes.

Referência: spec §4.2.

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

Referência: [spec §5.2](keel-spec.md#52-keelarena) e
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
interpretar expressões e efeitos de código C além do contrato de keel. Essa
limitação deve constar da especificação junto à garantia efetivamente oferecida.

Referência: spec §§4.5–4.6.

## Substituição e aridade fixa

`dim` fornece substituição numérica; `type` fornece a substituição do
parâmetro pelo tipo escrito, inclusive uma forma como `struct Person`;
`tags` fornece a substituição do parâmetro pelo `enum` de um conjunto
declarado. O significado de `N` é dado pelo modificador: pode
representar um rank, mas keel não impõe essa interpretação.

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

O parâmetro de tipo é opaco pelo mesmo motivo de a substituição ser textual.
Abrir protocolo sobre `T` exigiria dizer em que módulo o verbo mora quando o
genérico não importa o módulo do argumento — e isso é um sistema de traits:
bound na assinatura, implementação com dono, verificação do corpo contra o
bound. O genérico de keel sabe de `T` o que o header C genérico sabe do seu
`#define T`, e nada mais. Uma instância que menciona `T`, como
`outcome buffer T`, não é opaca: o modificador é conhecido, e é dele que vêm
os verbos.

**Instanciação degenerada.** `void` e argumento `const` são os dois casos em
que a instância não admite toda a superfície do genérico. A omissão de campos e
verbos não é análise de equivalência de tipos C nem eliminação geral de código
que mencione `T`: decorre do argumento escrito, é a mesma em toda instância com
o mesmo argumento, e mantém a instância função apenas do próprio nome (backend
§7.2). O qualificador de topo sai dos verbos que copiam `T` porque é o que o C
já faz com ele numa cópia. Um `typedef` que menciona o parâmetro não tem forma
escrita com argumento: o programa o alcança pelos verbos e campos que o
mencionam, ou declara o próprio `typedef` sobre a mesma forma C, que em C é o
mesmo tipo. E a declaração que não menciona parâmetro é tipo, `constexpr` ou
`inline` porque um módulo genérico nunca é a unidade compilada: uma definição
fora de linha que não pertence a instância nenhuma não teria onde ficar.

Essa escolha limita as formas de interface de um tensor, mas o tensor é um
exemplo de modificador, não o modelo de todo parâmetro `dim`. A orientação a
dados de keel permanece dentro dos limites de substituição e reconhecimento,
sem metalinguagem de templates ou análise semântica de tipos C.

Referência: [spec §4.3](keel-spec.md#43-módulos-genéricos).

## Parâmetro de tipo em função

`arena.alloc(a, T, n)` e `slice.from(T, p, n)` eram privilégios: a spec os
listava como os casos em que o tipo vinha escrito na chamada. O parâmetro
`type X` de função troca a lista por duas regras gerais, e as duas operações
passam a ser biblioteca comum (princípio 8).

A recusa da "função genérica livre" (tabela de recusas) é sobre **deduzir** o
tipo a partir de um argumento, o que exigiria saber o tipo de uma expressão C.
Um tipo **escrito** na chamada não pede dedução: é um token que keel já sabe
ler como tipo.

**A espécie é decidida pela linha `module`.** Se o nome do parâmetro é
parâmetro do módulo, ele já é token de substituição, e o tipo escrito na
chamada só pode dizer uma coisa: qual instância. Se não é, não há instância a
escolher, e o parâmetro é apagado em tamanho e alinhamento. O teste é léxico e
fica na declaração; quem chama não decide nada. O único risco seria um token
com dois sentidos, e ele não ocorre: o nome de um parâmetro do módulo tem
sempre o sentido de seleção, e um parâmetro apagado com nome de tipo conhecido
é recusado.

**Apagamento, e não uma função por tipo.** A alternativa considerada foi emitir
uma função por tipo escrito, como `keel_arena_alloc_i32`. Ela custa identidade,
mangling e um lugar para cada corpo, que é a maquinaria do módulo genérico
aplicada a uma função livre; e não ganha nada enquanto o corpo usa `T` só em
`sizeof` e `alignof`, porque depois do inline as duas formas geram o mesmo
código. Ela só produziria algo diferente se o corpo usasse `T` de outro jeito —
declarar `T x`, fazer aritmética sobre `T *` —, e aí já é a função genérica
livre de verdade, uma decisão bem maior. Por isso o uso de um parâmetro apagado
fica restrito a `sizeof`, `alignof` e `X *` na assinatura.

**A seleção precisa fixar a instância inteira.** Sem contêiner entre os
argumentos, nada mais determina a instância, então o verbo lista um `type` para
cada parâmetro do módulo. Um módulo com `dim` ou `tags` na linha não tem verbo
de seleção.

**`dim` não é parâmetro de função.** A motivação seria dimensionar um vetor
local, `f32 tmp[N]`. Apagado em `size_t n`, `tmp[n]` é VLA: opcional em C11 e
C23, ausente no MSVC, pilha sem limite verificável, `sizeof` em execução (ver
[o que o VLA errou](#o-que-o-vla-errou)). Não apagado, exige uma função por
valor de `N` — identidade, mangling, colocação de corpos —, a mesma porta que a
função por tipo reabriria. Ao contrário de `type`, `dim` não tem forma apagada
que não seja VLA, e a regra do apagamento não se estende aos outros parâmetros.
As alternativas já existem: uma arena de rascunho
(`arena.alloc(scratch, f32, n)`), ou capacidade fixa com contagem dinâmica, que
é o `[len, MAX]` de `extent`.

Referência: [spec §4.4](keel-spec.md#44-resolução-de-operações).

## Memória e visão: a direção da conversão

`buffer`/`slice` não são um par arbitrário de contêineres parecidos — são a
primeira instância de um padrão que se repete: um lado **memória**, que
possui o armazenamento e pode crescer, e um lado **visão**, que descreve um
trecho fixo sem possuir nada. `bitbuffer`/`bitslice`, `buffer(N)`/`slice(N)`
e `string`/`strview` são o mesmo par, com o mesmo layout de dependência.

A conversão só faz sentido num sentido: memória produz visão (`buffer` sabe
o comprimento atual, a capacidade, e pode entregar uma janela sobre o que já
tem), nunca o contrário (uma visão não tem de onde tirar armazenamento
próprio — "criá-lo" seria alocar, o que já tem nome, `clone`, e não é
conversão). Por isso o lado memória importa o lado visão — para devolvê-la —,
e o lado visão nunca importa o lado memória. Isso não é só estilo: é o que
evita um ciclo de import genuíno. Se o verbo de `range-index` fosse um único `of`
universal, vivendo no módulo da visão e aceitando o lado memória como
argumento (como uma leitura apressada de `slice.of` sugeriria), o lado visão
precisaria conhecer o tipo do lado memória — e o lado memória já precisa
conhecer o da visão, para devolvê-la de `partition` e do próprio `range-index`.
As duas importações juntas são circulares, e a linguagem já recusa isso
(`circular-import`, §4.1).

A saída, já implementada antes de estar escrita aqui: cada lado memória
declara seu próprio verbo de `range-index`, com o nome que fizer sentido para ele
— `buffer.as_slice`, não `buffer.of` — e o lado visão continua com o seu
próprio `of`, para recortar a si mesma, sem depender de nada além do próprio
tipo. `slice` nunca importa `buffer`; é sempre `buffer` que importa `slice`.

Por isso o nome do verbo de `range-index` é do módulo que o declara, e não do
protocolo — na base, `as_slice` em `buffer` e `of` em `slice`. A liberdade não é
estilo: é o que permite ao lado memória declarar o verbo sem que o lado visão
precise conhecê-lo, e o que mantém o grafo de imports acíclico.

Referência: [spec §4.5](keel-spec.md#45-indexação-e-range-index).

## Particionável e percorrível

A revisão anterior deixava `parallel` conhecer a divisão: a fórmula das faixas
era do núcleo, e a construção exigia do contêiner os mesmos verbos de `foreach`.
Isso obrigava dois compromissos que não eram necessários. O primeiro é que só
podia ser distribuído o que fosse indexável, ainda que a divisão de outra
estrutura seja escrevível por seu próprio módulo. O segundo é que keel
afirmava a disjunção das faixas — uma propriedade que ele não verifica em
estrutura alguma, e que só o autor do módulo pode sustentar.

`partition(x, k, w)` resolve os dois. O contêiner declara como se divide; keel
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

Referência: [spec §4.8](keel-spec.md#48-execução-particionada).

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

Referência: [spec §1.1](keel-spec.md#11-princípios-de-projeto), princípio 8,
e [§4.8](keel-spec.md#48-execução-particionada).

## Políticas e sinalização de interrupção

A quantidade de workers expressa a divisão solicitada pelo programa. Não fixar
um teto no núcleo permite que a execução use os recursos disponíveis sem
prometer simultaneidade. Uma parte vazia não exige acesso a dados: o corpo do
worker executa sobre uma fatia de comprimento zero.

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
programa, e keel não insere sincronização.

**O nome do bloco é único na função, e não só no escopo.** Dois blocos com o
mesmo nome em escopos aninhados declarariam dois símbolos de controle, e o de
dentro sombrearia o de fora: uma consulta escrita depois leria o bloco errado
sem que nada acusasse.

**O fim natural do worker não é veredito.** Só `win` conta para a política, e
só `fail` conta contra ela. É o que faz `ALL` ser satisfeito por workers que
simplesmente terminam, e o que impede `ANY` de ser satisfeito por quem não
achou nada. Dentro do corpo, só `interrupted` tem leitura definida, porque as
demais consultas dependem de workers que ainda executam.

Referência: [spec §4.8](keel-spec.md#48-execução-particionada).

## Conjuntos fechados e exaustividade

Um `enum` C é aberto para quem o lê: o compilador aceita qualquer inteiro no
lugar de uma constante, e um `switch` sem `default` não é erro. Nada disso é
corrigível por análise, porque keel não abre headers e não conhece as
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
`ambiguous-match-tags`, e a saída é escrever o parâmetro.

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
estado sem valor, `tagged Cycle void`, e a um valor etiquetado de verdade,
`tagged Kind struct Node`.

O despacho não exige `tagged`. `match` pede o verbo `tag` pela resolução
comum, e o conjunto vem da declaração `tags` associada ao módulo do operando.
É o que faz `corot` participar de `match` sem mudança de representação: o
código continua sendo um `i32`, e `corot.tag` o traduz para uma das três tags
declaradas pelo próprio módulo. Um operando cujo módulo não declara conjunto
não admite `match`, porque não haveria lista contra a qual verificar; o
programa continua podendo escrever `switch`.

**O conjunto também é tipo.** `tags Nome [ … ];` declara um `enum` cuja lista
keel conhece, e isso vale fora de qualquer módulo genérico: uma variável, um
parâmetro ou uma coluna de `extent` declarados com o tipo do conjunto são
operandos de `match`, e o conjunto é o próprio tipo. É o `tagged` sem o valor
associado, só que explícito. A marca que keel entende é a declaração `tags`, e
não o `enum`: um `enum` C, mesmo escrito num módulo keel, não tem uma lista que
keel reconheça como fechada, e continua pedindo `switch`. A representação não
muda: a variável é o `enum`, como em C. O `i32` continua sendo a largura do
campo de etiqueta do `tagged`, que é o que a ABI fixa.

Referência: [spec §4.9](keel-spec.md#49-conjuntos-de-tags-e-despacho).

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

Referência: [spec §4.9](keel-spec.md#49-conjuntos-de-tags-e-despacho).

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

Os três estados de `corot` são um conjunto fechado, e a spec os declara como tal:
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

Referência: [spec §5.5](keel-spec.md#55-keeloutcome-e-keelcorot).

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

A mudança foi na categoria dos produtores. `cowin`, `coagain` e `cofault`
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

Referência: [spec §5.5](keel-spec.md#55-keeloutcome-e-keelcorot).

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
execução, a mesma fatia reaproveitada, o código de cada participante legível
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

Referência: [spec §5.6](keel-spec.md#56-keelroutine).

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
Mover o estado para fora exigiria uma segunda fatia paralela, com dois
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

Referência: [spec §5.6](keel-spec.md#56-keelroutine).

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

Referência: [spec §1.1](keel-spec.md#11-princípios-de-projeto), princípio 4,
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

Referência: [spec §4.1](keel-spec.md#41-módulos-e-interoperabilidade).

## Marcadores e declarações

`array` registra um fato que o C já representa: o símbolo é um vetor. Com esse
registro, keel pode reescrever índices múltiplos e pedir dimensões por `sizeof`,
sem contar inicializadores nem converter o armazenamento para um formato próprio.

Num `extent`, `array` marca as colunas. É o mesmo registro — o campo é vetor —,
e o `extent` o usa para saber quais campos seguem o controle de extensão.

`ref` registra uma restrição sobre uma declaração. A varredura dos operadores
aplicados ao símbolo cabe no limite de análise; seguir todas as cópias do
endereço mudaria esse contrato. A aceitação de `NULL` permite receber o resultado
de uma alocação e verificar sua falha antes de acessar o elemento.

`constexpr` separa uma constante nomeada de um objeto constante com endereço.
Essa separação fornece um contrato comum aos perfis C11 e C23. Quando o programa
precisa de endereço, a declaração C `static const` já expressa a necessidade.
Quando keel precisa do número para substituir tokens, ler um literal conhecido
preserva esse limite sem introduzir avaliação de expressões C.

Referência: [spec §4.2](keel-spec.md#42-tipos-declarações-e-marcadores).

## Resolução por declaração

A assinatura escrita fornece a aridade e a forma de valor ou ponteiro; a
declaração do argumento fornece sua identidade conhecida. A adaptação usa essas
informações explícitas. Examinar o tipo resultante de uma expressão C arbitrária
exigiria outro modelo de análise.

É essa adaptação que obriga o objeto a ser escrito sem `&`, e o par
`buffer`/`slice` é o que decide. `buffer.length(&b)` seria inofensivo — o
parâmetro é ponteiro, e a emissão poria o mesmo `&`; `slice.length(&s)` seria
erro de tipo, porque ali o parâmetro é valor. Mesma grafia, resultado oposto, e
o que os separa — um modificador ser `byref` e o outro não — não está no ponto
de chamada. Absorver o `&` custaria zero no caso bom e entregaria, no ruim, uma
mensagem do compilador C sobre um tipo que o programa não escreveu. Recusá-lo
deixa uma regra só: o objeto é escrito como objeto, e quem decide sobre o
endereço é a declaração do módulo. É o mesmo que faz `buffer.length(b)` e
`slice.length(s)` se lerem igual apesar de um ser `byref`.

O reflexo contrário é legítimo, e por isso o diagnóstico paga: `byref` obriga o
`&` na chamada comum, então `f(&b)` e `buffer.length(b)` convivem em linhas
vizinhas. O `.` é o que as separa — à esquerda dele há um módulo, e o primeiro
argumento é o objeto, não um argumento C.

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
de qualquer uma delas sem que keel saiba que ele existe, e é o que impede que
a base receba tratamento que a spec não escreveu.


Um verbo que o genérico declara e a instância escrita não admite (`void` ou
`const`, spec §4.3) é recusado com nome, e não deixado para o compilador C. A
tradução já conhece a superfície da instância para emiti-la; recusar com o nome
do argumento que removeu o verbo não custa verificação alguma, e troca um erro
de declaração implícita por uma mensagem que diz o que de fato aconteceu.

Referência: [spec §4.4](keel-spec.md#44-resolução-de-operações).

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

A forma contável de `foreach` não impõe protocolo rígido: `first` e `limit`
bastam, porque o binder recebe o próprio contador e nenhum verbo de acesso por
posição é despachado.

**Recorte: erro em debug, saturação em release.** Um `range-index` fora de
`a <= b <= length(x)` quase sempre é erro de índice, e o debug o acusa. Em
release o resultado é definido — o fim é recortado no comprimento, e início
além do fim dá trecho vazio —, para que o programa que erra não leia memória
alheia. A saturação não é o contrato: é a rede de segurança. Tratá-la como
contrato, à maneira das fatias do Python, esconderia o bug de quem pede quatro
elementos perto do fim e recebe menos, e deixaria `x[a..b]` com um regime
diferente do de `x[i]`. Quem quer a forma total escreve o verbo, como faz com
`at`.

Referência: [spec §5.3–4.9](keel-spec.md#53-keelbuffer-keelslice-e-keelrange).

## Cursor explícito

Nem toda sequência responde por índice. Uma lista encadeada, um mapa, um fluxo
lido por demanda e uma fatia cujo comprimento só se conhece ao percorrer não
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
conversão nem seleção por sobrecarga, pela mesma razão que vale no resto de
keel — escolher entre formas pelo tipo do que foi escrito é análise que keel não
faz.

Na base, `buffer` e `slice` declaram `next` devolvendo o endereço do elemento,
e não uma cópia: o binder por ponteiro serve à escrita e nunca perde um
descritor por cópia. O cursor deles é uma posição em `size_t`, declarada uma vez
por módulo e não por instância, porque não depende do tipo do elemento — por
isso se escreve `buffer.cursor`, sem argumento.

A terminação não é provada. Um `next` que não avance produz laço infinito,
exatamente como o `while` equivalente escrito à mão; a construção não promete
mais do que a forma que substitui.

A forma de um binder em `walk` é reconhecida só para ser recusada, por
`walk-without-cursor`: a mensagem diz o que falta, em vez de um erro de sintaxe
sobre a vírgula.

Referência: [spec §4.7](keel-spec.md#47-travessia-sequencial).

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
`interrupted`, não dispara limpeza. Não há mais nenhuma região com
regras de salto próprias: com o fim da finalização das composições, as
restrições de entrada e saída voltam a ser as do cleanup léxico.

Referência: [spec §4.6](keel-spec.md#46-cleanup-léxico).

## Macros e sintaxe de keel

keel organiza, por uma sintaxe própria, operações que seriam expressas com
macros em C. A intenção é permitir escrever essas operações como construções
legíveis, com reconhecimento e expansão definidos, sem depender de colagem de
tokens, avaliação duplicada de argumentos ou outras técnicas de macro.

Chamar essa sintaxe de linguagem de uso não atribui a keel um sistema de tipos
independente. Os modificadores descrevem instanciações e transformações que
produzem declarações C. A tabela de símbolos guarda informações explícitas
necessárias à expansão; compatibilidade e validação semântica de tipos continuam
com o compilador C. Por isso a spec fala em tipos e declarações reconhecidos
por keel, em vez de uma categoria de “tipos keel”.

Referência: [spec §1](keel-spec.md#1-escopo-e-princípios).

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

Os produtores de `corot` seguem todos a mesma forma, e é o que torna as
duas famílias legíveis pela mesma regra: verbo qualificado, receptor no
primeiro argumento, saída escrita pelo programa.

Essa separação também define o default: `else v` consulta o resultado declarado
e chama `win(resultado,v)` quando ele falha. O setter `value(resultado,v)`
escreve somente o valor, e não serviria sozinho para estabelecer sucesso.

Referências: [spec §5.5](keel-spec.md#55-keeloutcome-e-keelcorot) e
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
do tipo com que se combina. Em `buffer struct Person people;`, `buffer`
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
resultado; o mecanismo de keel é aplicar o modificador ao tipo fornecido.

“Genérico” descreve aqui o mecanismo de substituição que permite escrever as
declarações uma vez. O C resultante pode ser obtido com técnicas como X-Macros
ou a inclusão repetida de um header sob uma definição temporária de `T`:

```c
#define T int
#include <buffer.h>
#undef T
```

O fragmento ilustra a técnica de expansão, não um header da base keel. Um header
construído para esse uso materializaria as declarações para `int`. keel fornece
sintaxe e regras de nomes para realizar essa geração diretamente, mantendo o
compilador C como verificador do resultado.

O nome `struct Person` é um argumento de tipo completo para essa substituição.
Reconhecer a palavra `struct` seguida do nome basta para preservar essa forma
na expansão; não exige analisar semanticamente seus campos. A mesma forma de
referência por tag existe para `union` e `enum`.

Referências: [spec §1](keel-spec.md#1-escopo-e-princípios),
[módulos e modificadores](keel-spec.md#43-módulos-genéricos) e
[resultados](keel-spec.md#55-keeloutcome-e-keelcorot).

## Por que a base é copyleft com exceção, e não GPL simples nem MIT

`cgen` não vincula a Base ao programa do usuário — ele **copia texto** dela para dentro do
`.h`/`.c` de todo projeto que passa pela ferramenta. É uma diferença que muda a análise de
licença por completo: "vincular biblioteca" é o caso clássico que LGPL resolve; "colar
fragmento de fonte alheio dentro do seu" não é.

Sob GPL simples, sem adendo, a leitura literal da §2 arrisca tornar **todo programa
compilado com keel** um "trabalho baseado" na Base, só pela transpilação — nenhum usuário
precisaria ter tocado a Base para o copyleft alcançá-lo. Isso mataria a adoção: ninguém
escreve `.k` de produção sabendo que o binário resultante pode ser cobrado a abrir o
próprio fonte por causa de uma struct de `buffer` que ele nunca editou.

Sob MIT, o problema é o oposto e o `Makefile` do repositório previa: qualquer um pode pegar
`cgen` e a Base, melhorá-los, e distribuir a versão fechada sem devolver nada — nem código,
nem crédito. Para um projeto acadêmico, dentro de uma universidade, isso é o pior dos dois
mundos: os autores perdem a visibilidade do próprio trabalho, e a comunidade perde o acesso
às melhorias.

**A saída não é nova — é a mesma que a FSF já usou para o problema gêmeo.** `libgcc` é GPL e
é embutida (inline, estática) em todo binário que o `gcc` produz; a saída não vira GPL por
causa disso porque a *GCC Runtime Library Exception* diz isso explicitamente. keel está na
mesma posição de `libgcc` — na verdade mais literal ainda, porque aqui não há nem "link", é
transpilação de texto puro.

A exceção que `LICENSE.md` adota é o mesmo desenho, com uma simplificação que a arquitetura
do keel permite: ela cobre **só uma coisa**. Modificar e redistribuir `cgen` já é coberto
pela GPL comum, sem exceção nenhuma — rodar um binário GPL sobre o próprio fonte nunca
implicou o fonte, é assim que compilar código proprietário com `gcc` sempre funcionou, e
isso vale tanto para o `cgen` de referência quanto para um `cgen` modificado que reconheça
construção nova da linguagem. Modificar e redistribuir a Base em si também já é GPL comum,
sem exceção — quem pega `base/keel/buffer.k`, melhora e redistribui deve o fonte de
volta, exatamente a proteção contra o cenário MIT do parágrafo acima. A exceção existe só
para a terceira situação, que é a única onde GPL comum erraria: o texto que a Base
contribui, por transpilação, para o *output* do `cgen` sobre o fonte do próprio usuário. Só
aí o usuário fica livre para licenciar como quiser.

Referências: [`LICENSE.md`](LICENSE.md), [`LICENSE.pt.md`](LICENSE.pt.md) (tradução
informal), [backend §4.2](keel-c-backend.md#42-o-prelúdio-keelk).

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

E é uma pergunta que o **cgen** faz o tempo todo. As regras 2 e 3 do backend
§4.3.2 mandam cada arquivo com corpo incluir o `.h` de quem ele chama; o gerador
descobre quem é chamado resolvendo o nome manglado, e o que ele tem em mãos,
naquele ponto, é exatamente um símbolo. Com a regra uniforme, o include é
concatenação; sem ela, é consulta a uma tabela de origem que precisaria existir
só para isso.

**A assimetria com o fonte é deliberada.** O `.k` continua morando no caminho do
módulo — `module app.cfg;` em `app/cfg.k` —, e o backend não o segue. A razão é
que os dois arquivos têm quantidades diferentes de informação: o `.k` **declara o
próprio nome na primeira linha**, então o caminho é redundante e pode servir de
conferência barata, que é o que `module-path-mismatch` faz. O header gerado não
declara nada sobre si; o nome do arquivo é o único identificador que ele tem, e
gastá-lo com meia informação seria desperdício.

Instância nunca teve escolha: `buffer i32` não é módulo e não tem caminho de
módulo para herdar, então o nome dela sempre foi o símbolo. A decisão aqui não
foi inventar uma convenção — foi **parar de manter duas**, estendendo ao módulo
a que a instância já obrigava.

## Dois headers, tipo e uso

Um módulo C comum é um `.h` e um `.c`. keel gera um arquivo a mais — o
`.type.h` — e a razão é uma só: **instância cria dependência no sentido
contrário do import.** `keel.buffer` importa `keel.outcome` para que `clone`
devolva `outcome buffer T`; e o layout de `outcome buffer T` contém o de
`buffer T` por valor, que é o caminho de volta. O caso de borda
`outcome buffer outcome i32` mostra os dois sentidos no mesmo par de
instâncias. Com um header só por instância (nomes abreviados):

```c
/* buf.h — buffer outcome i32 */
#ifndef BUF_H
#define BUF_H
typedef struct outcome_i32 { i32 code; i32 value; } outcome_i32;
typedef struct buf { size_t cap, len; outcome_i32 *ptr; } buf;
#include "obuf.h"                                  /* clone devolve obuf */
static inline obuf buf_clone(buf *b) { obuf r = {0}; r.value = *b; return r; }
#endif

/* obuf.h — outcome buffer outcome i32 */
#ifndef OBUF_H
#define OBUF_H
#include "buf.h"                                   /* the value field is by value */
typedef struct obuf { i32 code; buf value; } obuf;
static inline obuf *obuf_win(obuf *r, buf v) { r->code = 0; r->value = v; return r; }
#endif
```

Entrando por `buf.h`, compila. Entrando por `obuf.h`, a guarda pula o
`obuf.h` de volta, e o corpo de `buf_clone` chega antes de `obuf` existir:

```plain
In file included from obuf.h:3,
                 from b.c:1:
buf.h:8:15: error: unknown type name 'obuf'
```

Ordenar arquivos não é saída: o backend não vê o grafo do projeto, e a ordem
de entrada é de quem inclui. A saída é notar que as duas arestas são de
**espécies diferentes** — `obuf → buf` é de layout (o campo `value`), e
`buf → obuf` é de chamada e de assinatura (`clone`) — e separar as espécies em
arquivos (guardas omitidas):

```c
/* buf.type.h — layout only; the element is a pointer, the name is enough */
typedef struct outcome_i32 outcome_i32;
typedef struct buf { size_t cap, len; outcome_i32 *ptr; } buf;

/* obuf.type.h — layout only; value is by value, includes the .type.h, not the .h */
#include "buf.type.h"
typedef struct obuf { i32 code; buf value; } obuf;

/* obuf.h */
#include "obuf.type.h"
static inline obuf *obuf_win(obuf *r, buf v);
static inline obuf *obuf_win(obuf *r, buf v) { r->code = 0; r->value = v; return r; }

/* buf.h */
#include "buf.type.h"
#include "outcome_i32.type.h"                      /* get devolve outcome_i32 */
#include "obuf.type.h"                             /* clone devolve obuf      */
static inline obuf buf_clone(buf *b);              /* prototypes */
#include "obuf.h"                                  /* clone chama obuf_win    */
static inline obuf buf_clone(buf *b) {             /* corpos */
    obuf r = {0}; return *obuf_win(&r, *b);
}
```

A aresta de layout agora corre só entre `.type.h`, e `obuf.type.h` não volta
a `buf` por nenhum caminho de `.h`. A de chamada corre só entre `.h`, e
`obuf.h` não inclui `buf.h`. Os dois grafos são acíclicos cada um por conta
própria, e as duas ordens de entrada compilam — verificado com
`gcc -std=c11 -Wall -Wextra -pedantic`, entrando por `buf.h` e por `obuf.h`.

**Por que o corte cai entre L1 e o resto.** Layout é a única dependência que
ordem nenhuma dentro de um arquivo resolve: uma `struct` precisa do agregado que
contém por valor **completo, antes dela**, e arquivos que se incluem em ciclo não
garantem isso. Protótipo e corpo toleram ciclo: um `static inline` só precisa
estar declarado antes da chamada, e protótipo repetido é C legal. Então basta
uma fronteira de arquivo — a de layout — e o resto se resolve por ordem de
seção.

**Por que não há `.proto.h`.** A revisão anterior separava também as
assinaturas, num terceiro header, para que um arquivo com corpo pudesse pegar
os protótipos de outro sem os corpos. Mas a ordem das seções do `.h` — os
próprios protótipos antes de incluir o `.h` de quem os corpos chamam — já
entrega o mesmo: qualquer que seja a porta de entrada, todo protótipo alcançável
chega antes do primeiro corpo. O terceiro arquivo não comprava nada que a ordem
não comprasse, e custava um idioma que nenhum programador C escreve.

**Por que não separar no fonte.** A alternativa era deixar o gerador com um
header por módulo e empurrar a separação para a arquitetura da base — um
`keel/basetypes.k` com os modificadores, e os verbos nos módulos de sempre, como
o programador C faz com um `types.h` à mão. Resolve a base, mas custa à
linguagem: o modificador deixaria de morar no módulo dos seus verbos, e a
resolução por tipo (linguagem §4.4), a instanciação e os protocolos todos
supõem que mora. E o usuário, com o mesmo problema no próprio genérico, teria de
redescobrir a mesma arquitetura. O `.type.h` é esse `types.h`, escrito sempre
pelo backend, para todo módulo — e o programador nunca precisa.

**Por que ainda é C de sempre.** O par tipo/uso é o que o C já tem para
contêiner gerado — `KHASH_DECLARE` e `KHASH_INIT` no klib —, e o que o
programador C escreve quando dois headers precisam dos tipos um do outro. O
include de uso continua um só, `modulo.h`, e o `.type.h` existe para os
gerados; quem escreve o `.c` à mão não precisa saber que ele existe.

**Por que `constexpr` de módulo é tipo.** Ele pode dimensionar um campo —
`i32 itens[MAX];` — e ser argumento de `dim`. Se morasse junto com os
protótipos, o `.type.h` precisaria alcançar o `.h`, e o corte deixaria de
valer.

Referência: [backend §4.3](keel-c-backend.md#43-headers-de-instância).

## Comentários e quebras de linha

Um comentário do `.k` fala de keel: `/* o índice é avaliado uma vez */` descreve
`b[(*i)++]`, não a chamada de instância que o C recebe. Levá-lo ao gerado
arrisca enganar mais do que esclarecer, e a spec já diz que o conteúdo dele é
ignorado. O cgen faz o que a fase 3 da tradução C faz — comentário vira espaço
—, preservando as quebras.

As quebras são o que fica, porque são elas que mantêm o mapeamento de linhas
barato. `#line` é pegajoso: com todas as linhas do fonte presentes no `.c`,
inclusive as vazias que sobram do que foi para os headers, um `#line` no topo
cobre o arquivo, e só as expansões de várias linhas pedem outro. Linhas vazias
a mais não tiram o C do padrão — quem escreve C já as põe de propósito. Se um
dia incomodarem, cada sequência delas vira um `#line`, e nada mais muda.

`extern_c` é a exceção, e pela mesma razão ao contrário: o que está lá dentro é
C, e o comentário, também.

Referência: [backend §6](keel-c-backend.md#6-mapeamento-de-linhas).

## Arena é um tipo

`arena` já nomeia uma estrutura C que controla uma região de memória por um
topo de alocação. Não há um tipo `T` a modificar para declarar uma arena:
`typedef struct { ... } arena;` fornece o tipo, e `arena a;` declara um objeto
desse tipo. A declaração conserva essa forma no C emitido, com o nome
`keel_arena` e o inicializador descrito adiante.

A passagem do descritor por referência pertence ao contrato das operações;
ela não caracteriza um modificador. Da mesma forma, o `T` em
`arena.alloc(a, T, n)` é um parâmetro `type` apagado (ver [parâmetro de tipo em
função](#parâmetro-de-tipo-em-função)): indica o tipo dos objetos a alocar,
para obter tamanho e alinhamento, sem alterar o tipo de `a`.

A única coisa que keel acrescenta à declaração é o inicializador: toda
definição de `arena` sem inicializador escrito recebe `= {0}`. Em arquivo, o C
já zera o objeto; em bloco, não, e o pior uso possível — `arena.alloc` sobre um
descritor com lixo — escreveria num endereço qualquer. Com o descritor vazio, a
mesma chamada falha limpo e devolve `NULL`. A arena é o centro da memória em
keel, e três campos zerados custam pouco perto do erro que eliminam. O
inicializador não muda o que a declaração diz — continua sendo uma arena sem
região —, só tira o estado indeterminado. Um inicializador escrito é
preservado, e dar região à arena continua sendo trabalho dos construtores.

Referências: [spec: memória por região](keel-spec.md#52-keelarena)
e [backend: arena](keel-c-backend.md#54-arena).

## O que saiu, e o que entrou no lugar

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
| `soa struct`, com a reescrita `var[i].campo` e a marca no ponto de uso | `extent struct`, com grupos `[contagem, capacidade]` e acesso verificado | `extent`: da recusa à admissão |
| O tipo escrito na chamada como privilégio de `arena.alloc` e `slice.from` | O parâmetro `type` de função, apagado ou de seleção | Parâmetro de tipo em função |

Os diagnósticos acompanharam as construções. Os que observavam a máquina de
estados **mudaram de contrato** e hoje observam o conjunto de tags e o
despacho; os que dependiam da região de finalização, dos statements de retorno
cooperativo ou da política escrita entre colchetes **deixaram de existir**,
porque a condição que observavam deixou de ser expressável.

Na direção oposta, os diagnósticos novos são de protocolo, e não de forma
escrita: `not-partitionable`, `not-cursor-iterable`,
`partition-type-mismatch`, `cursor-type-mismatch`, `match-without-tag` e
`match-without-tags` recusam um tipo que não declara a operação exigida.

Referência: [spec §6.2](keel-spec.md#62-catálogo).
