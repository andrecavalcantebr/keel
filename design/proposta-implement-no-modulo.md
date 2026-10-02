# Proposta: `implement` no módulo, conformidade nominal

**Data:** 2026-10-02. **Status:** aceita e incorporada à spec e ao rationale
em 2026-10-02. A Base e o golden seguem pendentes (ver o fim do documento).

## Decisões que o texto realiza

1. `implement` sai do `modifier` e do `typedef` e vai para a linha `module`.
2. A conformidade é nominal: só atende um protocolo o tipo cujo módulo o declara
   na cláusula. Não há conformidade por estrutura no uso.
3. Um implementador por módulo: todos os protocolos da cláusula ligam `type C`
   ao mesmo tipo, que é modificador ou `typedef` keel do módulo.
4. Num genérico, a instância que perde um verbo exigido (§4.3, regras 11–13)
   não atende o protocolo.
5. `array` não é implementador do núcleo: indexação e `foreach` são tradução
   por sintaxe C. Os protocolos sobre `array` são de `keel.array`, por inteiro,
   com `length` devolvendo o binder. `keel.length` continua constante do núcleo.

Essas decisões retomam o que o `possibilidades.md` registrou em 2026-09-30
("cláusula fica no module", "nominal em tudo") e que a revisão de 2026-10-01
tinha substituído por conformidade estrutural com asserção opcional.

Duas consequências apareceram ao redigir, e entram no texto:

- **`keel.range` precisa declarar `IndexGet`.** O §4.7, regra 5, admite
  `foreach` de dois binders sobre `range` nomeado, o que pede `IndexGet`. Com a
  conformidade nominal, sem a declaração, essa forma passaria a ser recusada.
- **O receptor de `keel.array` não é modificador nem `typedef`.** É
  `array T v[size_t N]`. A regra do implementador único precisa de uma cláusula
  para esse caso.

## Texto, local por local

### §2.2, tabela "Palavras keel e onde valem"

Substituir a linha de `implement` por:

| `implement` | na linha `module`, depois dos parâmetros, seguido de `[` (§5.1) |

### §2.2, gramática

```ebnf
decl-module  ::= 'module' module-name [ dim-binder ] [ tags-binder ]
                 [ type-binder ] [ implement ] ';'

decl-modifier ::= 'modifier' IDENT [ 'byref' ] '{' <opaque> '}'

decl-typedef  ::= 'typedef' ( specifier | struct-spec | <opaque-no-parens> )
                  ( declarator [ 'byref' ]
                  | declarator { ',' declarator } ) ';'
```

`implement ::= 'implement' '[' qualified-name { ',' qualified-name } ']'` fica
como está. A nota que segue a gramática passa a ser:

> `byref` em `decl-typedef` exige um único declarador. A produção reserva a
> posição de `byref` em `typedef`; esta revisão não generaliza sua semântica,
> e a restrição específica da arena permanece (§5.2).

### §4.1, Sintaxe

Na frase "a forma genérica acrescenta os parâmetros da §4.3", acrescentar:

> …os parâmetros da §4.3, e qualquer módulo pode terminar a linha com a
> cláusula `implement` da §5.1.

### §4.3, Sintaxe

Substituir a frase do `implement`:

> Os parâmetros ficam na linha `module`, e não no `modifier`, porque a unidade
> instanciada é o módulo: todos os seus modificadores e funções compartilham os
> argumentos. A cláusula `implement [lista]`, depois dos parâmetros, declara os
> protocolos que o módulo implementa (§5.1).

E, no parágrafo seguinte, a última frase:

> Um módulo pode declarar mais de um modificador, e o nome do módulo não precisa
> coincidir com o de nenhum deles; só um tipo do módulo é o implementador dos
> seus protocolos (§5.1, regra 12).

### §5.1, Sintaxe: o exemplo do implementador

```keel
module keel.buffer type T implement [IndexPtr, IndexGet, Sliceable, Traversable, Partitionable];
pub modifier buffer byref { /* ... */ }
```

### §5.1, Regras 3 e 4: substituir

