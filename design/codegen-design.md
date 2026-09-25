# codegen — desenho da emissão

> © 2026 Faculdade de Engenharia Elétrica e de Computação, Universidade
> Federal do Amazonas (FEEC/UFAM). Licenciado sob
> [CC BY-SA 4.0](../LICENSE-DOCS.md) ([tradução](../LICENSE-DOCS.pt.md)) —
> plano de implementação, não contrato normativo; o `cgen` e a Base keel têm
> licença própria, ver [`LICENSE.md`](../LICENSE.md). Escrita e revisão
> tiveram auxílio de Claude Opus (Anthropic), sob direção humana.

**Documento de implementação.** Cobre o `engine/emit/` da
[`cgen-tool.md`](cgen-tool.md) §3.1 — a parte do cgen que transforma o que o
parser entendeu nos arquivos que o compilador C vai ler. O contrato do conteúdo
gerado é [`keel-c-backend.md`](../keel-c-backend.md) (o "backend"); o da
linguagem, [`keel-spec.md`](../keel-spec.md). Este documento não acrescenta
regra: ele fixa **como** produzir o que o backend exige, e em que ordem, para
que a implementação não precise reconstruir o raciocínio a cada construção.

Onde **decide** algo que os normativos deixam aberto, a decisão vem marcada
**[E*n*]** e listada na §11.

---

## 1. Escopo

O emissor recebe um módulo já parseado e a tabela de símbolos, e produz de dois
a três arquivos de texto (backend §4.1). Não abre arquivo, não escreve em disco
e não chama o `cc`: devolve buffers ao `tool.c`, que compara e renomeia
(spec da ferramenta §6). Não emite diagnóstico de linguagem — o parser já os
emitiu; os dois que são dele estão na §5.

**A saída é função da entrada, byte a byte** (backend §7.1). Essa é a
propriedade que governa todas as decisões abaixo: onde houver liberdade de
ordem, de espaçamento ou de escolha de nome, a implementação tem que fixar uma,
e a fixação tem que ser escrita aqui ou no backend. Não há "tanto faz".

**O golden é a especificação executável deste documento.** `golden/c23/` e
`golden/c11/` são o que o emissor deve produzir a partir de `base/`, e
`golden/cases/*/expected/` a partir de cada `.k` de caso. Quando este texto e o
golden discordarem, o golden está certo até que alguém decida o contrário e
mude os dois.

---

## 2. Os arquivos

O emissor mora em `engine/emit/` (desenho do cgen §3.1, **[D8]**) e não é um
arquivo só. **[E5] Um `.c` por funcionalidade**, com a mecânica comum em
arquivos próprios.

```plain
engine/emit/
    emit.c        o orquestrador: monta os três arquivos e chama o resto
    writer.c      KWriter — os dois contadores, o #line, o buffer      §8
    mangle.c      nome canônico → símbolo, arquivo, guarda             §5
    layer.c       camadas L0–L3, posicionamento, as quatro seções      §4
    instance.c    fecho, superfície degenerada, emissão de instância   §7
```

E uma por construção, na correspondência com as seções do backend §5:

| Arquivo | Backend | Construção |
| --- | --- | --- |
| `decl.c` | §5.1 | declarações; substituição local de nome |
| `container.c` | §5.2 | struct de instância e seus verbos |
| `index.c` | §5.3 | `x[i]`, `x[i,j]`, e o `assert` sob `--checks` |
| `arena.c` | §5.4 | a reescrita de `alloc(a,T,n)` e `from_stack` |
| `defer.c` | §5.5 | varredura de saídas e injeção do corpo |
| `match.c` | §5.6 | `tags` e o despacho |
| `loop.c` | §5.7 | `foreach` e `apply` |
| `entry.c` | §5.8 | a unidade de ponto de entrada |
| `parallel.c` | §5.9 | o gestor, o `#pragma` e os dois lowerings |
| `result.c` | §5.12–5.14 | `else`, `at`, `outcome` e `corot` |

Três observações sobre o corte:

**As seções 5.10 e 5.11 do backend não têm arquivo.** `keel.routine` e o par cursor/partição são
funções de instância como outras quaisquer — quem as emite é `container.c`. Um
arquivo para elas seria um arquivo vazio com um comentário dizendo que não faz
nada, e a tabela acima já diz isso melhor.

**`result.c` agrupa três seções** porque `else`, `at` e os produtores de
`outcome`/`corot` são a mesma peça vista de três lados: `x = f() else …` testa
o `failed` que `at` produz e que `win`/`fail` escrevem. Separá-los daria três
arquivos que só se chamam entre si.

