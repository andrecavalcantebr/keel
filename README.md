# keel

**Um pré-processador de C com sintaxe própria.** keel reconhece suas construções
no meio do seu código C, traduz o que reconhece e copia o resto intacto. O que
sai é C legível, que o seu compilador compila.

```keel
module reading;
import_c <stdio.h>;

pub int first(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return EOF;
    defer fclose(file);
    return fgetc(file);
}
```

```c
int reading_first(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return EOF;
    int result = fgetc(file);
    fclose(file);
    return result;
}
```

`FILE`, `fopen`, `fgetc` e `EOF` keel não conhece, e não precisa: ela reconheceu
`module`, `pub` e `defer`, e o resto atravessou. É essa a ideia inteira.

> **Estado: a linguagem está fechada, o compilador não existe.** Os quatro
> documentos normativos estão completos e sem questão em aberto, a biblioteca
> base está escrita em keel, e há 21 casos de teste com a saída C esperada nos
> dois perfis. O `cgen` — a ferramenta que faria essa tradução — está em fase de
> desenho. Hoje o repositório é uma especificação com um oráculo executável, não
> um compilador que você possa rodar.

---

## Por que existir

C não tem como expressar certas coisas sem macro. keel dá sintaxe a elas, com
uma regra que não abre exceção: **toda construção tem uma tradução para C
escrita e legível**, e o que a tradução custa está documentado. Você nunca
precisa adivinhar o que foi gerado.

O critério de admissão é estreito de propósito. Economizar tokens não basta —
uma construção só entra se expressar uma operação ou uma garantia que C não
oferece, ou oferece de um jeito ruim.

**O que keel não faz:** não analisa suas expressões C, não resolve tipos de
headers, não deduz o tipo de uma chamada que não conhece. Quem valida tipo é o
compilador C, sempre. keel verifica apenas as regras das próprias construções —
e quando não consegue decidir, ela recusa em vez de adivinhar.

## O modelo: ilhas num mar de C

Seu arquivo `.k` é C. Dentro dele há ilhas que keel reconhece — pelas palavras
que ocupam certas posições, e pelos símbolos que ela mesma registrou. O mar em
volta é copiado byte a byte.

Isso tem consequências que vale saber desde o começo:

- **O dialeto do seu C é assunto seu.** Use `nullptr`, `typeof`, o que quiser:
  keel não olha. Se você gerar sob o perfil C11 um módulo que usa palavra do
  C23, quem reclama é o compilador C, com a mensagem dele.
- **Macros não participam.** keel roda antes do pré-processador C. O que ela
  precisa reconhecer tem que estar escrito, não vir de expansão.
- **Comentários e literais são opacos.** `/* grade[1,2] */` é comentário, não
  indexação.

## O que a linguagem tem

### Módulos

Um arquivo, um módulo, e o nome do módulo é o caminho do arquivo. Os símbolos
saem prefixados — `pub int first` vira `reading_first`. `import` traz
outro módulo keel; `import_c` entrega um header ao compilador C; `extern_c`
delimita C que atravessa sem nenhum tratamento.

### Modificadores

Um modificador atua sobre o tipo que o segue e define sua representação e suas
operações:

```keel
buffer i32 data;              // sequência que cresce, com cap e len
slice const char text;        // vista de extensão fixa, não possui nada
outcome i32 r;                // um i32 mais um código de resultado
tagged Cycle void state;      // uma etiqueta de um conjunto declarado
```

`buffer T` e `slice T` são o par memória/visão: um lado possui o armazenamento
e pode crescer, o outro descreve um trecho e não possui nada. A conversão só
faz sentido num sentido, e a linguagem impõe essa direção.

### Cleanup léxico

`defer` registra trabalho para a saída do escopo, em ordem inversa. Não há
runtime: o backend varre os pontos de saída e injeta o corpo em cada um.

### Travessia

```keel
foreach (i32 x, size_t i : xs) { ... }           // índice
walk    (i32 *p, buffer.cursor c : xs) { ... }   // cursor
apply   (i32, xs, doubler);                      // função sobre cada elemento
```

### Execução particionada

```keel
parallel search ANY (size_t w : 0..4; slice i32 part : xs; (target, where)) {
    foreach (i32 x, size_t i : part) {
        if (parallel.interrupted(search)) break;
        if (x == target) { set(where, w, i + 1); win; }
    }
}
return parallel.ok(search);
```

O bloco divide o contêiner entre workers e declara um símbolo de controle que
você consulta depois. Quem divide é o verbo `partition` do próprio contêiner,
não uma fórmula embutida — então funciona sobre qualquer tipo que o declare. O
mecanismo de execução (série ou OpenMP) é escolha da invocação, e não muda a
semântica.

### Resultado e estado cooperativo