> 3. A conformidade é nominal: um tipo atende um protocolo quando o seu módulo o
>    declara na cláusula `implement` da linha `module`, diretamente ou por
>    composição. As construções (tabela abaixo) e as funções sobre protocolo
>    (§4.14) recusam o tipo que não o atende, com `protocol-not-satisfied`.
> 4. A cláusula é verificada no módulo que a declara. Para cada protocolo, o
>    módulo declara cada verbo exigido com o mesmo nome, a mesma aridade e o
>    receptor na mesma posição; o receptor casa pela adaptação da §4.4. Verbos
>    de módulos importados não contam. Cada verbo ausente é
>    `protocol-verb-missing`, e todos são listados na cláusula.

### §5.1, Regra 10: acrescentar ao fim

> A divergência é diagnosticada na cláusula.

### §5.1, Regras novas (12 a 14), depois da 11

> 12. O implementador é o tipo do receptor dos verbos exigidos. Todos os
>     protocolos da cláusula ligam `type C` a esse mesmo tipo, que é um
>     modificador ou um `typedef` keel declarado no módulo. Num módulo genérico
>     que não declara tipo, o receptor pode ser `array T v[size_t N]`, com `T`
>     parâmetro da linha `module`, e o implementador é o `array` do tipo do
>     elemento (§4.4, §5.3). Receptores de tipos diferentes, ou um receptor fora
>     dessas formas, são `protocol-receiver-mismatch`. Um módulo tem, portanto,
>     no máximo um implementador.
> 13. Num módulo genérico, a cláusula é verificada na declaração. Numa instância
>     em que as regras 11 a 13 da §4.3 retiram um verbo exigido, a instância não
>     atende o protocolo. O uso que o exige é `protocol-not-satisfied`, e a
>     mensagem nomeia o verbo, o argumento que o retirou e a cadeia de
>     propagação.
> 14. Os nomes da cláusula são resolvidos depois dos imports do arquivo. Os de
>     `keel.protocols` dispensam import (regra 9).

### §5.1, "Uso pelas construções": o parágrafo sobre `array`

Substituir "`array` continua com participação própria do núcleo (§4.2): …
Um conjunto `tags` direto dispensa `tag` (§4.9)." por:

> Sobre `array`, indexação e `foreach` são tradução do núcleo por sintaxe C
> (§4.2), e não conformidade: não exigem protocolo nem import. Os protocolos
> sobre `array` são os de `keel.array`, que os implementa por inteiro (§5.3);
> `x[a..b]` e as funções sobre protocolo (§4.14) exigem esse import. Um conjunto
> `tags` direto dispensa `tag` (§4.9).

### §5.1, Referências