**A fronteira que interessa é a do `writer.c`.** Nenhum dos arquivos de
construção escreve texto direto no buffer: todos passam pelo `KWriter`, que é
o único que conhece os contadores de linha. É a decisão **[E4]**, e é ela que
dá sentido ao corte — cada arquivo de construção decide **o quê** emitir, e um
arquivo só decide **onde a linha cai**.

O acoplamento que sobra entre eles é pequeno e num sentido só: `emit.c` chama
os demais; os demais chamam `writer.c`, `mangle.c` e `layer.c`. Construção não
chama construção, com uma exceção inevitável — `defer.c` roda **depois** de
todas, sobre o corpo já emitido, porque precisa dos pontos de saída que as
outras criaram (backend §5.5.1, "`defer` é a última passagem de fluxo").

---

## 3. A entrada

```c
typedef struct {
    KModuleDecl   *decl;        /* o que o .k declara, na ordem do fonte */
    KSymbolTable  *syms;        /* símbolos locais e o fecho de imports   */
    KInstanceSet  *instances;   /* as instâncias que os usos pediram      */
    bool           compiled;    /* é o módulo da invocação?               */
    KProfile       profile;     /* c11 | c23                              */
    KEmitOptions   opts;        /* --checks, --line, --parallel-lowering  */
} KEmitInput;
```

**`decl` preserva a ordem do fonte.** Não é conveniência: a ordem de emissão é a
ordem de declaração (§6), e um emissor que reordenasse teria que justificar a
nova ordem e mantê-la estável.

**`instances` já vem fechado.** Quem o fecha é o parser, ao resolver os usos;
o emissor não descobre instância nova enquanto emite. A razão está na §7.

---

## 4. Os três arquivos

O emissor produz `.type.h`, `.h` e — só quando `compiled` — `.c`. O backend §4.1
dá a tabela de quem vai onde, e ela se lê como uma função de duas entradas:

```
posição(declaração) = f(visibilidade, camada)
```

| Escrita no `.k` | `.type.h` | `.h` | `.c` |
| --- | --- | --- | --- |
| tipo, `pub` ou `priv` | definição | — | — |
| `constexpr` de módulo | definição | — | — |
| `import_c` | `#include` | — | — |
| função `pub` | — | protótipo | corpo |
| função `pub inline` | — | protótipo, depois `static inline` + corpo | — |
| função `priv` | — | — | corpo, com `static` |
| variável `pub` | — | `extern T v;` | `T v = …;` |
| variável `priv` | — | — | definição, com `static` |
| `extern_c { … }` | — | — | conteúdo intacto |

Três armadilhas que o backend nomeia e que o emissor tem que acertar na
primeira vez, porque as três compilam e só falham depois:

1. **Variável pública nunca sai `static` no `.h`.** Compila, linka, e dá uma
   cópia por unidade de tradução — um módulo escreve, outro lê zero, sem aviso.
2. **Tipo `priv` vai para o `.type.h`**, não para o `.c`. Privacidade é do keel;
   o arquivo não é o mecanismo.
3. **`import_c` vai para o `.type.h`**, a camada mais baixa, porque um tipo do
   módulo pode precisar do header (`pub struct Log { FILE *f; };`).

### 4.1 As quatro seções do `.h`

O `.h` tem quatro seções, nesta ordem (backend §4.3.2, regra 2), e o runner do
golden verifica a ordem sem compilar nada (invariantes I1 e I2):

```
1. #include do próprio .type.h, e do .type.h de todo tipo por valor
2. protótipos e declarações extern
3. #include do .h de cada módulo ou instância que os corpos chamam
4. corpos
```

**A seção 2 antes da 3 é o que permite recursão mútua** entre corpos do módulo,
e é por isso que `pub inline` leva protótipo *e* corpo, não só o corpo.

**A seção 3 depois da 2 é o que quebra o ciclo de chamada.** O ciclo de layout
já é acíclico pelo corte em dois; o de chamada fica inofensivo por esta ordem.

### 4.2 O cabeçalho e a guarda

Toda saída abre com uma linha (desenho do cgen §5.3) e fecha com a guarda
comentada:

```c
/* <caminho relativo a --dest-dir> — generated from <caminho do .k> by cgen, C23 profile. */
#ifndef <GUARDA>
#define <GUARDA>
…
#endif /* <GUARDA> */
```

A guarda é o caminho relativo em maiúsculas, com todo caractere fora de
`[A-Z0-9]` virando `_`. **[E1] O `#endif` leva sempre o comentário da guarda**,
em módulo e em instância — o golden tinha as duas formas, e um gerador só não
pode ter duas.

