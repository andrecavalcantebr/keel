# cgen — Especificação da Ferramenta

> © 2026 Faculdade de Engenharia Elétrica e de Computação, Universidade
> Federal do Amazonas (FEEC/UFAM). Licenciado sob
> [CC BY-SA 4.0](LICENSE-DOCS.md) ([tradução](LICENSE-DOCS.pt.md)) — este
> documento é prosa sobre a ferramenta, não código; o `cgen` e a Base keel têm
> licença própria, ver [`LICENSE.md`](LICENSE.md) e a nota do §8. Escrita e
> revisão tiveram auxílio de Claude Opus e Claude Sonnet (Anthropic), sob
> direção humana.

**Documento normativo.** Especifica o cgen, transpilador de referência de keel para C — C11 ou C23, conforme o perfil da §4.9: o que depende da invocação.

Três documentos dividem o assunto, por um critério só:

| Documento | É dono de |
| --- | --- |
| `keel-spec.md` | o que depende apenas do fonte — sintaxe, semântica, diagnósticos |
| `keel-c-backend.md` | o que depende do alvo — mangling, artefatos, `#line`, propriedades do conteúdo gerado |
| **`cgen-tool-spec.md`** (este) | o que depende da invocação — linha de comando, raízes de busca, timestamps, E/S |

A fronteira com o backend é fina e vale enunciar: **o backend diz o que o conteúdo gerado tem que ser; a ferramenta diz como ele chega ao disco.** Determinismo e "instância de modificador embutido é função pura do nome" são propriedades do conteúdo, e estão em backend §7. Escrita atômica, comparação antes de gravar e critério de timestamp são desta especificação.

## O que é keel

keel é orientação a dados sobre C. O nome é o da quilha: a peça que corre por baixo e dá forma ao resto sem aparecer.

keel é um **parser de ilhas** — ilhas de keel num mar de C. Ele não substitui o C nem tenta entendê-lo: reconhece as suas próprias construções, translada cada uma para C, e deixa todo o resto atravessar intacto. É a invariante da linguagem §1.3, e é dela que decorre a forma desta ferramenta.

O `cgen` é o transpilador de referência, e é um **driver sobre o compilador C padrão**. Trocar `CC = gcc` por `CC = cgen` num `Makefile` existente produz o mesmo executável que se escreveria à mão em C, a partir de um fonte muito menor.

> **A ferramenta chama-se `cgen`; a linguagem chama-se keel.** Os fontes são `.k`, os símbolos gerados levam `keel_`, e a linha de comando é `cgen`. Os três nomes são estáveis e não se confundem: o binário é um, a linguagem é outra, e o prefixo reservado é do backend.

---

## 1. Motivação

O cgen deve poder **substituir `gcc` ou `clang` no build system** e tudo continuar funcionando como antes. Trocar `CC = gcc` por `CC = cgen` num Makefile existente é o teste de aceitação da ferramenta, não um caso de uso secundário.

A promessa é exatamente essa, e nada além: o build que existe continua valendo, e nenhuma parte dele precisa ser reescrita. **O que um fonte `.k` acrescenta é uma regra**, porque nenhum build system tem regra embutida para o sufixo — como aconteceria com qualquer linguagem nova no projeto. A §4.10 mostra qual é.

Disso decorre quase todo o resto deste documento: o cgen aceita e repassa as opções do compilador C, dirige a compilação ele mesmo, mantém o significado de `-o`, e produz dependências no formato que make e ninja já consomem. Nada disso é para ser elegante — é para que o build de quem adota keel não precise ser reescrito.

A invocação direta no terminal, com as opções próprias do cgen, continua valendo e é o modo de depuração (§4.2); a §4.4 mostra a linha resultante.

---

## 2. O que a ferramenta é

Um evocador do parser. Ela resolve caminho de arquivo, decide se algo precisa ser regerado, escreve arquivo e formata mensagem. Nada mais.

A premissa deste documento é que **a linguagem evolui e a ferramenta não**. O que se congela é pequeno: a linha de comando, a pasta de saída, o formato de diagnóstico e o código de saída. Construção nova na linguagem acrescenta no máximo nomes de diagnóstico.

Os módulos genéricos da linguagem §4.3 são a primeira construção que cobra algo além disso, e vale registrar exatamente o quanto: **uma opção**, `--instance` (§4.3), e nenhum diretório novo nem código de saída novo. A opção existe porque um genérico não é traduzível sozinho — só com argumento —, e quem quiser a instância como unidade própria, com `.c` e `.o`, tem de dizer qual; `--instance` é a declaração `instance` da linguagem escrita na linha de comando, e não acrescenta regra de conteúdo. O resto são duas regras internas, a §4.5 e a §5, e pelo mesmo motivo nos dois casos: um arquivo gerado deixa de ser função apenas do fonte que o pediu.

O `parallel` é a segunda, e **cobrou uma opção** — `--parallel-lowering` (§4.8). Fica registrado, com a razão: a linguagem não escolhe o mecanismo de execução, e diz isso por escrito (spec §4.8); quem escolhe é o backend, e quem informa a escolha é a invocação, porque o cgen não compila e só a linha de comando sabe o que o alvo oferece. Ela é da categoria de `--profile` (§4.9), não da de `--pedantic-names`: seleciona entre lowerings especificados e muda o C gerado, sem mudar o que o programa significa.

> **Teste de que a separação funcionou.** Este documento nomeia construções da linguagem — `import`, `module`, `instance` — apenas para dizer de quem é a responsabilidade em cada ponto. Em nenhum lugar ele depende do que elas **significam**: nenhuma regra daqui mudaria se `defer` fosse redefinido amanhã. Quando uma construção nova aparecer, o que se pergunta é uma coisa só — ela faz um arquivo gerado deixar de ser função apenas do fonte que o pediu?

---

## 3. Separação de E/S

O parser não abre arquivo, não escreve arquivo e não termina o processo. Ao encontrar um `import`, ele **chama de volta a ferramenta** para carregar o módulo, e recebe um código de retorno.

A separação é de responsabilidade, não de empacotamento: são módulos do mesmo fonte, conversando por chamada de função. Não há biblioteca instalável nem protocolo.

Um ponto de entrada de cada lado:

