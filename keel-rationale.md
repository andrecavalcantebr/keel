# keel — Razões

Companheiro normativo de `keel-spec.md`. **Nada aqui é regra.** A regra está lá,
na seção de mesmo número; aqui está o por que ela é assim: o que foi descartado, o
caso que a produziu, e — onde cabe — o que o compilador C deixaria passar se a
verificação não existisse.

Convenções:

- Cada seção espelha a numeração da spec. Uma seção só existe aqui se houver
  razão registrada.
- **`§N` sem qualificação é seção da spec**, e não deste documento — é o que
  permite citar `§3.6` ou `§7.4`, que aqui não existem. Referência a outro
  documento leva o nome na frente: `backend §5.5.2`, `ferramenta §5`.
- Blocos marcados `/* o que sairia, se a checagem não existisse */` mostram o C
  que a ausência da regra produziria. São o que separa verificação necessária de
  redundância com o `cc1`.

**Estado:** acompanha `keel-spec.md` em documento único. A numeração espelha a
dele, e uma seção só existe aqui se houver razão registrada — daí os saltos.

---

## 1.1 O C é o alvo, não a fonte

### C usado como o assembly portável

Existe uma camada de software que todo o resto vai por cima: quase todo sistema
operacional, driver, runtime ou interpretador; uma linguagem nova, quando precisa
falar com outra linguagem, fala através dela; um alvo de hardware ganha um
compilador para ela antes de ganhar qualquer outra coisa. Essa camada é o C.

O C ocupa hoje o lugar que o assembly ocupou até os anos 70, mas de uma forma
ainda mais premente: é o denominador comum da máquina, mas de qualquer máquina —
ele é **portável**. É possível que um `.c` corra num microcontrolador de oito
bits e no cluster de GPU, com pouca ou nenhuma alteração.

Mas, da mesma forma que aconteceu com assembly, ninguém mais o escreve à mão:
escreve-se numa notação acima — as linguagens de alto nível —, e o assembly sai
do outro lado. Ele continua sendo lido, em depuração, em otimização, em
auditoria, mas não é onde o texto nasce. Com o C isso não aconteceu; continuamos
escrevendo à mão a camada de baixo.

Para evitar isso, as duas saídas que apareceram foram:

- **Sair do C.** C++, Rust, Zig, Go. Cada uma resolve problemas reais, e cada uma
  troca o denominador comum por um seu — outro compilador, outro runtime, outra
  ABI, outro conjunto de alvos suportados.
- **Ficar no C e improvisar.** Macro com colagem de token, X-macro, `void *` com
  `sizeof` no ponto de chamada. Funciona, compila hoje, e some do diagnóstico: o
  depurador mostra `void *`, o erro do compilador aparece numa linha expandida que
  ninguém escreveu, e duas bibliotecas nunca concordam sobre o que é um vetor de
  `Point`.

keel é a terceira, e dela saem três fatos — e é deles que os princípios saem:

> **1. O gerado é código que um humano assinaria.** Se você não consegue prever o C que uma construção produz, a construção falhou. É por isso que a
> especificação é escrita em pares keel/C.

> **2. keel não precisa entender C.** Um compilador não entende as macros do
> assembler: ele emite dentro delas. keel faz o mesmo — reconhece as próprias
> construções e copia o resto sem examinar.

> **3. O compilador C é o verificador final.** keel não reimplementa o sistema de
> tipos do C. Erro de tipo aparece no compilador C, mas com os nomes que você
> escreveu no fonte keel.

Os três reaparecem como regra na spec, e nenhum como texto repetido: o primeiro é
o princípio 2 — mais a convenção de redação em pares, que é do §1.1; o segundo é
o princípio 7 e a invariante do §1.3; o terceiro é o princípio 3.

### Quem escreve C pensa em máquina; quem escreve keel pensa em C

O programador C mantém na cabeça, o tempo todo, uma imagem aproximada das
instruções que sua linha vira. Não é a forma exata — não precisa ser —, mas é
fiel o bastante para decidir.

keel ocupa a mesma posição um andar acima. A imagem que ele pede que você
mantenha é a do **C gerado**, mais abstrata justamente porque C é linguagem de
alto nível. **Uma construção cujo C você não consiga prever é uma construção que
falhou** — e é essa a régua que decide o que entra na linguagem. É por isso que
esta especificação é escrita em pares.

## 1.2 O que keel acrescenta

### A motivação, em uma frase

> **keel é o que já se faz com truque de macro, escrito como linguagem.**

Todo programa C sério tem um contêiner genérico, e há três formas de
escrevê-lo: `#define T int` com colagem de token, X-macros, ou `void *` com
`sizeof` no ponto de chamada. As três funcionam, as três compilam, e as três
cobram a mesma coisa — o tipo some do diagnóstico, o depurador mostra `void *`,
o erro do compilador aparece expandido numa linha que ninguém escreveu, e duas
bibliotecas nunca concordam sobre o que é um vetor de `Point`.

keel gera o que a macro geraria — struct nomeada, funções inline, sem indireção
e sem overhead — com o nome do tipo intacto do fonte até a mensagem do
compilador C. Nenhuma das três formas deixa de compilar por causa de keel; o que
ele oferece é não precisar mais delas.

E o mesmo vale para o resto do vocabulário: cada construção de keel responde a
uma coisa que **o C não consegue expressar**, ou expressa mal. Unidade, limites
que viajam com o dado, memória por região, saída de escopo com limpeza, ponteiro
que diz um-ou-muitos. É esse o teste de admissão, e ele é mais estreito do que
"serve à orientação a dados": inverter layout, por exemplo, o C expressa
perfeitamente — `struct { f32 *x; f32 *y; }` —, e por isso `soa` não está aqui
(§1.3).

### O que o VLA errou

O VLA é o exemplo canônico da confusão que keel desfaz, e vale enunciar cedo
porque ele decide o desenho da biblioteca inteira.

> **O VLA solda duas decisões numa sintaxe só: *quanto*, e *onde e por quanto
> tempo*.**

É a solda que o inutiliza. Não dá para retornar, não dá para exceder o frame,
não dá para saber se falhou, e metade dos alvos não o implementa. keel separa as
duas decisões e dá um nome a cada uma: *quanto* é a capacidade do `buffer`, ou o
`n` de `arena.from_parent`; *onde e por quanto tempo* é a arena — ou o `array`,
ou a região crua.

Por isso `buffer` não é o VLA de verdade: ele é um descritor, retorna por valor,
e a vida do dado é a da arena, não dele. E a `arena` também não é:
`arena.from_stack` tem a forma de vida do VLA mas exige tamanho constante, e
`arena.from_parent` tem o tamanho de runtime mas o armazenamento não volta
sozinho. O que chega perto é a composição das duas, com a disciplina de pilha
**explícita** em vez de implícita — e escrito assim funciona onde VLA não
existe, o modo de falha é testável, e quem lê sabe de onde saiu a memória.

## 1.3 A invariante

### Por que não pragmas

A pergunta é legítima e volta sempre: o OpenMP acrescenta paralelismo ao C sem
tocar na linguagem, com `#pragma`. Por que keel não faz o mesmo?

Porque "usar pragma" são duas coisas, e só uma delas é a do OpenMP.

**Como veículo de sintaxe** — `#pragma keel buffer i32 xs` —, o pragma é só uma
forma de passar tokens não-C por um parser de C. Isso não é a abordagem do
OpenMP: é keel com sintaxe pior, e perdendo — diretiva é orientada a linha, não
aninha em expressão, e o diagnóstico passa a morar dentro de uma região que o
`cc1` trata como opaca.

**Como anotação sobre C que já é válido** — que é a abordagem do OpenMP — o
pragma tem a propriedade que explica a adoção dele: **ignorabilidade**. Sem
`-fopenmp` o programa compila e roda, serial e correto. Vale ouro, e é por isso
que a pergunta merece resposta em vez de descarte.

E a resposta é um teste:

> **Pragma decora; pragma não declara.** Se remover a diretiva torna o código
> inválido ou muda o sentido, não é anotação — é linguagem com sintaxe de
> diretiva.

#### O vocabulário rodado pelo teste

| Construção | Vira pragma? | Por quê |
| --- | --- | --- |
| `parallel` | **sim** | um `for` que já existe e já está correto |
| `constexpr` | **sim** | já é palavra do C23; keel só registra o nome |
| `array` (registro) | **sim** | marcador puro, some no lowering |
| `ref` | parcial | o qualificador some, mas a proibição pedem o parse |
| `v[i,j,k]` | não | `[1,2,5]` é o operador vírgula do C; a reescrita muda sentido |
| `buffer`, `slice`, `corot` | não | não há C válido para anotar — o tipo não existe até keel gerá-lo |
| `module`, `pub`, `priv` | não | não há statement; o que se anotaria é a existência de um `.h` |
| `defer` | não | ignorar a diretiva executa o cleanup no ponto de registro |
| `cofsm`, `coseq`, `copar` | não | o despacho por rótulo não existe em C; sem tradução o corpo não é alcançável |
| `x[a..b]`, `a..b` | não | `2..7` nem *lexa* como C (§3.6) |
| módulos genéricos | não | `modifier` declara um agregado que não existe sem substituição |

**`defer` é o caso que fecha a questão.** Ignorada a diretiva, o `fclose` roda no
ponto de registro — não é degradação, é bug. Perdeu-se a ignorabilidade, que era
o único prêmio, e manteve-se a sintaxe. E `defer` não é periférico: é uma das
três coisas que o §1.2 diz que o C não oferece.

#### O que sustenta o OpenMP, e que keel não tem

> **OpenMP funciona porque está *dentro* do compilador.** `map(tofrom: a[0:n])`
> precisa de `sizeof(*a)`. `declare reduction` precisa de resolução de tipo.
> `parallel for` sobre iterador precisa saber o que o iterador é.

keel roda antes do `cpp` e, pela invariante do §1.3, nunca sabe o que um nome do
C significa. Um keel-por-pragma nesse estágio fica com o pior dos dois lados:
sintaxe de diretiva **sem** o conhecimento de tipos que a justifica. E um
keel-por-pragma dentro do compilador deixa de ser driver portátil — vira plugin
de um compilador, e acaba o `CC=gcc` → `CC=cgen`.

**E há o argumento por evidência: o OpenMP não se manteve em anotação.** `a[0:n]`
não é sintaxe do C; é *array section*, inventada pela especificação.
`reduction(+:x)`, `omp_in`/`omp_out`, `depend(iterator(…))`, `declare mapper`. São
seiscentas páginas. Ele **cresceu uma linguagem dentro da diretiva**, porque
anotação não bastou. O pragma não poupa ninguém de desenhar uma gramática; ele
esconde a gramática num lugar onde o diagnóstico do compilador C não alcança.

#### O que keel fica devendo, e como paga

Duas coisas se perdem de verdade, e vale dizer quais.

**Ferramental.** `clang-format`, `clangd`, `ctags`, analisadores, o realce do
editor — tudo isso vem de graça num `.c` e não vem num `.k`. Não há contorno
barato; é custo pago.

**Ignorabilidade** — e essa keel paga por outro caminho. O OpenMP degrada para
serial; keel "degrada" para o `.c` e o `.h` gerados, que são C liso, versionáveis
e compiláveis por qualquer um sem a ferramenta. Não é uma versão pior do
programa: é o programa. A ausência de diretiva também apaga a §3.10 inteira e o
error 44 — o preço de disputar espaço léxico com o C —, e esses são custos que o
pragma não teria. Estão pagos conscientemente.

#### Onde keel concorda com o OpenMP

O critério não foi traçado contra o pragma; foi traçado pelo teste, e a maior
parte do que passa no teste keel **já faz assim**. `constexpr`, `array`, `ref`,
`import_c` e `extern_c` anotam e somem no lowering: é a filosofia do
pragma, aplicada onde ela cabe.

E `parallel` — a única construção que passa limpa — **baixa para um pragma**
(§4.7). keel não recusa a abordagem: ele a consome, no único lugar em que ela
é a ferramenta certa. É a mesma leitura do §1.3 que separa núcleo de biblioteca,
com outra pergunta:

> **Núcleo é o que nenhum módulo keel poderia escrever. Pragma é o que nenhuma
> declaração precisa nascer.**

### O que o limite deixa de fora

A spec enuncia o corolário em três linhas: keel não tipa expressão, e a posição
de contêiner é o máximo que a invariante permite. Aqui está a conta — o que fica
de fora, e o que cada item exigiria:

| Recurso | O que ele exigiria |
| --- | --- |
| Operador de propagação de erro | achar o operador dentro de expressão arbitrária |
| Tupla anônima e desestruturação de retorno | o tipo de uma **chamada** |
| Função genérica livre | deduzir o parâmetro de tipo do argumento no ponto de chamada |
| Corrotina com retomada de posição | entender o corpo, para hoistar locais através da suspensão |
| Lambdas e closures | idem, mais captura implícita |
| Fatiamento multidimensional | despacho pela **forma** dos argumentos, não pela contagem |
| Inversão de layout — `soa` | conhecer o **tipo dos campos**, para saber o que é invertível |

Sete recusas, um motivo só. Isso é o que garante que **keel não é uma versão
incompleta de alguma coisa**: as sete não foram cortadas por escopo, por prazo
ou por dificuldade — cada uma bate na mesma parede, e a parede é onde a camada
termina. Uma linguagem cujas recusas têm sete razões diferentes está inacabada;
uma cujas recusas têm uma razão só está terminada.

E a tabela é, lida do outro lado, o **roteiro do `cdod`**: é a lista de recursos
que a camada de cima existe para entregar, e a razão pela qual ela precisa tipar
expressão para entregá-los.

A cláusula `else` (§4.7) **não sai da primeira linha da tabela**, e é preciso
dizer por quê. Ela não é uma versão parcial do operador de propagação: é outra
construção, que resolve os dois casos dolorosos — testar e sair, ou testar e
repor um default, na mesma linha — sem pedir nada do que a camada não tem. Ela
faz o teste, que vem de um tipo escrito numa declaração; não faz a extração nem a
conversão, que viriam do tipo de uma **chamada**. O operador continua recusado,
pela razão de sempre e por mais duas que só aparecem quando se tenta desenhá-lo
(§4.7). Sete recusas, um motivo só — ainda.

## 1.4 Modelo de compilação


### Por que C23, e o que exatamente prende o alvo ali

> **Resolvido.** A lista de bloqueios foi a zero, e a saída não foi rebaixar a
> linguagem: foi separar obrigação de forma em dois pontos, e escrever um tipo
> que antes se deduzia. C11 é hoje um **perfil de geração** (`backend §9`), não
> um alvo alternativo. O registro abaixo fica porque o argumento continua
> valendo e porque explica por que o alvo *nominal* segue sendo C23.

A tentação de mirar mais baixo é real e tem nome: **embarcado**. Boa parte das
toolchains que interessam a keel fica entre C89 e C99, às vezes com um subconjunto
de C++ antigo ao lado. Um alvo C11 ampliaria o alcance de imediato. A pergunta é
quanto custa, e a resposta é mais estreita do que parece.

Rebaixando a lista item a item, quase tudo cai sem perda. `bool` é
`<stdbool.h>`; `alignof`, `alignas` e `static_assert` estão em `<stdalign.h>` e
`<assert.h>` desde o C11; `[[nodiscard]]` é auxílio de diagnóstico e sai sem
consequência semântica. E o `auto` do temporário de retorno do `defer` (§4.7)
deixou de ser necessário quando a gramática ganhou produção de função: o tipo de
retorno passa a estar capturado como sequência de token, e copiá-la sem entendê-la
é o que keel faz o tempo todo — o que o `auto` do C23 acrescenta ali é a
conversão de lvalue, e um temporário inicializado uma vez não precisa dela.

Sobravam **duas** construções, e as duas caíram — cada uma por um motivo que vale
guardar, porque nenhum dos dois foi "achar um truque de C11":

- **`constexpr`** (§4.2). As alternativas em C11 continuam sendo três, e duas não
  servem: `const int A = 10;` não é expressão constante em C, então `array u8
  memo[MAX]` viraria VLA — exatamente o que keel existe para não fazer; e `enum {
  A = 10 };` só serve a inteiro que caiba em `int`, perdendo `constexpr f32` e a
  semântica de `size_t`. Sobra o `#define`, e **o argumento contra ele estava
  errado**: "keel gerando macro" só é contradição se a macro for do tipo que keel
  dispensa, que é a que **constrói estrutura** — colagem de token, X-Macro,
  `TRY`/`CATCH`. Uma constante nomeada não constrói estrutura, não some do
  depurador e é o que um C99 bem escrito faz. O que o `#define` de fato custava
  era outra coisa, e foi ela que precisou de desenho (§4.2).
- **`typeof_unqual`**, na captura do `defer [now]`. Aqui não havia gramática a
  invocar: o identificador capturado vem de declaração C que keel não examina.
  A saída foi **parar de deduzir** — a entrada de captura passou a levar o tipo
  escrito, como `slice.from(char, p, n)` e como a cláusula `else` já faziam
  (§4.7). A dedução era a exceção na linguagem, não a regra.

Nenhuma das duas foi resolvida rebaixando o C. Foram resolvidas **movendo a
fronteira entre linguagem e backend** para o lugar certo: o §4.2 nomeava um
lowering quando devia nomear uma obrigação, e o §4.7 deduzia um tipo quando o
resto da linguagem manda escrevê-lo. Nos dois casos o alvo C11 só expôs um
problema que já estava lá.

**E o C++ deixou de ser necessidade para virar opção.** As duas construções
existem em C++ desde o C++11 — `constexpr` é dele, e `decltype` cobria o
`typeof_unqual` —, e enquanto elas bloqueavam, um backend C++11 era a única saída
para toolchain travada em C99 com compilador C++ ao lado. Com o perfil C11 essa
saída deixou de ser obrigatória; ela continua fazendo sentido para quem só tem
compilador C++, e os obstáculos dela continuam valendo:

- **Literal composto** — `(geom_Point){0.0f, 0.0f}` não é C++ padrão, e keel o
  emite em construção normal (§2.1). É o que mais dói.
- **`restrict`** não existe em C++ padrão, e keel o repassa verbatim em declarador
  C comum (§4.2) — é a forma recomendada para kernel numérico, e ela não
  atravessaria.
- **Ligação** — o gerado precisaria de `extern "C"` para continuar sendo o
  denominador comum que o §1.1 descreve; sem isso o C++ deixa de ser alvo e passa
  a ser destino.
- **Membro final flexível**, se algum backend o usar.

Note quais **não** estão na lista, e costumam ser os primeiros lembrados:
parâmetro `void` é C++ válido, e o cast de `void *` já sai explícito no gerado
por outra razão — `arena.alloc` emite `(T *)` para carregar o tipo (§4.4).
Os dois já estão pagos.

E há uma terceira forma, que não é escolher um alvo: **perfis de geração**. O
gancho já existe — `backend §1` diz que um segundo backend deve as mesmas
obrigações e "pode pagá-las de outro jeito, **onde a linguagem não tiver nomeado
a forma**". Essa cláusula é justamente a regra que decidiria o que um perfil pode
variar: o lowering do `defer` pode; o do `parallel` não, porque a linguagem
nomeou o OpenMP (§4.7). Se isso vier a existir, é especificação **de
backend**, não de linguagem — nada nesta spec muda por causa dela, que é o teste
de que a divisão entre os dois documentos está certa.

## 1.5 Metaprogramação de pré-processador

### Por que a contagem é por alternativa, e não sobre o texto

keel precisa achar o fim de cada corpo de função sem expandir o pré-processador,
e para isso conta delimitadores. Havia dois desenhos, e a diferença entre eles não
é de implementação — é de **o que o diagnóstico mede**.

O primeiro é o óbvio, e foi o que a spec descreveu por uma edição inteira:
diretiva é um token opaco, conta-se `{}` sobre o fluxo, e pronto. Zero
consciência de pré-processador. Ele quebra no idioma mais comum que existe em
código de plataforma:

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

**E é só aí que ela está.** O custo do desenho novo é reconhecer oito palavras;
não é ler o que vem depois delas. Testar `#ifdef MIPS` seria keel decidir por
plataforma, e mataria cross-compilation — o mesmo argumento que já recusou o
`load_blob` tipado, chegando pelo outro lado. Ler a palavra `#ifdef` não decide
nada: é reconhecer onde uma alternativa começa, e as duas coisas só parecem
vizinhas porque moram na mesma linha.

**Por que todas as alternativas têm de concordar, e não cada uma fechar o que
abriu.** A segunda regra é mais simples de enunciar e recusa exatamente o caso
acima: as duas alternativas do `#ifdef MIPS` abrem uma chave e não a fecham. Ela
exigiria delta zero em cada ramo, o que proíbe o idioma que a mudança existe para
admitir. Concordância é a condição certa porque é a que o consumidor precisa: ele
não quer que o ramo seja neutro, quer saber com que profundidade continuar depois
do `#endif`.

### O que continua recusado, e por que não tem saída

```c
#ifdef DEBUG
    if (verbose) {
#endif
        registra(r);
#ifdef DEBUG
    }
#endif
```

Este é frequente, e a recusa é deliberada. Aqui a estrutura **realmente** depende
de `DEBUG`: com ele, `registra` está dentro de um `if`; sem ele, está solto. Não
há o que concordar, e keel teria de escolher.

A diferença entre este caso e o das plataformas é o preço da saída. Lá, contornar
significaria partir a linha em duas funções e mudar o código de verdade. Aqui há
duas saídas baratas — `#ifdef` em volta de statements inteiros, ou tirar a
variação da estrutura e pô-la no valor:

```c
    if (DEBUG_VERBOSE && verbose) { registra(r); }   /* #define DEBUG_VERBOSE 0 */
```

A segunda é melhor C do que o original: o ramo desligado continua sendo tipado
pelo compilador, o que o `#ifdef` não dá, e o `if` some na otimização de qualquer
forma. **A limitação encolheu para o caso em que contorná-la melhora o código.**

### Por que três classes de token, e por que `#elif` é `#else`

A distinção não podia ficar dentro do lexer. Quem conta profundidade precisa saber
**onde zerar** ao cruzar uma alternativa, e isso é informação do fluxo de tokens,
não estado privado de quem tokeniza. Daí `TK_PPC_IF`, `TK_PPC_ELSE` e `TK_PPC_END`
serem classes, e não um campo: são as três transições da contagem — marca,
confere e restaura, confere e sai —, e classe separada poupa despacho secundário
em quem consome.

`#elif` cai na mesma classe de `#else` **porque a condição não é lida**. Sem
avaliá-la, os dois produzem a mesma estrutura: fecha a alternativa anterior, abre
a próxima. A recusa de olhar reaparece como simplificação, que é o padrão desta
linguagem inteira.

O alvo é C23, então `#elifdef` e `#elifndef` entram junto com `#elif`. Fora deles
a classe é `TK_PPC`: `#define`, `#include`, `#pragma` e `#embed` não movem
delimitador, e continuam sendo a linha opaca que sempre foram.

### Por que não há `load_text` nem `load_blob`

Embutir arquivo em tempo de compilação é necessidade real e frequente — tabela,
shader, modelo, fonte de bitmap. A construção proposta seria
`char tabela[] = load_text("assets/tabela.txt");`, e ela é recusada porque **o
C23 já faz isso**:

```c
char tabela[] = {
#embed "assets/tabela.txt" suffix(, 0)
};

unsigned char raw[] = {
#embed "models/data.bin"
};
```

`#embed` é diretiva, então atravessa verbatim e quem a expande é o `cpp`.
`suffix(, 0)` põe o terminador, `limit(n)` corta, `if_empty(…)` cobre arquivo
vazio, `__has_embed` detecta suporte. Pelo princípio 9 — *isto
responde a algo que o C não consegue expressar, ou expressa mal?* — a resposta é
não, e desde 2023 nem "expressa mal" se sustenta. Seria uma segunda grafia para
o que o pré-processador passou a fazer direito: a mesma recusa do `def`
(§4.2), pelo mesmo motivo.