Não há data nem versão no cabeçalho: entraria no arquivo e quebraria o
determinismo.

---

## 5. Nomes

O mangling é do backend §2.1 e se resolve por concatenação, sem tabela:

```
símbolo   = <módulo com . → _> [ _ <argumento canônico> ]* [ _<sufixo de aridade> ]
arquivo   = <componentes-pai do módulo>/<símbolo>.<ext>
guarda    = <caminho> em maiúsculas, não-alfanumérico → _
```

Ordem importa: **primeiro o tipo, depois a aridade** — `keel_slice_i32_ptr1`,
nunca `keel_slice_ptr1_i32`.

O sufixo de aridade só existe **quando o verbo é declarado em mais de uma
aridade** (backend §2.1). A forma que recebe só o contêiner não leva sufixo; as
demais levam o número de argumentos além dele. Verbo de aridade única não leva
sufixo, quantos argumentos tenha — `get(b,i)` é `_get`, não `_get1`.

**A grafia gerada nunca contém `__` nem começa por `_` seguido de maiúscula.**
Qualificador que começa por `_` perde o underscore e baixa a caixa: `_Atomic u32`
dá `_Atomic_u32` como argumento canônico, e `keel_buffer__Atomic_u32` teria `__`
no meio — então sai `keel_buffer_atomic_u32`.

Duas verificações são do emissor, porque só ele conhece o nome final:
`name-too-long` (teto de 255; sob `--pedantic-names`, 31 para nome com ligação
externa e 63 para os demais) e
`canonical-name-collision`.

---

## 6. A ordem de emissão

> **[E2] A ordem de emissão é a ordem de declaração no `.k`, em toda seção.**

Dentro de cada uma das quatro seções do `.h`, e dentro do `.type.h` e do `.c`,
as declarações saem na ordem em que o fonte as escreve. Uma instância emite os
verbos na ordem em que o genérico os declara.

Parece óbvio e não é: o golden tinha `keel_outcome_i32.h` emitindo `value`(24)
antes de `code`(23), e `keel_arena.h` em ordem alfabética. Ordenar por outro
critério — alfabética, por camada, por uso — é estável e determinístico, e
ainda assim errado: quem lê o gerado ao lado do fonte perde a correspondência, e
o `#line` de cada declaração passa a saltar para trás.

A exceção é a **seção 1 do `.h`**, cujos `#include` não vêm do fonte. Ali a
ordem é: o próprio `.type.h`, depois os demais **na ordem em que o primeiro uso
aparece no fonte**.

---

## 7. As instâncias

### 7.1 Quem instancia é quem usa

Uma instância é criada pelo **uso**, não pela declaração do tipo argumento
(backend §4.3). Dois módulos que usam `buffer i32` geram o mesmo header, byte a
byte, e a segunda escrita é no-op — é isso que torna a corrida sob `make -j`
inofensiva.

### 7.2 A instância sai inteira

> **O header de instância embutida é função apenas do próprio nome** (backend
> §7.2).

Disso decorre a regra que mais custa a implementar e que é a menos intuitiva:
**a instância emite todos os verbos do genérico**, e não os que o módulo que a
pediu chama. Recortar faria o arquivo ser função de quem o usa, e aí dois
módulos com usos diferentes escreveriam conteúdos diferentes no mesmo caminho,
em laço, retriggando compilação — exatamente o que o §7.1 existe para evitar.

### 7.3 O fecho transitivo

Emitir a instância inteira puxa as instâncias que os verbos dela mencionam:

```
slice i32   ──at──▶   outcome i32
            ──clone──▶ outcome slice i32
buffer i32  ──clone──▶ outcome buffer i32
            ──as_slice, partition──▶ slice i32
```

O fecho é calculado pelo parser, antes da emissão (§3), e converge: `outcome T`
não declara verbo que crie instância nova.

**O grafo de layout tem de ser acíclico** (backend §4.3.1). Campo por valor é
L1→L1; campo por ponteiro é L1→L0 e corta a aresta. `layout-cycle` é o
diagnóstico quando não corta.

### 7.4 Instanciação degenerada

Nem todo verbo do genérico sobrevive a todo argumento (spec §4.3):

| Argumento | O que some |
| --- | --- |
| `void` | os campos `T campo`/`T *campo`, e todo verbo que mencione o parâmetro em posição de valor — restam os de controle |
| `const T` | os verbos que escreveriam através do parâmetro |