```plain
ferramenta → parser:  processa(fonte, nome_do_módulo) → artefatos, diagnósticos
parser → ferramenta:  carrega(nome_do_módulo) → ok | já_carregado | não_encontrado | erro
```

`já_carregado` é o caso comum a partir do segundo import do mesmo módulo: os símbolos já estão na tabela e o parser só emite a inclusão.

As duas chamadas são mutuamente recursivas, e é nessa pilha que vive a detecção de ciclo de import: "em carga" é `processa` já estar na pilha para aquele módulo. A regra é da linguagem (`keel-spec.md` §4.1); o que este documento registra é que ela não custa estrutura própria.

**Módulo genérico não acrescenta chamada nem estado a esta interface.** Ele é carregado como qualquer outro; o que muda é que o parser **retém o fluxo de tokens** em vez de descartá-lo depois de gerar, para substituir o parâmetro quando alguém escrever a instância. Reter é estado que depende apenas do fonte, logo é do parser — pelo critério de atribuição da tabela de abertura deste documento, a ferramenta continua dona só do que depende da invocação.

---

## 4. Invocação

```plain
cgen [opções do cgen] [opções do compilador C] <arquivo.k>
```

`cgen` é um **driver no molde do gcc**. Ele transpila e, por padrão, chama o compilador C sobre o que gerou. Substituir `gcc` por `cgen` num Makefile existente é a forma pretendida de uso, e é o que dispensa manifesto: quem sabe a lista de `.c` gerados é o próprio cgen.

A ferramenta processa o arquivo dado. Cada `import` encontrado dispara `carrega`, recursivamente.

### 4.1 Partição das opções

O cgen possui um conjunto **fechado e pequeno** de opções próprias. Elas são **sempre consumidas e nunca repassadas** ao compilador C — uma opção do cgen que vazasse para o `cc` faria o build inteiro falhar com mensagem que não ajuda ninguém.

Tudo que não estiver nesse conjunto é repassado verbatim, na ordem em que apareceu.