`outcome T` é resultado final, de dois estados. `corot` é estado cooperativo, de
três — sucesso, andamento, falha. A cauda `else` trata a falha no ponto da
declaração:

```keel
outcome slice i32 copy = slice.clone(a, source) else return -1;
```

### Conjuntos de tags

`tags` declara um conjunto fechado; `match` despacha sobre ele com
exaustividade verificada:

```keel
tags Status [SUCCESS = -1, ONGOING = 0, FAILED = 1];
```

## Protocolos: como o seu tipo entra nas construções

Isto é o que mais distingue keel de um gerador de código com tipos
privilegiados. Uma construção do núcleo não conhece uma lista de tipos
abençoados — ela procura **verbos, por nome e aridade**. Declare os verbos e o
seu módulo participa.

| Para usar | Declare |
| --- | --- |
| `foreach` de dois binders, `apply`, `x[i]` | `length`, e `get` ou `ptr` |
| `foreach` de um binder | `first`, `limit` |
| `walk` | `begin`, `has_next`, `next` |
| `parallel` | `partition` |
| `x[a..b]` | o verbo de `range-index` do seu módulo, na aridade escrita — o nome é seu |
| `match` | `tag` |
| `else` | `failed`, e `win` para a forma com valor padrão |

Não há registro a fazer, marcação a escrever nem permissão a pedir. O PPC não
distingue um módulo da base de um módulo seu ao resolver.

## A base

Nove módulos, escritos em keel, com uma exceção de licença para que o C gerado
do seu programa não seja arrastado ao copyleft (veja abaixo). Todos exigem
import explícito — o único implícito é `import keel types;`, que traz só os
nomes de tipo primitivos.

| Módulo | O que traz |
| --- | --- |
| `keel.arena` | armazenamento por região, com tempo de vida explícito |
| `keel.buffer`, `keel.slice`, `keel.range` | as sequências, a vista e o intervalo |
| `keel.outcome`, `keel.corot` | resultado final e estado cooperativo |
| `keel.tagged` | valor com etiqueta |
| `keel.routine` | composição cooperativa, `seq` e `par` |
| `keel.parallel` | o símbolo de controle de um bloco `parallel` |

Os tipos primitivos são `i8`…`i64`, `u8`…`u64`, `f16`, `f32`, `f64` e `bf16`,
com largura e representação fixadas — não aliases de `int`.

## Como se compilaria

`cgen` é um driver no molde do `gcc`: transpila e chama o compilador C sobre o
que gerou. Trocar `gcc` por `cgen` num Makefile existente é a forma pretendida
de uso.

```sh
cgen -I src --profile=c23 src/app/main.k -o app
```

As opções próprias são poucas e fechadas; tudo o mais atravessa verbatim para o
compilador C. `-o` é dele, não do cgen.

## O repositório

| Caminho | O que é |
| --- | --- |
| [`keel-spec.md`](keel-spec.md) | a linguagem: o que depende só do fonte |
| [`keel-rationale.md`](keel-rationale.md) | as razões; nada ali é regra |
| [`keel-c-backend.md`](keel-c-backend.md) | o que depende do alvo: nomes, artefatos, perfis |
| [`cgen-tool-spec.md`](cgen-tool-spec.md) | o que depende da invocação |
| [`base/`](base/) | os nove módulos da base, em keel |
| [`golden/`](golden/) | 21 casos com a saída C esperada, nos dois perfis |
| [`design/`](design/) | desenho da implementação, e as ideias que não são v0 |

A divisão entre os quatro documentos é normativa e usada como critério: quando
uma decisão trava, a primeira pergunta é em qual deles a regra mora.

Começando a ler: [`keel-spec.md` §3, "keel por exemplos"](keel-spec.md#3-keel-por-exemplos)
mostra a linguagem em pares keel/C, e é o caminho mais curto para ver se ela faz
sentido para você.

## Licenças

São duas, e a separação é deliberada.

- **A base e o `cgen`**: GPLv3 **com uma exceção** ([`LICENSE.md`](LICENSE.md),
  [nota em português](LICENSE.pt.md)). O `cgen` copia trechos de fonte da base
  para dentro do C gerado de todo projeto — não é vínculo de biblioteca, é fonte
  colado. A exceção isenta esse texto gerado, para que o seu programa não seja
  arrastado ao copyleft por transitividade de transpilação.
- **A documentação**: CC BY-SA 4.0 ([`LICENSE-DOCS.md`](LICENSE-DOCS.md),
  [nota](LICENSE-DOCS.pt.md)).

© 2026 Faculdade de Engenharia Elétrica e de Computação, Universidade Federal do
Amazonas (FEEC/UFAM). Escrita e revisão tiveram auxílio de Claude (Anthropic),
sob direção humana.