**A omissão é transitiva**, e é aí que a implementação erra: um verbo cuja
emissão chamaria outro que não existe naquela instância também não é emitido.
O caso real, que o golden contém:

```
slice const char   perde set      (escreve s.ptr[i])
                   perde clone    (memcpy com destino const)
                   perde at       (chama outcome.win1, que outcome const char perdeu)
outcome const char perde value1, win1
```

A regra de parada é natural: o fecho de omissão converge porque o grafo de
chamada dentro de um genérico é finito e acíclico.

**Qualificador de topo não sobrevive à cópia**: `get` sobre `slice const char`
devolve `char`, não `const char`. O C ignora o qualificador em retorno por
valor, e emiti-lo só produz `-Wignored-qualifiers`.

Chamar um verbo que a instância não tem é `verb-not-in-instance`, e quem o emite
é o parser — mas ele precisa da superfície da instância, que é o emissor quem
calcula. **[E3] O cálculo da superfície é uma função pura do par (genérico,
argumentos), exposta ao parser**, não um efeito colateral da emissão.

---

## 8. O mapeamento de linhas

A invariante do backend §6 é mecânica e dispensa lista de casos:

> Dois contadores — a linha do `.k` em tradução e a linha da saída. **Sempre que
> divergem, emite-se `#line`.** Enquanto andam juntos, nada é necessário.

A implementação natural é um escritor que conhece os dois contadores:

```c
typedef struct {
    keel_buffer_char out;
    size_t           out_line;     /* linha corrente da saída        */
    size_t           src_line;     /* linha do .k que estamos emitindo */
    keel_slice_char  src_path;     /* caminho normalizado, com .k     */
    bool             line_enabled; /* --line=off desliga tudo         */
} KWriter;

void k_emit_at(KWriter *w, size_t src_line, keel_slice_char text);
```

`k_emit_at` compara, emite `#line` se preciso, escreve o texto e atualiza os
dois contadores. **Nenhum outro ponto do emissor escreve `#line`** — é o que
impede o mapeamento de se perder por um caso esquecido.

Quatro consequências que o emissor tem que honrar:

1. **Texto copiado nunca é reindentado.** Reformatar destrói a coluna de todo
   diagnóstico do compilador C naquela região.
2. **Em corpo de função, uma linha de fonte ocupa uma linha de saída.** Uma
   chamada de builtin longa sai numa linha só. É a única concessão deliberada ao
   princípio 2, e ela se paga: é o que faz o erro do C cair na linha certa.
3. **O `.c` preserva a estrutura de linhas do fonte.** O que foi para header
   deixa linha vazia. Um `#line 1` abre, e daí em diante só as expansões de
   várias linhas ressincronizam.
4. **Comentários viram espaços da mesma largura**, com as quebras preservadas e
   o espaço sobrando no fim da linha cortado.

**Declaração sintetizada não leva `#line`**: a declaração adiantada de um campo
por ponteiro, o `#include` do próprio `.type.h`. Falha ali é bug de ferramenta,
não erro do usuário, e apontar para o `.k` mentiria.

### 8.1 A nota de instanciação

`#line` leva o erro dentro de uma instância ao fonte do genérico — certo, mas não
diz **qual** instanciação quebrou. A instância carrega a posição do primeiro uso
que a criou, e o cgen acrescenta:

```plain
coll.k:14:12: error: invalid operands to binary + […]
sim.k:7:1: note: in the instantiation of coll.stack at geom.Point
```

Esse `note:` é do cgen. É a única informação que o mapeamento sozinho não
alcança, e ele exige que a instância guarde a posição de criação.

---

## 9. Determinismo: onde ele escapa

O backend §7.1 exige saída byte a byte idêntica em qualquer máquina. As fontes
de variação, e o que fazer com cada uma:

| Fonte | Remédio |
| --- | --- |
| Iteração sobre hash table | ordenar por chave antes de emitir, ou não iterar — usar a lista ordenada do fonte (§6) |
| Ordem de descoberta de instância | o fecho é ordenado pelo símbolo canônico |
| Caminho absoluto no `#line` | a string é o caminho **normalizado do módulo**, com `.k`, nunca o do sistema de arquivos |
| Data, versão, hostname | não entram em arquivo gerado, em hipótese alguma |
| Ordem de `readdir` nas raízes de `-I` | a resolução de módulo é pela ordem dos `-I`, não pela do diretório |
| `%p`, endereço, contador global | não aparecem em nome gerado |

**O teste é barato e deve existir desde o primeiro dia:** gerar duas vezes na
mesma árvore e comparar byte a byte; gerar com as raízes em ordem embaralhada e
comparar.