| Opção | Efeito | Padrão |
| --- | --- | --- |
| `-I <dir>` | raiz de busca de módulos; repetível, ordem significativa | `.`, só quando nenhum `-I` é dado |
| `--base-dir <dir>` | raiz da base; ver §8 | `<executável>/../lib/base` |
| `--instance "<M.mod> <args>"` | compila a instância, em lugar de um `.k`; ver §4.3 | — |
| `--dest-dir <dir>` | raiz da saída gerada; os componentes-pai do módulo viram diretórios sob ela, e o nome do arquivo é o símbolo (backend §4.1) | `./gen` |
| `--stop-after=<fase>` | interrompe após `lex`, `parse` ou `gen`; ver §4.2 | não interrompe |
| `--checks=on\|off` | verificações de limite no código gerado, o `debug` do [catálogo da spec](keel-spec.md#62-catálogo) | `on` |
| `--line=on\|off` | emissão de `#line` para o fonte keel; ver backend §6 | `on` |
| `--main <módulo>` | gera a unidade com o ponto de entrada do C, chamando o do módulo indicado; ver §4.7 | não gera |
| `--cc=<programa>` | compilador C a invocar | `cc` |
| `--pedantic-names` | baixa o teto de comprimento de nome gerado de 255 para 63; ver backend §2.4 | desligado |
| `--parallel-lowering=<auto\|serie\|openmp>` | mecanismo de execução emitido para `parallel`; ver §4.8 | `auto` |
| `--profile=<c11\|c23\|auto>` | padrão C a que o gerado se conforma; ver §4.9 | `auto` |
| `-f` | força regeração, ignorando timestamp | desligado |
| `--cgen-version`, `--cgen-help` | | |

`-o` **não** pertence ao cgen: é do compilador C e mantém o significado de sempre. Foi por isso que a saída gerada ganhou `--dest-dir`.

> **Nota — `-fno-strict-aliasing` é repasse, e é deliberado que seja.** O respaldo
> de tipo-caractere da arena é uma guarda nomeada da linguagem §6.3, e quem a
> honra é o backend (backend §5.4.1). Num alvo em que ela não se sustente, o
> remédio é essa flag do compilador C — que atravessa verbatim, como qualquer
> outra, **sem opção própria do cgen**. Ter uma seria uma segunda fonte de verdade
> sobre uma decisão que já é do `cc`, que é o mesmo argumento pelo qual o cgen
> **lê** `-fopenmp` em vez de ter uma opção que o ligue (§4.8). E ela não entra por omissão: o custo recai sobre todo
> programa, e o risco que ela paga não foi reproduzido em nenhum compilador
> testado.

> **Nota — por que `--pedantic-names` é opção e não padrão.** O teto de nome é do
> backend, não da ferramenta, e o backend já escolheu 255 pelo pior caso do
> levantamento (backend, Anexo). A flag existe para o alvo **fora** do
> levantamento — um linker antigo, um ambiente que só prometa o mínimo do padrão
> —, e ela só **aperta**: não há forma de subir o teto, porque acima de 255 não há
> o que prometer sem saber qual é o linker. É a mesma disciplina de `--checks`:
> a ferramenta liga e desliga verificação, nunca inventa garantia.

---

> **Nota — não há flag de VLA, e a de threads não é o que parece.** A primeira existiu enquanto `arena.from_stack` admitia tamanho de runtime; com a exigência de constante em tempo de compilação (linguagem §5.2) o lowering ficou único, e a flag que forçaria o outro caminho deixou de ter o que ligar. A segunda nunca existiu e não deve existir: **ligar** o OpenMP é da flag do compilador C — `-fopenmp` —, que o cgen repassa verbatim e apenas lê (§4.8). Uma opção do cgen que o ligasse seria uma segunda grafia para a mesma coisa, e as duas poderiam discordar. O que o cgen tem é outra coisa: `--parallel-lowering` **seleciona entre os lowerings que o backend especifica**, e por isso muda o gerado em vez de mudar a linha do `cc`.

Opção longa não reconhecida vai para o compilador C, e tem que ir: o próprio gcc tem opções longas (`--param`, `--sysroot`). Consequência aceita: erro de digitação numa opção do cgen chega ao `cc`, que reclama nomeando a opção.

**Quatro opções do compilador C são lidas pelos dois lados**, e nenhuma delas é filtrada: `-I`, para achar módulos; `-std=`, para o perfil (§4.9); `-fopenmp`, para o lowering (§4.8); e as de dependência — `-MMD`, `-MD`, `-MF`, `-MT` —, para o depfile (§4.10). Todas mantêm exatamente o significado do gcc e são repassadas íntegras; o cgen apenas também as lê, sempre pela mesma razão: a decisão já está escrita na linha, e ler é a única forma de os dois lados nunca discordarem. Nada impede que `.c` e `.h` escritos à mão convivam com os `.k` nos mesmos diretórios, e é isso que a passagem íntegra preserva.

O cgen **acrescenta** um `-I` a mais na chamada ao `cc`: o do `--dest-dir`, que é fonte confiável de C tanto quanto os diretórios do usuário. Sem ele as inclusões do código gerado não resolvem.

Sem arquivo de configuração e sem variável de ambiente: o que afeta a saída vem da linha de comando.

### 4.2 Parada por fase

O análogo de `-E`, `-S` e `-c`. Cada fase escreve em `stdout` e não invoca o compilador C.

| Valor | Interrompe após | Saída |
| --- | --- | --- |
| `lex` | tokenização | fluxo de tokens com posição |
| `parse` | reconhecimento e tabela de símbolos | ilhas reconhecidas e interface pública do módulo |
| `gen` | geração | escreve os `.c`/`.h` e para |

`--stop-after=gen` é o modo para quem prefere dirigir a compilação C por conta própria. Uma flag com enumeração, em vez de três flags, é o que permite acrescentar fase quando a linguagem crescer sem mexer na superfície congelada.

### 4.3 Compilação separada

Com `--stop-after=gen`, ou com `-c` repassado ao compilador C, cada módulo é compilado isoladamente — e não há nada a descobrir.

**Uma invocação escreve um `.c`, e só um: o do que ela compila** (backend §4.1). Módulos importados e instâncias implícitas saem só em headers, que não são alvo; o `.c` de um importado é escrito pela invocação dele. Não existe alvo gerado sem fonte correspondente, e portanto não há lista dinâmica para o build acompanhar por glob, manifesto ou arquivo agregador. Isso importa especialmente para ninja e cmake, cujos grafos são estáticos e montados antes da compilação: alvo novo aparecendo no meio do build seria justamente o que eles não sabem tratar.

**Módulo genérico não compila sozinho.** `cgen coll.k` sobre um módulo com parâmetro é `fonte-generico`: o parser não tem o que pôr no lugar de `T`, `N` ou `E`. Importado, ele gera os próprios headers — o que não menciona parâmetro, backend §4.4.1 — como qualquer módulo. Um genérico entra no build de duas formas, e a ferramenta não descobre nenhuma delas sozinha:

- **por uso**, com os corpos `inline` nos headers da instância e os fora de linha no `.c` do módulo que declara `instance` (linguagem §4.3) — um `.k` que o usuário escreveu, com o `.o` e a regra que ele mesmo pôs no build;
- **por invocação**, com `--instance`, que é a mesma declaração escrita na linha de comando:

```sh
cgen -c --instance "coll.stack i32" -o coll_stack_i32.o
cgen -c --instance "keel.buffer.buffer app.geom.Point" -o keel_buffer_app_geom_Point.o
```

O argumento é o uso como a declaração `instance` o escreve, com uma exigência a mais: **tudo é qualificado por inteiro** — o modificador, `keel.buffer.buffer` e não `buffer`, e os argumentos, `app.geom.Point` e não `Point` —, porque na linha de comando não há `import`, e é o nome inteiro que fixa sem ambiguidade o módulo, o modificador e o arquivo de saída. O último componente do nome é o modificador; o resto é o módulo, achado pelas raízes como num `import`. A invocação produz os três headers da instância e o `.c` dela — `gen/coll/coll_stack_i32.c` —, e o `.c` recebe os corpos fora de linha, como receberia o do módulo que declarasse `instance`. O nome do arquivo é o símbolo (backend §4.1), então a regra `%.o: %.c` sobre o destino vale sem exceção.

`--instance` ocupa o lugar do `.k`: uma invocação tem um ou outro, e ter os dois é `fonte-multiplo`. Sobre genérico inteiramente `pub inline` — toda a base —, o `.c` sai só com o include, e o aviso é o `instance-inutil` da linguagem.

**A colisão de símbolos é verificada no que a invocação alcança**, e é isto que a
linguagem §4.1 promete: o módulo em tradução mais o fecho transitivo dos seus
`import`. Módulos que não se importam não entram no conjunto de nenhuma invocação,
e a colisão entre eles é erro de link — a mesma falha, com a mesma mensagem, de
dois `.c` definindo o mesmo global. Acumular símbolos entre invocações para fechar
isso faria o diagnóstico depender da ordem e do histórico das execuções, contra a
§6.1.

A ferramenta também **não gera** o módulo de instanciação. Ela compila um módulo por vez e nunca tem o programa inteiro à vista, então não conhece a união das instâncias usadas; acumular a lista entre invocações faria o conteúdo depender da ordem e do histórico das execuções, contra a §6.1, e exigiria que o build iterasse até ponto fixo, contra a §1.

### 4.4 Exemplo

`src/main.k` declara `module main;` e importa `geom` e `net.http`.

```sh
cgen -I src -c src/main.k -o main.o -O2 -Wall
```

O cgen parseia `src/main.k`; cada import carrega e, se desatualizado, gera os headers do módulo importado. Ao final existem:

```plain
gen/main.type.h  gen/main.proto.h  gen/main.h  gen/main.c
gen/geom.type.h  gen/geom.proto.h  gen/geom.h
gen/net/net_http.type.h  gen/net/net_http.proto.h  gen/net/net_http.h
gen/keel.type.h  gen/keel.proto.h  gen/keel.h
gen/keel/keel_buffer_geom_Point.type.h  .proto.h  .h   ...
```

E a chamada emitida é uma só, sobre o módulo que foi pedido:

```sh
cc -c gen/main.c -o main.o -I src -I gen -O2 -Wall
```

`-c` continua significando uma unidade de tradução, um objeto. Os `.c` dos módulos importados não são escritos nem compilados aqui: eles têm regra própria, disparada por `cgen -c src/geom.k` e `cgen -c src/net/http.k`. **Importar não é compilar.**

**O fonte e o gerado não se chamam igual, e isso é regra e não descuido**:
`src/net/http.k` produz `gen/net/net_http.h`. O caminho do `.k` é o nome do
módulo, cobrado por `module-fora-do-caminho`; o nome do gerado é o símbolo, para
que o `#include` seja função do nome do tipo. O backend §4.1 dá a razão dos dois.
Uma regra de padrão do make sobre esses nomes precisa da transformação, não de
`%`: é um motivo a mais para o depfile da §4.5 ser a interface com o build.

O link é transparente — sem `.k` na linha, o cgen repassa tudo:

```sh
cgen -o prog main.o geom.o http.o
```

### 4.5 Depfile

O cgen emite dependências no formato de depfile do gcc — o mesmo de `-MMD`. É o único formato de interoperação que make e ninja consomem nativamente, e é o que informa ao build quando reexecutar o cgen porque um `.k` importado mudou. O gatilho, o alvo e a fusão com o que o compilador C reporta estão na §4.10; esta seção responde por **o que entra** no fecho.

**O fecho é transitivo sobre imports, não direto.** A regra não é "os módulos que este importa", e sim "todo `.k` alcançável a partir dele". O motivo é a instância: as funções saem inline num header, então toda unidade de tradução que a inclui embute o código. Editar um módulo genérico três níveis acima tem que retriggar quem alcança aquele header, e não apenas quem escreveu a instância.

Sem genéricos a diferença era invisível, porque o conteúdo da instância não dependia de fonte nenhum (§5). Com eles, um depfile direto deixaria um corpo antigo sobreviver dentro de um `.o` — falha silenciosa, e a pior classe de bug de build.

**O depfile e o critério da §5 são o mesmo fecho, vistos dos dois lados.** Este diz ao build **quando reinvocar** o cgen; aquele diz ao cgen **quando não pular**. Os dois têm de concordar, e concordam por serem o mesmo conjunto — o `.k` alcançável por `import`. Se só um deles fosse transitivo, o build reinvocaria o cgen para nada, ou o cgen regeneraria sem ninguém pedir.

**A metade do compilador C é o que ele reporta.** Cada módulo e cada instância
saem em `.type.h`, `.proto.h` e `.h` (backend §4.3.2), e é o compilador C que
reporta quais deles a unidade de fato incluiu. Nada muda aqui — o depfile
continua sendo o que ele reporta, fundido com o fecho de `.k`. Não há lista a
montar: a granularidade vem da separação dos arquivos, não de análise da
ferramenta.

### 4.6 Identidade do módulo

A identidade do módulo é **declarada no fonte**, e a ferramenta apenas confere se o caminho relativo à raiz concorda com ela (`keel-spec.md` §4.1).

Isso fecha um furo que a raiz sozinha não fecharia: `cgen src/a.k -I .` e `cgen src/a.k -I src` têm, cada uma, uma raiz só — passariam por qualquer regra de contagem de raízes — e ainda assim produziriam prefixos de símbolo diferentes para o mesmo arquivo, com falha só no link. Com a declaração, uma das duas invocações é diagnosticada.

O fonte também tem que estar sob exatamente uma raiz; sob nenhuma ou sob mais de uma, é erro.

A forma normalizada resultante é a mesma usada no `#line`, o que resolve de saída a diferença entre `./src/a.k` e `src/a.k`.

### 4.7 Ponto de entrada

A linguagem não fixa qual módulo é a entrada do programa: todo módulo pode ter a sua função de entrada, e ela é símbolo manglado como qualquer outro (`keel-spec.md` §4.1). Escolher qual delas vira o `main` do C é decisão de build, e é `--main <módulo>` que a expressa.

O argumento é **nome de módulo**, não caminho de arquivo — coerente com o que se escreve no fonte.

A unidade gerada é curta, vai para arquivo próprio sob o destino, e a sua forma está em backend §5.8.

**Ela não é escrita dentro do `.c` do módulo**, e o motivo é o critério da §5: se fosse, o conteúdo do gerado passaria a depender da flag de invocação, e o mesmo fonte produziria arquivos diferentes conforme quem chamou por último. O timestamp deixaria de significar o que significa. Em arquivo próprio, o `.c` do módulo continua sendo função pura do seu fonte.

Vale a mesma escrita idempotente da §6: rodar com e sem a flag não retriga compilação alheia.

Diagnósticos próprios:

- `--main` sobre módulo que não tem função de entrada.
- `--main` sobre módulo cuja função de entrada não está pública.

Sem a flag, nenhuma unidade de entrada é gerada — o caso normal de quem está só compilando módulos para objeto.

### 4.8 Lowering de `parallel`

A linguagem não escolhe o mecanismo de execução de um `parallel`: série, OpenMP, pool de threads ou outro produzem execuções que a semântica já permite, e nenhuma delas exige diagnóstico (spec §4.8). Quem escolhe é o backend; quem informa a escolha é a invocação, porque o cgen não compila e só a linha de comando sabe o que o alvo oferece.

| Valor | O cgen pede ao backend |
| --- | --- |
| `openmp` | o lowering por diretiva OpenMP |
| `serie` | as partes uma depois da outra, sem diretiva |
| `auto` | procura `-fopenmp` ou `-fopenmp=…` entre as opções repassadas: achando, `openmp`; senão, `serie` |

**`auto` é o padrão, e ele lê a linha de comando em vez de adivinhar.** A flag do OpenMP já está lá, escrita pelo usuário, e é a mesma que o `cc` vai receber — então lê-la é a única forma de os dois nunca discordarem. `-fopenmp` continua sendo repassado íntegro (§4.1); o cgen apenas também o lê, como faz com `-I` (§4.1) e com `-std=` (§4.9), e pelo mesmo motivo.

**Pedir `openmp` sem `-fopenmp` na linha faz o cgen recusar a invocação**, como qualquer combinação inválida de opções. O programa continuaria correto em série, mas o que foi pedido não pode ser entregue, e entregar outra coisa em silêncio é pior do que parar. Não há o caso inverso: `serie` está sempre disponível.

**Não há mais diagnóstico de indisponibilidade.** Ele existia quando o lowering era um só e a ausência de OpenMP era um desvio a relatar; com a escolha do mecanismo pertencendo ao backend, executar em série deixou de ser desvio e passou a ser uma das emissões previstas. Quem exige paralelismo de verdade escreve `--parallel-lowering=openmp`, e aí a ausência para o build no cgen, antes de compilar.

**Esta flag muda o C gerado**, e por isso entra na ressalva da §5, com `--profile`, `--checks` e `--line`. O que ela não muda é o conjunto de execuções permitidas, que é o mesmo nos três valores — é o que permite que o padrão seja `auto`.

---

### 4.9 Perfil de geração

O backend tem dois perfis — **C11** e **C23** —, e eles diferem na grafia do gerado
e em uma obrigação de diagnóstico, nunca no que a linguagem aceita (backend §9).
Quem decide é a invocação, porque o cgen não compila:

| Valor | O cgen emite |
| --- | --- |
| `c23` | perfil C23 |
| `c11` | perfil C11 |
| `auto` | procura `-std=` entre as opções repassadas: `c23`, `gnu23`, `c2x` ou `gnu2x` dão C23; **qualquer outro valor, e a ausência, dão C11** |

`auto` é o padrão e lê a linha de comando em vez de adivinhar — a mesma disciplina
do `-fopenmp` na §4.8 e do `-I` na §4.1. O `-std=` continua sendo repassado íntegro;
o cgen apenas também o lê, para que os dois nunca discordem.

**A ausência de `-std=` não é neutra, e a assimetria precisa estar escrita.** Um gcc
recente sem `-std=` compila em `gnu23`: o **compilador** está em C23 enquanto o
cgen, por omissão, emite C11. Isso não quebra o build, e a razão é uma propriedade
de mão única — **a saída do perfil C11 é C23 válido, e a recíproca é falsa.** Emitir
C23 para um compilador em modo C11 não compila; emitir C11 para um compilador em
modo C23 compila. Um padrão que só pode perder diagnóstico é preferível a um que
pode quebrar a compilação.

O que se perde nessa combinação está listado no backend §9.1, e é concreto: o
`[[nodiscard]]` dos builtins não é emitido, e o `constexpr` cujo valor não caiba no
tipo deixa de ser recusado. Recupera-se escrevendo o que já se queria dizer —
`-std=c23` na linha, ou `--profile=c23` se o padrão do compilador tiver de ficar
implícito por outra razão.

**Por que não perguntar ao compilador qual é o padrão dele.** Descobrir isso exige
invocar o `cc` e interpretar a resposta, que varia por compilador e por versão. Seria
uma segunda fonte de verdade sobre a mesma decisão, e as duas poderiam discordar — o
mesmo argumento pelo qual o cgen **lê** `-fopenmp` em vez de ligá-lo (§4.8). A regra
lê o que está escrito na linha, e só.

**Esta flag muda o C gerado**, como a `--parallel-lowering` da §4.8. Ela seleciona
entre lowerings especificados, como `--checks` e `--line`, e entra na ressalva da §5.

---

### 4.10 No build

Um build precisa de três coisas da ferramenta: um alvo previsível, dependências
que ele saiba ler, e nenhuma surpresa sob `-j`. Esta seção dá as três, e fixa o
que a §4.4 mostrava só por exemplo.

**Um fonte por invocação** — um `.k`, ou um `--instance` (§4.3). Mais de um é erro da ferramenta, com código 2.
O cgen dirige a compilação, e `-c` e `-o` mantêm o significado do gcc: um `.k`,
uma unidade de tradução, um objeto. Fontes que **não** são `.k` na mesma linha —
`.c`, `.o`, `.a` — atravessam para o compilador C como sempre, e é o que permite
um projeto misto.

**A regra canônica produz o objeto**, e o C gerado é intermediário que o build
nunca nomeia:

```make
CC     = cgen
CFLAGS = -I src -O2 -Wall
KFLAGS = --dest-dir gen

%.o : %.k
	$(CC) $(KFLAGS) $(CFLAGS) -MMD -c $< -o $@

prog: main.o geom.o net/http.o
	$(CC) $^ -o $@

-include $(OBJS:.o=.d)
```

**Quem quiser ver o `.c` escreve a outra regra**, e ela é escrevível porque o
alvo é previsível: a §4.6 exige que o nome do módulo concorde com o caminho
relativo à raiz, e o backend §4.1 dá o nome do gerado a partir do módulo. Para
módulo de um componente só, o `%` fecha; com diretório, o nome do arquivo repete
o caminho — `src/net/http.k` sai em `gen/net/net_http.c` —, e `%` não expressa
essa repetição, então a regra se escreve por módulo:

```make
gen/%.c : src/%.k
	cgen --stop-after=gen -I src --dest-dir gen $<

gen/net/net_http.c : src/net/http.k
	cgen --stop-after=gen -I src --dest-dir gen $<
```

Em ninja, a mesma coisa com `deps = gcc`:

```ninja
rule keel
  command = cgen --dest-dir gen -I src -MMD -MF $out.d -c $in -o $out
  depfile = $out.d
  deps = gcc
```

#### O depfile

O gatilho é o do gcc: **sem `-MMD` ou `-MD` na linha, nenhum depfile é escrito.**
Com um deles, o arquivo sai onde `-MF` disser, ou no nome derivado que o gcc
usaria, e o alvo é o de `-MT`, ou o `.o`.

O conteúdo é que é próprio da ferramenta:

> **O depfile lista, numa regra só, o que o compilador C reportaria e mais o
> fecho de `.k` alcançável por `import`.** Os dois conjuntos vão fundidos, e não
> em duas regras.

As duas metades são necessárias e nenhuma cobre a outra: o compilador C reporta
os headers — gerados e do sistema — que a unidade incluiu; só o cgen sabe que
`gen/A.c` é função de `A.k` e de todo `.k` que ele alcança. Sem a segunda metade,
editar um módulo importado não retriga nada e o `.o` fica obsoleto em silêncio —
que é a falha da §4.5.

**Fundidas numa regra só, e não acrescentadas como segunda regra**, porque o make
aceitaria as duas mas o ninja lê a primeira e ignora o resto: uma regra a mais
seria silenciosamente perdida justamente onde o grafo é estático.

Em `--stop-after=gen` não há compilador C a consultar, e o depfile tem só o fecho
de `.k`, com o `.c` gerado como alvo.

#### Uma invocação gera os headers do fecho

`cgen -c src/main.k` também escreve os headers de `geom` se ele estiver
desatualizado (§4.4) — nunca o `gen/geom.c`. Para o build isso significa que
**duas regras podem escrever o mesmo header**, a de `main` e a de `geom`, e as
duas propriedades que tornam isso inofensivo já estão especificadas: o conteúdo é determinístico (backend §7.1), então os dois
escritores produzem os mesmos bytes; e a escrita é temporário mais `rename`
(§6), então nenhum leitor vê arquivo pela metade. O trabalho repetido é absorvido
pela regra de não tocar arquivo idêntico (regra 2 da §6): o segundo escritor compara,
encontra igual, e não mexe no timestamp.

O `.c`, esse, tem um escritor só: a regra do próprio módulo.

A consequência prática é que o build **não precisa** declarar os headers dos
módulos importados como saídas da regra. Ele declara um alvo por módulo, e a
redundância se paga sozinha.

---

## 5. Critério de atualização

O mesmo do make, resolvido com chamadas ao sistema.

> **O gerado de um módulo é função do fonte dele e da interface dos módulos que
> ele importa** (backend §7.2). O critério compara, então, o gerado com **o
> mais novo entre o próprio `.k` e os `.k` alcançáveis por `import`,
> transitivamente**.

A interface de um módulo são dois arquivos desde o backend §4.3.2 — o `.proto.h`
e o `.type.h` que ele inclui —, e o critério é o mesmo para todos os gerados do
módulo — os três headers, e o `.c` quando é ele o compilado: **o operando da
comparação é o mais antigo deles**. Comparar contra um só deixaria
passar o caso em que a geração anterior parou no meio.

- gerado não existe, ou é mais antigo que esse máximo → parseia e gera
- gerado é mais recente → não parseia; **mas os símbolos precisam entrar na tabela**

**Por que o fecho transitivo, e não só o próprio `.k`.** A tradução de `A` lê a
interface de `B`: o `&` de adaptação vem do parâmetro declarado no callee, e o
despacho lê a assinatura de lá (linguagem §4.4). Trocar `slice i32` por
`slice i32 *` num `pub` de `B` muda o C de `A` sem editar `A.k`. Comparando só
contra o próprio fonte, `A.k` continua mais velho que `gen/A.h`, o cgen pula, e o
`.c` obsoleto vai para o compilador C.

**A metade que já estava certa era a de fora.** O depfile é transitivo (backend
§7.3), então o make **reinvoca** o cgen sobre `A`. O que faltava era esta metade:
sem ela o cgen é reinvocado e não faz nada, e as duas não se encontram. O sintoma
é build que quebra com erro de tipo do compilador C depois de editar um arquivo
que não é `A.k`, e que só `-f` conserta.

**Custa o que já se pagava.** O cgen visita cada import ao carregá-lo, então o
`stat` do `.k` importado já acontece; o que se acrescenta é reter o máximo ao
subir de volta na recursão. Módulo já visitado nesta invocação não é revisitado,
e o fecho é finito porque import circular é diagnóstico.

O segundo caso é a única sutileza. Pular o parse economiza geração, não conhecimento: o parser ainda precisa dos símbolos públicos do módulo para despachar builtin e validar uso. Duas saídas possíveis, e a escolha é de implementação:

1. Parsear o fonte mesmo assim, apenas registrando a interface pública e sem gerar. Simples, custa um parse.
2. Manter um arquivo de interface ao lado do gerado, para ler em vez de reparsear. Mais rápido, mais uma coisa a manter em dia — e ela **tem de registrar também a lista de `import`**, senão o critério acima não é avaliável sem abrir o `.k`.

A v1 vai pela primeira. A segunda é a porta de saída se o tempo de build incomodar, e não muda nada fora daqui.

**A decisão sobre um módulo é tomada depois de carregar os imports dele**, que é a ordem em que a recursão já acontece: primeiro o fecho, depois a comparação. Não há mudança de ordem a fazer — só o valor a reter na volta.

Import circular é diagnóstico. Módulo já carregado nesta invocação não é reprocessado.

**A comparação é de timestamp, e opção não tem timestamp.** É a limitação que
sobra depois do fecho transitivo, e ela é de outra espécie: as quatro opções que
selecionam lowering — `--checks`, `--line`, `--profile` e `--parallel-lowering` —
mudam o conteúdo do gerado sem tocar fonte nenhum, então trocar qualquer uma delas **não** invalida o
que já está no `--dest-dir`: os gerados continuam mais recentes que o `.k` e o módulo é pulado. É
para isso que existe o `-f`, e trocar de perfil sem ele deixa a árvore com módulos
dos dois. Uma implementação pode registrar as opções de lowering ao lado do gerado e
comparar também por elas; a v1 não o faz, e a obrigação fica com quem invoca.

### 5.1 Headers de instância

Instância de modificador **embutido** — `buffer`, `slice` — tem conteúdo que é função pura do nome, e o nome é o nome do arquivo (backend §7). A verificação é um `stat`: existe, pula; não existe, gera e escreve. Não se lê nem se compara.

Instância de modificador **do usuário** não tem essa propriedade. O conteúdo de `coll_stack_i32.h` é função do nome **e do corpo de `coll.k`**: editar o `push` do genérico muda o arquivo sem mudar o nome dele.

> Para instância de genérico do usuário, o critério é o **timestamp do fonte do módulo genérico** contra o do header de instância. Mais recente, regenera.

Vale para os três headers da instância, e não só para o `.h`: editar `coll.k`
pode mexer no corpo do modificador tanto quanto nos verbos. A comparação é contra
o mais antigo dos três, pela razão da §5.

É a mesma comparação da §5, com o fonte do genérico no lugar do fonte do módulo — nenhum mecanismo novo, apenas outro operando. O mesmo vale para o `.c` de um módulo que declare `instance`, e para o `.c` de uma instância pedida por `--instance`: ele depende do corpo do genérico, não só do próprio fonte.

**Limitação conhecida:** se a própria ferramenta mudar, os gerados ficam obsoletos sem que nenhum timestamp de fonte acuse. Apagar `./gen` resolve, e `-f` existe para isso. É a mesma limitação do make.

O caso irmão — "o módulo genérico mudou" — **não** é limitação: ele tem fonte, tem timestamp, e está coberto pela regra acima.

---

## 6. Escrita de arquivos

Três regras, todas com motivo concreto:

1. **Gerar em memória primeiro.** Não se escreve nada se houver diagnóstico de severidade `error` — a ferramenta não deixa saída pela metade. É o que dá sentido operacional ao "`error` interrompe a geração" da spec da linguagem.
2. **Não tocar arquivo idêntico.** Comparar o conteúdo gerado com o existente; se igual, não escrever, nem o timestamp. Senão a compilação C é retriggada à toa, e o critério da §5 passa a mentir.
3. **Temporário e rename.** Escrever em temporário **no mesmo diretório**, com nome único por processo, e renomear. `rename()` é atômico: nenhum leitor jamais vê arquivo pela metade. O mesmo diretório importa porque rename entre sistemas de arquivos deixa de ser atômico; o nome único, porque senão dois processos colidem no próprio temporário.

A regra 3 existe por causa de `make -j`: o mesmo header de instância pode ser gerado por dois cgen concorrentes e lido por um `cc1` no mesmo instante. O que torna essa corrida **inofensiva** é o determinismo do conteúdo, que é obrigação do backend (backend §7): os dois escritores produzem os mesmos bytes, então tanto faz quem renomeia por último.

### 6.1 Determinismo — as fontes de variação que são da ferramenta

Mesmas entradas, saída byte a byte idêntica, em qualquer máquina. A exigência é do backend; o que segue é a lista do que, na implementação, costuma quebrá-la.

O parser é determinístico por natureza — o risco está no gerador, e as fontes são poucas e conhecidas:

- **Cabeçalho de cortesia.** "gerado por cgen X em DD/MM/AAAA" faz todo arquivo diferir a cada execução, e a regra 2 da §6 nunca mais dispara.
- **Contador de temporários compartilhado pela invocação.** Temporários e rótulos numerados por um contador global fazem o mesmo módulo sair diferente conforme quantos módulos foram processados antes dele — e no modelo da §4 isso acontece sempre que se compila `A` que importa `D` em vez de `D` direto. O contador reinicia por função.
- **Iteração de tabela hash.** Emitir protótipos iterando o mapa de símbolos dá ordem variável, e com chave por endereço a ordem varia entre execuções da mesma entrada. Emitir em ordem de fonte, com lista ordenada ao lado do mapa.
- **Caminho como recebido.** `./src/a.k` e `src/a.k` são a mesma entrada e não podem gerar bytes diferentes em `#line` ou em include guard. Normalizar (§4.6).
- **Locale e ordem de leitura de diretório.** Ordenação e enumeração precisam ser explícitas, não herdadas do ambiente.
- **Conteúdo dependente da invocação.** Nada do que a linha de comando pede pode entrar no arquivo de um módulo; o que é pedido vira arquivo separado. É o que a §4.7 faz com o ponto de entrada, e a regra vale para o que vier depois.

Genéricos não abalam isso. O conteúdo de uma instância de genérico depende de mais uma entrada — o fonte do módulo genérico —, e não da invocação: dois processos concorrentes que leiam o mesmo `coll.k` produzem os mesmos bytes. O que essa entrada a mais muda é o critério de atualização (§5.1), não o determinismo.

É também o que permite teste por comparação de saída.

---

## 7. Diagnóstico e código de saída

```plain
<arquivo>:<linha>:<coluna>: <severidade>: <mensagem> [<nome>]
```

Formato do gcc, que editor e IDE já sabem parsear. Severidades `error`, `warning`, `info`. Contexto adicional em linhas `note:` seguintes — é onde entra, por exemplo, a nota de instanciação de backend §6.1.

Toda posição é no fonte `.k`, nunca no arquivo gerado.

Todo diagnóstico tem nome estável em kebab-case, usado por `-Wno-<nome>`. `error` não se desliga. **A lista de diagnósticos é a [tabela da spec](keel-spec.md#62-catálogo)**; aqui fica só a convenção de formato.

`-W<nome>` e `-Wno-<nome>` são lidos pelo cgen, e o repasse ao compilador C é
**condicionado**: repassa-se o que **não** é nome de diagnóstico do keel. Um
`-Wno-` de nome do gcc precisa chegar lá; `-Wtypes-sombreado` não pode, porque
opção `-W` desconhecida é **erro** no gcc — não aviso — e derrubaria a compilação:

```plain
gcc: error: unrecognized command-line option '-Wtypes-sombreado'
```

O filtro é o que a §7 já tem: a [tabela da spec](keel-spec.md#62-catálogo) é a lista dos
nomes que ficam. Nomes fora dela atravessam sem que o cgen os entenda, que é a
mesma disciplina de `-I` (§4.1). `-Werror` é a exceção que vale para os dois
lados, porque não é nome de diagnóstico — é o que transforma qualquer `warning`
do keel em parada, para quem quer o build estrito.

| Código | Situação |
| --- | --- |
| 0 | sucesso, com ou sem `warning` |
| 1 | pelo menos um `error` da linguagem; nada foi escrito |
| 2 | erro da ferramenta; a tabela é a da §7.1 |

### 7.1 Diagnósticos da ferramenta

A tabela da spec é a lista dos diagnósticos **da linguagem** — os que dependem do
fonte. Restam os que dependem da invocação, e eles são da ferramenta:

| Identificador | Condição |
| --- | --- |
| `fonte-multiplo` | mais de um fonte na mesma invocação: dois `.k`, ou um `.k` e um `--instance` (§4.10) |
| `fonte-generico` | o `.k` da invocação é módulo genérico, e não há `--instance` (§4.3) |
| `instancia-invalida` | `--instance` que não nomeia modificador de módulo genérico por inteiro, ou cujos argumentos não casam com os parâmetros dele (§4.3) |
| `base-nao-encontrada` | nenhuma das raízes da §8 contém `keel.k` |
| `fonte-nao-encontrado` | o `.k` nomeado não existe ou não pode ser lido |
| `fonte-fora-de-raiz` | o `.k` não está sob nenhuma raiz `-I` (§4.6) |
| `fonte-em-varias-raizes` | o `.k` está sob mais de uma raiz (§4.6) |
| `modulo-nao-encontrado` | um `import` que nenhuma raiz resolve (§3) |
| `main-sem-entrada` | `--main` sobre módulo sem função de entrada pública (§4.7) |
| `lowering-indisponivel` | `--parallel-lowering=openmp` sem `-fopenmp` na linha (§4.8) |
| `opcao-invalida` | opção do cgen com valor fora do enumerado |
| `falha-de-escrita` | E/S falhou ao escrever sob o `--dest-dir` (§6) |

Quatro regras governam a tabela:

- **Todos são `error`, e todos saem com código 2.** Não há `warning` de
  ferramenta: o que ela sabe, ela sabe antes de traduzir, e seguir adiante
  produziria artefato que ninguém pediu.
- **Nenhum se desliga.** Eles têm nome pela mesma razão que os da linguagem — a
  mensagem fica estável e o teste pode citá-la —, e não para que `-Wno-` os
  alcance. `-Wno-` continua valendo só para `warning` da linguagem (§7).
- **Os dois espaços de nome não se cruzam.** Um identificador desta tabela nunca
  aparece na da spec, e vice-versa; é o que permite ao filtro de `-W` da §7
  decidir olhando uma lista só.
- **A posição é do fonte quando existe uma.** `modulo-nao-encontrado` e
  `main-sem-entrada` apontam a linha do `import` ou o módulo pedido; os demais
  não têm posição, e saem no formato do gcc para erro de invocação:

```plain
cgen: error: mais de um fonte .k na invocação [fonte-multiplo]
```

---

## 8. Distribuição e a base

`cgen` é distribuído como uma árvore relocável: descompactar o `.tar.gz` (ou o
`.zip`) em qualquer lugar e pôr `bin/cgen` no `PATH` — por symlink ou pelo
próprio `PATH` — é a instalação inteira. `install.sh` e `uninstall.sh` fazem só
isso. A base nunca é referenciada por caminho absoluto fixo, então mover a
árvore não quebra nada.

```plain
keel-1.0.0-linux-x86_64/
  README.md
  install.sh  uninstall.sh
  bin/cgen
  lib/base/
    keel.k
    keel/
      arena.k  buffer.k  slice.k  outcome.k
      corot.k  range.k  tagged.k  routine.k  parallel.k
  lib/src/
    …            o fonte do cgen: ferramenta, parser e backend, com a base de apoio dele
```

**`lib/base` é a fonte dos módulos `keel.*`**, e o nome do módulo é o caminho:
`keel.k` é o prelúdio, `keel/buffer.k` é `keel.buffer`. **`lib/src` é o fonte
do próprio cgen**, para quem quiser reconstruí-lo. Na primeira versão ele traz o
C da sua base de apoio já gerado, porque o cgen ainda não compila a si mesmo; a
partir da segunda, traz `.k`.

**Resolução da base, nesta ordem:**

1. `--base-dir <dir>`, se passado.
2. `<diretório do executável>/../lib/base`, com o executável resolvido em tempo
   de execução **até o arquivo real**, seguindo symlink, pela chamada que o
   sistema oferece para isso — `/proc/self/exe` no Linux, `_NSGetExecutablePath`
   no macOS, `GetModuleFileNameW` no Windows; `argv[0]` e `PATH` onde não houver
   nenhuma.

Nenhuma das duas contendo `keel.k` é `base-nao-encontrada` (§7.1). **Não há
variável de ambiente**, pela §4.1: o executável sabe onde está, e o que sobra é
uma base fora da instalação, que é o que `--base-dir` cobre.

**A base é raiz de busca do cgen, e não do compilador C.** Ela entra como
**última** raiz de módulo — depois de todos os `-I` do usuário, inclusive o
default `.` — e não vai para a chamada ao `cc`: só tem `.k`, que o `cc` não lê.
Por ser a última, um `-I` de usuário que contenha um módulo `keel.*` sombreia o
da base — é assim que se desenvolve ou testa uma base alternativa sem reinstalar.

**A base é sempre lida, nunca escrita.** Cada projeto gera a sua cópia dos
headers dela no próprio `--dest-dir`, com o mesmo critério de atualização da §5,
e é esse o `-I` a mais que o `cc` recebe (§4.1). Não há cache compartilhado
entre projetos nem exigência de permissão de escrita sobre a instalação, e não
há checagem de versão entre binário e base: os dois chegam juntos no mesmo
prefixo, então são a mesma versão por construção.

> **Licença.** A base distribuída em `lib/base` é a mesma Base de
> `src/base/`, sob GPLv3 com a exceção que isenta o código que ela contribui
> ao gerado do usuário. Ver [`LICENSE.md`](LICENSE.md) e
> [rationale](keel-rationale.md#por-que-a-base-é-copyleft-com-exceção-e-não-gpl-simples-nem-mit).

---

## 9. Fora de escopo

| Item | Motivo |
| --- | --- |
| Manifesto próprio de dependências | o cgen dirige a compilação C ele mesmo (§4); no fluxo separado, a compilação separada da §4.3 e o depfile da §4.5 cobrem o caso, em formatos que os build systems já conhecem |
| Ser compilador C | o cgen invoca `--cc`, não substitui; diagnóstico de C vem do compilador C, com a posição já mapeada por `#line` |
| Package como unidade de distribuição | `module` resolve identidade; agrupamento e distribuição são outra coisa, e não são v1 |
| Arquivo de interface por módulo | otimização da §5, não requisito |
| Language server, formatador, depurador | superfícies próprias |
| Arquivo de configuração | tudo vem da linha de comando |
| Geração automática do módulo de instanciação | exigiria visão do programa inteiro, que o modelo por módulo não tem (§4.3). A forma que funcionaria é um modo de programa inteiro — percorrer os imports a partir de uma raiz e escrever o arquivo —, e ele não dispensa a regra de build, só a manutenção da lista; além disso não serve para biblioteca, que não tem raiz. Fica registrado como a saída caso a declaração manual incomode |
| Obrigar todo projeto a ter um módulo de instanciação | a ferramenta não tem conceito de projeto — identidade é do módulo (§4.6), e manifesto e configuração já estão fora. O diagnóstico que forçaria a regra dispararia em `cgen hello.k`, cobrando atrito no primeiro minuto de uso por um caso do décimo mês. O idioma de um `instances.k` por projeto é documentação, não regra |