> Referências: [Rationale: conformidade nominal no módulo](keel-rationale.md#conformidade-nominal-no-módulo).

### §4.14, Regra 4

> 4. O tipo concreto deve atender o protocolo pelas regras 3 e 13 da §5.1.

### §5.3, `keel.array`

Na tabela de chamadas, uma linha nova antes de `array.get`:

| `array.length(v)` | `size_t` | a extensão recebida pelo binder; é o `length` dos protocolos |

O bloco "Conformidade e procedência" começa assim, no lugar das duas primeiras
frases:

> As linhas `module` de `keel.buffer` e `keel.slice` declaram
> `implement [IndexPtr, IndexGet, Sliceable, Traversable, Partitionable]`; a de
> `keel.range`, `implement [Countable, IndexGet, Partitionable]`; a de
> `keel.array`, `implement [IndexPtr, IndexGet, Sliceable]`.

A regra 13 passa a ser:

> 13. `keel.array` não declara tipo nem modificador: a instância é a família de
>     verbos de um tipo de elemento, e o implementador é o `array` desse tipo
>     (§5.1, regra 12). `array.length` é o verbo dos protocolos e devolve o
>     binder. As constantes de `array` (`keel.length`, `keel.capacity`,
>     `keel.dim`) ficam no núcleo (§4.2).

### §5.4, `keel.tagged`

```keel
module keel.tagged tags E type T implement [Taggable];
pub modifier tagged { i32 tag; T value; }
```

### §5.5, `keel.outcome` e `keel.corot`

```keel
module keel.outcome type T implement [Failable, Winnable];
pub modifier outcome { i32 code; T value; }

module keel.corot implement [Taggable];
pub tags Status [SUCCESS = -1, ONGOING = 0, FAILED = 1];
pub typedef struct { i32 code; } corot;
```

O parágrafo "As instâncias com `T` diferente de `void` também atendem
`Winnable`…" passa a ser:

> Em `outcome void`, `win(r, v)` é retirado (§4.3, regra 11), e a instância não
> atende `Winnable` (§5.1, regra 13): o `else` de default sobre ela é recusado.

### §6.2, Catálogo

- `protocol-verb-missing`: "Verbo exigido por protocolo da cláusula `implement`
  ausente no módulo".
- `protocol-not-satisfied`: tira o "(v1)" e passa a valer para construções e
  funções. Condição: "Tipo cujo módulo não declara o protocolo exigido pela
  construção ou pelo parâmetro, ou instância que perdeu um verbo exigido".
  Contrato: §5.1.
- Linha nova: `protocol-receiver-mismatch`. Condição: "Verbos exigidos pela
  cláusula `implement` com receptores de tipos diferentes, ou receptor que não
  é modificador nem `typedef` keel do módulo". Severidade `error`, keel, na
  tradução, §5.1.

## Rationale

### Substituir "Conformidade estrutural com asserção opcional" por

> ### Conformidade nominal no módulo
>
> Um protocolo nomeia um contrato; a cláusula `implement` registra quem o
> cumpre. Juntos, associam um tipo e o código que opera sobre ele numa unidade,
> como um objeto, mas de tempo de tradução: não há vtable, teste em execução nem
> objeto de interface. A instância de um genérico é a materialização dessa
> unidade em código C e em algumas estruturas.
>
> A cláusula fica no módulo, e não no tipo, porque os verbos são do módulo. A
> resolução procura o verbo no módulo do contêiner (§4.4), e não há sobrecarga
> por tipo C: um módulo só pode dar um `length` a um receptor. O implementador
> único é corolário disso, e a regra só o torna explícito. Na prática, um
> módulo que implementa protocolos tem um modificador, e alguns poucos tipos
> associados a ele, como o cursor de `keel.buffer`.
>
> A conformidade é nominal porque as construções a consomem: um `foreach` que
> aceitasse qualquer tipo com os verbos certos tornaria a cláusula um
> comentário. Com ela, o diagnóstico sai no implementador, com todos os verbos
> ausentes listados, e não no uso, com o nome manglado de uma chamada C.
>
> `array` não é tipo, e o núcleo não o trata como implementador: `x[i]` e
> `foreach` sobre `array` são a sintaxe do C. Quem implementa protocolos sobre
> `array` é `keel.array`, por inteiro, com a extensão recebida pelo binder. Assim
> o contrato de cada protocolo tem um só dono, e as constantes de tradução
> (`keel.length`, `keel.dim`) continuam no núcleo, que é quem as conhece.
>
> Composição significa conjunção. A alternativa entre leitura por valor e por
> endereço pertence ao `foreach`, que aceita `IndexGet` ou `IndexPtr` para
> binder por valor. O mesmo princípio separa `Failable` e `Winnable`: tratamento
> sem default não exige um verbo que escreve valor.

### "Por que os parâmetros genéricos ficam na linha module": última frase

> …criaria unidades de instanciação diferentes. Pelo mesmo motivo, `implement`
> também fica na linha `module`: os verbos que a cláusula promete são do módulo.

## Fora desta proposta, mas consequência dela

- **Base (`base/keel/*.k`):** a cláusula na linha `module` de `buffer`, `slice`,
  `range`, `array`, `tagged`, `outcome` e `corot`, e o verbo `length` em
  `array.k`:
  `pub inline size_t length(array T v[size_t N]) { (void)v; return N; }`.
- **Golden:** `019-walk/lst.k` e os `tasks.k` de 022, 023 e 024 ganham
  `implement [Traversable]`. Como já registrado no `possibilidades.md`, essa
  mudança vai junto com o código do cgen que lê a cláusula, para o `run.sh` não
  ficar vermelho; o `expected/` não muda.
- **`requisitos-v0.md`, item 5:** a tabela de protocolos passa a ser indexada
  por módulo implementador.
- **`proposta-spec-papeis-protocolos-keel-code.md`:** a nota de status pode
  remeter a esta proposta.