O build fecha sozinho, o que costuma ser a objeção seguinte: `gcc -MMD` sobre o
`.c` gerado emite a dependência do asset, então editar o arquivo retriga a
compilação. O depfile do `cgen` não precisa saber que ele existe.

### O caso tipado é o que keel não pode fazer

`#embed` entrega **bytes**, então `f32 buf[] = { #embed "data.bin" }` daria um
float por byte. A construção que faltaria seria justamente a que reinterpreta os
bytes — e é aí que ela bate numa regra que já existe:

> keel não testa macros de plataforma. Isso **preserva cross-compilation**.

Emitir `{1.5f, 2.25f, …}` a partir de um binário obriga keel a decidir
*endianness* e representação de ponto flutuante. Compilando de x86 para um alvo
big-endian, o dado sairia errado **em silêncio**. Não é dificuldade de
implementação: é a construção pedindo à camada uma informação que ela recusa ter
de propósito. Base64 não resolve — o problema não é transportar os bytes, é
interpretá-los.

A forma correta em C é curta, e põe cada decisão onde ela pertence:

```c
alignas(f32) static const unsigned char raw[] = {
#embed "models/data.bin"
};
static const f32 *buf = (const f32 *)raw;
```

O `alignas` resolve o alinhamento, que é a única armadilha real do cast. A
*endianness* fica com quem escreveu o arquivo, que é a única pessoa que sabe
como ele foi escrito — e se precisar de conversão, ela é um laço visível no
`init`, não uma decisão escondida no compilador.

---

## 1.6 O núcleo, as bibliotecas e o prelúdio

> Espelha **§1.6 e §1.7** da spec, que se separaram; a razão é uma só e fica aqui.

### Por que o prelúdio encolheu para uma linha

A base era implícita, e a justificativa era que ela não é opcional. É verdade
sobre `i32` e falsa sobre o resto: quem tem uma arena própria, um `outcome`
próprio ou um `TRY`/`CATCH` de macro tem projeto legítimo, e a injeção implícita
o punia — dois `types` injetando `arena` é o error 12, e ele disparava sobre a
linha certa.

O corte ficou onde há diferença de espécie. Os tipos de tamanho fixo não são
biblioteca substituível: o §4.2 faz da **grafia** a identidade do tipo, e
pedir `import` para uma regra de grafia seria cerimônia sem contrapartida. Os
módulos da base são módulos, e módulo se importa — como a stdlib sempre se
importou.

O que se ganha não é pureza, são três coisas concretas: nome de usuário nunca
colide com nome da base; o grafo de dependências do arquivo está escrito no
arquivo; e a partição do §1.6 passa a ter **um regime de nomes**, em vez de um
para a base e outro para a stdlib, por razão nenhuma além de história.

O que se perde são quatro linhas no topo do arquivo, e o argumento do `#include
<stdio.h>` responde por elas: `printf` está na libc e o header se escreve assim
mesmo. Escrever de onde um nome vem é o hábito da linguagem hospedeira, não um
imposto novo.

### Por que o diagnóstico do import é sobre a forma, e não sobre a lista

Três identificadores seguidos não têm leitura em C, e é isso — e só isso — que o
reconhecimento usa. Não há consulta a nome conhecido, e portanto não há consulta
à tabela de `typedef`, que é a linha que keel não cruza. `long long int ii;` tem
um identificador porque `long` e `int` são palavras-chave; `struct buffer b;` tem
dois pelo mesmo motivo.

A consequência boa é que o diagnóstico serve a modificador de usuário sem uma
letra de caso especial: a base ganha a `note` com a linha pronta porque o
compilador a conhece de fábrica, e o resto ganha a `note` genérica. É o princípio 8
aplicado a um diagnóstico em vez de a uma construção.

E há um buraco honesto: `arena a;` são dois tokens, indistinguíveis de `Foo a;`
com `Foo` vindo de header. Sem o import, quem reclama é o compilador C, com
*unknown type name* — princípio 3, e a mensagem já é clara. Cobrir esse caso
exigiria saber que `arena` é tipo, que é exatamente o que o import passou a
declarar.

### Por que `arena` tem categoria própria

A tentação é classificá-la como biblioteca, e havia até uma prova: `arena` é
nomeada e despachada pela regra geral de modificadores, e a migração para módulo
escrito em keel não mudaria uma letra de nome nenhum. Mas essa prova é
**nominal**, e o critério é **semântico** — "o que nenhum módulo keel poderia
escrever". As duas coisas não são a mesma.

E na auditoria semântica a arena reprova em quatro linhas: procedência do que é
retornado, `arena.from_array` exigir `array u8`, `arena.from_stack` exigir
constante, e uso de filha depois de `reset` do pai. Nenhum módulo keel escreve
isso hoje: o protocolo de despacho teria que **ganhar vocabulário** primeiro, o
que é a definição de não-expressável. Duas das quatro são inclusive análise de
segunda fase, feita depois que o arquivo inteiro foi varrido.

Então ou a partição é de três vias, ou uma das duas metades mente. A terceira
categoria é a leitura honesta: pela grafia, biblioteca; pelas verificações,
parser. E ela tem exatamente um habitante, o que é o sinal de que a categoria
foi criada para descrever um fato, não para acomodar uma exceção.

---

## 3 Gramática


### Por que `<opaco>` é o terminal central

Uma gramática de ilha não é uma gramática de C com buracos: é o inverso — uma
gramática pequena com um terminal enorme. `<opaco>` é o que torna a invariante
da §1.3 verificável em vez de aspiracional, porque toda produção que não sabe o
que fazer com um trecho **tem para onde reduzi-lo**.

A consequência é boa e é medível: a gramática tem menos de setenta produções, e
nenhuma delas menciona `typedef`, expressão, precedência ou declarador
completo do C. A ambiguidade clássica — `(a)(b)` é chamada ou conversão? — não
aparece porque as duas leituras reduzem ao mesmo `<opaco>`.

### Por que `..` não é um token

Fundir `2..7` num token de intervalo exigiria **retrair o número já emitido**, e
faria um token conter sequência balanceada arbitrária: `xs[(a+1)..(b*2)]` é
legal, e nada impede que os lados contenham chamadas, casts e vírgulas dentro de
parênteses. Um lexer que carrega estrutura balanceada dentro de um token deixou
de ser lexer.

Deixando `..` como pontuação, a forma do intervalo vira problema do parser, que
é onde o balanceamento já existe. O lexer continua não decidindo nada — que é a
regra da §3.6 e é o que mantém as duas camadas separáveis.

### Por que o intervalo dispensa delimitador

A pergunta natural é por que não existe uma sintaxe explícita, tipo
`range(a, b)`. A resposta é que o intervalo só é **reconhecido** em regiões que
já são delimitadas por outra coisa: os `[ ]` do índice, o `:` e o `)` do
`foreach`, o `=` e o `;` de uma inicialização. Dentro delas, o `..` de topo se
acha pelo balanceamento, e não há terceira leitura possível — `..` não é C.

Daí também a assimetria entre `recorte` e `intervalo`. O lado vazio só existe
dentro do índice porque **é o contêiner que preenche a ponta que falta**; fora
dos colchetes não há de onde tirá-la. É a mesma economia que percorre o
documento: a forma existe onde a informação existe.

### A posição de contêiner não é um sistema de tipos

`contentor` parece tipagem e não é: é uma **tabela fixa de tipos de retorno**
avaliada sobre símbolos que keel mesmo declarou. Cada nó tem tipo em função
apenas do primeiro argumento, e a avaliação nunca entra em `<opaco>`. É o preço
mínimo do despacho de verbo — sem ele, `buffer.push(x, v)` não teria como escolher a
função —, e é o único lugar em que ele é pago.

É também onde a camada foi raspada de propósito. `f(y)[i]` e `(cast)x[i]` são
recusados não porque sejam difíceis, mas porque aceitá-los significaria tipar o
retorno de uma chamada e o resultado de uma conversão — e a partir daí não há
onde parar: o próximo pedido é o operador de propagação, e depois a
desestruturação. A gramática é o lugar onde esse limite fica escrito em vez de
prometido.

### Por que o argumento nunca contém `*`

A forma `buffer (i32*) x;` foi considerada e recusada. Ela colidiria com a
sintaxe de argumento de `dim` — `tensor(3) f32` —, que é exatamente a mesma
forma, e a desambiguação passaria a depender de **espaçamento** (`i32*` contra
`i32 *`), que é a pior saída possível numa linguagem cujo lexer é o de C.

A consequência é direta e desejada: `*` e `[N]` pertencem ao declarador, nunca
ao argumento. Isso elimina de saída a colisão entre `const char *` e
`char *const` — dois tipos C distintos que uma normalização ingênua funde num
nome canônico só. Buffer de ponteiros continua possível, com um `typedef` de uma
linha, que é onde essa complexidade pertence.

### Por que o prefixo do C é reconhecido em vez de atravessar opaco

Reconhecer `static`, `alignas` e companhia parece contrariar o instinto da
invariante: são palavras do C, e keel não quer saber de C. Mas o teste do §1.3 é
outro — *keel precisa saber o que este nome significa?* — e a resposta aqui é
não. `alignas(64)` é copiado sem exame; o que keel faz é apenas **não perder o
resto da linha**.

E perder o resto da linha era o custo real. `alignas(64) array u8 memo[N];` não
casava nenhuma produção, então a declaração inteira virava `<opaco>`, `memo`
deixava de ser registrado como `array u8`, e o `arena.from_array(a, memo)` da
linha seguinte era recusado pelo error 37 — sobre um vetor que está correto, com
uma mensagem que fala do tipo errado. O usuário não tinha como descobrir que a
causa era o `alignas`.

Esse é o pior modo de falha que a linguagem pode ter, e ele reaparecia em cada
prefixo: `static buffer i32 contador;` num bloco, `[[maybe_unused]]` numa
declaração de contêiner. Nenhum deles muda o tipo; todos apagavam o símbolo.

O alinhamento é o caso que decide, porque é a única forma de pedir arena
sobre-alinhada (§4.4) e porque orientação a dados o pede o tempo todo —
linha de cache, largura de vetor SIMD, fronteira de página. Uma linguagem cujo
alvo é layout explícito não pode ter o `alignas` como a coisa que faz o símbolo
desaparecer.

A regra que sai disso é estreita, e é geral de propósito: **nada que o C escreva
antes de um especificador pode fazer uma declaração de keel desaparecer.**
Enumerar só os casos que apareceram primeiro — `alignas` e `static` — deixaria a
mesma armadilha armada para o próximo, e o próximo já existe:
`_Atomic buffer u32 contadores;` é declaração perfeitamente razoável de um
descritor compartilhado, e `[[maybe_unused]] slice char s;` é higiene comum.

`_Atomic` é o caso que mostra por que a divisão tinha de ser por **posição**, e
não por palavra. Antes do modificador ele qualifica o descritor e não muda o
tipo do elemento; dentro do argumento ele qualifica o elemento e **produz tipo** —
`buffer _Atomic u32` tem `sizeof` e `alignof` próprios e é outra instância que
`buffer u32`. As duas leituras são legítimas, se escrevem diferente, e a
gramática as separa sem consultar nada.

`restrict` não aparece nesta discussão, e é por não ser tipo: é promessa sobre um
acesso por ponteiro, com o mesmo estatuto do `*` que o argumento também não
admite — e por isso ele vive em declarador C comum, e não sobre contêiner (§4.2).

Sobrou uma obrigação para o backend, e ela vem daqui: `_Atomic u32` no nome
canônico não pode virar `_Atomic_u32` no símbolo, porque `keel_buffer__Atomic_u32`
tem `__` — reservado à implementação pelo C. É a mesma disciplina do error 18,
lida do outro lado.

### Por que `dim` vem antes de `type` na linha `module`

A ordem é rígida, e não por gosto. É ela que permite ao parser conhecer a
**assinatura completa** do modificador — quantas dimensões, quantos tipos — a
partir da linha `module`, antes de parsear qualquer uso. É a mesma propriedade
que dispensa os chevrons: `tensor(3) f32` não precisa de `<>` para se separar de
uma expressão porque o parser já sabe, da declaração, que `tensor` toma um
numeral e um tipo, nessa ordem.

Sem a ordem fixa, a linha `module` deixaria de ser suficiente e a assinatura
teria que ser remontada por análise — que é trabalho de tipagem entrando pela
porta dos fundos.

---

### Por que não existe literal de instância

`(buffer i32){…}` seria o único ponto em que um nome de tipo keel precisaria ser
reconhecido **dentro** de uma expressão, e reconhecer nome de tipo em expressão é
o começo de tipar expressão. O custo de não ter é um verbo por modificador — que
já existia de todo jeito, porque `of` e `from` carregam a distinção
vista/asserção (`§4.11`). Quem declara `modifier` declara o construtor: uma
linha, no lugar certo, uma vez.

O mesmo raciocínio recusa `sizeof(buffer i32)`. Ele seria legítimo e inofensivo,
e é justamente por isso que fica de fora: aceitá-lo abriria a posição, e a
posição é o limite.

---

## 3.10 Reconhecimento das palavras contextuais


### Por que nenhuma palavra é rígida

Uma palavra-chave rígida custa um identificador a todo programa que já existe.
`buffer`, `slice`, `defer`, `range` são palavras que qualquer base de código em C
usa como nome de variável — e keel não pode quebrá-las sem violar o princípio 1,
que diz que nenhum programa C válido muda de sentido ao passar por ele.

A alternativa seria escolher palavras que ninguém usa, e o resultado seria uma
linguagem com vocabulário feio para resolver um problema que o lookahead resolve.
Contextual custa uma cláusula por palavra na gramática; rígido custaria uma
migração a cada usuário.

### A observação que sustenta tudo

> Sequência de N identificadores seguidos, com N ≥ 3, nunca é declaração válida
> em C.

É essa frase que faz `buffer i32 xs;` ser reconhecível sem ambiguidade, e é dela
que sai a forma justaposta em vez de chevrons ou parênteses. Com N = 2 a
sequência é declaração (`Point p;`), e por isso `buffer b;` continua sendo o tipo
do usuário — o que também explica por que o modificador **precisa** de argumento
para ser reconhecido, e não pode ter forma de zero argumentos.

A regra é sintática e não consulta tipo nenhum, o que a mantém do lado certo do
corolário do §1.3.

### Por que a rigidez é regional em vez de por palavra

Dentro de `extern_c` seria possível desligar palavra por palavra — `defer` não
vale aqui, `buffer` vale. Uma regra só, valendo para todas, tem duas vantagens
que se somam: o lexer a implementa com um bit em vez de uma tabela, e o usuário a
memoriza numa frase. E ela não custa nada, porque `extern_c` existe justamente
para declarar coisas do C, onde nenhuma palavra de keel teria o que fazer.

Que `extern_c` só ocorra em nível de arquivo é o que permite o bit em vez de uma
pilha — daí o error 7 não ser cerimônia: ele protege a implementação de precisar
de estado aninhado.

### Visibilidade é transitiva; injeção não é

As duas regras do `import` parecem inconsistentes e não são. **Visibilidade** é
transitiva e segura, porque nome externo é qualificado por omissão: acrescentar
um `import` não muda o sentido de nenhuma linha já escrita, e por isso não há
vazamento de namespace a evitar.

**Injeção** é o oposto exato. Um nome que era token opaco — vindo de um `typedef`
que keel não enxerga — passa a ser reescrito, e nem keel nem o compilador C
acusam: os dois lados acham que está tudo certo, e o programa muda de sentido.

Daí as três consequências que a spec enuncia: a injeção é explícita, dois `types`
injetando o mesmo nome é error, e o import lista o que entrou nu. E daí também a
recusa de tornar o prelúdio uma linha só (§1.3) — reexportação seria injeção
transitiva, e o custo dela é não se poder mais saber o que está em escopo lendo o
próprio arquivo.

### Por que `instance` é a única ressalva do documento

Todas as ambiguidades com C se resolvem por lookahead fixo. Uma não:
`instance G A;` tem exatamente a forma de `buffer i32 xs;`, e o que as separa é
`instance` estar na primeira posição. Se um modificador se chamasse `instance`,
as duas leituras seriam `instance(G(A))` declarando uma variável e a declaração
de instanciação — e nenhum número de tokens à frente as separa.