---

## 10. O que cada construção pede do emissor

A tradução de cada construção é do backend §5. A tabela abaixo diz o que cada
uma exige da mecânica deste documento, que é o que a implementação precisa ter
pronto antes de atacá-las:

| Construção | Backend | Precisa de |
| --- | --- | --- |
| declaração com modificador | §5.1 | mangling; troca local de nome, sem gramática de declarador C |
| contêiner (struct + verbos) | §5.2 | instância inteira (§7.2), fecho (§7.3), camadas |
| açúcar `x[i]`, `x[i,j]` | §5.3 | despacho por aridade; `assert` sob `--checks` |
| `arena` | §5.4 | reescrita de `alloc(a,T,n)` materializando `sizeof`/`alignof`/cast |
| `defer` | §5.5 | varredura dos pontos de saída do escopo; injeção em cada um, ordem inversa |
| `tags` e `match` | §5.6 | `switch` com exaustividade já verificada; `tag-out-of-range` sob `--checks` |
| `foreach`, `apply` | §5.7 | temporários de contêiner e comprimento, em bloco próprio |
| ponto de entrada | §5.8 | unidade separada, só sob `--main` |
| `parallel` | §5.9 | símbolo de controle fora do bloco; `#pragma` com cláusulas em ordem fixada |
| `keel.routine` | §5.10 | nada de especial: são funções de instância |
| cursor e partição | §5.11 | nada de especial: são verbos |
| `else` | §5.12 | declaração mais `if`, numa linha — sem `#line` |
| `at` | §5.13 | chamada de instância; o `if` é semântica, não verificação |
| `outcome`, `corot` | §5.14 | instância comum; `outcome void` é degenerada (§7.4) |

**Ordem de implementação sugerida**, que é a ordem em que o golden deixa de
falhar por mais casos, todas do backend §5: 5.1 → 5.2 → 5.3 → 5.5 → 5.7 → 5.12
→ o resto.
`defer` (backend §5.5) vem cedo de propósito: é a construção que mais mexe na estrutura
do corpo, e descobrir tarde que o escritor de linhas não a suporta custa caro.

---

## 11. Decisões deste documento

| | Decisão | Por quê |
| --- | --- | --- |
| E1 | `#endif` leva sempre o comentário da guarda | o golden tinha as duas formas; um gerador só não pode ter duas, e a comentada é a que ajuda em header longo |
| E2 | A ordem de emissão é a ordem de declaração, em toda seção | qualquer outra ordem é estável e ainda assim quebra a correspondência com o fonte que o `#line` promete |
| E3 | A superfície de uma instância é função pura de (genérico, argumentos), exposta ao parser | o `verb-not-in-instance` é do parser e precisa dela; calcular duas vezes, com dois códigos, é onde a divergência nasce |
| E4 | Todo `#line` sai por um único ponto do código, o `KWriter` | a invariante do §6 do backend é fácil de enunciar e fácil de furar; concentrá-la num lugar é o que a torna verificável |
| E5 | Um `.c` por funcionalidade em `engine/emit/`, mais cinco de mecânica comum | um `emit.c` único seria o maior arquivo do projeto e o que mais muda; o corte por construção faz cada mudança do backend §5.x cair num arquivo só |

---

## 12. Testes

O golden já é a suíte deste documento, e o runner já verifica o que dá para
verificar sem compilador: o corte em duas camadas (I1), a ordem das seções do
`.h` (I2), a existência do `.c` por módulo compilado e sua ausência em genérico.

O que falta acrescentar quando o emissor existir:

1. **Comparação byte a byte** do gerado contra `expected/` — hoje o runner
   compila o `expected/`, não o compara com nada, porque não há o que comparar.
2. **Determinismo**: gerar duas vezes, comparar; embaralhar a ordem das raízes,
   comparar.
3. **O caso do `#line`**: o 016 já afirma posições sem escrever número nenhum,
   lendo os marcadores `/* ERROR */` do próprio fonte. É o molde para os demais.

---

## 13. Pendências

| | Onde | Divergência |
| --- | --- | --- |
| C1 | backend §5.9 × golden | a ordem das cláusulas do `#pragma` está fixada no §10.6 do backend, mas nenhum caso exercita captura de dois escalares ou de dois `byref` — a ordem dentro de cada cláusula não está sob teste |
| C2 | este documento × golden | o fecho de instâncias da base foi calculado à mão nesta rodada; quando o emissor existir, o fecho que ele calcular tem que dar exatamente os mesmos 7 arquivos, e isso não está verificado por nada |