A saída poderia ser uma regra de precedência ("instance na primeira posição
sempre ganha"), e ela funcionaria. Foi recusada porque tornaria **silenciosamente
inalcançável** um modificador legalmente declarado: quem escrevesse
`pub modifier instance {…}` teria um tipo que nunca poderia ser usado, sem
nenhuma mensagem dizendo por quê. Error na declaração custa uma linha e diz o
motivo no lugar certo.

### Por que a redeclaração é recusada em vez de classificada

keel poderia tentar decidir se `FILE *xs;` é declaração — bastaria classificar o
primeiro identificador. Só que classificar exige saber que `FILE` é um nome de
tipo, e a invariante do §1.3 diz que keel não sabe isso sobre nome nenhum do C.

Então sobra recusar o padrão em que ele erraria, que é o princípio 5 na letra.
O custo é pequeno e localizado: um `IDENT IDENT` em início de statement cujo
segundo nome é símbolo de keel. Renomear a variável local resolve, e a mensagem
diz isso.

É o único ponto do documento em que a invariante aparece como **restrição ao
usuário** em vez de propriedade da implementação — e vale registrar exatamente
por isso, porque é a fatura da decisão de não entender C.

---

## 4.1 Unidade


### Por que a identidade vem do caminho **e** da declaração

Derivar só do caminho mantém a descoberta trivial — nome vira caminho direto, sem
índice a manter. Mas a raiz de busca é da invocação, e sem a declaração no fonte
a identidade dependeria de como a ferramenta foi chamada:

```sh
cgen src/a.k -I .        # módulo src.a
cgen src/a.k -I src      # módulo a
```

As duas têm uma raiz só — passariam por qualquer regra de contagem de raízes — e
produziriam prefixos de símbolo diferentes para o mesmo arquivo, com falha só no
link. Com a declaração, uma das duas é diagnosticada.

Derivar só da declaração, por outro lado, exigiria um índice: dado
`import net.http;`, onde está o arquivo? A combinação das duas dá descoberta por
caminho e identidade por fonte, e nenhuma das duas sozinha dá as duas coisas.

### Por que a colisão é checada sobre símbolos, não sobre prefixos

Parece bastar verificar que nenhum prefixo de módulo é prefixo de outro. Não
basta:

```plain
módulo net       com função http_get   → net_http_get
módulo net.http  com função get        → net_http_get
```

Os prefixos `net_` e `net_http_` são distintos, e ainda assim os símbolos
colidem. Isso é consequência de o separador de módulo e o separador de nome serem
o mesmo `_` no C — e não há como evitá-lo sem inventar um separador que não é
identificador C válido.

Então a checagem é sobre o **conjunto gerado**, e é feita antes de escrever
qualquer coisa. Custa uma tabela de hash sobre nomes que já foram calculados de
qualquer forma, e converte uma falha de link — que aparece longe, sem posição de
fonte — num error com os dois caminhos na mensagem.

### Por que `types` não injeta função

A divisão entre espaço de tipos e o resto não é estilo, é o que torna a
construção segura. Pela invariante do §1.3, keel não conhece nenhum nome vindo de
`import_c` — então um nome nu injetado que coincidisse com símbolo daquele header
seria reescrito **em silêncio**, sem que nenhum dos dois lados pudesse acusar.

E os nomes que materializam esse risco na prática são todos **função**:
`remove`, `abort`, `exit`, `div`, `index`, `time`. O espaço de tipos do C é
pequeno e quase todo sufixado `_t`, o que torna o risco residual pequeno — e o
`info` 14 o cobre, listando o que entrou nu no ponto exato onde alguém pediu.

**Constante de enum é o caso mais perigoso**, e por isso é escopada no tipo em
vez de injetada. Ela colide com **macro** da libc — `EOF`, `SEEK_SET`, `NULL` —,
e essa colisão é invisível para os dois lados: se keel reescreve um `SEEK_SET`
nu, a macro simplesmente deixa de expandir, e não há erro em lugar nenhum.
Escopar no tipo mata o risco por construção: `PARADO` nunca aparece nu, então não
há o que colidir.

### Por que a injeção não propaga

Se propagasse, acrescentar um `types` em qualquer ponto do grafo mudaria o
sentido de arquivos que ninguém tocou. Um nome que em `a.k` era token opaco do C
— vindo de um `import_c` que keel não enxerga — passaria a ser reescrito porque
alguém três módulos abaixo escreveu uma palavra. É o risco da §3.10, agora sem
ninguém para controlá-lo, e quem paga não é quem escreveu.

E a checagem de colisão de injeção, que hoje se decide lendo **um arquivo só**,
precisaria do grafo inteiro — com o error apontando para um `import` que o usuário
não escreveu.

Vale notar que não foi preciso proibir: como `types` é puramente de fonte e a
interface gerada sempre sai qualificada, **não existe canal por onde a propagação
aconteceria**. A regra descreve uma consequência, não impõe uma restrição — que é
a forma mais barata de uma regra existir.

### Por que import circular é error em vez de suportado

Não é limitação de análise. Em C, ciclo de valor é impossível — `struct A` com um
`struct B` por valor precisa do layout completo de `B`, e vice-versa. Ciclo só
existe por ponteiro, e para isso o C exige declaração adiantada em vez de
inclusão mútua.

Gerar isso automaticamente exigiria **quebrar cada `.h` em dois** — um de forward
declarations e um de definições —, o que destrói a propriedade de um `.h` e um
`.c` por módulo. Os include guards não salvam: eles terminam a recursão, mas ao
custo de um dos dois headers enxergar o outro pela metade.

A saída — subir o tipo compartilhado para um módulo de definições — é a mesma que
um programador C usaria, e é por isso que a recusa não custa expressividade.

### O orçamento de análise, e por que ele importa

As três fases não são descrição de implementação: são o teto que a linguagem se
impõe. Faltava um critério para o **custo de análise**, que é por onde uma base
cresce sem ninguém notar — cada construção nova pede só mais um pouco do parser,
e nenhuma sozinha parece cara.

O teste de que é o critério certo, e não um enunciado a posteriori, é que ele
explica retroativamente decisões que foram tomadas por outros motivos: função
genérica livre, sobrecarga, o limite de `*out = slice.of(b)`, a entrada do
`cofsm`. A linha do limite documentado é a mais útil das quatro — ela deixa de
ser desculpa e passa a ser a regra funcionando.

### Por que `extern_c` resolve uma direção só

O bloco dá ao mundo keel acesso a símbolos do mundo C. A direção inversa —
símbolo keel com nome fixo — é resolvida por um adaptador em C, e a tentação é
achar isso pobre. Não é, e a alternativa é pior.

Uma construção de "definição sem mangling" obrigaria `extern_c` a significar duas
coisas: *região sem keel* no bloco, e *só desliga o mangling* no prefixo. Isso
quebra o estado único ligado/desligado do lexer (§3.10), que é o que torna a
regra de regionalidade uma frase.

E o adaptador não é remendo: keel foi desenhado para viver num mar de C, e um
adaptador **é** C. A assinatura é conferida de graça pelo compilador C, porque o
adaptador inclui o header do módulo — se a função keel mudar, o erro aparece ali.
A função exportada continua sendo keel de verdade, com `defer`, arena e
contêineres, o que uma construção de definição sem mangling não conseguiria
oferecer.

---

## 4.2 Tipos e marcadores
### Tipos de tamanho fixo


#### Por que `constexpr` nomeia uma obrigação, e não um lowering

O §4.2 dizia "o lowering é a declaração copiada verbatim", e essa frase era o
bloqueio inteiro do alvo C11 — não a construção, a frase. Pela regra dos perfis
(`backend §1`), um backend só paga de outro jeito **onde a linguagem não tiver
nomeado a forma**; ali ela tinha nomeado, então nenhum perfil podia emitir outra
coisa, por melhor que fosse.

Trocar a forma pela obrigação — símbolo com o tipo escrito, válido onde o C exige
expressão constante, inicializador conferido — não afrouxa nada: o que a
linguagem promete continua idêntico, e ganhou-se um alvo. **É o teste de que a
divisão entre os três documentos está no lugar certo**, e é o mesmo teste que o
§1.4 aplica aos perfis de geração.

#### Por que agregado não é `constexpr`

`constexpr int A[3] = {1,2,3};` é C23 válido e keel o recusa (error 121). A razão
não é o perfil C11 — é que a construção não entrega ali o que promete. Um agregado
não serve onde o C exige expressão constante **em padrão nenhum**: `A[0]` não é
expressão constante inteira nem sob C23, então `array u8 memo[A[0]]` viraria VLA
com ou sem perfil. Registrar o símbolo como constante seria registrar uma promessa
falsa.

E o contorno não custa nada, porque já existe e não é de keel: `static const int
A[3] = {1,2,3};` é C de sempre, atravessa como `<opaco>` e faz exatamente o que a
pessoa queria. A recusa é sintática — `[` no declarador ou `{` depois do `=` —, o
que a mantém decidível sem olhar o tipo.

#### Por que os primitivos não podem ser biblioteca

O princípio 8 diz que núcleo é o que nenhum módulo keel poderia escrever, e
os primitivos passam nesse teste por um motivo específico: `typedef _Float16 f16;`
escrito num módulo é um `typedef` **do C**, que keel não enxerga. O nome ficaria
opaco, e opaco não serve como argumento de modificador — onde a grafia é a
identidade do tipo.

`buffer f16` precisa nomear a mesma instância em todo módulo do build, e só a
camada zero garante isso. Dito de outro modo: **um formato numérico ou está na
camada zero, ou não existe como elemento de contêiner.** Não há meio-termo de
biblioteca, e é por isso que decidir o que entra ali é decisão de linguagem e não
de escopo.

#### Por que `int` é error e `size_t` não

Parece inconsistente — os dois são tipos do C com largura de plataforma. A
diferença é **grafia alternativa**.

`int` pode ser escrito `int`, `int32_t` ou `i32`, e as três dariam instâncias
diferentes com o mesmo layout. Normalizar todas para `i32` seria pior que
recusar: numa plataforma de `int` com 16 bits, `buffer int` passaria a ter
elementos de 32 bits **em silêncio** — mudar o C, contra o princípio 1. E como
keel não testa macro de plataforma (§1.4), não há como decidir corretamente.

`size_t` tem uma grafia só e não tem irmão na família keel — não existe `usize`.
`char` idem: é o byte de texto, e `u8` é o byte de dado; são papéis diferentes,
não sinônimos. Sem grafia alternativa, não há instância duplicada a evitar.

O enunciado que sustenta a regra é mais forte que higiene de nome:

> **Contêiner descreve layout de memória, e `int` não descreve um layout.**

#### `i4` e `u4`: o diagnóstico está certo, a solução é outra

O argumento a favor é bom, e é de uso real: larguras sub-byte são **tipo de
armazenamento**, quase nunca de aritmética — a conta acontece em `i32` depois da
conversão. Guardar um nibble em `u8` desperdiça metade da memória, que é
exatamente o oposto do motivo pelo qual alguém escolheu quatro bits.

Mas `u4` como tipo da camada zero não funciona, e o motivo não é gosto:

**Não há endereço.** O §4.7 exige de todo percorrível `length` e — conforme a
forma do binder — `get` ou `ptr`; o açúcar `x[i]` é **definido** como
`*ptr(x, i)`. Não se endereça um nibble, então `buffer u4` não teria o que
devolver em `ptr(x, i)`, e `x[i]` deixaria de existir para esse contêiner. Um
elemento sem endereço é categoria nova atravessando `foreach`, `apply` e
`parallel`.

**E a instância genérica não sabe empacotar.** O modificador `buffer` emite
`T *ptr` e conta em `sizeof(T)`. Com `sizeof(u4)` inexpressável, `buffer u4` não
é uma instância do `buffer` que existe — é outro contêiner com o mesmo nome.

**Bitfield não resolve, e piora.** `struct { u8 lo:4, hi:4; }` tem ordem de
alocação **definida pela implementação**: o padrão não fixa em que ponta `lo`
cai. Um contêiner de nibbles sobre bitfield teria layout dependente de
compilador, e a pergunta "qual metade do byte" vazaria para o fonte do usuário. É
a mesma classe de recusa do `load_blob` (§1.4): representação definida pela
implementação não entra na semântica.

**A saída é biblioteca, e é barata.** Nada disso precisa do núcleo:

```keel
module keel.nibble;

pub typedef struct { size_t cap, len; u8 *ptr; } nibbles;

pub inline u8   get(nibbles *n, size_t i) { return (i & 1) ? (n->ptr[i>>1] >> 4)
                                                           : (n->ptr[i>>1] & 0xF); }
pub inline void set(nibbles *n, size_t i, u8 v) { ... }
pub inline size_t length(nibbles *n) { return n->len; }
```

Um módulo comum, com tipo próprio e verbos próprios — a mesma forma da `arena`.
`nibble.get(n, i)` e `nibble.set(n, i, v)`, e ele é percorrível por `foreach` com
binder por valor, porque declara `length` e `get`. O que ele **não** declara é
`ptr`, e o error 68 já cobre isso com uma mensagem que lista as aridades que
existem.

E o índice é o do nibble, não do byte: **a divisão por dois e a escolha da metade
ficam dentro do módulo**, que é onde o empacotamento pertence. Um builtin que
dissesse "alto ou baixo" exporia o layout, e um contêiner existe para escondê-lo.
Quem quiser o byte usa o `buffer u8` de baixo.

Vale a pena ver o que isso ilustra, porque a lição é geral: a construção que
parecia precisar de um tipo novo na camada zero precisava, na verdade, de um
**módulo**. É o mesmo movimento que tirou `soa` daqui e que manteve a conversão
saturante fora do núcleo.

---

### `ref`, e por que `restrict` saiu


#### De onde `ref` veio

`ref` é o que sobrou de `unique` depois da troca do modelo de memória.
Modificadores de posse — `unique`, `shared`, `weak` — pertencem ao modelo
**individual**, em que cada objeto tem dono e alguém o libera. Num modelo de
**grupo**, ninguém libera um ponteiro e a contagem por objeto não tem o que
fazer. O que continuava valendo era a distinção que o C não faz — ponteiro para
**um** elemento contra ponteiro para muitos —, e essa não é sobre posse: é sobre
aritmética. `ref` é ela, e nada mais.

O caso concreto é `ptr(x,i)`. Sem `ref`, ele devolveria um `T *` sobre o qual
alguém escreve `+3` e sai do contêiner sem que nada reclame — o endereço de um
elemento e a base de uma travessia teriam o mesmo tipo, que é exatamente a
confusão que o resto da linguagem existe para desfazer.

#### O custo de implementação de `ref`

`ref` é a única construção do núcleo que obriga o parser a olhar **dentro** de
expressões, fora da gramática de contêiner: detectar `p + 1` exige varrer o fluxo
de tokens procurando símbolos `ref` adjacentes a `+ - ++ -- [`. É análise em
nível de token, sem gramática de expressão — o teto da camada continua de pé —,
mas é o ponto em que ele foi raspado de propósito.

#### Por que `restrict` sobre contêiner saiu

Ele existiu, com lowering por hoisting: na declaração emitia-se um `T *restrict`
local com a base, e a indexação resolvia contra ele. **A construção foi retirada
depois de medida**, e as três razões se somam.

**A primeira é do padrão, e não depende de compilador.** O hoisting deixava dois
caminhos de acesso ao mesmo armazenamento — o ponteiro hoistado e o `x.ptr` de
dentro dos verbos. `x.ptr` não é *based on* o hoistado, então qualquer programa
que indexasse **e** chamasse `push`, `get` ou `set` no mesmo escopo era
comportamento indefinido. Restringir os verbos fecharia isso, ao custo de proibir
metade do vocabulário do contêiner justamente onde ele foi declarado.

**A segunda é medida, e derrubou a contrapartida.** O mesmo laço, GCC 13 a `-O3`:

| Forma | bytes |
| --- | --- |
| `restrict` direto nos parâmetros ponteiro | **132** |
| membro `restrict` em struct passada **por valor** | 134 |
| **hoisting para local — o lowering que existia** | **218** |
| membro `restrict` em struct por ponteiro | 218 |
| sem `restrict` nenhum | 218 |

O hoisting **não era honrado**: 218 bytes, idêntico a não ter a construção. O que
o `restrict` de contêiner comprava era zero, e o que custava era UB. Isolamos a
causa — não é a struct, é o hoisting: hoistar de parâmetro ponteiro comum também
dá 218.

**A terceira fecha a saída alternativa.** `restrict` na posição do descritor não é
C válido (`invalid use of 'restrict'`), então o qualificador teria de ir no
**membro** — que funciona para descritor passado por valor (134) e faz um tipo de
struct distinto. E aí `gemm(restrict slice i8 a, …)` não poderia ser chamada com
um `slice i8` comum: `incompatible type for argument 1`. `restrict` viraria viral
na fronteira, que é exatamente onde ele precisa não ser.

#### O que ficou no lugar, e por que é melhor

A forma que mede 132 — a melhor das cinco — já era escrevível, e é a que o
programador C escreve:

```keel
priv void gemm_kernel(i8 *restrict a, i8 *restrict b, i8 *restrict c,
                      size_t m, size_t n) { … }

pub void gemm(slice i8 a, slice i8 b, buffer i8 *c, size_t m, size_t n) {
    gemm_kernel(slice.ptr(a), slice.ptr(b), buffer.ptr(c), m, n);
}
```

O kernel toma ponteiros e promete o que `restrict` promete; a fronteira de
contêiner fica na função de fora, onde `length` e `capacity` ainda existem — o
custo alegado do "escape manual" era perder contêiner e `length`, e ele não se
sustenta quando a divisão é essa. **É o princípio 9 lido ao contrário:** entra o
que o C expressa mal ou não expressa, e aqui o C expressa melhor.

Saíram junto o lowering de hoisting, os diagnósticos 100 e 102, e o `restrict` no
binder de `foreach` e `parallel`. O **101** ficou, com outro sentido: recusar a
grafia e apontar o par acima.

#### O que a partição do `parallel` garante, e que não era o `restrict`

Vale separar, porque as duas coisas eram confundidas. A partição do §4.7 é
normativa e **disjunta**: duas faixas tocam elementos diferentes do **mesmo**
objeto, e para isso o C nunca precisou de `restrict` — `a[i]` e `a[j]` com `i≠j`
não têm o que provar. Essa garantia é da fórmula da faixa, não do qualificador, e
sobrevive inteira à retirada.

O que a arena **não** garante é não-sobreposição entre contêineres **diferentes**:
`slice.of(b,0,5)` e `slice.of(b,3,8)` saem da mesma arena e se sobrepõem. Era esse
o caso do `restrict` — e é esse que o kernel de ponteiros passa a cobrir, medindo
melhor.

Há uma consequência a registrar: a afirmação de que as faixas contíguas de
`parallel` "deixam o laço interno na forma que o compilador C auto-vetoriza"
(§4.7) **depende desta seção**. Sem modelo de aliasing, a forma é vetorizável
e o compilador não pode provar que é.

---

### `array`


#### O que a checagem de `buffer.of` pega

```keel
i32 *p = buffer.ptr(xs);
buffer i32 b = buffer.of(p);
```

```c
/* o que sairia, se a checagem não existisse */
keel_buffer_i32 b = keel_buffer_i32_as(p, sizeof p / sizeof(i32));   /* 8/4 == 2 */
```

Duas de capacidade, em silêncio, num contêiner que o programador acredita ter
cem. É o bug clássico do vetor decaído, e o `cc1` não tem o que dizer: `sizeof p`
sobre um ponteiro está perfeitamente correto. O marcador é o que separa "vetor de
verdade, cujo `sizeof` significa alguma coisa" de "ponteiro" — e essa é a
informação que C perde e ninguém recupera.

#### Por que o marcador é o que torna a notação legítima

`w[1,2,5]` já significa alguma coisa em C: é o operador vírgula, e o valor é
`w[5]`. Reescrever toda indexação com vírgula transformaria em silêncio o sentido
de código C válido — a única coisa que a linguagem não pode fazer, sob pena de
não ser mais um superconjunto. Restringir a reescrita a símbolos `array`
conhecidos resolve isso sem heurística nem exceção: o que keel não registrou, ele
copia.

Vale notar que o conflito só existe em posição de expressão. No declarador,
`[1,2,5]` sequer é C válido, então ali a notação não disputa nada.

#### Por que vetor C de verdade, e não bloco plano

Linearizar `[2,3,4]` num bloco de 24 elementos e reescrever os índices seria
possível, e é o que uma biblioteca de tensor faz. Mas o C **sabe** expressar
vetor multidimensional estático: o tipo resultante é `i32[2][3][4]`, o compilador
o verifica, o depurador o exibe, e código C já escrito que faz `v[i][j][k]`
continua funcionando sobre o mesmo símbolo. Inventar outra representação custaria
tudo isso para não comprar nada.

#### Por que não há forma `dim(v)`

Como "a dimensão única" ela seria `keel.length(v)`, que já existe. Como "o rank" seria
inútil: `k` tem que ser constante, então quem escreve a chamada já sabe o rank
estaticamente. Que `dim(v,0)` continue legal em vetor 1D é redundância
deliberada, da mesma espécie de `keel.length(v)` valer `keel.capacity(v)` para `array`:
existe para que código escrito contra `buffer` leia igual contra `array`.

#### Por que nD em parâmetro é permitido e 1D não

Parece inconsistente e é o contrário. Em C, o decaimento apaga apenas a
**primeira** dimensão: em `void f(array i32 v[2,3,4])`, o parâmetro vira
`i32 (*)[3][4]` e as duas dimensões restantes continuam no tipo, verificadas pelo
compilador **no chamador**. Já em 1D não sobra nada — `array T v[N]` e `T *v` são
o mesmo ponteiro, e o `N` escrito ali é uma afirmação que nada sustenta. O
unidimensional é recusado por ser *menos* seguro, não por ser mais simples, e a
substituição existe: `slice T` atravessa a chamada carregando o tamanho.

---

### `constexpr`


#### Por que não uma palavra própria

A proposta natural é uma marca de keel — `constant MAX_LEN = 4096;` — e ela
morre numa linha: **para gerar a declaração C, keel teria que deduzir o tipo do
literal.** `4096` é `int`? `unsigned`? `size_t`? A resposta exige tipar uma
expressão, que é exatamente o teto da camada (§1.3).

O caso que torna isso visível é o literal de string:

```keel
constant HELLO = "HELLO";     /* geraria o quê? */
```

`constexpr char HELLO[] = "HELLO";` é a declaração certa, e não há como
chegar nela a partir de `constant` sem que keel classifique o inicializador. Mas
o problema não nasce no `"HELLO"` — ele já estava no `4096`, apenas escondido
por `int` ser a resposta que quase sempre serve.

Escrevendo `constexpr`, o tipo vem do usuário e a verificação vem do compilador
C. É o princípio 3, e o de sempre: **a informação existe no fonte porque alguém
a escreveu, não porque a linguagem a adivinhou.**

#### Por que `def ID <opaco>` foi recusado

Seria a forma de um `#define` com outro nome, e é a única proposta desta
conversa que a linguagem tinha obrigação de recusar. Dois motivos, e o segundo é
o que fecha.

**Seria substituição textual executada por keel** — precisamente o que a §1.5
mantém fora, agora entrando pela porta de dentro. E **keel existe para tornar o
truque de macro desnecessário**: uma linguagem que substitui `#define` por um
`#define` próprio não trocou nada, só mudou quem escreve. Contra o princípio 2,
de quebra: ninguém escreveria à mão uma camada de substituição e assinaria o
resultado como C gerado.

#### Ler uma palavra do C não fere a invariante

Parece que fere, e vale dizer por que não. A invariante do §1.3 é sobre **nomes
de tipo**: keel nunca precisa saber o que um nome do C significa. Registrar que
`MAX_LEN` é um símbolo constante não exige saber o que `size_t` é — o tipo
continua opaco, e continua sendo o compilador C quem o verifica.

O que muda é menor do que parece: keel passa a reconhecer **uma** palavra a mais
em início de declaração, e a extrair dali **um** nome. É a mesma espécie de
varredura que o `ref` já obriga (achar `p + 1` no fluxo de tokens) e que a regra
de redeclaração já faz. Não é gramática nova.

E há o precedente exato, que é o `array`: uma marca que não gera tipo, não vira
modificador, registra um símbolo e some. `constexpr` é a mesma categoria com uma
diferença a favor — a marca **já existe no C**, então nem somir ela precisa. Que
a construção nova caia numa categoria que já tinha habitante é o argumento de
que ela é a leitura certa, e não um enxerto.

#### O custo, e ele é zero relativo a hoje

`#define MAX 4096` e `enum { MAX = 4096 };` continuam invisíveis, e o idioma é
universal. Mas o que se perde já estava perdido: hoje os diagnósticos 32, 36, 89
e 94 exigem decidir se um token é constante, e diante de um `IDENT` vindo de
macro keel **não tem como saber**. Ou aceita só dígitos — e aí ninguém pode dar
nome a um tamanho — ou os quatro são letra morta. `constexpr` é o que os torna
implementáveis; não reconhecê-lo não devolve nada.

O que a escolha cobra é uma mensagem bem escrita: o error tem que nomear a saída,
porque quem escreveu `#define` não vai adivinhar. E cobra uma dependência real
do C23 — mas de **reconhecimento**, não de geração: keel não emite `constexpr`,
ele copia o que o usuário escreveu. Num alvo pré-C23 o usuário volta ao
`#define` e perde os quatro diagnósticos, que é exatamente a situação de hoje.

**Extensão possível, deliberadamente fora da v0:** reconhecer também
`enum { A = …, B = … };`, que é o idioma pré-C23 para a mesma coisa. O corpo é
`IDENT [ '=' <opaco> ] { ',' … }` entre chaves que o lexer já balanceia. É
puramente aditivo e não muda nenhuma decisão desta seção — motivo pelo qual não
precisa entrar agora.

#### O que ele eliminou: a forma literal do `dim`

O binder de dimensão tinha duas formas — `dim N`, identificador, e `dim 2`,
literal, descrita como "a primeira com o argumento escrito na definição em vez
do uso". A descrição não se sustenta, e a pergunta que a derruba é de uma linha:
**`dim 2` liga qual símbolo?** Nenhum. O binder existe para dar ao corpo um nome
que ele substitui — `size_t dims[N]`, a aridade de `ptr(m,i,j)` —, e a forma
literal não dá nome a coisa alguma: o corpo escreveria `2` na mão.

A forma honesta do que ela queria dizer seria `dim N = 2`, que liga o nome e fixa
o valor. E é aí que ela morre, porque

```keel
module mat dim N = 2 type T;
```

é a mesma coisa que

```keel
module mat type T;
priv constexpr int N = 2;
```

— com sintaxe própria, numa posição especial, para o que uma declaração comum já
faz desde o §4.2. Duas grafias para o mesmo objeto é exatamente o que a regra
de grafia única do argumento (§4.2) recusa em outro contexto, e pelo mesmo
motivo.

A divisão que sobra é limpa, e cada metade faz uma coisa só:

| O rank | Como se escreve |
| --- | --- |
| **varia por instância** — `tens(3) f32`, `tens(4) f32` | binder `dim N` na linha `module` |
| **é fixo** — `matrix f32` | numeral nos campos, **um acessor por aridade** |

E a segunda metade é mais barata do que parecia enquanto a forma literal existia.
A tentação era pôr o rank num `constexpr` do corpo, mas isso não funcionaria: o
§4.2 diz que keel registra que o símbolo é constante e **nunca lê o valor**, e
o açúcar de rank cheio precisa do número — para montar o `(size_t[N]){…}` e para
diagnosticar `k > N`. Com `constexpr` o parser saberia que `N` é constante e não
saberia que vale 2.

O que resolve é não precisar do número. Rank fixo declara `ptr(m,i)` e
`ptr(m,i,j)` por extenso, e o açúcar base do §4.6 — que despacha por
**contagem de índices**, e já recusa aridade inexistente com o error 68 —
entrega o mesmo `m[i,j]` sem uma linha de mecanismo novo. Então `dim` não é a
forma geral da qual o rank fixo seria um caso: é o mecanismo específico do rank
que varia, e existe porque com `tens(3)`, `tens(4)` e `tens(5)` não há como
escrever os acessores à mão.

Isso reduz o alcance dos errors 101 e 102, que passam a valer só sobre
modificador com `dim`, e tira do rank fixo as três coisas que a forma por vetor
custa: o literal composto que ninguém escreveu, a dependência de SROA para ele
sumir, e o laço que roda em `-O0`.

---

## 4.3 Forma dos tipos compostos


### Por que o nome canônico é fixado na declaração

A alternativa seria fixá-lo no uso — cada módulo nomearia a instância à sua
maneira, e o backend reconciliaria. Isso não fecha: `buffer geom.Point` escrito
em dois módulos tem de dar o **mesmo tipo C**, senão a atribuição entre eles
falha no compilador. Fixar na declaração é o que faz o nome ser função apenas do
tipo, e não de quem escreveu.

Daí também o argumento carregar a própria qualificação. `buffer Point` com
`Point` injetado por `types` e `buffer geom.Point` são a mesma instância, porque
o nome canônico do argumento é `geom.Point` nos dois casos — `types` é
puramente de fonte (§4.1), e a identidade não pode depender de uma escolha
de escrita.

### Por que a constante de enum é escopada no tipo, e não no módulo

O torto original é do C: ele pôs constante de enum no **espaço ordinário**, junto
com funções e variáveis, em vez de escopá-la no tipo. Isso força o prefixo, e o
prefixo de módulo sozinho não bastaria — dois enums do mesmo módulo com uma
constante de mesmo nome continuariam colidindo, e `Estado.PARADO` ao lado de
`Tarefa.PARADO` é caso corriqueiro.

Escopar no tipo resolve os dois de uma vez, e tem uma propriedade que decide:
**`PARADO` nunca aparece nu**, então não há o que colidir com macro da libc. Se a
constante fosse injetada nua por `types`, um `SEEK_SET` reescrito faria a macro
deixar de expandir — sem erro em lugar nenhum (§4.1).

E o mangling não precisou de exceção: o nome canônico composto cai na mesma regra
recursiva que dá `keel_buffer_geom_Point`. O `cofsm` já fazia assim para as
constantes do seu enum de estados; a regra deixou de ter caso especial.

### Por que o argumento nunca contém `*` — a consequência

A regra está na §3, e aqui está o que ela compra. Como `*` e `[N]` pertencem ao
declarador, **o argumento nunca contém `*`** — e isso elimina de saída a colisão
entre `const char *` e `char *const`, dois tipos C distintos que uma normalização
ingênua funde num nome canônico só.

O preço é `typedef i32 *pint;` para ter um buffer de ponteiros, e é onde essa
complexidade pertence: quem quer um contêiner de ponteiros está declarando um
tipo novo, e escrever isso torna a intenção visível.

### Por que declarador composto não registra símbolo

`buffer i32 (*f)(void);` é ponteiro para função que devolve um buffer. O tipo
precisa ser substituído e a instância precisa ser gerada — senão o C não compila
—, mas `f` não é contêiner, e registrá-lo faria `buffer.length(f())` parecer
legal.

Não foi preciso proibir a declaração para chegar aí: `f()` é chamada de função C,
que não está na gramática de contêiner, então o error 19 já a recusa com a
mensagem certa. O parser continua pequeno onde importa, sem recusar forma que o C
escreve e que o lowering resolve por substituição.

### Por que não há ciclo possível no aninhamento

Vale registrar porque parece um risco e não é. O tipo é escrito **por extenso** —
`buffer slice i32` —, então a profundidade é finita por construção e a ordem de
geração é topológica sem ninguém calcular nada. Recursão só apareceria se um
modificador pudesse se referir a si mesmo na **definição**, e isso é error
separado (§4.9).

O limite prático é o teto de identificador do alvo, e a 255 caracteres ele deixa
de ser restrição sentida.

### Capacidade fixa: uma decisão da biblioteca que o núcleo cobra

`slice` não cresce e `buffer` não realoca — isso parece regra da Parte II, e é.
Mas ela é **pré-requisito de duas coisas do núcleo**, e por isso está enunciada
aqui.

E vista derivada de contêiner nunca fica pendurada — o problema clássico de
iterador invalidado por crescimento não existe, porque não há crescimento. É a
decisão da biblioteca pagando juros num lugar onde ninguém a tinha cobrado.

---

### Por que a autorreferência é legal

`struct Val { slice Val itens; }` é a forma de todo valor de linguagem funcional,
de todo nó de árvore com filhos e de todo caminhador de diretório. Recusá-la
custaria caro e não compraria nada: o descritor guarda `Val *`, e ponteiro para
tipo incompleto é C desde sempre.

A circularidade aparente se desfaz sozinha porque as duas metades da instância
pedem coisas diferentes — a struct do descritor pede a **declaração**, os verbos
que devolvem `T` por valor pedem a **definição**. Existe uma ordem que satisfaz
as duas, ela é única, e é do backend emiti-la. Em keel a exigência é a do C: o
`typedef` adiantado vem antes, e quem cobra é o compilador C.

### Por que a colheita desce por agregado keel, mas não por `union`

Colher campo cujo tipo é agregado que keel declarou não alarga a invariante: são
nomes que keel já tinha na tabela, e nenhum header C entra por aí. O que se ganha
é o caminho de contêiner com mais de um nível — `s->pool->ns` —, que é o formato
de toda struct de contexto. Sem isso, parser, compilador e servidor pagam um
parâmetro a mais em cada função por uma regra que não protegia nada.

`union` é outra história, e fica de fora por dois motivos que se somam. O
primeiro é que campo de `union` não tem tempo de vida próprio: o mesmo
`slice u32` seria válido ou lixo conforme a tag, e nenhum verbo de keel tem como
saber qual. O segundo é que o caminho de campo passaria a atravessar membro
anônimo, e a tabela deixaria de ser uma lista de nomes para virar uma árvore de
posições. O nó plano — tag e campos no topo — é a forma que orientação a dados
quer de todo jeito, e agora ela também é a forma que a linguagem sabe descrever.

---

## 4.4 Memória
### O modelo


#### Por que três regras bastam

A pergunta natural é o que falta — e a resposta é que o que faltaria só existe no
outro modelo. Num modelo de posse individual seria preciso dizer quem libera,
quando, o que acontece com cópias, o que acontece com ciclos, e o que acontece
quando dois donos discordam. Nada disso tem lugar aqui, e o motivo é uma frase:
**é a ausência de aquisição individual que faz a arena não precisar de nenhum
mecanismo que um alocador com dono precisaria.**

`unique`, `shared` e `weak` foram descartados por isso, não adiados. Eles
pertencem ao modelo em que cada objeto tem dono e alguém o libera; num modelo de
grupo ninguém libera um ponteiro, e a contagem por objeto não tem o que fazer. O
único caso legítimo que sobrava — distinguir ponteiro para **um** elemento de
ponteiro para muitos — não é sobre posse, é sobre aritmética, e virou o `ref`
(§4.2).

#### Por que a checagem de escape não é redundância com o `cc1`

Este é o C que sairia se ela não existisse:

```c
/* o que sairia, se a checagem não existisse */
static keel_slice_i32 sim_tmp(void) {
    keel_arena s = {0};
    unsigned char keel__st0[4096];
    keel_arena_from_array(&s, keel__st0, sizeof keel__st0);
    keel_buffer_i32 b = keel_buffer_i32_as(
        (i32 *)keel_arena_alloc_n(&s, 16, sizeof(i32), alignof(i32)), 16);
    return keel_buffer_i32_as_slice(&b);   /* .ptr aponta para keel__st0, que morre aqui */
}
```

O endereço morto sai **dentro de uma struct**. GCC e Clang avisam em
`return &local` e em `return local_array`; ponteiro em campo de struct retornada
escapa dessa análise no caso geral. Não é redundância com o C — é cobrir um
buraco que o descritor criou. O contêiner que carrega os próprios limites é o que
torna a linguagem útil, e é também o que esconde o endereço do compilador; a
verificação é o preço dessa troca, e ela é paga na camada que sabe de onde a
arena veio.

#### Por que o limite fica documentado em vez de fechado

`*out = slice.of(b);` escapa exatamente igual e não é diagnosticado. Fechá-lo
exigiria análise de fluxo — saber que `out` aponta para fora do frame —, e isso
não é uma verificação a mais: é uma categoria de análise que a camada não tem, e
que arrastaria consigo a necessidade de tipar expressão.

A escolha é conservadora nos dois sentidos. Onde a procedência é conhecida,
diagnostica; onde o símbolo foi reatribuído de outra origem, cala. Um falso
positivo custaria mais que o furo: seria a linguagem recusando programa correto,
que é o que o princípio 5 evita ao mandar rejeitar só o que não se sabe traduzir.

#### A fronteira com o modelo individual não é limitação da linguagem

`getline` é o exemplo canônico porque a incompatibilidade é **silenciosa até a
primeira linha longa**: ele `realloc`a sempre que a linha não couber, então um
ponteiro vindo de `arena.alloc` compila, roda enquanto as linhas forem curtas, e
quebra depois. É a pior forma de erro — a que passa no teste.

Mas isso existiria igual num programa C que misturasse arena e `malloc`; não é
keel que a cria. O que keel acrescenta é o **lugar onde escrevê-la**:
`slice.from` no ponto de chamada, `defer` para o cleanup. A fronteira deixa de
ser convenção lembrada e passa a ser duas linhas visíveis.

---

### `arena`


#### Por que arena por valor é pior que buffer por valor

O diagnóstico é o mesmo — `byref-param` — e a razão é a mesma: os dois são
descritores, e descritor copiado é descritor divergente. Mas a consequência é de
outra ordem.

```c
/* o que sairia, se a checagem não existisse */
static void sim_f(keel_arena a) {
    keel_arena_alloc_n(&a, 100, sizeof(i32), alignof(i32));   /* sobe o `top` da cópia */
}
```

Uma cópia do descritor tem o próprio `top`. Alocar nela sobe o topo da cópia; o
do chamador fica onde estava, e a alocação seguinte dele **devolve os mesmos
bytes**. Onde o `buffer` perde um `len` — dado que não aparece —, a arena entrega
duas partes do programa escrevendo na mesma memória, sem que nada acuse.

As duas regras somem juntas no dia em que arena e buffer forem
`modifier … byref` (§4.9): é o mesmo bit, e ter um nome só para as duas desde
já é o que garante que a migração não invente vocabulário.

#### Por que não existe uma quinta forma de nomear a origem

A alternativa tentadora é keel arranjar memória sozinho quando o tamanho não for
constante — um pool estático implícito, digamos. Não se faz, por três razões que
se somam.

O **nome do construtor deixaria de dizer a origem**, que é a única informação que
o leitor do ponto de chamada precisa. A **procedência para a análise de escape**
(§4.4) ficaria indefinida, porque a tabela de lá decide olhando qual
construtor foi usado — um pool implícito não tem linha naquela tabela. E ninguém
escreveria esse pool escondido à mão, o que o põe contra o princípio 2.

> **O programa tem que nomear uma fonte de memória em algum ponto**, e há
> exatamente quatro maneiras de nomear: um `array u8`, um pai, um vetor constante
> no frame, ou uma região crua vinda do C.

#### Por que `u8` e não qualquer vetor

Parece higiene de nome e não é. Sobre um `array f32 x[100]`, a chamada
reaproveitaria como bytes um vetor que talvez ainda esteja vivo como float — e
exigir `u8` faz a declaração dizer que aquilo é armazenamento bruto e nada mais.
É esse *segundo leitor* que o `u8` elimina: onde há um vetor vivo com outro tipo,
há dois caminhos de acesso com tipos disjuntos, e é aí que o otimizador tem
licença para reordenar. Sobre `u8` não há segundo leitor a proteger.

**Aqui houve um erro, e ele está registrado porque a correção move um argumento.**
A edição anterior dizia que o argumento decisivo era do C — *tipo-caractere pode
aliasar qualquer tipo de objeto* — e que era essa regra que tornava legal a arena
entregar um pedaço do próprio armazenamento como `Particle *`. **A regra existe e
vale no outro sentido.** Ela permite ler a representação de um objeto **por**
bytes; não dá tipo efetivo a um vetor de bytes declarado. Objeto com tipo
declarado tem esse tipo como efetivo, e só armazenamento **alocado** recebe tipo
pela escrita — que é por que `from_memory` sobre `malloc` é a única das quatro
origens estritamente conforme.

O erro não era conservador, ao contrário do 63 do backend: ele sustentava dois
construtores centrais com uma permissão que não existe. Corrigi-lo não muda
nenhum programa e não muda nenhum byte gerado — muda o que a spec promete.

#### Por que a suposição é nomeada, e não escondida

Descoberto o buraco, havia três saídas. **Restringir** — `from_array` e
`from_stack` só para `u8` puro, e tipo não-caractere exigindo `from_memory` —
devolveria `malloc` ao centro do design e mataria o caso embarcado, que é onde a
arena mais serve. **Calar** manteria uma frase falsa sustentando a base da base.
Sobrou **nomear**, e a §7.1 já tinha a forma: *"sem extensão de compilador, salvo
onde uma seção diz o contrário e nomeia a guarda"*.

Não há rota padrão para as outras três origens, e vale dizer que foram
procuradas: união não resolve — o tipo efetivo passaria a ser a união, e
`Particle` não é membro dela —; `max_align_t[]` troca um tipo declarado por
outro; e emitir a união dos tipos efetivamente alocados exigiria keel saber quais
são, o que atravessa chamada de função e morre no princípio 7. A única saída
conforme em C é `malloc`.

A suposição também não é exótica: é a de todo alocador escrito em C, e é uma das
áreas que o próprio comitê reconhece mal servida pelo modelo de tipo efetivo, que
foi escrito para objetos e não para quem os fabrica. O que separa keel de quem
apenas a comete é que aqui ela está **escrita, delimitada e com dono**.

#### Por que honrá-la é do backend, e não da linguagem

A spec diz que a arena entrega objetos sobre armazenamento que ela não alocou.
*Qual* armazenamento sai, com que alinhamento, e qual é o remédio de build num
alvo em que a suposição não se sustente, é decisão do alvo — e decisão do alvo é
de `keel-c-backend.md`, pela divisão do §7 da spec.

É a mesma separação que já existe em `f32`: `f32` **é** binary32, e
`typedef float f32;` mais o `static_assert` são como *este* backend entrega
binary32. Um backend cujo alvo ofereça armazenamento sem tipo declarado toma essa
rota e não deve nada; o backend C não tem essa rota, e por isso paga com uma
guarda nomeada e um remédio documentado. Sem a separação, trocar de backend
obrigaria a reescrever a definição da arena — e definição de construção não
deveria se mover porque a saída mudou.

#### Por que a invalidação é semântica, e não proteção

`reset` do pai invalida a filha, e keel não faz nada a respeito em tempo de
execução. Não é omissão: pela regra 2 do §4.4, keel não rastreia quem recortou
de quem, e ponteiro entregue por `arena.alloc` é **endereço nu** — nenhum estado
dentro da struct alcançaria os ponteiros já distribuídos. Uma proteção real
exigiria indireção em todo acesso, que é o oposto do que a arena existe para
oferecer.

O que resta é o caso léxico, e ele é pego com precisão porque o alvo foi escolhido
com cuidado: **não é o `reset`, é o uso da filha depois dele**. Proibir o `reset`
mataria o idioma que devolve o recorte, que é o uso correto e frequente. Proibir
o uso posterior custa uma varredura em ordem de fonte e não recusa nenhum
programa legítimo.

Fora do escopo — pai recebido por referência e resetado num callee — não há
diagnóstico, e é deliberado: resetar arena que se recebeu é padrão legítimo, o de
um agregador que processa em etapas, e proibi-lo custaria mais do que o erro que
evitaria.

#### Por que a contagem não é multiplicada no ponto de chamada

`arena.alloc(a, T, n)` poderia emitir `keel_arena_alloc(&a, n * sizeof(T), …)`, e a
leitura seria mais direta. Não emite, porque **`n` quase nunca é uma constante do
programa**: ele vem de cabeçalho de arquivo, de campo de pacote, de argumento de
linha de comando. É a superfície clássica, e o modo de falha é o pior que existe
— o produto transborda, a arena entrega uma região pequena, o programa acredita
ter `n` elementos e escreve além dela. Nada acusa: a alocação **teve sucesso**.

Em 64 bits o caso parece hipotético, e para um `f32` exige `n` acima de 4,6·10¹⁸.
Em **32 bits** — que é alvo de primeira classe aqui — o mesmo `f32` transborda em
`n > 1,07·10⁹`, que é um vetor de um bilhão de elementos. Isso não é hipótese, é
tamanho de dado real numa máquina em que ele não cabe.

Passar `n` e `sizeof(T)` separados é a forma do `calloc`, e por isso mesmo: a
biblioteca C resolveu este problema exatamente aqui, e resolveu assim. Com
`sizeof(T)` constante no ponto de chamada, o teste vira uma comparação contra uma
constante quando a função é inlinada; sem inline, é uma divisão por alocação —
custo irrelevante para uma operação que serve blocos.

**E a verificação é uma só, num lugar só.** Vale a pena dizer por que não vira
uma família: `arena.alloc` é o único verbo em que keel escreve a multiplicação.
`clone` multiplica um comprimento que já coube na memória uma vez, logo não
transborda por construção; `buffer.from(p, cap)`, `slice.from(T, p, n)` e
`arena.from_parent` recebem contagem ou bytes e não multiplicam nada. A
superfície é de um ponto, e é por isso que fechá-la custa uma linha.

**Por que `NULL` e não diagnóstico de tradução.** Porque `n` é de tempo de
execução — não há o que decidir na tradução. O canal já existe, o
`[[nodiscard]]` já obriga a olhar, e quem trata falta de capacidade trata
transbordamento pelo mesmo caminho, sem aprender nada novo. O que se acrescenta é
o diagnóstico de **debug**, pela razão que o alinhamento já tinha estabelecido: o
`NULL` diz que falhou e não diz por quê, e as três razões pedem correções
diferentes.

#### Por que o sobre-alinhamento se pede no tipo

Um `alloc` que pedisse alinhamento por argumento espalharia a exigência pelos
pontos de chamada, e um `arena.alloc(a, Vec8, n)` esquecido em outro arquivo estaria
errado sem que nada acusasse. Quem exige 32 bytes é o **tipo** — carga vetorial,
*store* não-temporal e linha de cache são propriedades do dado. Declarado uma vez
no `alignas` do struct, o compilador C confere em toda parte.

Daí também não faltar uma quarta posição em `arena.alloc`: ela existiria para
repetir, em cada chamada, uma informação que já está na declaração.

#### Por que a arena não desce para módulo comum

A grafia diz que ela é biblioteca: `arena` é nomeada e despachada pela regra
geral, e a migração para módulo escrito em keel não mudaria uma letra de nome
nenhum. Mas as quatro verificações da §4.4 não são expressáveis pelo
protocolo de despacho — duas exigem saber qual construtor produziu o símbolo, uma
exige varredura em ordem de fonte, e uma exige a fase que varre o arquivo inteiro
antes de resolver.

Para a arena descer de verdade, o protocolo teria que **ganhar vocabulário** para
dizer essas quatro coisas — e desenhar esse vocabulário é onde traits nascem. É
trabalho de outra rodada, e a categoria "privilegiada" existe para dizer isso em
voz alta em vez de fingir que a linha já está do lado certo.

**Arena tipada — pool de slots fixos — fica de fora, e não é a mesma coisa
adiada.** É outra estrutura de dados, com outros benefícios, e depende apenas de
a arena ter virado módulo: aí ela é um modificador de biblioteca como qualquer
outro, sem construção nova na linguagem.

---

## 4.5 Contêineres


### Por que a capacidade é fixa

É a filosofia do C aplicada ao descritor: tamanho é fixo, mas pode ser
determinado em tempo de execução no ponto de criação. Uma vez fixado, não muda.

A consequência é a que sustenta o resto da linguagem: **vista derivada de
contêiner nunca fica pendurada por crescimento, porque não há crescimento.** É
isso que faz `slice` poder ser copiado à vontade.

Crescimento automático exigiria realocação, realocação exigiria um alocador com
dono, e alocador com dono é o modelo que a arena existe para não precisar
(§4.4).

### Por que o OOM não precisou de tipo de erro

Se `arena.alloc` devolveu `NULL`, o buffer nasce com `cap == 0`: `push` falha
limpo, `buffer.capacity(xs) == 0` é o teste, e nada fica indefinido. Um
`outcome T` ali seria uma struct a mais para transportar a informação que o
ponteiro nulo já carrega.

O que isso custa é que `alloc` e `push` ficam **fora** da cláusula `else`, que só
alcança tipo que declara `failed` (§4.7) — e ponteiro não declara, nem
poderia, com a polaridade invertida. É um `if` escrito, na linha seguinte, e é o
mesmo `if` que se escreveria em C. Embrulhar o retorno para ganhar a cláusula
seria pagar uma struct em todo caminho de alocação para poupar uma linha em
alguns; a conta não fecha, e o ponteiro nulo continua sendo a resposta certa.

### Por que não existe `set_length`

`length` de um buffer significa *quantos elementos foram empilhados*; quando o C
escreve por baixo, o buffer genuinamente não sabe. Afirmar um comprimento que ele
nunca ganhou quebraria a invariante que os verbos existem para manter, e um
`push` seguinte apenderia depois dela.

O que sobra é `slice.from`, e o ganho é que a passagem por fora fica **visível no
ponto de chamada**: quem leu do `read` sabe que está prometendo o tamanho. O custo
é não poder `push` depois de o C ter preenchido um prefixo — uso raro e confuso,
que fica de fora.

### Por que `buffer` sobre vetor C `const` é error

`clear` seguido de `push` escreveria em memória const. Não é caso de aliasing nem
de otimização: é escrita em objeto declarado imutável, comportamento indefinido
do C, e a saída é uma linha — vetor const entra como `slice const char`.

### Por que `range` não é núcleo

O intervalo tem sintaxe própria, e a tentação é concluir que o tipo é da
linguagem. Mas o princípio 8 pergunta outra coisa: *nenhum módulo keel poderia escrever
isto?* A forma `a..b`, não — é regra léxica e produção de gramática. O tipo, sim,
e trivialmente.

Privilegiá-lo custaria as duas coisas que a seção entrega: `first`/`limit`
perderia a função, porque o lowering leria o campo direto, e a forma de um binder
passaria a valer para **um** tipo em vez de para qualquer um que declare os
verbos. Em troca de nada — as formas seriam núcleo de todo jeito.

É a mesma divisão de `"abc"` e `string`: a forma é da linguagem, o tipo é da
biblioteca.

### Por que `range` não declara `ptr`

Um intervalo não tem armazenamento: `first + i` é calculado, não guardado. A
consequência é que `r[i]` não existe e o binder por ponteiro não se aplica — e as
duas saem do protocolo, sem caso especial e sem marcador de "só por valor".

**A ausência do verbo é a informação.** Anotá-la seria dar nome a metade do
conjunto, que é exatamente o que a disciplina do §4.7 existe para evitar.

### Por que a linearização de um módulo é interna

No dia em que uma matriz alinhar cada linha a múltiplo de oito elementos para
carga vetorial, `i * colunas + j` vira `i * passo + j`. Todo verbo do módulo
continua certo; todo `slice.of(dados(m), i*cols, (i+1)*cols)` escrito à mão passa
a ler lixo, **em silêncio**.

É a mesma opacidade que o warning 50 cobra dos campos gerados, aplicada ao layout
que o módulo escolheu. E há uma assimetria que morde e não aparece na sintaxe: em
row-major denso a linha é contígua e a coluna não é — uma pode devolver `slice`,
a outra precisa de passo.

---

## 4.7 Fluxo
### `defer`


#### Por que léxico, e não uma pilha em tempo de execução

A alternativa óbvia — registrar os cleanups numa pilha e desenrolá-la na saída —
custa uma estrutura de dados por frame, uma alocação ou um limite arbitrário, e
um caminho de código que não aparece no C gerado. Nada disso sobrevive ao
princípio de que o C gerado é legível e previsível: quem lê a saída veria chamadas
a um runtime, não a chamada que escreveu.

Sendo léxico, `defer` não é uma construção nova de execução — é uma reescrita.
O custo é zero e o código gerado contém exatamente as chamadas que o fonte
contém, repetidas nos pontos de saída. O preço é que tudo que a análise estática
não enxerga escapa: por isso os limites de `longjmp` e de `return` vindo de macro
são *limites*, não bugs.

#### Por que o temporário de retorno leva o tipo escrito, e por que ele sai sempre

Duas perguntas, uma resposta só: o parser não olha a expressão do `return`.

O tipo, então, vem da assinatura — e aí está o problema que decidiu a forma: **tipo
do C não se compõe por prefixo.** `size_t` cabe na frente de um nome; `i32
(*)(void)` não — a declaração correta seria `i32 (*keel__rv0)(void) = expr;`, com o
nome no meio. Havia três saídas e a terceira é a que ficou:

- **Remontar o declarador** para cada forma de retorno. É reimplementar a gramática
  de declaração do C dentro de keel, e o princípio 3 recusa.
- **`auto` do C23**, que devolve o tipo da expressão sem que ninguém o escreva.
  Funciona, e foi o que valeu por uma edição — ao preço de prender o alvo em C23
  por causa de um temporário.
- **Copiar os tokens quando o nome está no fim, e recusar quando não está.** É a
  que ficou. `size_t f(…)` dá `size_t keel__rv0 = expr;` por cópia; `int
  (*f(void))[10]` é o error 120, com `note` mandando usar `typedef`.

A terceira parecia a mais pobre e é a mais coerente, porque **o mesmo problema
aparece em três lugares** — aqui, no `constexpr` (§4.2) e na captura do `[now]`. Nos
três keel precisa reconstruir uma declaração a partir de um tipo que o usuário
escreveu, e nos três a reconstrução só é possível com o nome no fim. Uma regra, um
diagnóstico, três consumidores; e a linguagem parou de depender de duas palavras do
C23 para materializar duas cópias.

O contorno é `typedef`, e keel já o exige em situação idêntica: `buffer i32 *x` é
ponteiro para buffer, e buffer de ponteiros pede `typedef i32 *pint;` (§4.3). Onde
a grafia do declarador esconde a intenção, a linguagem manda nomear o tipo — não é
regra nova, é a mesma.

E sair sempre é mais barato que decidir quando não sair. `return maior;`, com
`maior` local e `fclose` sem acesso a ela, não precisaria do temporário — mas
estabelecer isso exige saber o que o cleanup faz, e keel não olha para dentro de
`fclose`. A aproximação léxica existe e é correta — "identificador local desta
função cujo endereço não aparece no corpo" —, só que ela custa uma varredura
nova, uma exceção na regra e um caso a mais para o usuário aprender. O que ela
compra é uma cópia que o compilador C elimina antes de emitir instrução. A troca
não vale.

O preço fica à vista, e é aceito: `{ size_t keel__rv0 = maior; fclose(fp); return
keel__rv0; }` onde `fclose(fp); return maior;` bastaria. É código que ninguém
escreveria à mão — e o C gerado não é para ser escrito à mão, é para ser lido.
Ler uma cópia supérflua custa menos que ler duas formas diferentes e ter de
descobrir qual regra escolheu cada uma.

**A função `void` é o mesmo teste, não uma exceção.** `return fn();` com `fn`
devolvendo `void` é C válido, e ali `void keel__rv0 = fn();` não compila. Isso não
é descobrível pela expressão — `fn` pode vir do C, opaca. É descobrível pela
assinatura da função que está sendo traduzida, que keel já analisou. Daí a forma
se ancorar no tipo de retorno da função: é o único lugar onde a informação existe
sem inspecionar coisa nenhuma. E `expr` continua sendo avaliada, como statement,
antes do cleanup — o valor é que não existe.

#### Por que o `return` de macro está registrado como limite

`longjmp` é raro e todo mundo sabe que ele pula cleanup. O `return` de dentro de
macro não é raro: o idioma `#define TRY(e) if ((rc = (e)) < 0) return rc;` é comum
justamente no código que `defer` veio substituir — abrir recurso, testar, sair.
Quem migra esse código para keel escreve `defer fclose(fp);` e mantém o `TRY`, e o
cleanup some sem aviso. Não há como avisar: o §1.4 põe keel antes do
pré-processador, e do ponto de vista de keel `TRY(f())` é uma chamada. Registrar o
limite é a única coisa que a spec pode fazer por esse usuário.

E vale dizer o que a regra **não** está julgando: `TRY`/`CATCH` por macro é C
perfeitamente válido, e continua compilando sob keel como sempre compilou. O
ponto é outro — é que ele deixa de ser necessário. O idioma existe porque C não
tem saída de escopo com limpeza garantida, e a macro é a forma de fabricar uma;
`defer` é essa saída na linguagem, com o cleanup visível na linha em que o
recurso é adquirido em vez de espalhado por cada ponto de retorno. É RAII sem
destrutor, sem tipo e sem tabela: o benefício que o C++ dá amarrando limpeza ao
tempo de vida de um objeto, obtido aqui amarrando-a ao escopo léxico — que é a
metade que os programas em C realmente usam.

O atrito é justamente o da migração, não o da convivência: quem mantém o `TRY` e
adota o `defer` fica com um retorno que keel não enxerga limpando um recurso que
keel prometeu limpar. Trocar a macro pelo `defer` resolve os dois de uma vez.

#### Por que corpo de controle sem chaves é error

```keel
if (p) defer free(p);
```

O cleanup é emitido nos pontos de saída do escopo, e o escopo aqui é o da função —
`defer` sem bloco não cria escopo próprio. O `free(p)` sairia em todos os pontos
de saída, inclusive nos alcançados quando `p` era nulo e o `defer` nunca foi
registrado:

```c
/* o que sairia, se a checagem não existisse */
void f(int *p) {
    if (q) { free(p); return; }    /* p nulo: o defer não rodou, o free sai igual */
    free(p);
}
```

A alternativa seria uma flag booleana por `defer` condicional, testada em cada
saída. Isso contradiz o "sem pilha em runtime" e, pior, esconde o mecanismo: o C
gerado ganharia variáveis e `if`s que o usuário não escreveu. Entre gerar código
errado, gerar código escondido e recusar, a recusa é a única que preserva os dois
princípios. E a correção é de um caractere: pôr as chaves.

#### Por que com chaves é warning, e não error

`if (p) { defer free(p); }` está **correto**: o cleanup roda ao sair do bloco do
`if`, que é bem definido e às vezes é exatamente o que se quer. Erro seria recusar
código válido. Mas a leitura ingênua da linha é "libera `p` quando a função
terminar" — e a distância entre as duas leituras é um bug de recurso mantido vivo
tempo demais, ou liberado cedo demais. O warning existe para nomear o escopo, não
para desencorajar a construção; por isso a mensagem diz *onde* o cleanup roda em
vez de sugerir mudança.

**Corpo de laço não avisa** porque ali a execução por iteração é o uso idiomático,
não a surpresa: `mark` no topo, `defer reset` logo abaixo, e a arena volta ao
mesmo ponto a cada volta. Avisar nesse caso treinaria o usuário a desligar o
diagnóstico, e ele deixaria de valer no caso do `if`.

#### Por que salto sobre `defer` pendente é error, e não heurística

Um salto que entra num escopo por cima do ponto de registro deixa o escopo com
cleanup pendente que nunca foi registrado, ou pula a saída de um que foi. Dá para
tentar classificar caso a caso — saltos para trás, para a frente, entre irmãos —
mas cada classificação vira uma regra que o usuário teria de aprender para
entender por que o seu salto específico passou e o do colega não. Salto sobre
escopo com `defer` é raro o bastante para que a recusa uniforme custe pouco, e a
uniformidade é o que torna a regra explicável em uma linha.

#### Por que a regra é sobre **salto**, e não sobre `goto`

A edição anterior nomeava o `goto` e parava ali, e o buraco era o `switch`:

```keel
switch (op) {
case 0: defer cleanup();
case 1: break;              /* entrou por cima do registro */
}
```

`case` é rótulo, e entrar por ele é entrar no meio do bloco exatamente como um
`goto` — só que sem a palavra que faria o leitor desconfiar. A varredura textual
acha um `defer` anterior ao `break` e emite o cleanup; em execução, o registro
nunca aconteceu.

Enunciar a regra sobre **salto** fecha isso e melhora o argumento, porque a lista
do C é fechada: há exatamente duas maneiras de entrar no meio de um bloco, `goto`
e rótulo de `case`. Cobertas as duas, a propriedade é **total** — *escopo com
`defer` pendente tem uma entrada só* —, e não uma aproximação conservadora. É a
mesma forma da regra que o C tem para tipo variavelmente modificado, e a mesma
razão.

E o predicado é exato, o que não era esperado. A condição é *rótulo `case`
posterior a um `defer` do mesmo corpo de `switch`* — e ela recusa só os programas
em que existe entrada por cima do registro:

| Forma | | |
| --- | --- | --- |
| `case 0: defer c(); … case 1: break;` | recusa | certo: `case 1` pula o registro |
| `case 0: break; case 1: defer c(); …` | aceita | certo: o `break` é anterior ao registro e não recebe cleanup |
| `case 0: case 1: defer c(); break;` | aceita | certo: o fallthrough registra antes de sair |

Não é recusa conservadora com falso positivo tolerado: é o predicado certo. E a
custo baixo — a varredura do corpo do `switch` já acontece para montar a pilha de
`defer`, e o que se acrescenta é **uma bandeira por corpo de `switch`**, sem o
lookup de rótulo que o `goto` exige. É a análise mais barata da linguagem.

#### Por que sombreamento sob `defer` implícito é error

A forma sem `[now]` não copia nada — o corpo é colado nos pontos de saída e
resolvido lá. Se um escopo mais interno redeclara um nome que o corpo usa e tem
ponto de saída, o cleanup liga ao símbolo errado, **em silêncio**: compila, roda,
e faz outra coisa. Era o único dos problemas do `defer` que não produzia
diagnóstico nenhum.

**A razão decisiva não é essa, e só apareceu com o §5.5.2 do backend.** Ali o
cleanup passou a sair de duas formas — escada de rótulos quando toda saída é
`return` ou fim natural, inline nas demais. E as duas **discordam** sob
sombreamento: a escada emite o corpo uma vez, no fim do escopo do `defer`, onde o
símbolo interno já saiu de escopo e a ligação está certa; o inline cola em cada
saída, inclusive dentro do escopo que sombreia. Sem o error 123, a forma de
emissão mudaria o sentido do programa, e um lowering que muda sentido não é
lowering. **É a recusa que mantém as duas formas equivalentes**, e é isso que a
promove de higiene a necessidade.

Renomear na colagem resolveria e foi descartado: o corpo do `defer` é `<opaco>`
copiado verbatim, e reescrever símbolo dentro dele obrigaria keel a decidir o que
é uso de nome e o que é campo, designador ou membro — que é entender a expressão.
A recusa custa uma varredura e não custa invariante.

#### Por que o `goto` de dentro do escopo continua legal

O error 38 podia ter sido "qualquer `goto` em função com `defer` pendente", e a
recusa uniforme teria sido mais curta de enunciar. Foi estreitada de propósito,
porque a metade que ela deixa passar **é** a saída do 123:

```keel
int x = 1;
defer usa(x);
{ int x = 2; goto fim; }    /* origem dentro do escopo do rótulo: legal */
fim:
return;                     /* o cleanup sai aqui, com o `x` certo */
```

O perigo do 38 é entrar num escopo **sem ter passado** pelo registro, e isso
depende da **origem** do salto, não do rótulo. Salto de dentro do escopo para
rótulo do mesmo escopo já passou pelo registro em toda execução que o alcança.
Proibi-lo custaria o idioma que resolve o sombreamento sem `[now]`, e não
compraria segurança nenhuma.

E a lógica que isso desenha vale escrever, porque é ela que faz as duas regras
serem uma: **se o sombreamento é necessário, o ponto de saída não pertence ao
escopo que sombreia.** Levá-lo para o escopo do `defer` — que é o que o `goto`
faz — põe a ligação onde ela nunca esteve em dúvida, e o cleanup continua
automático. O programador escreve o salto; keel continua escrevendo o cleanup.

#### Por que a lista de captura só existe depois de `[now]`

Sem essa amarração a gramática fica ambígua:

```keel
defer (fp) (fclose(fp));
```

é indistinguível — para um parser de ilha, que não tipa expressão — entre "difere
`fclose(fp)` capturando `fp`" e "difere a expressão `(fp)(fclose(fp))`, uma
chamada por ponteiro de função". Resolver isso exigiria saber se `fp` é ponteiro
de função, isto é, tipar a expressão; é precisamente o que esta camada não faz.
`[now]` é o token que desambigua, e `later` com lista é error porque a lista sem
`now` não teria sentido — a captura só existe quando há um instante de cópia.

**E foi por isso que a lista acabou dentro do colchete.** Com ela do lado de fora,
`defer [now] (void)fn();` volta a ter duas leituras — `(void)` é lista de
parâmetros legítima e cast legítimo —, e separá-las exigiria casar os parênteses
para olhar o token seguinte. Dentro do colchete a ambiguidade não pode nascer:
depois do `]` vem sempre o código adiado. O ganho de forma veio junto e é maior
que o do parser — o `defer` passou a ter dois slots, `defer <opções> <corpo>`, em
vez de dois para `[later]` e três para `[now]`.

**O tipo escrito na entrada não foi concessão ao alvo C11**, embora tenha sido ele
a expor a questão. Deduzi-lo exigia `typeof_unqual` sobre um identificador cuja
declaração keel não examina — e a linguagem já resolve esse caso do mesmo jeito em
todo lugar onde ele aparece: `slice.from(char, p, n)` leva o tipo escrito porque
keel não conhece o de `linha`; `arena.alloc(a, T, n)` idem; a cláusula `else` só
existe em posição de declaração porque o teste precisa de um tipo escrito. A
captura por dedução era a exceção. Escrevê-la deixou a linguagem mais regular, e
de quebra a entrada virou exatamente o membro da struct que o backend emite
(`backend §5.5`), sem uma reescrita no meio.

O default é `[later]` porque é o comportamento que a maioria dos usos quer: o
`defer` fecha o recurso *que estiver ali no fim*, e reatribuir a variável entre o
registro e a saída deve mudar o que é fechado. `[now]` é para o caso oposto — o
laço que reatribui `fd` a cada volta e quer fechar o descritor daquela iteração.

### `foreach` e `apply`


#### Por que ele é construção e não açúcar

`foreach` é **o único ponto do programa em que keel sabe que uma travessia está
acontecendo**, e é essa propriedade — não a economia de digitação — que paga a
construção.

Ela cobrou juros duas vezes, e as duas a favor. Se o `foreach` é o único ponto em
que a travessia é conhecida, ele é também o único em que ela pode ser **partida**
— e o `parallel` (§4.7) é isso, mais nada: duas travessias que já existiam e
um `;`. E é o único ponto em que ela pode ser **protegida**: o error 71, que
recusa `push` sobre o contêiner percorrido, não teria onde morar num `for`
escrito à mão.

Escrito como açúcar sobre `length` e `get`, nada disso existiria — o laço voltaria
a ser texto, e keel voltaria a não saber o que ele faz.

#### O conjunto de verbos é um trait, e não pode virar um

Exigir `length` mais `get`/`ptr` de um tipo é o embrião de um sistema de traits.
A diferença que impede o crescimento é uma só: **o conjunto não recebe nome.**

Sem nome, ele não pode aparecer como restrição de parâmetro genérico — que é a
primeira linha da regra de fechamento do §4.9. No dia em que este parágrafo
ganhar a palavra `Container` com maiúscula, o passo seguinte é
`modifier G where Container`, e o passo depois é resolução por conformidade.

Com o intervalo os conjuntos passaram a ser dois, e a disciplina não mudou: dois
conjuntos sem nome ainda são zero traits. O dia perigoso continua sendo aquele em
que um deles vira `where`.

#### Por que o índice é obrigatório

A objeção óbvia é a verbosidade: quem não usa `i` escreve um binder à toa. A
resposta está no C gerado, e é concreta.

```c
/* o que sairia, se o binder fosse opcional */
for (size_t keel__i0 = 0; keel__i0 < keel__n0; keel__i0++) {
    i32 v = keel_buffer_i32_get(keel__c0, keel__i0);
```

`keel__i0` fica na região que o usuário mais lê quando o compilador C reclama de
alguma coisa — dentro do corpo dele, num nome que ele não escolheu. Exigir o
binder apaga uma família inteira de nome gerado, e não custa nada: `i` é lida e
incrementada pelo próprio `for`, então nunca dispara aviso de variável não usada.

O argumento é o mesmo que faz o `cofsm` exigir nome e variável de estado, e o
`parallel` exigir nome: **onde o gerado precisaria inventar um símbolo, a
construção pede o nome.**

#### Por que a forma de intervalo tem um binder e não dois

Porque sobre um intervalo o valor **é** o índice, e dois nomes ali seriam dois
nomes para a mesma contagem. Não é exceção à regra do binder obrigatório — é a
regra sobre um lowering diferente: o `keel__i0` que ela recusa não chega a
existir.

E é o que decide a leitura da forma inteira sem tipar nada: a vírgula antes do
`:` está à vista, e é ela que diz se o que vem depois é percorrível ou contável.

#### Por que `auto` é copiado e só o literal é substituído

Nas três formas com tipo escrito, keel emite a palavra verbatim e nunca aprende o
tipo de nada — não há tabela de conversão a consultar, nem decisão sobre
`unsigned long` contra `size_t`. É o §1.3 na letra.

A substituição existe num caso só, e por uma razão que não é de tipos: `auto i =
1` deduziria `int`, e o laço passaria a contar em `int` sobre limites `size_t`.
Escrever `size_t` ali deixa o C mais informativo do que `auto` deixaria, e é a
única linha em que keel decide um tipo — sobre um literal que ele mesmo produziu.

#### Por que a travessia é sempre linear

Percorrer uma matriz de uma vez exigiria a construção escolher a ordem dos eixos,
e ordem de varredura é decisão de desempenho — cache, vetorização, layout. O
princípio 6 manda deixá-la visível, então o que se escreve é um laço por
dimensão, cada um sobre algo contíguo.

O intervalo não muda isso: ele recorta o **linear**, e quem dá linha de agregado
de rank ≥ 2 é o acessor parcial do módulo que possui a linearização (§4.10). É a
mesma resposta que o §4.6 dá para `x[i, a..b]` não existir.

#### Por que `apply` carrega o `T`

Ele não é redundante com o tipo do elemento, que o contêiner já dá: escolhe entre
**valor e ponteiro**. Deduzi-lo exigiria olhar a assinatura de `fn`, que pode vir
de `extern_c`, onde só o nome é conhecido — a invariante do §1.3 fecha o caminho.

Que o argumento seja exatamente o binder do `foreach` sem o nome não é economia
de gramática: é o que garante que as duas formas não divirjam. `apply` é
`foreach` com o corpo fixo, e tudo que ele decide sobre travessia já estava
decidido — inclusive o índice obrigatório, porque duas convenções para a mesma
travessia seria a única coisa a se ganhar recusando.

#### Por que o error 70 tem número próprio

Ele é o 25 (`get` sobre elemento-contêiner) visto de outro ângulo, e a checagem é
a mesma. O que muda é a saída: no `foreach` ela não é `ptr(grid, 3)`, é o `*` no
binder. Mensagem que manda usar `ptr` dentro de um `foreach` manda o usuário
desfazer a construção — daí o número separado.

---

#### Por que `apply` ganhou contexto

`apply` sem contexto era `foreach` cujo corpo é uma chamada que não captura nada
— e "não captura nada" é a mesma coisa que a tabela do §1.3 diz que keel não faz.
Na prática isso o deixava sem uso: travessia de programa real quer a arena, o
buffer de saída, a tabela de símbolos. Escrito assim, ele era a única palavra do
núcleo cuja ausência não se notaria.

Os argumentos de contexto custam zero à invariante, porque são `<opaco>`: keel
não os conta, não os examina e não os reordena. E eles dão a `apply` a razão de
existir que faltava, que não é brevidade:

> No `foreach`, o contexto **atravessa** a construção sem ser escrito — o corpo é
> léxico e alcança o escopo inteiro. No `apply`, o contexto é **declarado** no
> ponto de chamada, porque o corpo é outra função.

São dois regimes de acoplamento, e a escolha entre eles é de projeto, não de
gosto. Quem quer a travessia acoplada ao escopo escreve `foreach`; quem quer a
fronteira visível — porque a função é testável sozinha, ou vai para outro módulo,
ou vai virar ponteiro — escreve `apply` e vê no ponto de chamada exatamente o que
o corpo alcança.

---

### `parallel`


#### Por que ele não trouxe gramática nova

`parallel` é o produto de duas travessias e uma lista, e as três já existiam: um
binder sobre intervalo (§4.7), dois binders sobre contêiner (§4.7), e a
lista de captura do `defer` (§4.7). O que ele acrescenta é a decisão de que a
primeira roda em paralelo.

Isso não é elegância retroativa — é a consequência do `foreach` ser o único ponto
em que a travessia é conhecida. Uma vez que ela é conhecida, parti-la não pede
nada novo: até o error 71, que protegia o laço contra `push` no corpo, passa a
proteger a partição sem uma linha de regra a mais.

**E `parallel` aninhado é error pelo mesmo raciocínio invertido:** k × k threads
não é o que a escrita diz, e a forma útil — dividir de novo dentro da faixa — é
outro `k`, não outro bloco.

#### Por que a promessa é "execução permitida", e não "resultado único"

A edição anterior prometia que `ok` e `failed` eram determinísticos e que as
escritas do corpo não dependiam do escalonamento. A primeira metade não se
sustenta fora de `ALL`, e a segunda também não, **pelo mesmo motivo e não por
dois**: a bandeira.

Quem para na bandeira não executa o resto do corpo. Não executa as escritas, e
não executa um `fail` que viria depois — então duas faixas que discordariam sobre
`failed` numa ordem concordam noutra, sem corrida sobre dado nenhum. O que varia
é **quanto de cada faixa chegou a rodar**, e isso é o que a política pede: uma
busca que não parasse as outras faixas seria um `ALL` com nome errado.

A saída não é apertar o contrato, é enunciá-lo na altura certa. `ALL` não tem
bandeira, e ali o resultado **é** único — o que salva o caso de map, que é a
maioria. Fora de `ALL`, o que se pode prometer é que a série é uma das execuções
permitidas, e é exatamente isso que faz o `#pragma` ignorável ser correto e não
um degrau abaixo.

O que fica no lugar da promessa antiga é a regra que o programa precisa: **não
dependa de quantas faixas terminaram.** Escreva por `w` em contêiner disjunto,
reduza em série depois do bloco. Quem faz isso tem resultado único de fato; quem
não faz estava escrevendo um programa que a construção nunca cobriu.

#### Por que OpenMP, e não `<threads.h>`

A escolha decide o que o §4.7 pode prometer, e ela se paga em quatro lugares
de uma vez.

**O `#pragma` é ignorável, e aqui a ignorabilidade é exata.** Sem OpenMP, os dois
laços aninhados percorrem tudo em série — e rodar as faixas em sequência é um dos
entrelaçamentos que a §4.7 já permite, porque a ordem entre faixas não é
definida. Não é uma versão pior do programa: é o mesmo programa, num
escalonamento que ele já admitia. O contador da política, a bandeira e os três
predicados sobrevivem inteiros. Com `thrd_create` isso não existiria: sem
`<threads.h>` o arquivo simplesmente não compila.

**As duas travessias do fonte viram os dois laços do gerado.** O `omp parallel
for` reparte o laço **dos workers**, não o dos elementos — então `w` é a variável
de indução do laço externo, escrita pelo usuário, e não `omp_get_thread_num()`.
O binder que a §4.7 exige sai de graça, e a aritmética da faixa fica dentro
do corpo, onde keel a controla: a fórmula `(n + k - 1) / k` é normativa e não
depende de `schedule`.

**A lista de captura já era o vocabulário do OpenMP.** Escalar por cópia é
`firstprivate`; instância `byref` por ponteiro é `shared`. Não houve tradução a
inventar — as duas regras da §4.7 foram escritas antes desta decisão e
casaram sem uma emenda. E as cláusulas fazem o compilador C cobrar a lista, que é
melhor do que keel cobrá-la.

**E não há runtime a escrever.** Pool, agendamento e barreira são de quem
implementa o OpenMP. keel emite uma diretiva, a aritmética das faixas e umas
poucas atômicas — o que sobra é o que ele escreveria à mão.

O custo, escrito porque existe: **OpenMP não é C.** É a única dependência de keel
fora do compilador C, e é o preço da construção inteira — daí ela ser a última
seção do §4.7, e daí ela existir.

**`<threads.h>` volta se a linguagem ganhar monitores e executivos.** Faixa
disjunta não tem o que proteger, e por isso `parallel` não precisa de thread
nomeada, de tempo de vida de thread nem de primitiva de sincronização — o OpenMP
cobre a construção inteira. Estado compartilhado e protegido é outro modelo, e
esse sim pediria threads de primeira classe, com identidade e ciclo de vida
próprios. Enquanto ele não existir, importar `<threads.h>` seria trazer
vocabulário para um problema que a linguagem não tem.

#### Por que `k` é escrito e nunca deduzido

Deduzir exigiria consulta de concorrência de hardware, que o C não tem e que
seria plataforma — o que a construção existe para não precisar.

E ser constante de compilação é o que torna decidível a exigência do §4.7
sobre `interrupted`: sem saber se o alvo é zero, o error 85 não pode ser dado nos
dois sentidos.

> **Registro honesto:** o segundo argumento original era outro — com `k` de
> runtime, o vetor de threads e o de argumentos seriam VLA. Sob OpenMP não há
> vetor de threads, e `num_threads(k)` aceita expressão de execução. O que sobra
> é a decidibilidade do 85, mais legibilidade. É argumento suficiente, e é menos
> do que era.

#### Por que a política é um número, e por que zero é "todos"

*Quantas vitórias antes de parar* é a leitura literal, e sob ela `ALL` é zero sem
truque nenhum. O ganho é do gerado: **um caminho só**, sem caso por política — um
contador atômico comparado com o alvo, e alvo zero nunca dispara.

`ALL` e `ANY` serem palavra posicional, e não nome injetado, é o que faz um
`#define ALL` vindo de header C não colidir com nada: elas não chegam ao C
gerado, o que sai é o número.

#### Por que o join é incondicional

A tentação é deixar `ANY` matar o bloco assim que alguém vence. Não se faz, e a
razão é tempo de vida: as capturas vivem no frame do bloco, e uma thread
sobrevivente lendo aquilo depois que o frame morreu é um furo que a linguagem
**não tem vocabulário para expressar** — a análise do §4.4 cuida de arena,
não de frame.

Com join incondicional o furo não existe, e nada novo precisou ser inventado. A
política muda o que o bloco reporta e quando liga a bandeira, nunca quem ele
espera.

Sob OpenMP isso deixa de ser sequer uma decisão de geração: a barreira implícita
no fim do `omp parallel for` **é** o join, e não há como não a ter. A construção
pediu o que o mecanismo já dava.

#### Por que o ponto de interrupção é escrito

Injetar o teste no topo de cada iteração seria comportamento que não está
escrito. E seria a decisão errada por um motivo próprio: *onde* aceitar
interrupção é do programa — no topo do laço, depois do trabalho caro, ou dentro
de um laço interno.

Daí o error 85 valer nos dois sentidos. Sob política diferente de `ALL`, corpo sem
`interrupted` faz a política mentir; sob `ALL` não há bandeira, e escrever o
verbo promete algo que não acontece. A verificação é varredura do corpo por um
nome, e é decidível porque a política é constante — que é a segunda coisa que a
constância compra.

#### Por que escrever num escalar capturado é error

A escrita numa cópia por worker morre com o worker, e o chamador não vê nada. É a
mesma falha do `buffer` por valor e do `get` sobre elemento-contêiner, uma camada
adiante — e a mesma resposta: transformar perda silenciosa em diagnóstico.

**Quem cobra mudou com o OpenMP, e vale registrar.** No desenho por thunk a cópia
era campo de um struct gerado, e bastava emiti-la `const` para o compilador C
recusar a escrita — princípio 3, de graça. Sob `firstprivate` não há struct onde
pôr o `const`, e a cópia é uma variável comum: escrever nela é C perfeitamente
válido.

Restavam duas saídas. Renomear a variável dentro da região devolveria o `const`,
mas exigiria reescrever identificadores **no corpo**, que é texto (§1.3) — está
fora. Aceitar a escrita como semântica de `firstprivate` seria abrir mão de um
diagnóstico que já estava especificado, por causa de uma troca de mecanismo.

Então keel passa a cobrar sozinho: error **114** `captura-escrita`, varredura
léxica do corpo por atribuição a um nome da lista. É a mesma varredura do 71 e do
85, e cabe no mesmo orçamento (§4.1). O custo honesto é que uma verificação
saiu do compilador C e entrou em keel — o único ponto em que a escolha do OpenMP
andou para trás.

#### Por que a lista de captura não é deduzida

Descobrir quais identificadores o corpo toca é entender o corpo, e statement é
texto (§1.3). Pior: pelo mesmo §1.3, keel não sabe quais nomes existem, porque o
corpo pode tocar nomes vindos de `import_c`.

O desenho que sobra é o único possível, e ele funciona: as cláusulas emitidas
fazem de um local não listado um erro do compilador C, nomeando a variável, na
linha do `.k` que o usuário escreveu. É o princípio 3, num lugar onde a
invariante poderia ter falhado e não falhou.

#### Por que a ausência de corrida não custou análise

```plain
faixas disjuntas + capturas escalares por cópia = nada a proteger
```

Não foi preciso analisar nada para saber. O que atravessa por ponteiro —
contêiner capturado, arena — é compartilhamento de verdade, e é responsabilidade
de quem escreveu: é o §4.4 na letra, que já dizia que compartilhar arena
entre threads exige sincronização escrita pelo usuário.

É por isso que `monitor`, `lock` e `executive` estão fora **desta** camada.
Estado compartilhado e protegido é o modelo **oposto** ao que a arena escolheu —
uma arena por thread, partição em vez de guarda. Um `monitor` que injeta `lock`
no corpo de toda função pública esconde justamente a decisão que o princípio 6
manda deixar visível: quem segura qual lock, sobre quais dados, por quanto tempo.
E não faz falta agora: `<threads.h>` e `<stdatomic.h>` atravessam por `import_c`,
e um monitor é um mutex mais uma struct.

**É essa camada, e não `parallel`, que traria threads de primeira classe.**
Monitor e executivo precisam de thread com identidade e ciclo de vida — algo a
nomear, iniciar e esperar —, e aí `<threads.h>` deixa de ser vocabulário
importado à toa. Enquanto a resposta for partição, o OpenMP cobre a construção
inteira e a linguagem não precisa saber que threads existem.

#### Por que `parstatus` é enum, e `corot` não tem constantes

No join não existe `ONGOING`, porque a thread acabou; e no `corot` não existe
`INTERRUPTED`, porque ninguém interrompe ninguém. Fundir os dois daria a cada um
um estado impossível de alcançar, e tipo com estado inalcançável é pior que dois
tipos.

Mas eles não são dois enums: `parstatus` é enum porque os três valores do join
são valores mesmo; `corot` **não tem constantes de status**, porque FAILURE lá é
uma região do `code` e não um valor (linguagem §4.8). Expor `corot.FAILURE`
seria mentira, e o acesso fica todo nos predicados.

A assimetria entre as duas famílias é honesta e é a diferença entre as camadas:
no preemptivo alguém de fora liga uma bandeira; no cooperativo ninguém interrompe
ninguém, e quem sobrou `ONGOING` **é** o interrompido.

---

### Cláusula `else`


#### Por que ela não é o operador de propagação

Todo mecanismo de propagação de erro precisa de quatro coisas:

| | O que exige |
| --- | --- |
| teste — este valor é falha? | o tipo do retorno da chamada |
| extração — o valor bom | idem |
| conversão — erro do callee para erro do caller | o tipo de retorno da função que envolve |
| saída com cleanup | os pontos de saída do escopo |

keel tem a quarta de graça: o `defer` já é dono de todos os pontos de saída
(§4.7) e já emite o temporário e o cleanup. Em C essa é a parte difícil, e
aqui ela estava pronta antes de a construção existir.

Das outras três, a cláusula pega **uma** — o teste — e devolve as outras duas ao
programa. Não por economia, e sim porque as duas que ela devolve são exatamente
as que exigiriam tipar expressão.

E o operador de verdade — `f(x)?` no meio de uma expressão — não sai da tabela
das sete recusas (`§1.3`) nem com um parser maior. Ele bate em três paredes, e a
primeira é a menos grave:

1. **A grafia colide.** `x?` só se distingue de `x ? y : z` procurando o `:` de
   mesma profundidade, com ternários aninhados no caminho.
2. **O operando não tem fronteira à esquerda.** Achar onde a subexpressão começa
   é a gramática de expressão do C, e ela precisa da tabela de `typedef` para
   separar `(T)x` de `(x)` — o único ponto do C em que a sintaxe depende dos
   tipos, e a linha exata onde a camada termina (§1.3).
3. **Içar o operando muda o sentido.** `a && try f()` avaliaria `f`
   incondicionalmente, porque o temporário sai antes do statement. Isso é o
   princípio 1 quebrado, e continua quebrado com um parser completo: o problema
   é semântico, não sintático.

Vale registrar que a terceira parede é a que fecha a discussão. As outras duas
são sobre o que keel consegue analisar; essa é sobre o que o C significa.

#### Por que a forma é declaração

A frase que decidiu o desenho:

> O teste precisa do tipo do retorno de uma chamada. keel não tipa chamada. Logo
> o tipo tem que estar **escrito** — e o único lugar onde um tipo é escrito é uma
> declaração.

Daí a construção não ser um operador em posição de expressão, e sim uma cauda de
declaração. É a mesma economia do intervalo (`§3`): a forma existe onde a
informação existe, e o delimitador não precisa ser inventado porque já estava
lá — ali os `[ ]` e o `:`, aqui o `=` e o `;`.

O reconhecimento sai de graça pelo mesmo motivo. `else` é palavra-chave do C e
não aparece em expressão nenhuma, então um `else` de topo dentro de uma
declaração é sinal inequívoco, sem lookahead e sem consultar tipo. Foi a única
palavra candidata com essa propriedade — e, sendo palavra-chave, é também a
única palavra de keel que **não pode sombrear identificador do usuário**, o que
a torna estritamente melhor que uma palavra nova (`§3.10`).

#### Por que escalar não é falível

A tentação era testar `!x` para qualquer escalar, e ela é forte porque cobriria
`open`, `read` e metade da libc. Mas zero é sucesso em `open` e falha em
`fopen`; `-1` é falha em `read` e valor legítimo em `getopt`. Não existe
convenção, e uma construção que escolhesse uma delas estaria adivinhando — que é
o que keel não faz em lugar nenhum.

#### Por que ponteiro também não é falível

Esta é a que eu tinha errado, e vale escrever o argumento inteiro porque ele
inverte o anterior.

A tentação aqui é maior, e por uma razão boa: nulo é falha por construção do C, e
é o modo de falha de `arena.alloc` e de `buffer.push`. Parecia que o teste ali não
era convenção, e sim a linguagem.

Mas a convenção existe, e é a **oposta** da única que a base fixa. Em `outcome`,
zero no `code` é sucesso; num `T *`, zero é a falha. Uma cláusula que cobrisse os
dois faria a palavra `else` significar *"testou o zero"* num caso e *"testou o
não-zero"* no outro, e o leitor precisaria saber o tipo para saber o que foi
testado. Isso é exatamente o que a construção existe para evitar: a forma é
declaração justamente para que o teste seja legível a partir do que está escrito,
e "o que está escrito" não pode valer duas coisas contrárias.

O custo é real e é aceito: `arena.alloc` e `buffer.push` continuam pedindo o `if`
escrito. Uma linha, a mesma que se escreveria em C, e que diz qual é o teste. A
cláusula não foi feita para poupar `if`; foi feita para dar a um tipo de resultado
o teste que só o tipo conhece.

Uma consequência agradável: com ponteiro fora, **o protocolo tem um caso só**.
Um tipo é falível quando declara `failed`, e não há segunda regra a lembrar nem
segunda linha a emitir.

#### Por que `corot` fica de fora do protocolo

A versão anterior desta seção celebrava que `corot` compunha de graça, por já ter
um verbo `failed`. Estava errada, e o erro era de altura: o verbo existia, mas o
**tipo** não serve.

`corot` tem três status linearmente independentes — `SUCCESS`, `FAILURE`,
`ONGOING` —, e nenhum é a negação de outro, porque "não terminou" não é sucesso
nem falha. Um predicado binário não particiona esse tipo, e uma cláusula
construída sobre ele teria de decidir em silêncio o que fazer com o terceiro
estado. Qualquer decisão estaria errada em metade dos usos.

E há o argumento que fecha, que é sobre o que a operação significa: **propagar não
é o que se faz com um passo que não terminou.** Um `FAILURE` reporta ao laço que
dirige a máquina, e é ele quem decide se recomeça, se troca de estado ou se
desiste. Empurrar a falha para o chamador de cima pularia exatamente a máquina
que existe para tratá-la.

Como o despacho de keel é por nome, excluir `corot` é fazer o protocolo perguntar
por **dois** nomes em vez de um: exige `failed` e proíbe `ongoing`. `corot`
declara os três predicados (§4.8) e cai fora pelo segundo.

A alternativa era renomear o predicado dele — `corot.fail`, não `corot.failed` —,
e ela foi descartada por outra seção deste documento: `corot.fail(r)` lê como
ordem, e `cofail` já é uma. Duas coisas opostas com a mesma palavra é pior do que
um nome proibido na tabela.

E o nome proibido não é exceção escrita para um tipo: qualquer modificador de
usuário com três estados cai fora pela mesma linha, e nenhum tipo é privilegiado.
O que a tabela responde continua sendo *este módulo declara este verbo?* — só que
duas vezes.

#### Por que há duas formas, e por que separá-las não é espiar

São duas coisas que se faz com um resultado ruim — **sair** ou **repor** —, e a
cláusula tem uma forma para cada. `else return -1` sai; `else string.from("(anon)")`
repõe o valor que faltava. A segunda é o que faz `outcome` servir de `optional`
sem um segundo tipo: quando não há valor, escreve-se na mesma linha o valor que
houver de haver.

Uma versão anterior desta seção recusava a segunda forma, com um argumento que
parecia bom: *classificar o operando é examinar o operando*. Ele confunde dois
níveis, e a distinção é a que decide.

**Classificar semanticamente o que o operando faz** — aquilo sai? repara? não faz
nem uma coisa nem outra? — continua fora, e continua sendo a razão de a cláusula
não verificar o corpo da forma de saída. Ali o statement é `<opaco>` e keel não
sabe o que ele faz.

**Olhar se o primeiro token é `return`** é outra coisa. É léxico, é lookahead
fixo de um token sobre um `TK_CKW` que o lexer já emite, e não consulta tipo nem
avalia nada. É a mesma espécie de decisão das cinco do §3.5 — e nenhuma delas
foi acusada de espiar. A regra que sobrou é a mais curta possível: `{` ou palavra
de salto do C é saída, o resto é default.

Some também a objeção de que sintetizar `x = ⟨expr⟩` seria escrever o que ninguém
escreveu. keel não escreve uma atribuição inventada: ele chama `win`, o construtor
que o programa chamaria, com a expressão que o programa escreveu dentro. É a
mesma emissão de qualquer outro verbo, e o princípio 2 fica de pé — a linha
gerada é a que se escreveria à mão.

E a distinção que as duas formas servem não é *falha versus falta*. Essa não
existe mesmo: não ter valor e ter valor errado são o mesmo estado formal, e é por
isso que `NONE` mora do lado de lá de `failed` junto com os erros. A distinção é
outra, e é sobre o **programa**: em um caso este escopo não tem mais o que fazer,
no outro ele tem um default. O tipo não sabe qual dos dois é; quem sabe é quem
escreve, e é por isso que a escolha é uma forma sintática e não uma propriedade
do valor.

A promessa passa a ser diferente em cada forma, e as duas se escrevem sem
constrangimento. A de saída garante que o teste aconteceu e que o statement rodou
— nada sobre o símbolo, porque prometer mais exigiria análise de fluxo. A de
default garante que **o símbolo tem valor válido**, e essa é forte: é a única
garantia da linguagem sobre um resultado, e ela só pode existir porque ali é keel
quem escreve a atribuição.

Ela continua passando o teste de admissão do princípio 8 assim mesmo: o teste vem do
tipo, e o `if` é emitido no lugar onde só o parser sabe que há uma declaração com
inicializador. Nenhum módulo keel escreveria isso.

#### Por que não vale para atribuição

`x = f() else return -1;` foi considerada e recusada. Ela funcionaria, e a
recusa não é técnica: é que a garantia da construção é sobre um ponto do
programa — *daqui para a frente* —, e atribuição não cria ponto nenhum. O
símbolo já existia antes, possivelmente bom, e passaria a ser bom de novo. A
frase que descreve a construção deixaria de ser verdadeira, e uma construção que
precisa de duas frases para ser descrita está mal desenhada.

#### O que ela custou

Uma palavra, que não custa identificador. Uma produção de declaração. Uma regra
de teste com **um** caso — o verbo `failed`, pelo despacho que já existia — e uma
separação de formas por um token. Cinco diagnósticos. Um tipo na base, `outcome`,
que a cláusula precisava ter o que testar (§4.10). Nenhuma análise de tipo nova,
nenhuma mudança em `defer`, `foreach`, `parallel` ou nos genéricos.

E ela passa o teste de admissão do princípio 8 sem folga nem sobra: nenhum módulo keel
escreveria isto, porque precisa dos pontos de saída de escopo e do cleanup — o
mesmo argumento que admitiu o `defer`. O gerado é o `if` que se escreveria à mão,
e o princípio 2 fecha.

---

#### Por que função de genérico sem contêiner é error em vez de omissão

A terceira linha da tabela do §4.9 sempre valeu para tipo, constante e
variável, que são selecionados por serem escritos. Para **função** ela não tem
como valer: a instância vem do primeiro argumento ou do retorno, e sem um dos
dois o ponto de chamada não tem de onde tirá-la. Isso não era recusa, era
silêncio — e silêncio em regra de despacho vira divergência entre
implementações.

O caso que expõe a lacuna é o `bind` de uma mônada, `m T → (T → m U) → m U`: ele
atravessa duas instâncias, e `U` não é parâmetro do módulo que declara `m`. A
recusa aqui é a mesma da "função genérica livre" do §4.9, chegando por outro
lado — e é bom que as duas tenham a mesma resposta, porque são a mesma falta.

---

## 4.8 Cooperativo

### Por que o estado é dado, e não local escondido

Uma máquina que declarasse o próprio estado num local serviria a uma máquina por frame. Declarado pelo usuário, num campo, ele serve a mil agentes num vetor contíguo — que é o caso que a linguagem existe para atender.

É o princípio 6 com as três pernas ao mesmo tempo: o layout é do programa (o campo está na struct dele), o tempo de vida é do programa (o vetor é dele), e a travessia é do programa (o `foreach` é dele). Nenhuma das três ficou escondida atrás da construção, e é isso que separa este desenho de uma corrotina com runtime.

**E é por isso que a variável tem tipo, e não é um `int`.** Passar por engano o estado de outra máquina vira erro do compilador C em vez de compilar em silêncio. O preço é que o estado pode vir de fora — de memória, de arquivo, da rede — e chegar fora de faixa; daí o `default:` no gerado, e o diagnóstico 66 em debug.

### Por que declarar e operar são formas separadas

A primeira edição declarava o tipo de estado na cabeça do `cofsm`, dentro da função. Três coisas quebravam:

- o tipo nascia num *statement* e vivia em escopo de arquivo — às vezes no `.h`, quando um `struct` público tinha campo dele —, e nada na sintaxe dizia isso;
- a visibilidade era **inferida do uso**, o único caso da linguagem: em todo o resto keel exige `pub`/`priv` escrito, e o error 12 existe justamente porque adivinhar seria ambíguo;
- o sítio da definição se movia com um token opcional — com a lista `[…]` quem definia o conjunto era ela, sem a lista eram os rótulos.

Separar resolve os três de uma vez, e o critério que decide onde a declaração fica é o **escape** — o mesmo do §4.4, aplicado a estado em vez de memória. Se nada sai da função, o tipo e a variável nascem juntos ali; se o estado precisa sobreviver às chamadas, só o tipo sobe, e `pub`/`priv` decide o header.

O ganho que não estava no plano: o programador **não escolhe entre duas sintaxes**. Ele escreve onde a coisa vive, e a forma acompanha.

### Por que a lista de estados é obrigatória

Sem ela, o valor de cada estado é consequência do *layout do corpo*: mover um bloco muda o número de todos, e muda qual é o inicial. Para uma construção cujo estado é dado que vive em struct — e que pode vir de memória, de arquivo ou da rede —, isso é frágil de um jeito específico: uma reorganização inocente invalida estado persistido, e nada acusa.

Ela compra três coisas, e as três se pedem. O conjunto de estados **é interface**, e interface inferida do miolo destoa de um documento em que posicionamento é explícito. A checagem fica **exaustiva nos dois sentidos** — o error 99 pega o estado que foi planejado e esquecido, que sem a lista não seria erro nenhum. E o `.h` **fecha sem ver o corpo**, que é o que permite declarar a máquina no header e implementá-la no `.c`.

Na primeira edição a lista era opcional, e o custo dessa opcionalidade era exatamente a exaustividade: com os rótulos definindo o conjunto, exaustividade é tautologia.

### Por que não há verbo de transição

`costate(X)` existiu, e era atribuição pura que não transferia controle. Feita a separação acima, ele deixou de ter o que fazer: a variável de estado tem nome e está à vista nos dois casos, e o nome nu da constante já vale dentro do módulo. `ag->s = ATACA;` é C comum e diz a mesma coisa.

Manter os dois seria duas grafias para uma operação, e a construção que tem menos regras porque tem menos poder é a que sobrevive. Sumiram junto dois diagnósticos — os antigos 63 e 64 —, cujos números foram reaproveitados.

### Por que o laço é do programa

Esta é a decisão que mais encolheu a construção. A primeira edição fixava que **uma passagem executa um estado**, e daí saíam regras sobre o que o fim do bloco de estado significa, sobre quando a transição custa uma chamada, e uma discussão inteira sobre se `coseq` deveria correr até bloquear.

Com o `cofsm` reduzido a um *switch*, nada disso é da linguagem. Quem quer uma passagem por estado não escreve laço; quem quer correr até bloquear escreve `while (1)`. As duas políticas são o mesmo código com uma linha de diferença, escrita pelo programador, no lugar onde ele decide o orçamento de tempo da rodada.

A pergunta *"uma chamada executa um estado ou até bloquear?"* simplesmente deixou de existir — era pergunta errada, feita a quem não devia respondê-la.

### Por que os verbos saem da função, e não do bloco

Havia três níveis de saída — o bloco de estado, o `switch`, a função — e um vocabulário para dois. A confusão custou uma edição inteira, e a resposta é curta: **a `cofsm` não tem valor de retorno, porque bloco não retorna.** Valor pertence a função.

Daí a tabela do §4.8 ter duas linhas e não três, e daí `cobreak` ser palavra separada: ele sai do *switch* e continua na função, que é uma coisa que nenhum dos três verbos faz.

E é a mesma regra que decide o lado oposto: `coseq` e `copar` **têm** valor porque são funções inteiras, fornecidas pela linguagem. Uma frase decidindo dois casos em direções opostas é o sinal de que ela está certa.

### Por que os verbos não são `break`, `yield` nem `pause`

`break` promete terminação, e a máquina não terminou. `yield` e `pause` prometem retomar **naquela posição**, e nada retoma posição nenhuma: a próxima chamada entra pelo topo da função.

E reusar `break` teria custo além do nome: um `break` do usuário, dentro de um laço próprio no corpo do estado, mataria a máquina em silêncio. É o mesmo argumento que o `parallel` usa para `win`, `fail` e `interrupted` não serem `break`.

`coagain` diz o que acontece: esta chamada acabou, chame de novo.

### Por que o nome nomeia coisas diferentes em `cofsm` e em `coseq`

Parece incoerência e não é: **o nome nomeia o que escapa.** No `cofsm` escapa o estado, e o campo do usuário precisa de um tipo escrito — o nome é o tipo. No `coseq`, no `copar` e no `parallel` não escapa estado nenhum, e a única coisa que sobra é o status — o nome é o status, e é o símbolo que se interroga depois do bloco.

A regra vale para as quatro construções da linguagem que levam nome, e é ela que dispensa uma justificativa por construção.

### Por que `coseq` e `copar` existem, se `cofsm` já bastava

Bastava, e o custo aparecia em repetição de transição: uma cadeia de N etapas obriga a repetir a mesma tríade de testes em cada estado, e dá N chances de escrever o próximo errado.

Mas a razão que as põe na linguagem, e não na biblioteca, é outra e é decisiva:

> Não há como escrever `coseq(f1(a,b), f2(), f3(10,30))` como função. Os argumentos são ligados **no ponto de escrita**, e as chamadas acontecem **depois e repetidamente**, dentro do corpo da máquina. Guardar isso é closure, e closure está na tabela do §5. Vetor de ponteiro para função também não serve, porque as assinaturas variam.

Sem closure, a única saída é a linguagem emitir as chamadas textualmente. É o teste de admissão do §1.4 passando sem folga: o C não expressa isso.

**Composição homogênea continua não pedindo construção nenhuma** — uma máquina, mil agentes, é o `foreach` de sempre, e é a forma superior. O que ganhou construção foi a composição **heterogênea**.

### Por que elas produzem `outcome`, e não `corot`

Porque nunca devolvem ONGOING. "Parou no meio da cadeia" e "parou no meio da rodada" não querem dizer nada: elas entram, rodam e saem. Um resultado com um estado inalcançável cobra do leitor para sempre — todo `corot.ongoing(x)` sobre elas seria código morto que nada sinaliza —, e custaria a cláusula `else`, que é a ergonomia mais visível que essas construções podem ter.

A divisão que sobra é limpa: **`corot` é o tipo de quem cede o controle; `outcome` é o de quem termina.** A troca de tipo na fronteira não é ruído — ela marca exatamente onde a cooperação acaba.

### Por que `ONGOING` vale zero, e por que o sinal decide o resto

`corot` e `outcome` têm o mesmo layout e **não** a mesma leitura do zero, e as duas escolhas são normativas.

No `outcome`, `OK` é zero porque `{0}` tem que ser um resultado válido com valor presente. No `corot`, `ONGOING` é zero porque o zero ali é o estado **neutro**: uma struct zerada diz "ainda não terminou", que é a única leitura segura para algo que ainda não rodou. Isso paga em três lugares — `{0}` e `memset` deixam um `corot` pronto para a primeira chamada, os participantes de um `copar` já nascem pendentes sem inicialização escrita, e um agente inteiro fica pronto com uma inicialização agregada.

O resto cai do sinal: negativo é SUCCESS, zero é ONGOING, positivo é FAILURE **e o número é qual**. Três regiões, uma comparação cada, e o código de falha é o ordinal sem offset. Foi a expansão do `copar` que expôs isso — só escrevendo o corpo da máquina é que se vê que `corot.pending()` seria necessário se o zero fosse sucesso.

**E é por isso que não há constantes de status.** FAILURE é uma região, não um valor; expor `corot.FAILURE` seria mentira. Sobram os predicados, que é onde o acesso deve estar de qualquer forma.

### Por que os predicados estão no passado

`corot.fail(r)` lia como ordem — *fail r* —, e na mesma família existe `cofail`, que **é** uma ordem. Duas coisas opostas com a mesma palavra, separadas só pelo prefixo.

`failed` desfaz isso sozinho, e não é invenção: o `failed(nome)` do `parallel` e o `failed` do protocolo de tipo falível já usavam o passado. A troca alinha com o que existia em vez de abrir um terceiro caminho.

Daí a regra do vocabulário, que cabe numa linha: **palavra `co*` sai da função; verbo `corot.*` lê um valor.**

### Por que o resultado passa por um símbolo

A forma direta jogaria metade do resultado fora:

```keel
if (corot.value(fn2(a)) >= 20) coagain;   /* o status foi descartado em silêncio */
```

É a mesma falha que o `[[nodiscard]]` de `push` e de `arena.alloc` existe para impedir. Exigir o temporário obriga a segurar as duas metades e decidir sobre cada uma.

Que a gramática de contêiner já recusasse chamada de função como nó é conveniência, não a razão: a razão é que um resultado de dois campos com um campo lido é um bug esperando, e o único lugar onde ele pode ser recusado é o ponto em que o nome não foi dado.

### Por que não há passagem de encerramento

Uma edição inteira prometeu que quem sobrou num `coseq` ou num `copar` receberia
**uma passagem de encerramento**, e deixou em aberto como a participante saberia
que aquela era a passagem. A pergunta foi respondida recusando a construção: não há
passagem, a máquina para de chamar, e a limpeza é do bloco que contém a máquina.

**O espaço de desenho era fechado, e isso é o que decidiu.** Qualquer forma de a
participante consultar a máquina — inclusive uma palavra reservada no corpo dela,
que era a saída mais tentadora — precisa expandir para uma expressão C, e uma
expressão C alcança exatamente três coisas: um global, um parâmetro, ou um campo de
algo que a função já tem. Não há quarta: C não dá acesso ao frame do chamador, e o
que a palavra reservada pediria é escopo dinâmico, que é runtime. A participante é
função opaca, compilada à parte, e keel não vê o corpo dela. Então "palavra
reservada" nunca foi uma opção nova — era açúcar sobre um dos três canais:

- **global** quebra reentrância e threads, e esconde justamente a decisão que o
  princípio 6 manda deixar visível;
- **global nomeado pela máquina** é pior que o global puro num ponto: a
  participante passa a referenciar o símbolo de **um** `copar`, e nunca poderá
  participar de dois. Morre a propriedade que faz a construção valer a pena —
  participante ser função comum, reusável e testável sozinha;
- **parâmetro escrito** funciona e é honesto, mas obriga toda função cancelável a
  carregar um argumento que não tem nada a ver com o trabalho dela.

**E o reenquadramento que fechou a questão:** as duas construções são cooperativas.
A participante **retorna** entre passagens, então não existe execução a interromper
no meio — o que existe é uma chamada que não acontece mais. O que se queria não era
um canal de comunicação, era uma convenção de chamada dizendo "esta é a última". O
paralelo com o `interrupted` do `parallel` (§4.7) era enganoso: lá as faixas correm
de fato ao mesmo tempo, e o corpo é escrito dentro do bloco, onde keel enxerga onde
pôr o ponto de interrupção.

**Nada fica preso, e é isso que torna a recusa barata.** Dois fatos, e o segundo é
o que fecha:

- `defer` roda no `coagain`, e **tem** que rodar, porque `coagain` é `return` de
  verdade e o frame morre. O que dura uma passagem já foi liberado ali.
- O que atravessa passagens mora no estado que o **chamador passou**. Em `copar
  coleta ANY { parser(a, ag); … }`, `a` e `ag` são do bloco de cima — foi ele quem
  escreveu as chamadas e os argumentos. Não existe estado órfão a resgatar, e
  limpar uma linha abaixo do `}` é sempre possível.

Some-se a arena: no perfil que keel mira, o que atravessa passagens quase sempre é
memória, e memória sai no `reset` do escopo. Sobram descritores de SO adquiridos
entre passagens — raros nesse perfil, e já morando na struct do usuário.

A saída recusada continua disponível e é **aditiva**: um parâmetro escrito não
quebra nada do que existir até lá. Se o uso real mostrar que dói, ela entra sem
invalidar programa nenhum — que é a razão de fechar agora em vez de esperar.

## 4.9 Módulos genéricos


### Por que o parâmetro é do módulo e não do `modifier`

Duas razões, ambas de legibilidade do fonte, não de parser. Se cada modificador
trouxesse o seu binder, toda assinatura teria que repeti-lo —
`size_t length(stack type T *s)` — porque a função não pertence a nenhuma
declaração de agregado. E com dois modificadores no arquivo, o `T` nu de uma
função livre não teria a qual dos dois se referir.

O preço é que todos os modificadores de um módulo compartilham a lista de
parâmetros, e ele é o preço certo: um `map` sobre `K,V` é outro módulo porque é
outra coisa.

### Por que não `stack<T>` nem `stack(T)`

As duas grafias importam para dentro de keel uma ambiguidade que ele não tem. O
`<>` existe em C++ porque lá o parser precisa separar template de comparação; o
`()` colidiria com chamada, que é justamente o que se pagou para eliminar ao
tirar `buffer()` da lista de chamáveis.

A justaposição prefixa não precisa de nenhum dos dois, porque keel conhece o nome
do modificador e sua aridade **antes** de parsear o uso — o import já os
registrou. Com aridade conhecida, `map i32 stack f32 m;` é notação polonesa e lê
sem uma vírgula. E ela cai na vaga que o §3.10 já reservou: N ≥ 3 identificadores
seguidos nunca é declaração válida em C.

**E `tensor(3)` não reabre nada.** A objeção é contra `()` em volta de um
**tipo**, onde a forma colide com chamada. Aqui ela não colide, e o que separa as
duas não é o numeral: é que **`modificador ( … )` é obrigatoriamente seguido de um
argumento de tipo**. `tensor(DIM) f16 t;` casa `modificador argumento declarador`;
`x = tensor(DIM);` não tem argumento depois do `)`, não casa `decl-keel`, e cai
como `<opaco>` — que é o que ele é. Chamada de função nunca é seguida de nome de
tipo, e por isso a vaga é livre mesmo com identificador dentro dos parênteses.

#### Por que o argumento de `dim` aceita `constexpr`, e por que substitui o valor

A restrição a literal era mais apertada do que precisava, e cobrava no uso normal:

```keel
constexpr i8 DIM = 3;
tensor(DIM) f16 grade;      /* o rank tem nome, e o nome documenta */
```

Admiti-la custa **uma segunda exceção** à regra de que keel não lê o valor de um
`constexpr` (§4.2), e a exceção foi desenhada para ser a menor possível: o
inicializador tem de ser **um literal decimal**, e o que keel faz é ler um token
de uma declaração que ele mesmo registrou. `constexpr i8 DIM = 2 + 1;` continua
fora, porque dobrar constante é avaliar expressão, e aí a exceção viraria um
avaliador. Macro e constante de enum continuam fora por outro motivo — keel não
as enxerga (§1.4), e não há declaração de onde ler.

**O que substitui é o valor, e a razão decisiva não é de identidade.** A de
identidade já bastaria — com a grafia no nome, `tensor(3) f32` e `tensor(DIM) f32`
com `DIM == 3` seriam instâncias distintas de layout idêntico, e o princípio 4
estaria fragmentado por escrita. Mas a que fecha é de build:

```keel
/* a.k */  constexpr i8 DIM = 3;   tensor(DIM) f16 t;
/* b.k */  constexpr i8 DIM = 4;   tensor(DIM) f16 u;
```

Com a grafia, os dois pedem `tens_tensor_DIM_f16` com **layouts diferentes**. O
header de instância é escrito por quem usa, então ele deixaria de ser função das
entradas, e o determinismo do `backend §7.1` cairia — a falha aparece como duas
invocações se sobrescrevendo em laço sob `make -j`, que é exatamente o cenário
que aquela regra existe para impedir.

Com o valor, `tensor(DIM) f16` e `tensor(3) f16` geram **o mesmo C, byte a byte**,
e a ergonomia fica inteira: trocar `DIM` de 3 para 4 regenera como
`tens_tensor_4_f16`, porque a instância é gerada pelo uso. E a invalidação já
estava resolvida: mudar `DIM` num módulo muda o C de quem escreve `tensor(DIM)`, e
o critério transitivo de `ferramenta §5` cobre pelo fecho de imports.

### Por que não há indexação parcial, e por que `dim` não faz conta

As duas saíram juntas, e por um motivo só: **quem escreveria o que falta não sabe
o que precisa saber.**

A indexação parcial pedia uma **família** de acessores — `ptr(t,i)`, `ptr(t,i,j)`,
… até `N−1` —, cuja *quantidade* depende de `N`. Escrevê-la caberia ao autor da
biblioteca, que é quem sabe o que o layout significa; só que `N` só existe no
ponto de uso. Se ele escrever até a aridade cinco, um uso com `N = 3` deixa as de
aridade 4 e 5 **declaradas e chamáveis**, indexando além do rank. E se keel a
gerasse, o bug moraria em código que ninguém escreveu, num lugar em que ele
pareceria do programador — o princípio 2 na direção em que ele mais custa.

Nada se perde na operação principal: o acessor por vetor `ptr(t, idx[static N])`
**já serve todo rank**, e o compilador C desenrola o laço de índices por SROA. O
que se perde é o descritor de rank reduzido, e ele é outro assunto.

**Aritmética no argumento — `view(N-1)` — morreu na mesma pergunta.** Ela parecia
resolver o rank reduzido dentro de um `dim` só, e tem dois problemas. O primeiro é
que `constexpr i32 M = N - 1;` dentro do próprio `std.view` regride ao infinito:
`view(3)` declara algo que devolve `view(2)`, que devolve `view(1)`, que pede
`view(0)`, `view(-1)` — o error 53, e sem caso-base escrevível. O segundo é de
escala: `IDENT ± NUM` cobre redução de um eixo e `unsqueeze`, e quebra em
`reshape`, contração e `einsum`, que relacionam ranks **independentes** e são o
que define uma biblioteca tensorial.

A saída é o parâmetro a mais, e ela é estritamente melhor: cobre tudo, e **keel
não precisa calcular nada**. A relação vira `static_assert(M == N - 1, …)`, que
depois da substituição é expressão constante comum do C, conferida pelo
compilador com a mensagem que a biblioteca escolheu — princípio 3. Uma relação
arbitrária como `R == A + B - 2*K` cabe ali e não caberia em produção nenhuma da
gramática.

O que sobrou para a linguagem é o **piso**: valor ≥ 1, error 125. Ele não é
higiene — é o caso-base que a família de ranks precisaria escrever à mão, e sem
ele `rank(1,0)` geraria `size_t dims[0]`, que não é C.

### Por que a vaga `alias(…)` é reservada em vez de decidida

Um módulo com dois `dim` precisa que a chamada diga qual instância é, e a forma
natural seria `rank(3,2).axis(v, 0, i)` — os numerais escritos, como o `k` do
`parallel` (princípio 6). A decisão não foi tomada porque a biblioteca ainda não
foi escrita, e só escrevendo-a se vê se essa chamada aparece uma vez ou cinquenta.

Mas **não decidir também é decidir**, e é aí que estava a armadilha:
`rank(3,2).axis(…)` é **C válido** — chamada devolvendo struct, seguida de acesso
a campo — e hoje atravessa verbatim, porque o `.` de keel só resolve com
identificador nu à esquerda. Dar sentido a ela mais tarde mudaria, em silêncio, um
programa que compila hoje. Isso é o princípio 1, e é a categoria de falha que este
documento recusa em todo lugar.

Reservar desfaz o dilema, e é seguro nas duas direções: hoje a forma é o error
126, com a saída na mensagem; adotá-la depois transforma erro em aceito; desistir
transforma erro em passagem verbatim. **Nenhum programa que funcionava muda em
qualquer dos dois futuros** — que é a única propriedade que se pede de uma vaga
guardada.

### Por que a declaração sem parâmetro é emitida uma vez

A regra original — tudo pertence à instância, mencione `T` ou não — é mais
simples de enunciar e estava errada num caso que aparece duas vezes na base.
`outcome.OK` e `outcome.NONE` não mencionam `T`, e uma cópia por instância
significaria elas não existirem como nome nu: existiriam
`keel_outcome_i32_OK` e irmãs, e o nome curto não teria de qual sair.

A exceção custa uma varredura léxica — o nome do parâmetro aparece, ou não
aparece, nos tokens da declaração — e devolve a expectativa certa: quem mexe com
resultados espera `OK` e `NONE` ao lado de `outcome T`, com o
mesmo qualificador. Separar as constantes em módulo próprio também resolveria, e
cobraria o preço errado: dois nomes de módulo para um assunto só.

O limite é o de sempre: a exceção é sobre **menção**, que é sintática. Não há
análise de dependência, não há "usa indiretamente", e uma declaração que mencione
`T` de qualquer jeito volta a ser por instância.

### Por que a menção inclui o modificador, e não só o parâmetro

A varredura pergunta se a declaração escreve `T` **ou** o nome de um modificador
do módulo, e a segunda metade não é zelo. `size_t length(stack *s)` não escreve
`T` e é função da instância — o `stack` no parâmetro já diz isso, e sem a segunda
metade ela iria para o módulo, onde o tipo do parâmetro não existe.

O critério continua sendo **uma varredura de tokens**, com dois nomes em vez de
um, e continua não havendo análise de dependência. E ele não briga com a tabela
de identidade da §4.9: o que a tabela decide é a **qual** modificador a função
pertence; o que a varredura decide é **se** ela pertence a alguma instância. Uma
declaração que passe pela varredura e não case com nenhuma linha da tabela é
`M.<nome>` sobre `A`, que é a terceira linha de lá.

### Por que `byref` é um bit, e por que ele tem duas regras

Sem o marcador, ou toda instância seria por referência — proibindo `slice`, cuja
passagem por valor é o uso normal — ou nenhuma seria, reabrindo a armadilha do
descritor copiado.

E ele tem que produzir **duas** regras, não uma, porque a segunda é fácil de
esquecer:

```c
/* o que sairia, se só a passagem por valor fosse checada */
keel_buffer_i32 a = ..., b = ...;
a = b;                       /* C perfeitamente válido */
keel_buffer_i32_push1(&a, 1);
keel_buffer_i32_push1(&b, 2);   /* os dois escrevem no mesmo armazenamento */
```

Atribuição entre instâncias copia o ponteiro junto, e as duas passam a escrever
na mesma memória. É warning e não error porque às vezes é o que se quer, e porque
é local e visível.

É deliberadamente um bit, **e não um vocabulário**. Traits, constraints e
interfaces começam exatamente aqui, e a regra de fechamento do §4.9 é o que
impede o próximo passo.

### Por que validar com `T` opaco

É o que evita revalidar a cada instância, e é possível porque tudo que keel
verifica — gramática de contêiner, despacho, `defer`, escape de arena — trata `T`
como nome de tipo desconhecido, exatamente como já trata todo tipo vindo do C.

Daí decorre a propriedade que mais importa: **keel não herda a ilegibilidade de
diagnóstico dos templates de C++.** Lá, o ponto de instanciação executa resolução
de sobrecarga e dedução, e é de onde vêm as mensagens. Aqui o template é checado
uma vez, a instância é gerada às cegas, e quem reclama do resultado é o
compilador C — princípio 3, palavra por palavra.

### Por que o sufixo de aridade não é sobrecarga

A aridade está escrita no ponto de chamada, contar argumentos é sintático, e
nenhum tipo de argumento é examinado para escolher candidato. Não há conjunto de
candidatos, não há ordenação por melhor casamento, não há conversão implícita
para viabilizar uma chamada.

É o que separa `ptr(m)` de `ptr(m,i,j)` sem abrir a porta que a sobrecarga
escrita pelo usuário abriria — a qual muda o mangling de toda função do módulo.

### Por que `dim` entrou, quando parâmetro de valor estava fora

A objeção original era que a aridade do gerado dependeria do parâmetro: `ptr` com
N parâmetros exige emitir declarações em laço, que é metaprogramação de tempo de
compilação.

O que mudou não foi a objeção — foi a descoberta de que o acessor **não precisa**
de N parâmetros. Recebendo `size_t idx[static N]`, o parâmetro só dimensiona
vetor e limita laço, que é a metade que a objeção já admitia ser substituição. O
fechamento do §4.9 — *`dim` nunca gera declaração* — é a objeção antiga
reescrita como regra em vez de recusa.

O preço aparece em `-O0`, onde o vetor é materializado e o laço roda, e está
escrito porque existe. Em release o `for` tem limite constante, o compilador C o
desenrola e o vetor some por SROA.

### Por que `x[i,j]` é sobrecarga de operador, e por que está tudo bem

O que `x[i,j]` significa passa a ser decidido pelo módulo que declarou `ptr`.
Isso é sobrecarga de operador, e não adianta chamar de outra coisa.

O que a mantém a uma distância segura são duas ausências: resolve-se por **nome e
contagem de índices**, ambos sintáticos, e **nenhum tipo de argumento é
examinado**. É a mesma fronteira que o sufixo de aridade já traçara, aplicada a
um operador em vez de a um nome.

E o `ptr` tem que continuar à vista, porque com `dim` aparece no gerado um
literal composto que ninguém escreveu. Daí o error 102: modificador com `dim` mas
sem `ptr` de rank cheio faz `x[i,…]` falhar nomeando a assinatura que falta. O
usuário descobre o mecanismo no momento em que ele importa.

### Por que a base sai idêntica da regra geral

`buffer` e `slice` são, sob esta leitura, os modificadores dos módulos
`keel.buffer` e `keel.slice`, e a regra de encurtamento do §1.3 devolve
`keel_buffer_i32` e `keel_slice_char` — os símbolos de sempre.

Que o caso da base saia sem uma letra de diferença é o argumento de que a
generalização é a leitura certa do desenho, e não um enxerto. Se fosse enxerto,
alguma coisa teria mudado de nome.

### O que a regra de fechamento realmente diz

A tabela do §4.9 é um **caso particular do orçamento de análise do
§4.1**. Especialização, constraints e dedução no ponto de chamada não estão
fora por serem difíceis de gerar — estão fora por exigirem do parser mais do que
coleta na fase 1 e análise dentro de um escopo.

Quando aparecer a próxima ideia para os genéricos, o teste é aquele, não esta
tabela. E o critério de leitura continua sendo o do §1.3: consigo abrir o C
gerado e identificar o mecanismo que teria escrito à mão?

---

## 4.10 Stdlib
### `string` e `strbuf`


#### Por que eles são stdlib e não biblioteca de usuário

Texto é o caso em que a divergência custa mais: se cada biblioteca trouxer o seu
tipo de string, elas não compõem, e a fronteira entre duas passa a exigir cópia.
O critério da stdlib no §1.6 — *o que as bibliotecas precisam concordar* — foi
escrito com este caso na cabeça.

E é o oposto do princípio 8: um módulo keel escreve `string` sem
dificuldade nenhuma. Ela está no degrau de cima por acordo, não por poder.

**São dois módulos pela razão do §3.10:** o alias é o qualificador do verbo, e um
módulo só daria um qualificador para as duas metades. `keel.buffer` e `keel.slice`
são dois pelo mesmo motivo, e a simetria não é decorativa — ela é o que faz
`strbuf.append` e `buffer.push` lerem igual.

#### Por que não são tipos novos

`typedef` do C não cria tipo, então `string` **é** `slice const char`. Uma
biblioteca que receba `slice const char` aceita a `string` de outra sem saber que
ela existe, e não há conversão a escrever em fronteira nenhuma.

Isso parece contradizer o princípio 4 e não contradiz: ali dois `Point` idênticos
são tipos distintos porque vêm de **duas** declarações. Aqui há uma só, e o
segundo nome é notação.

O preço é real e vale a pena dizer: **não existe `string.length`.** Os verbos de
medida e de acesso são os da base — `slice.length(s)`, `s[i]`, `s[a..b]` — e
escrever o qualificador errado é o error 109. Em troca, `std.string` declara só o
que é de texto, e a lista de verbos fica curta o suficiente para caber na cabeça.

#### Por que `char` e não `u8`

`char` é a unidade de texto do C: `printf`, `strcspn` e toda API C recebem o
ponteiro sem cast. `u8` é armazenamento bruto — é o que `arena.from_array` exige,
e pela regra de aliasing do C —, e texto guardado como `u8` obrigaria a converter
na borda de cada chamada.

Quem quer bytes escreve `slice u8`, que é outra coisa e se chama outra coisa. As
duas grafias existirem para propósitos diferentes é o §4.2 funcionando, não
uma inconsistência.

#### Por que `string` não é terminada em NUL

Porque recortar é a operação que texto pede o tempo todo, e um tipo terminado em
NUL não pode ser recortado sem alocar. Com o comprimento no descritor, `s[a..b]`
custa zero e não copia.

A ponte para o C não sumiu — ela ficou **explícita**, e mora no `strbuf`:
`strbuf.cstr(sb)` escreve o `'\0'` acima de `len`, sem contá-lo, e devolve `NULL`
quando não há um byte sobrando. Falha de execução, mesmo canal de `arena.alloc` e
de `push`, e o `[[nodiscard]]` obriga a olhar.

O que se ganha é que a conversão aparece no fonte, no ponto em que ela acontece,
em vez de estar embutida em todo valor do tipo.

#### Por que `split` tem saída por parâmetro

Porque a alternativa seria devolver duas `string`, e keel não tem tupla (§4.8).
O `ref` (§4.2) é o que faz a assinatura dizer a verdade: `rest` é **um**
elemento, não um vetor, e aritmética sobre ele é error.

É o único verbo da stdlib com saída por parâmetro, e está registrado como
exceção para que não vire padrão.

#### Por que não há formatação

`printf` e companhia são C, atravessam por `import_c`, e escrevem em
`buffer.ptr(sb)` com `buffer.capacity(sb)` — que é exatamente o par que
`snprintf` pede. Reimplementá-las exigiria variádicos com tipagem, e tipar
argumento variádico é tipar expressão do C: a invariante do §1.3 fecha o caminho
antes de a discussão começar.

---

### `tensor` e `view`


#### Por que são dois tipos e não um

`tensor` é denso e possui; `view` tem passo e não possui. Um tipo só teria que
carregar `passos` sempre — e aí o caso comum, que é denso, pagaria `N` campos e
uma multiplicação a mais por acesso sem usar nenhum dos dois.

É a mesma divisão de `buffer` e `slice`, um rank acima, e com o mesmo critério de
`byref`: o dono não se copia, a vista se copia.

**Conversão para `slice` é verificada, nunca implícita.** `tensor(1) f64` é
estruturalmente um `slice f64` e não é um `slice f64`, pelo princípio 4 — e uma
vista com passo diferente de 1 não descreve região contígua nenhuma.

#### Por que rank é parâmetro e não campo

Rank em campo é escolha legítima e tem um caso real: um tensor lido de arquivo,
cujo rank vem do modelo. Ele paga em `assert` de execução — `v[i,j]` sobre um
descritor de rank 3 só falha rodando.

Com `dim`, o rank sobe para o **tipo**, e `t3d[i,j]` vira erro de compilação: o
`_ptr2` não existe e não tem como existir. A pergunta que decide entre os dois é
uma só — *o rank é conhecido onde o tipo é escrito?* —, e ela tem resposta
diferente em domínios diferentes, o que é motivo para os dois conviverem e não
para a linguagem escolher.

O preço de `dim` é um tipo por rank, e ele aparece no nome canônico:
`std_tensor_tensor_3_f32`. É orçamento de comprimento de nome, e a 255 caracteres
não aperta ninguém.

#### Por que o recorte multidimensional é verbo

`m[i,2..8]` e `m[2..8,i]` têm a mesma aridade e destinos diferentes, e o açúcar
despacha **contando** argumentos, sem examinar nenhum. Escolher pela forma de
cada argumento é regra de despacho nova.

E ela compraria só ergonomia: o tipo de retorno já existe (`view`) e o tipo de
argumento que faz o verbo ler bem também (`range`). `view.sub(v, 2..8, 0..4)` é a
escrita, e é o que a regra da linearização interna (§4.5) já mandava fazer.

#### Por que a travessia continua sendo por dimensão

`foreach` exige contêiner linear, e nem `tensor` nem `view` o são acima de rank 1.
Percorrer é escrever um laço por dimensão, cada um sobre algo contíguo ou com
passo conhecido.

É deliberado: a ordem em que se varre um bloco multidimensional é decisão de
desempenho — cache, vetorização, ordem dos eixos — e o princípio 6 manda
deixá-la visível. Uma forma que percorresse tudo de uma vez escolheria essa ordem
pelo programador, em silêncio.

#### Por que `buffer(N)` continua fora, mesmo com `dim` dentro

A razão deixou de ser de mecanismo e passou a ser de identidade. `buffer` é
`{cap, len, ptr}` — dono de região linear, índice sem multiplicação. Um
`buffer(2)` precisaria de `dims` e `passos`, e aí **deixa de ser um buffer**: é
`tensor(2)` com outro nome, e pelo princípio 4 seriam dois tipos de mesma forma e
nomes diferentes.

`buffer` carrega armazenamento; tensor carrega álgebra. Pedir a um que carregue o
outro é o que esta linha recusa. `slice(N)` cai junto, e já tem nome: é `view`.

---

### `outcome`


#### Por que ele é biblioteca, e por que agora

`outcome T` não custou mecanismo, pelo mesmo motivo que `corot` não custou
(`§4.8`): é modificador de um módulo, nomeado e despachado pela regra geral.
O que mudou foi a demanda. Antes da cláusula `else` (§4.7) um tipo de
resultado seria conveniência; depois dela ele é a peça que a fecha, porque a
cláusula pede um tipo que declare `failed` e a base **não oferecia nenhum** —
`corot` não serve, e a seção anterior diz por quê. Sem `outcome`, a cláusula
nasceria sem nada para testar a não ser tipo de terceiro.

E a divisão entre os dois é limpa: `corot` tem três status porque existe uma
passagem que não terminou; `outcome` tem dois porque não existe. Usar `corot`
como tipo de resultado geral faria o `ONGOING` significar nada em metade dos
usos — e é a mesma razão, vista do outro lado, pela qual ele fica fora do
protocolo `failed`.

#### Por que `outcome`, e não `error`, `result` nem `fallible`

`error` e `result` são dos identificadores mais escritos em C — `int result;`
está em toda base de código —, e um nome de tipo da base que sombreie identificador
comum produz warning 47 no caminho normal. `fallible` tem problema pior: *falível*
já é o predicado do §4.7, na família de *percorrível* e *contável*, e o
documento precisa das duas palavras na mesma frase. Além disso os modificadores
são substantivos — `buffer`, `slice`, `arena`, `range`, `corot` —, e um adjetivo
destoaria da série.

`outcome` é substantivo, é raro como identificador, e não promete o que não
entrega: `result` puxa `Result` de Rust e a expectativa de `map`, `and_then` e
`?`, que a cláusula `else` deliberadamente não dá.

#### Por que um tipo, e não `optional` e `result` separados

Porque a diferença entre os dois é o que se faz com o código, não o que o tipo
guarda. O struct é o mesmo, o predicado é o mesmo, e o que muda é qual código
está lá. Dois tipos exigiriam conversão entre eles — e conversão é o que a
cláusula recusou explicitamente.

A pergunta que fecha o argumento é: **qual é a diferença efetiva entre não ter um
valor e ter um valor errado?** Nos dois casos não há valor utilizável, nos dois o
teste é o mesmo, e nos dois o que resta é decidir como consertar. A distinção
existe na cabeça de quem escreve, e o lugar dela é o que vem depois do `else`,
não o sistema de tipos.

E a forma de default é a prova de que o tipo único era o desenho certo:
`else ⟨expr⟩` repõe o valor sem perguntar **por que** ele faltava. Um `optional`
e um `result` separados dariam duas construções para a mesma linha, ou uma
construção que precisaria distinguir os dois — que é a conversão, de novo, com
outro nome.

Zero é sucesso, e não-zero é falha ou falta. É a única convenção que a base fixa
sobre um escalar, e ela contradiz de propósito a recusa do §4.7 — lá keel se
nega a adivinhar se zero é sucesso num `i32` qualquer. A diferença é de quem é o
tipo: sobre `i32` vindo da libc, keel estaria adivinhando; sobre o campo de um
modificador que a base declara, ela está **definindo**.

#### Por que `NONE` é reservado, e por que não é derivado de `OK`

`NONE = !OK` foi considerado e não funciona: `!0` é `1` em C, e `1` é o primeiro
código que todo catálogo de erro usa. `~0` daria `-1`, o segundo mais usado. E a
derivação descreveria mal o modelo: o tipo tem um `OK` e todos os demais valores
do lado de lá de `failed`, e `NONE` é **um** deles, escolhido — não o complemento
de `OK`.

Vale ser preciso sobre o que `NONE` é, porque a palavra "código de erro" o
descreve mal. Ele não diz que houve um erro; diz que **não há valor**, e é isso
que dá ao tipo o papel de `optional`. Os catálogos do programa dizem *o que deu
errado*; `NONE` diz *não deu errado nada, simplesmente não há o que devolver*.
Para quem **testa** os dois são o mesmo estado, e é por isso que o tipo tem dois e
não três. Para quem **lê o código** são coisas diferentes, e a constante existe
para que essa leitura tenha nome.

`INT32_MIN` é a escolha porque nenhum catálogo real chega lá, então a reserva não
estreita a faixa útil. Nomear a constante em vez de escrever o literal é higiene:
se a largura do código mudar, o sentinela acompanha.

#### Por que `clone` devolve `outcome` e `alloc` não

Parece incoerência e é o contrário: é a mesma regra aplicada a duas categorias
de retorno, e ela cai da linguagem em vez de ser escolhida.

Um verbo que devolve **ponteiro** tem para onde apontar o erro. `NULL` é um valor
fora da faixa útil que o C já reserva, todo mundo já testa, e ele não custa uma
struct. `arena.alloc` e `buffer.push` reportam assim, e embrulhá-los em `outcome`
seria pagar um campo em todo caminho de alocação para transportar a informação
que o ponteiro nulo já carrega.

Um verbo que devolve **valor** não tem essa saída. `buffer`, `slice`, `string`,
`tensor` são structs, e **não existe struct fora da faixa**: um `buffer` mal
construído continua sendo um `buffer`, e qualquer sentinela teria de ser inventada
— capacidade zero, comprimento zero, ponteiro nulo lá dentro. Cada uma dessas é
uma convenção, cada uma vale para um tipo e não para o outro, e todas obrigam
quem chama a saber qual é a de hoje. É exatamente a adivinhação que o §4.7
recusa, entrando pela porta de quem produz em vez da de quem testa.

Daí a regra do §4.11, e ela é curta: **ponteiro falha em `NULL`, valor falha em
`outcome`.** Nenhuma das duas categorias imita a outra, e a polaridade invertida
entre elas — que é o que tira ponteiro da cláusula `else` — deixa de ser um
problema, porque cada lado fica com a sua.

O sinal de que a regra está na altura certa é não ter exceção na base: `clone` e
`at` são os dois verbos que devolvem valor e podem falhar, e os dois devolvem
`outcome` pelo mesmo motivo, sem que nenhum outro precisasse de ajuste. E o tipo
que eles devolvem não foi inventado para isso — `outcome` já estava lá, posto
pela cláusula `else`, e absorveu os dois casos sem uma linha nova.

A stdlib ainda está por escrever, e a regra é justamente o que ela vai herdar em
vez de decidir de novo: quando `tensor.alloc` e `string.clone` forem
especificados, a categoria do retorno já diz como eles falham.

#### Por que o código é `i32` e não um parâmetro

`outcome T E` seria um módulo de dois parâmetros, e duas instâncias com o mesmo `T`
e `E` diferentes não se convertem — o que devolveria, pela porta dos fundos, a
conversão de erro que a cláusula `else` recusou explicitamente. Com o código
fixo, `outcome i32` de um módulo é `outcome i32` de outro, e a composição entre
bibliotecas não pede acordo. O catálogo de códigos é do programa, que é onde ele
sempre esteve em C.

#### O que ele não é

Ele não é `Result`, e não tem `map`, `and_then` nem `?`. Encadeamento sem
escrever o `return` exige o operador de propagação, que continua na tabela das
sete (`§5`). O que existe é a linha do §4.7 — teste da construção, tipo da
base, `return` do programa — e ela é a fronteira, não um degrau para outra coisa.

---

## 4.11 Despacho e açúcar de indexação


### Por que `at` existe, se já há verificação em `debug`

Porque as duas não verificam a mesma coisa, e tratá-las como uma só era o furo.

A verificação de `debug` em `get`, `set` e `ptr` cobre o índice que o **programa
produziu**: contador de laço, worker, resultado de `find`, aritmética sobre
`length`. Ali o erro é bug do programa, ele é reproduzível, e pegá-lo em
desenvolvimento é o suficiente — em release a checagem sai porque o que ela
protegia já foi consertado. Essa é a esmagadora maioria dos índices, e é por isso
que `x[i]` pode ser barato sem constrangimento.

O índice que vem de **dado** é outra espécie. Campo de arquivo, argumento de
linha de comando, byte de protocolo: nada no programa o limita, ele não é
reproduzível, e o valor mau chega **em produção**, que é exatamente onde a
checagem de `debug` não está. Consertar o programa não o elimina — ele é entrada
válida de um programa correto.

A tentação era ligar as checagens em release e chamar o problema de resolvido.
Ela é ruim por dois lados: paga em todo índice para proteger a minoria, e some
com a distinção — o programa deixa de dizer, no fonte, quais índices ele não
controla. `at` faz o contrário: **paga só onde é necessário, e escreve onde é.**
Ler `buffer.at` numa linha é ler "este índice veio de fora", e é informação que
nenhum `-D` de compilação carrega.

Que ele devolva `outcome` não foi decisão sua: é a regra do modo de falha
(§4.11) aplicada a um verbo que devolve valor. E que fora de faixa seja `NONE` e
não um código de erro é a mesma economia — nada deu errado, a posição
simplesmente não tem valor, que é o que `NONE` diz. `at` não precisou de
mecanismo nenhum: dois verbos que já existiam, compostos.

### Por que `at` não ganhou açúcar

Porque o açúcar tem de continuar significando *barato*. `x[i]` é a forma que
aparece em todo laço, e ela promete, pela grafia, que não há custo escondido. Uma
sintaxe para `at` — `x[[i]]`, `x[i]?`, qualquer uma — daria duas formas parecidas
com custos diferentes, e a diferença ficaria na pontuação.

Quem paga a verificação escreve o nome dela. É a mesma razão de `slice.from`
carregar o `T` e de `arena.from_stack` dizer a origem: **o que custa, ou o que
promete, aparece escrito.**

### Por que a tabela guarda valor-ou-ponteiro

É a única informação que o despacho precisa e que a expressão não dá. `x` sozinho
não diz se é `buffer i32` ou `buffer i32 *`, e emitir `&x` no caso errado é erro
do compilador C numa linha gerada — ou, pior, um `&` sobre ponteiro que compila e
aponta para o lugar errado.

Guardá-la na **declaração** é o que faz `buffer.length(b)` e
`buffer.length(w->ps)` saírem certos sem o parser tipar expressão nenhuma. É
informação da declaração, não da expressão, e é por isso que ela cabe na fase 1.

### Por que o qualificador não decide a chamada

Se ele decidisse, `slice.of(xs)` sobre um `buffer` teria que achar uma função em
`keel.slice` que soubesse ler um `buffer` — e aí keel precisaria escolher entre
candidatos pelo tipo do argumento, que é resolução de sobrecarga.

Deixando o despacho com o primeiro argumento, a escolha continua sendo pelo tipo
de **um** símbolo declarado, que é o que a posição de contêiner sempre foi. O
qualificador vira leitura, e é conferido — nunca consultado.

**Por que ele existe, então.** Porque um verbo nu não diz de quem é, e a
linguagem inteira é construída sobre saber de quem cada nome é. `length(b)` e
`push(x, v)` num arquivo com dez módulos importados são exatamente a ambiguidade
que o §4.1 gastou uma seção para eliminar em todo o resto.

### Por que o despacho tem um passo 2

Sem ele, um módulo comum não poderia declarar função cujo primeiro parâmetro é
contêiner: a chamada tentaria despachar, não acharia o verbo no tipo do
argumento, e falharia. Só módulo genérico teria verbos.

O passo 2 custa uma consulta a uma tabela que o `import` já carregou, e o que ele
compra é a stdlib de texto (§4.10) — dois módulos não-genéricos cujos verbos se
escrevem como os da base. Que o `&` venha do **parâmetro declarado** e não de a
chamada ser verbo é o que faz `buffer.push(sb, c)` e `strbuf.append(sb, s)` terem
a mesma forma; a informação é a mesma tabela do §4.11, lida do outro lado.

E ele não abre nada: pelo §1.3 keel só enxerga a interface de módulos que ele
mesmo gerou, então "`m` declara `f`" é consulta a declaração keel, nunca a header
C.

### Por que o verbo de conversão é qualificado pelo resultado

`slice.of(b)` e `buffer.length(b)` têm qualificadores que apontam para lados
diferentes, e a regra podia ter sido "sempre o do primeiro argumento" — que
daria `buffer.as_slice(b)`.

Não é o que se escolheu, porque o leitor de uma linha de conversão está
procurando **o tipo que vai ter na mão depois dela**. `slice i32 s = slice.of(b);`
diz `slice` duas vezes e casa; `slice i32 s = buffer.as_slice(b);` obriga a ler
até o fim para saber o que sai. É a mesma economia que faz `of` e `from` terem
nomes diferentes em vez de aridades diferentes do mesmo nome (§4.11).

O custo é que a regra tem dois casos em vez de um, e o error 109 existe para que a
diferença não seja aprendida por tentativa.

### Por que `buffer.from` toma a instância do alvo

É a única construção do documento em que o alvo decide, e a exceção se paga
porque `buffer.from` **só faz sentido quando o buffer é guardado**: quem não
guarda não tem onde empilhar. A forma sem alvo não tem uso legítimo, e por isso
recusá-la (113) não custa nada.

`slice.from` não podia fazer o mesmo: a vista é consumida na hora, dentro de uma
chamada, e não há alvo nenhum. Daí ela carregar o `T` — que é a mesma razão de
`arena.alloc(a, T, n)` carregar o dele.

### Por que `get` é error sobre elemento-contêiner

```c
/* o que sairia, se a checagem não existisse */
keel_buffer_i32 tmp = keel_buffer_buffer_i32_get(&grid, 3);
keel_buffer_i32_push1(&tmp, 42);
```

Compila sem um aviso sequer: copiar struct é operação legítima, e `&tmp` é
endereço perfeitamente válido — de um temporário. O `len` do elemento real não
sobe, e o dado empilhado desaparece.

É a mesma falha do parâmetro por valor, uma indireção adiante. A diferença é que
aqui **nem existe assinatura onde marcar `byref`**: quem copia é o `get`, e o
marcador está no tipo do elemento, não no do contêiner. Por isso a verificação
tem que ser do parser, e por isso ela está na lista do que um modificador do
usuário ainda não consegue exprimir sozinho.

`x[i]` é a saída, e é por isso que ele é definido como `*ptr(x,i)` e não como
`get`: lvalue com aliasing correto, que é o que se queria desde o começo.

### Por que o açúcar baixa por função e nunca por macro

Para que cada argumento seja avaliado exatamente uma vez. `x[i++]` incrementa `i`
uma vez só, e uma macro não teria como prometer isso sem um temporário por
argumento — que é código que ninguém escreveria à mão.

É também o que torna `x[a..]` a única forma que precisou de restrição: ali o
contêiner sai **duas** vezes, e reemitir `grid[i++]` avaliaria o `i++` duas
vezes. A exigência de caminho sem índice (84) é o preço, e ela é decidível por
varredura.

### Por que `x[i]` é lvalue e `x[a..b]` é rvalue

Não é escolha: `slice.of` devolve descritor por valor, e não há o que
desreferenciar. O asterisco de `*ptr(x,i)` simplesmente não aparece na segunda
forma.

A consequência é boa e não custou nada: `xs[2..7] = s;` é erro do compilador C
sobre um rvalue, no ponto certo, e keel não precisou de diagnóstico próprio.

### Por que não existe `x[i, a..b]`

A forma com vírgula despacha **contando** argumentos, e um intervalo num dos
eixos exigiria escolher o verbo pela *forma* de cada argumento: `m[i,2..8]` e
`m[2..8,i]` têm a mesma aridade e destinos diferentes.

Isso é regra de despacho nova, e compraria só ergonomia — o tipo de retorno já
existe (`view`, §4.10) e o tipo de argumento que faz o verbo ler bem também
(`range`, §4.5). `view.sub(v, 2..8, 0..4)` é a escrita, e **recorte em mais de
uma dimensão é verbo de biblioteca**.

É o mesmo teste dando respostas opostas em dois lugares, e é isso que o torna
útil: *dá para decidir olhando só os tokens?* Em `xs[2..7]` o `..` está à vista, e
entra. Em `xs[r]` não há nada à vista — distinguir de `xs[i]` exigiria tipar o
índice —, e por isso a escrita é `slice.of(xs, r)`, verbo comum, sem açúcar.

---

### Por que o alvo do `from` pode ser campo

A regra dizia "símbolo", e struct de contexto inicializa buffer em campo —
`p->ns = buffer.from(…)` é a forma comum, não a exótica. A informação que decide
a instância é a mesma nos dois casos: a declaração do campo, que keel colheu
(§4.3). Restringir a símbolo não protegia nada; só recusava o caso frequente.

### Por que o aviso de `byref` não vale sobre verbo produtor

O warning existe para pegar `a = b;` entre dois donos, que copia o ponteiro
junto. `pts = buffer.from(…)` não tem dois donos: não havia dono antes. Como
estava escrito, o aviso disparava sobre o construtor canônico — inclusive nos
exemplos deste documento —, e aviso que dispara no caminho normal é aviso que se
aprende a ignorar, que é a pior coisa que um diagnóstico pode virar.

### Por que o aviso de campo não vale dentro do módulo

Os campos são do backend **para quem usa**. Para quem declara o modificador, eles
são o layout que ele escolheu, e os verbos precisam tocá-los — não há outra forma
de escrever `length`. A regra global tornaria impossível implementar um
modificador de usuário sem ruído, o que seria estranho num documento cujo §4.9
mostra `return s->len;` como exemplo.

---

## 5 O que keel não tem

### A última linha é diferente das outras seis

`soa` merece nota porque a sua ausência não se sente como falta de
expressividade. **C expressa SoA perfeitamente** — `struct { f32 *x; f32 *y; }`
é a estrutura, e quem a escreve assim não precisa de palavra nenhuma. O que uma
construção `soa` compraria é escrever AoS e guardar SoA: afordância de
paradigma, não capacidade ausente. É a única das sete que falha o teste de
admissão de keel por esse motivo, e não por tipagem.

Mas ela falha o de tipagem também, e é por isso que está na tabela. Aqui ela
seria mutilada: sem o tipo dos campos não há como distinguir ponteiro-para-vetor
de ponteiro-para-grafo, nem decidir entre `pos[N][3]` e `pos[3][N]`, nem descer a
inversão por campos que são eles próprios invertíveis. As três limitações são a
mesma falta.

E a saída dela tem um efeito que vale registrar: **`soa` era a única coisa que
obrigava keel a enumerar os campos de um agregado.** Sem ela, o parser nunca
precisa olhar dentro de um struct para saber o que tem lá — só para registrar as
declarações que são dele. A invariante fica mais estreita, que é sempre a
direção certa.

## 7 Conformidade


### Por que recusar demais também é não-conformidade

A metade que costuma faltar numa cláusula de conformidade é a de baixo:
**programa que a spec aceita tem que compilar.** Sem ela, uma implementação
conservadora — que recusasse toda construção duvidosa — seria conforme, e a
linguagem passaria a ser o que a implementação mais restritiva aceita.

Para keel isso importa mais que o normal, porque a invariante do §1.3 é escrita
como proibição a quem implementa. Uma implementação que abrisse headers poderia
recusar programas legítimos com toda a boa intenção do mundo, e a cláusula é o
que a impede — e, no sentido inverso, poderia **aceitar** o que este documento
recusa, e aí o teste do §1.3 deixaria de decidir o que entra na linguagem.

### Por que a lista de comportamento definido pela implementação é fechada

Se ela fosse aberta, `parallel` num alvo sem threads seria "definido pela
implementação" e cada uma faria uma coisa. Fechando a lista, o que sobra é error
com nome e número — 103, 108, 14 —, e a compilação para dizendo qual recurso
falta, em vez de gerar código diferente em silêncio.

É a resposta que o §1.4 já dava para todo recurso opcional do C: **provar no C
gerado e parar**, porque keel não testa macro de plataforma e é isso que preserva
cross-compilation.

### Por que keel não acrescenta comportamento indefinido

Todo comportamento indefinido de um programa keel é do C gerado, e é do C. As
três superfícies listadas na §7.4 são promessas do programador, não construções
que a linguagem tenha inventado, e cada uma tem diagnóstico onde é decidível por
varredura — 102, 59 — e não tem onde não é.

Poderia ter sido diferente: bastaria uma verificação de execução em cada acesso
para transformar promessa em garantia. Custaria indireção, e indireção é o oposto
do que a arena existe para oferecer. É o princípio 6 pagando o
preço dele, com o preço escrito.

### Por que uma implementação não pode acrescentar palavra

Toda palavra de keel vale **por posição** (§3.10), e nenhuma custa um identificador
ao usuário. Acrescentar uma muda o sentido de programas já escritos que usem
aquele identificador naquela posição — em silêncio, e sem que o autor do programa
tenha como saber.

Uma extensão que faça isso não é extensão: é outra linguagem com o mesmo nome. O
que sobra para quem quer estender é o que a partição do §1.6 já entrega — **um
módulo** —, e é o mesmo teste valendo para quem implementa a linguagem e para
quem a usa.

### Por que a garantia de estabilidade é só sobre `import` sem `types`

É a única que se usa o tempo todo, e é a única que é verdade. `import M;` não
sombreia nada, porque nome externo é qualificado por omissão — então acrescentar
um não muda o sentido de nenhuma linha já escrita.

`import M types;` **pode** mudar: um nome que era token opaco do C, vindo de um
`typedef` que keel não enxerga, passa a ser reescrito. É por isso que a injeção é
explícita, que o error 16 existe, e que o import emite o `info` 14 listando o
que entrou nu. Prometer estabilidade ali seria prometer o que a §3.10 já mostra
ser falso.

