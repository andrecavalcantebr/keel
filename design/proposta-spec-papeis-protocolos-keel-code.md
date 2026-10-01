# Proposta de revisão da spec: papéis, protocolos e especialização de funções

**Data:** 2026-10-01. **Status:** proposta para revisão; não normativa.
Este documento não altera a spec, o backend, a Base nem o compilador.
As regras candidatas abaixo precisam de aceite antes da incorporação.

## 1. Objetivo e base da revisão

A revisão reúne três mecanismos:

1. Papéis de procedência e invalidação, retirando os privilégios da arena sem
   perder as verificações existentes.
2. Protocolos nominais, consumidos pelas construções atuais e por funções
   especializadas pelo tipo concreto do argumento.
3. Substituição de `type T`, `dim N` e `keel_code C` em funções, com duas
   formas de materialização: função C especializada ou expansão no chamador.

A marca `keel_code` na função determina a expansão no chamador. A mesma marca
em um parâmetro determina que esse argumento é código de tradução. Uma função
com parâmetro de código não se torna, por isso, um molde expandido no chamador.

Fontes consultadas no branch `ccr-5d3353d1-2c59hv`:

| Documento | Blob consultado |
| --- | --- |
| [Spec](../keel-spec.md) | `d29491ae515d36ffb7309e0e35283230fe6fcf15` |
| [Possibilidades](possibilidades.md) | `979b9e4e9fb10ca351f0fdcfc076b87d0a635c6f` |
| [Backend](../keel-c-backend.md) | `fb7207dac68aeaf4ae443be0ccf20864d788ab6c` |
| [Impacto de moldes](mold-impacto.md) | leitura da versão do branch |
| [Impacto de protocolos](protocolo-impacto.md) | leitura da versão do branch |

Referências de seção neste documento são da spec atual, salvo indicação
explícita de backend ou ferramenta. As novas subseções ainda não têm numeração
normativa. Os exemplos de sintaxe nova são candidatos, não código aceito hoje.
O C mostrado é lowering lógico; nomes finais, guards e perfis pertencem ao
backend.

### 1.1 Fronteira preservada

Keel continua sem inferir tipos de expressões C, abrir headers para resolver
tipos ou provar o comportamento de corpos C. Pode consultar declarações
conhecidas, substituir parâmetros, registrar relações entre símbolos e
reanalisar o código produzido por suas próprias transformações.

Protocolos e papéis são contratos declarados. Sua implementação não cria um
sistema geral de ownership, borrow checking, despacho virtual ou coleta de
lixo. A política de segurança continua escolhida pelo programador.

Não entram nesta revisão: realocação, sobrecarga geral de operadores, lambdas,
tuplas, `source`, `pub import` ou a linguagem de tensores. Também não se
propõe transformar todas as construções do núcleo em moldes.

## 2. Papéis e generalização da arena

### 2.1 Texto candidato: papéis de assinatura

Uma assinatura keel pode declarar:

| Papel | Posição | Significado |
| --- | --- | --- |
| `parent` | parâmetro | origem de armazenamento e dependência de validade |
| `child` | parâmetro ou retorno | produto cuja procedência depende dos pais da chamada |
| `invalidates` | parâmetro | operação que invalida os recursos identificados por esse argumento |

Os papéis são contextuais e desaparecem do C emitido. Não alteram, por si,
layout, passagem por valor/referência ou o corpo da função.

```keel
pub bool from_array(child arena *a, parent array u8 storage);
pub bool from_parent(child arena *a, parent arena *owner, size_t bytes);
pub bool from_memory(child arena *a, u8 *memory, size_t bytes);
pub child T *alloc(parent arena *a, type T, size_t count);
pub void reset(invalidates arena *a);
pub void restore(invalidates arena *a, size_t mark);
pub size_t mark(arena *a);
```

`mark` apenas observa o topo: não recebe `invalidates`.
`from_memory` não transforma um ponteiro cru em prova de procedência externa.

**Regras candidatas:**

1. Uma chamada reconhecida estabelece, para cada produto `child`, dependência
   de todos os argumentos `parent` conhecidos.
2. Um `array` automático conhecido introduz origem local com seu escopo de
   duração. Um `array` estático não introduz origem automática local.
3. As relações transmitem origens locais e dependências transitivamente.
   Ausência de origem local conhecida não constitui prova de segurança.
4. Um retorno `child` pode transmitir essas informações ao símbolo que
   recebe diretamente o resultado, em declaração ou atribuição reconhecida.
   Não se deduz procedência de uma expressão C arbitrária que envolva a chamada.
5. Retornar um símbolo cuja procedência conhecida inclui armazenamento
   automático local à função produz `region-escape`.
6. Invalidar uma origem invalida seus produtos conhecidos e seus descendentes.
   O uso posterior reconhecido no mesmo escopo produz diagnóstico.
7. Uma construção reconhecida substitui as dependências anteriores do produto.
   Atribuição não acompanhada perde a informação e deixa o símbolo sem
   garantia; não o comprova válido.
8. A análise registra a relação declarada sem provar o resultado booleano do
   construtor. Não faz análise de sucesso/falha de expressões C.
9. Papéis em assinaturas conhecidas são usados no ponto de chamada, inclusive
   quando a função está em outro módulo. Não se deduzem efeitos adicionais de
   seu corpo nem se seguem aliases, campos e cópias arbitrárias.
10. Um molde é analisado depois da substituição no contexto efetivo do uso.
    Seu armazenamento automático participa dessas mesmas regras.

A implementação mantém um grafo de relações entre símbolos reconhecidos e
informação de escopo no tradutor. Não materializa esse grafo em runtime.

**Precisão a fechar: alvo de invalidação.** Resetar um alocador invalida seus
produtos, mas deixa o próprio alocador reutilizável. Liberar uma entidade de
pool invalida a entidade e seus produtos. A palavra `invalidates`, isolada,
não descreve toda a pós-condição do alvo. A regra mínima proposta distingue
uma origem reutilizável de um produto `child` consumido: o primeiro conserva
o descritor utilizável; o segundo exige reconstrução antes de novo uso.
Essa distinção precisa ser fechada na análise de símbolos, incluindo objetos
que são simultaneamente pai e filho. Não se deve diagnosticar todo uso de uma
arena após seu próprio `reset`, nem permitir que resetar uma filha revalide
uma dependência já invalidada no pai.

### 2.2 O que sai do privilégio de arena

| Regra atual | Mecanismo geral proposto | Preservação necessária |
| --- | --- | --- |
| Escape de armazenamento local | `parent`/`child` e origem do `array` | erro ao retornar produto de respaldo local conhecido |
| Filha usada após reset/restore do pai | `invalidates` e grafo de dependências | inclui uso da filha em seu próprio `reset` |
| Argumento de `from_array` | contrato geral de parâmetro `array` | extensão conhecida e elemento admissível |
| Tamanho constante de `from_stack` | `dim N` da função-molde | sem VLA |
| Armazenamento de `from_stack` no chamador | expansão `keel_code` | mesma duração do bloco do chamador |
| Inicialização implícita `{0}` | inicializador padrão declarado pelo tipo | arquivo, bloco, estático e vetores |
| Proibição de parâmetro arena por valor | propriedade `byref` de tipo comum | não basta pôr ponteiros nos verbos |
| Alocação tipada | substituição de tipo ou adaptação já existente | tamanho, alinhamento e retorno tipado |
| Overflow, capacidade e alinhamento | corpos da Base e contrato do backend | falha limpa e aritmética sem overflow |

O núcleo só deixa de reconhecer arena pelo nome quando todas essas linhas
tiverem substituto. Papéis e protocolos, sozinhos, não substituem a
inicialização padrão nem a restrição de passagem.

**Inicialização padrão — contrato candidato.** O tipo pode declarar um
inicializador de definição. Ele é inserido apenas quando o objeto não possui
inicializador escrito; `extern` e campos não recebem inserção. Para arena,
o padrão é `{0}`, também para vetores. Não há chamada implícita de construtor
nem de cleanup. A grafia dessa propriedade, e de `byref` para tipos comuns,
permanece ponto de aceite: esta proposta não inventa uma sintaxe como se já
estivesse acordada.

**Alocação — contrato preservado.** A Base recebe contagem, tamanho e
alinhamento separadamente; verifica multiplicação e capacidade; alinha o
endereço entregue; retorna `NULL` na falta de espaço ou overflow. Em debug,
o overflow conserva o diagnóstico correspondente. `reset` e `restore`
não limpam o armazenamento nem o devolvem ao sistema. Não há `realloc`.

A suposição de respaldo de tipo-caractere do backend §5.4.1 permanece
documentada: mover arena para a Base não resolve por si as regras de tipo
efetivo do C. Não se acrescenta promessa de portabilidade mais forte.

### 2.3 Geração em runtime: implementação explícita e opcional

Neste documento, **geração** significa contador de validade (`epoch` ou
`generation`); não se confunde com geração de código pelo tradutor.

**Texto candidato:**

> Papéis não acrescentam metadados em execução. Um módulo pode implementar
> verificação de validade por gerações em sua representação e em seus verbos.
> Os campos, atualizações, verificações, comportamento de falha e custos são
> parte do contrato desse módulo. Keel preserva e traduz esse código pelas
> regras comuns; não injeta contadores ao reconhecer `parent` ou
> `invalidates`.

Uma região verificada pode guardar `epoch`; uma visão, `link` para o dono
e `stamp` com a geração observada na construção. Uma arena que atua nos dois
papéis pode guardar os três campos.

```c
/* Representation sketch, not a new mandatory arena layout. */
struct checked_region {
    size_t top, cap;
    unsigned char *memory;
    struct checked_region *link;
    size_t epoch, stamp;
};

static bool region_chain_valid(const struct checked_region *r) {
    while (r->link != NULL) {
        const struct checked_region *owner = r->link;
        if (r->stamp != owner->epoch) return false;
        r = owner;
    }
    return true;
}
```

O exemplo pressupõe descritores vivos, endereços estáveis e cadeia acíclica.
A biblioteca valida antes de usar a região, registra a geração na construção
e a incrementa nas invalidações especificadas.

**Hierarquia:** comparar apenas `child.stamp == parent.epoch` não detecta,
sozinho, que o avô invalidou o pai. Para preservar a regra transitiva, a
variante verificada deve validar a cadeia, como no exemplo, ou oferecer outra
representação com garantia equivalente. Não se promete custo constante para
uma cadeia de profundidade arbitrária sem desenhar esse outro mecanismo.

| Política | Espaço adicional | Trabalho em runtime |
| --- | --- | --- |
| Sem geração | nenhum | nenhum custo de geração; papéis continuam estáticos |
| Dono com época | um contador por dono | atualização em cada invalidação |
| Visão com vínculo | um ponteiro e um contador por visão, mais padding | leitura da geração e comparação no acesso verificado |
| Arena pai e filha | um ponteiro e dois contadores por descritor, mais padding | registro ao construir; validação da cadeia, O(profundidade) no desenho acima |
| Handle por índice e geração | índice e geração no handle; metadados por slot no pool | teste de índice, ocupação e geração, conforme o contrato |
| Pool com reset por época global | época adicional no pool e informação correspondente na referência | verificação da época além da geração do slot |

Os tamanhos são simbólicos: dependem do alvo, dos tipos dos contadores e do
alinhamento. Não se fixa que um ponteiro tenha oito bytes.

`KEEL_CHECK` pode controlar a verificação de debug. A condição não deve ter
efeitos; incrementos e manutenção de estado não ficam dentro do check.
**Desligar checks não remove automaticamente campos e atualizações.** Para
não pagar esse custo, o programa escolhe uma representação sem geração.
Como proposta inicial, variantes nominais distintas são preferíveis a layout
dependente de `KEEL_CHECKS`: preservam o contrato atual do backend §5.17,
que usa o mesmo C gerado com checks ligados ou desligados. Uma política futura
de layout por configuração precisaria garantir consistência de ABI de todo
o programa.

A checagem por geração cobre cópias de descritores que ainda passam pelos
verbos verificados. Não cobre acesso por ponteiro cru depois de sair do verbo,
nem autoriza dereferenciar o vínculo de um dono que já morreu. Não há
realocação neste modelo; duração do dono, cópia/movimentação de seu descritor,
reutilização de endereço e concorrência continuam sendo questões concretas
que a biblioteca deve especificar.

**Volta do contador:** largura maior apenas adia o problema de ABA. A
implementação deve definir limite, recusa de reuso ou outra política ao
esgotar a geração; não pode prometer detecção eterna com contador finito.

### 2.4 Restore: manter a diferença entre validade e armazenamento

Uma época global incrementada em todo `restore` invalida todas as referências
verificadas, inclusive as anteriores à marca. Isso só é correto se for o
contrato explícito da variante, não uma implementação silenciosa de
preservação das referências anteriores.

Proposta mínima:

- na análise lexical, manter a invalidação conservadora das filhas conhecidas,
  conforme a regra atual;
- numa variante verificada simples, documentar que qualquer `restore`
  invalida todas as referências verificadas anteriores;
- se a biblioteca quiser preservar as anteriores à marca, exigir metadados
  suficientes para distinguir alocações e reutilizações. Comparar apenas o
  offset com o topo não resolve ABA após nova alocação.

A política precisa ser escolhida antes de publicar uma arena verificada.
O estado dos bytes preservados não implica validade de uma referência.

### 2.5 Pool, handle e defer

Os contratos de alocação em grupo e individual são independentes:

| Protocolo proposto | Operações | Invalidação |
| --- | --- | --- |
| `GroupAllocable` | `alloc`, `reset` | produtos da região em lote |
| `SingleAllocable` | `alloc`, `free` | recurso individual |

A grafia acima acompanha a discussão; substitui os nomes provisórios
`GroupAllocatable` e `SingleAllocatable` quando houver aceite.
Um pool pode declarar ambos. `mark` e `restore` são verbos da biblioteca,
sem obrigatoriedade de pertencer a um protocolo. Um contrato hierárquico só
precisa virar protocolo se algum consumidor exigir seus verbos; os papéis
já descrevem a dependência sem esse protocolo.

Exemplo de assinaturas para um pool, omitindo a representação:

```keel
pub child entity alloc(parent pool *p);
pub void free(parent pool *p, invalidates entity e);
pub void reset(invalidates pool *p);
pub child T *ref ptr(parent pool *p, parent entity e);
```

O ponteiro derivado depende tanto do pool quanto da entidade. A liberação de
`e` não invalida os demais elementos do pool. Cópias de `e` escapam à
análise lexical; a geração do slot permite verificá-las em runtime.
Uma entidade composta apenas por índice e geração exige contrato de
pertencimento ao pool, ou identidade adicional do dono: dois pools podem
ter o mesmo índice e a mesma geração.

`defer pool.free(p, e);` permite liberação individual na saída do escopo,
pelas regras existentes de `defer`. Não há registro implícito pelo papel,
destrutor automático ou prova de liberação única. O programa organiza a
posse e evita dupla liberação.

## 3. Generalização dos protocolos

### 3.1 Texto candidato: declaração, composição e conformidade

> Um protocolo é um símbolo nominal que declara um conjunto de operações.
> Pode ser consumido por uma construção do núcleo ou usado como tipo de
> parâmetro de função. A conformidade é declarada pelo implementador e
> verificada nos limites do contrato de análise de keel.

Sintaxe de composição preservada das possibilidades:

```keel
protocol IndexTraverse [Indexable, Traversable];
```

A cláusula `protocol` na linha `module` declara conformidade e não é
binder nem import. Os nomes nela usados são resolvidos depois de coletar os
imports do módulo, já que `module` é a primeira construção do arquivo.
A cláusula vale para cada modificador do módulo; conjuntos diferentes
continuam exigindo módulos diferentes na proposta inicial.

**Binders — ajuste recomendado, sujeito a aceite.** Declarar binders por
protocolo, como `protocol Indexable type T { ... }`, em vez de obrigar
todos os protocolos de `keel.protocols` a compartilhar os binders de um
módulo genérico. Isso permite protocolos sem tipo de elemento, de um tipo
ou de outras aridades no mesmo módulo. Protocolos compostos inicialmente
exigem listas compatíveis por espécie e ordem; aceitar implementadores com
binders adicionais exige um mapeamento explícito ainda não proposto.
A restrição precisa ser exercitada com `range`, `array` e futuros
containers multidimensionais antes de fechar a gramática.

**Conformidade:**

1. O implementador declara os verbos no próprio módulo; imports não suprem
   verbos por transitividade.
2. Keel verifica nome, aridade sintática, categorias dos parâmetros de tradução,
   posição do receptor e papéis.
3. O implementador deve disponibilizar os verbos exigidos aos consumidores.
4. A composição calcula a união dos requisitos. Losango com o mesmo requisito
   é aceito; ciclos e requisitos conflitantes são erros.
5. A compatibilidade semântica de tipos C e retornos não é provada por keel.
   O compilador C valida o código concreto emitido. Conversões implícitas
   podem aceitar diferenças: não se promete equivalência completa de assinatura.
6. Protocolos não geram vtables, objetos de interface ou um tipo C de runtime.

### 3.2 Uso imediato pelas construções

A regra atual da §5.1, segundo a qual basta declarar os verbos sem registro,
é substituída por conformidade nominal.

| Construção | Protocolo | Operações |
| --- | --- | --- |
| `x[i]`, `foreach` de dois binders | `Indexable` | `length`; `ptr` para endereço; `get` ou acesso previsto para valor |
| `x[a..b]` | `Sliceable` | `length`, `as_slice` |
| `walk` | `Traversable` | `begin`, `has_next`, `next` |
| `parallel` | `Partitionable` | `partition` |
| `foreach` de um binder sobre faixa | `Countable` | `first`, `limit` |
| `match` | `Taggable` | `tag` |
| `else` | `Failable` | `failed`; `win` na forma que o exige |

A nominalização não muda o significado das construções nem transforma
`parallel` em promessa de execução simultânea. O backend conserva seus
lowerings e custos.

**Alternativas dependentes da construção.** A spec atual permite `get`
no lugar de `ptr` para binder por valor, e exige `win` apenas em uma
forma de `else`. Um conjunto incondicional de protótipos não reproduz isso.
Antes da incorporação, escolher entre protocolos mínimos compostos
(capacidades distintas) ou requisitos alternativos/condicionados ao uso.
Não se deve obrigar um container sem elemento endereçável a inventar `ptr`.
Esta é uma pendência normativa, não uma prova de que basta copiar a tabela.

`array` mantém sua participação intrínseca e tradução própria; inicialmente
não é argumento de parâmetro de protocolo. `tags` direto em `match`
também conserva sua regra. O `as_slice` de `keel.array`, o produto
`range` do literal e `parallel.control` continuam vínculos enumerados
com a Base. Retirar o privilégio da arena não elimina esses outros vínculos.

A Base e os módulos do golden que hoje participam estruturalmente precisam
de imports e cláusulas de protocolo. Os estudos identificam os casos
`019-walk`, `022-linux-list-import-c`, `023-linux-list-extern-c` e
`024-linux-list-module`; a lista deve ser conferida na migração.
Não se muda o fonte antes de o parser aceitar a cláusula.

### 3.3 Funções sobre protocolo

```keel
size_t count(Indexable items) {
    return length(items);
}
```

Cada tipo concreto conhecido de `items` determina uma instância da função.
O parâmetro protocolo desaparece: a assinatura C recebe o tipo concreto,
por valor ou referência conforme `byref`. Não há teste de conformidade
em runtime.

- O argumento inicial deve ser símbolo de tipo conhecido. Não se infere o
  tipo de `count(make_items())`.
- Cada ocorrência é independente: `f(Indexable a, Indexable b)` não exige
  que os dois tipos sejam iguais.
- O corpo só pode usar as operações declaradas pelo protocolo, mesmo se o
  tipo concreto possuir outras.
- O corpo é validado contra o contrato e depois especializado com o
  implementador concreto. As dependências geradas entram no fecho normal.
- Recursão para a mesma instância é chamada de função normal; produção
  ilimitada de novas instâncias exige limites e diagnóstico.

### 3.4 Consulta de tipo declarado

Proposta auxiliar para cursores, partições e produtos de alocação:

```keel
void inspect(Traversable items) {
    walk (f64 *value, keel_declared(begin(items)) cursor : items) {
        /* use value */
    }
}
```

`keel_declared` consulta o tipo declarado de símbolo ou retorno de verbo
keel resolvido; não avalia a chamada. Não aceita uma expressão C arbitrária
nem equivale a `typeof`. A assinatura pode ser resolvida após a ligação
dos parâmetros da instância, e o backend escreve o tipo concreto em C11 e
C23. Ciclos de consulta e informação indisponível são erros.

O exemplo fixa o elemento como `f64`; a consulta do cursor não prova que
qualquer implementador produz `f64`. Essa compatibilidade continua sujeita
à validação do código concreto.

Omissão dos argumentos de `keel_declared` nos binders fica fora desta
proposta inicial. Produtos que serão reutilizados em assinaturas de protocolo
precisam de escopo de referência definido; não se inventa um nome C universal
para todos os cursores ou todas as entidades.

## 4. Funções especializadas por tipo, dimensão e código

### 4.1 Dois eixos independentes

| Declaração | O que é substituído | Materialização |
| --- | --- | --- |
| Função comum com parâmetros de tradução | tipos, dimensões, fragmentos e tipos concretos dos protocolos | função C especializada |
| Função marcada `keel_code` | os mesmos parâmetros | corpo expandido no ponto de uso |
| Parâmetro `keel_code C` | fragmento nas ocorrências de `C` | não determina sozinho a materialização da função |

`inline` do C continua separado: uma função especializada pode ser
`static inline`, mas o compilador C decide sua otimização. A expansão
`keel_code` é uma transformação de keel e muda efetivamente o escopo
em que as declarações do molde existem.

### 4.2 Parâmetros e a mudança necessária no type atual

| Parâmetro | Entrada | Efeito |
| --- | --- | --- |
| `type T` | tipo escrito e reconhecido | substituição simbólica de tipo |
| `dim N` | constante conhecida pelas regras atuais de `dim` | substituição pelo valor canônico |
| `keel_code C` | fragmento de tokens keel | substituição nos pontos de uso de `C` |
| Protocolo | símbolo de tipo concreto conhecido | especialização e resolução de seus verbos |
| Valor comum | expressão | argumento de runtime, avaliado uma vez por chamada |
| Papel em parâmetro | símbolo reconhecido nas formas admitidas | preservação das relações de procedência |

Tipos, dimensões e fragmentos não são argumentos de runtime. Um fragmento
pode executar zero, uma ou várias vezes conforme o lugar em que foi inserido.
Um `C;` dentro de laço executa a cada iteração; dois `C;` duplicam o
fragmento. Isso não viola a avaliação única dos argumentos de valor.

**Mudança incompatível a decidir.** A §4.4 atual dá a `type T` de função
duas espécies: seleção de instância de módulo e apagamento para tamanho e
alinhamento. Não há hoje substituição livre de um tipo próprio da função.

Para completar o modelo solicitado, recomenda-se:

1. Preservar `type T` que seleciona o parâmetro homônimo do módulo.
2. Fazer `type T` próprio da função declarar especialização, tornando
   `T` utilizável em declarações e expressões admitidas após substituição.
3. Migrar o apagamento implícito para wrappers tipados sobre funções cruas
   compartilhadas, ou reservar uma forma explícita de apagamento em proposta
   posterior. Não escolher apagamento ou especialização por heurística do corpo.

Isso substitui as regras 13–18 da §4.4 e muda a ABI gerada das funções
atualmente apagadas. Deve ser aprovado como mudança, não descrito como mera
extensão compatível.

Exemplo: o trabalho pesado da alocação continua compartilhado:

```keel
pub inline child T *alloc(parent arena *a, type T, size_t count) {
    return (T *)alloc(a, count, sizeof(T), alignof(T));
}
```

O wrapper se especializa por `T`; a forma crua de quatro argumentos
permanece única. Não há obrigação de copiar o algoritmo de alocação para
cada tipo. O protocolo de grupo pode exigir a forma crua; a forma tipada é
conveniência da biblioteca.

### 4.3 Uma função comum especializada por código

```keel
module sample;

pub inline void consume(type T, T next_value, keel_code C) {
    record(next_value);
    C;
}
```

Uso, supondo `next()` com retorno compatível e `actions.mold` função
pública sem captura:

```keel
sample.consume(i32, next(), actions.mold());
```

C lógico:

```c
static inline void sample_consume_i32_code_K(i32 next_value) {
    sample_record(next_value);
    actions_mold();
}

/* At the original call site: */
sample_consume_i32_code_K(next());
```

`K` é um identificador ilustrativo de especialização, não o algoritmo de
mangling proposto. `actions.mold()` executa onde `C` foi usado, depois de
`record`. `next()` continua argumento de uma chamada C normal.
Não se copia `consume` para a função chamadora. Com `T` já determinado
pelo módulo, o exemplo pode conservar a forma de uso
`consume(next(), mold())`.

A instância se especializa pelo código tanto quanto pelo tipo. Substituir
`actions.mold()` por outro fragmento produz outra instância. Repetir a
mesma combinação reutiliza a identidade existente, respeitando a ligação
C escolhida.

### 4.3.1 Tipo, dimensão e código na mesma assinatura

```keel
pub inline T repeat(type T, dim N, T value, keel_code C) {
    for (size_t i = 0; i < N; ++i) {
        C;
    }
    return value;
}
```

```keel
i32 result = sample.repeat(i32, 4, next(), actions.mold());
```

A chave inclui `i32`, o valor canônico `4` e a identidade do fragmento.
A função C recebe apenas `i32 value`; seu corpo tem o limite `4` e a
chamada `actions_mold()` dentro do laço. `next()` é avaliado uma vez,
e o fragmento executa quatro vezes. O laço permanece um laço C; substituir
`dim N` não obriga keel a desenrolá-lo. O retorno continua sendo retorno
da função especializada.

Marcar essa mesma declaração com `keel_code` mudaria a materialização:
o corpo seria expandido na posição admitida do chamador, com o parâmetro de
valor ligado a temporário e o retorno tratado pelas regras da seção 5.

### 4.4 Escopo, captura e categoria do fragmento

**Contrato inicial recomendado:**

1. O primeiro formato de argumento de código é uma expressão de tokens
   balanceados. Vírgulas internas requerem delimitadores, como nas chamadas.
   A forma com bloco de statements é extensão separada.
2. Os nomes próprios do corpo são resolvidos no módulo da função.
   Nomes livres conhecidos do fragmento são resolvidos no local onde o
   argumento foi escrito, preservando sua identidade.
3. A especialização de uma função comum não captura automaticamente variáveis
   automáticas do chamador. Um fragmento que depende delas precisa receber
   os dados por parâmetros explícitos, ou é recusado.
4. Não se permite acesso acidental aos locais do corpo por coincidência de
   grafia. Um futuro mecanismo de slots/captura requer sintaxe e contrato.
5. Símbolos privados, funções `static` de outra unidade e nomes C opacos
   dependentes do contexto precisam de política de emissão. A versão inicial
   pode recusar especialização compartilhada que dependa deles; não se promete
   que mover tokens para um header preserva ligação automaticamente.
6. Expansão de função-molde no chamador permite referências ao escopo efetivo
   do uso, mas mantém a origem dos tokens e evita captura acidental pelo corpo.
7. Fragmentos de expressão não introduzem `return`, `goto`, `break`
   ou `continue` externos. A forma de bloco futura precisará definir o
   destino de cada saída e sua interação com `defer`.

A proposta atual de código não constitui um sistema de closures.
Ela já atende ao caso de funções e expressões sem captura implícita.

### 4.5 Identidade, mangling e artefatos

A chave de especialização contém:

- identidade da declaração e da instância de módulo, se houver;
- argumentos de tipo e dimensão canônicos;
- tipos concretos dos parâmetros de protocolo, na ordem escrita;
- tokens dos argumentos de código e contexto necessário para resolver seus
  símbolos com a mesma identidade.

Texto igual em dois módulos não basta: `mold()` pode nomear funções
diferentes. Texto diferente por alias pode nomear a mesma entidade, mas não
se exige deduplicação por equivalência semântica de C.

**Ponto de backend obrigatório.** Não cabe concatenar um fragmento arbitrário
no identificador C. É necessário aprovar uma codificação determinística
(limitada, ou digest com tratamento de colisão), mantendo a descrição legível
em diagnósticos/mapas. Usar digest exige revisar a decisão atual contra
truncamento com hash; não se introduz isso silenciosamente. Nome baseado na
ordem de descoberta não serve à compilação separada.

O backend atual tem **três artefatos**, não apenas um par: `.type.h`,
`.h` e `.c` para a unidade compilada; instâncias apenas alcançadas
normalmente geram headers. A proposta preserva essa divisão:

- tipos necessários chegam pela camada de `.type.h`;
- protótipos e corpos inline da instância vão ao header apropriado;
- corpo fora de linha tem dono explícito, conforme a política de `instance`;
- o header da instância não muda conforme a existência de um emissor externo;
- dependências do corpo e do fragmento entram no depfile;
- a mesma identidade produz o mesmo conteúdo, independentemente do consumidor.

A grafia de `instance` para funções com argumentos de código e a opção
equivalente da CLI precisam ser definidas. Não se pressupõe um
`instances.k` automático: pode-se manter posse explícita e adiar a
materialização fora de linha dessas funções até fechar sua sintaxe.

`pub inline` versus `pub` deve seguir a declaração, como no backend
atual. O estudo anterior de protocolos fala em inline por padrão; esta
proposta recomenda preservar a regra explícita do backend e corrigir essa
divergência antes de incorporar.

Compartilhar a identidade evita gerações redundantes, mas `static inline`
ainda pode produzir cópias por unidade de tradução. Não há promessa de uma
única cópia física no binário.

## 5. Funções-molde: expansão no chamador

### 5.1 Texto candidato

> Uma função marcada `keel_code` é um molde. Após a resolução da chamada,
> keel liga seus parâmetros, substitui tokens, renomeia declarações locais,
> trata o retorno e analisa o resultado no ponto de expansão. O molde não
> possui função nem endereço C próprios.

Parâmetros de valor são ligados uma vez a temporários do tipo declarado,
em ordem textual dos parâmetros na expansão. Esta é uma regra específica
do molde; funções comuns conservam a ordem de avaliação da chamada C.

Parâmetros de papel preservam o símbolo efetivo, sem copiar o descritor:
a adaptação entre objeto e ponteiro segue a assinatura e as regras da §4.4.
Não é substituição textual cega de `a` por `*a` ou `&a`.

Locais reconhecidos são renomeados de modo consistente por vínculo e escopo,
não por substituição global da mesma grafia. Literais, campos após `.` e
`->`, comentários e identificadores não vinculados ao parâmetro não são
alterados. Declaradores opacos que impeçam higiene são recusados na primeira
versão, em vez de receber uma falsa promessa de higiene sobre todo C.

### 5.2 Return e posições de expansão

**Forma mínima proposta:**

- molde de valor: um único `return expressão;`, final e no nível superior;
- molde `void`: término natural ou `return;` final;
- retornos antecipados e saídas que cruzem a fronteira do molde são recusados;
- a expressão de retorno é avaliada uma vez, com o tipo declarado, e seu
  resultado substitui o valor da chamada;
- não há `return` residual que retorne da função chamadora.

Para moldes com declarações ou statements, admitir inicialmente apenas:

1. chamada como statement completo, descartando o resultado se houver;
2. chamada como inicializador completo de declaração simples;
3. chamada como lado direito completo de atribuição a símbolo simples.

Essas posições resolvem o exemplo de `from_stack` sem analisar o
sequenciamento de expressões C arbitrárias. Não içar prelúdios através de
`&&`, `||`, `?:`, condições/incrementos de laços ou argumentos
de outra chamada. Sub-statements sem chaves são recusados quando requerem
inserção de declarações.

Molde apenas de expressão, sem prelúdio nem ligação executável de argumentos,
pode ser substituído como expressão parentetizada, mantendo a avaliação no
lugar original. Ampliar as posições dos demais moldes exige regra própria.

Esta delimitação substitui a divergência dos estudos anteriores entre permitir
prelúdio em condições e recusar corpo executável em expressão. O uso
`bool ok = arena.from_stack(a, 1024);` fica expressamente admitido;
`if (!arena.from_stack(...))` deve ser reescrito com variável anterior
na primeira versão proposta.

Não criar bloco sintético em torno de todo o molde: isso encurtaria a duração
do armazenamento de `from_stack`. Blocos explicitamente escritos dentro do
corpo mantêm seus limites.

### 5.3 From_stack como biblioteca

```keel
pub keel_code bool from_stack(child arena *a, dim N) {
    alignas(alignof(max_align_t)) array unsigned char storage[N];
    return from_array(a, storage);
}
```

Há uma decisão prévia necessária: `from_array` hoje exige `array u8`,
enquanto o backend de `from_stack` usa `unsigned char`.
Recomenda-se um contrato geral de parâmetro de array de bytes que aceite
explicitamente as formas aprovadas, preservando a passagem da extensão.
A alternativa é um helper de Base com assinatura apropriada e papéis.
Não se deve retirar a validação nem depender de um desvio especial por
nome de arena.

Sob essa condição, o uso:

```keel
module app;
void run(arena *a) {
    bool ok = arena.from_stack(a, 1024);
    /* remaining code */
}
```

tem o seguinte C lógico no perfil C11:

```c
void app_run(keel_arena *a) {
    _Alignas(_Alignof(max_align_t)) unsigned char keel__storage0[1024];
    bool keel__result0 =
        keel_arena_from_array(a, keel__storage0, sizeof keel__storage0);
    bool ok = keel__result0;
    /* remaining code */
}
```

O armazenamento dura até a saída de `run`; a análise registra sua origem
local e a transmite para o descritor reconhecido. Isso não transforma uma
escrita por parâmetro de saída em prova interprocedural: o chamador de
`run` continua fora dessa garantia, como no contrato atual.

O exemplo não zera os 1024 bytes, preservando o custo atual. Escrever
`= {0}` no molde solicita inicialização do armazenamento e pode acrescentar
trabalho proporcional a `N`. A inicialização do descritor arena é outra
operação, de tamanho fixo.

`inline` do C não substitui esse mecanismo: otimizar uma chamada não muda
o tempo de vida do objeto automático definido na função de origem.

### 5.4 Defer, recursão e limites

A expansão acontece antes da passagem final de `defer`.
Um `defer` inserido participa do escopo onde foi efetivamente expandido;
não ganha um escopo de chamada fictício. Na função comum especializada,
continua pertencendo à função especializada.

O expansor exige detecção de ciclos de expansão, limite de profundidade e
limite de tokens produzidos. Argumentos aninhados finitos, como `f(f(x))`,
não são por si recursão infinita; o controle considera a origem e a cadeia
de expansões. Duplicação exponencial pode ocorrer sem ciclo e precisa de
limite independente.

Erros devem mostrar uso, declaração do molde e cadeia de expansão. O backend
define `#line` sem perder a localização original dos fragmentos.

## 6. Impactos na spec atual

| Seção | Alteração proposta | Compatibilidade e limite |
| --- | --- | --- |
| §1.1 | explicitar especialização e expansão como transformações previsíveis | mantém C como verificador final |
| §§1.2–1.3 | permitir substituir e reanalisar tokens keel; consultar tipos declarados e contratos | continua sem expandir macros C ou inferir expressões |
| §1.4 | documentar novos contextos de reconhecimento | rever conflitos sem reservar palavras globalmente sem necessidade |
| §2.1 | preservar origem dos tokens e regras de fragmentos | strings e diretivas não sofrem substituição ingênua |
| §2.2 | gramática de papéis, protocolos, `dim` e `keel_code` em funções; propriedades de tipo a fechar | distinguir marca da função e categoria do parâmetro |
| §2.3 | novas ilhas por chamada a molde e função especializada | reconhecimento por símbolo conhecido |
| §§2.4–2.5 | condicionais, nomes e diagnósticos das novas formas | não avaliar condições do PPC |
| §3 | exemplos integrados e limites visíveis | atualizar exemplos de `from_stack` e de protocolo |
| §4.1 | exportar contratos e corpos necessários à especialização | mantém módulo/caminho e imports não transitivos |
| §4.2 | origem de `array`, tipo declarado, inicializador padrão e `byref` de tipo comum | não impor geração a buffers ou views |
| §4.3 | distinguir binders de módulo, protocolo e função; reutilização e limites | cláusula `protocol` não altera aridade do modificador |
| §4.4 | ligação de parâmetros e especialização; papéis na chamada; revisão do type apagado | principal mudança incompatível de emissão |
| §4.5 | conformidade nominal do acesso e preservação dos checks | resolver alternativas `ptr/get` |
| §4.6 | ordem da expansão antes de `defer` | nenhum cleanup implícito |
| §4.7 | `Countable`, `Indexable`, `Traversable`; cursor declarado | não inferir tipo C arbitrário |
| §4.8 | `Partitionable` e tipo declarado da partição | manter captura, controle, worker e custos atuais |
| §§4.9–4.10 | `Taggable`, `Failable` | preservar exaustividade e diferença entre formas de `else` |
| §4.11 | registrar que extent não passa a exigir geração | layout e verificações atuais permanecem |
| §5.1 | substituir adesão estrutural por nominal; inventário de contratos | retirar arena da lista somente após substituição completa |
| §5.2 | trocar privilégios por remissão a papéis e contrato da Base | manter documentados os limites do respaldo e do runtime |
| §5.3 | assinaturas de produtores de buffer/slice com papéis quando aplicáveis | não declarar invalidação onde a operação não invalida |
| §§5.4–5.7 | cláusulas nominais e referências aos protocolos | não mudar comportamento dos módulos |
| §5.8 | situar bibliotecas futuras separadamente do mecanismo | não antecipar Tensor nem handles como requisito de v0 |
| §6.1 | separar erro de tradução, erro C e falha em runtime | não confundir papel com check geracional |
| §6.2 | migrar catálogo e acrescentar erros de especialização | IDs candidatos na tabela seguinte |
| §§6.3–6.5 | explicitar garantias condicionais e política opcional de geração | não prometer segurança geral nem custo zero de todos os usos |

### 6.1 Diagnósticos

| Atual ou novo | Proposta |
| --- | --- |
| `arena-escape` | generalizar para `region-escape` |
| `child-arena-after-reset` | generalizar para `child-region-after-invalidation`; precisar o consumo do próprio recurso |
| `arena-from-array-not-u8` | substituir por validação geral de argumento array/bytes, com ID a fechar |
| `nonconstant-arena-stack` | coberto por `nonconstant-dim`; conservar restrição de contexto do molde |
| `byref-param` | preservar e estender a tipos comuns com propriedade declarada |
| `alloc-overflow` | preservar na biblioteca/backend em execução; falha limpa também sem checks |
| `type-param-outside-size` | retirar para parâmetros agora especializados; só conservar se restar apagamento explícito |
| `protocol-on-parameter` | manter para tipo opaco de módulo onde a regra ainda se aplica |
| `protocol-not-satisfied`, `protocol-verb-missing` | conformidade no uso e no implementador |
| `verb-not-in-protocol` | uso fora do contrato no corpo genérico |
| `circular-protocol`, `protocol-verb-conflict` | composição inválida |
| `instance-depth` e limite de quantidade/tamanho | expansão de instâncias sem limite prático |
| `mold-position`, `mold-return`, `mold-argument` | uso ou corpo fora da forma permitida |
| `circular-mold`, `mold-depth`, `mold-size` | expansão não terminante ou excessiva |
| captura de código não suportada / contexto indisponível | novos IDs a fechar, erro no argumento com referência ao uso |
| consulta de tipo declarado indisponível/cíclica | novos IDs a fechar |

Conformidade nominal não substitui diagnósticos específicos de índices,
binders, exaustividade ou fluxo. Só substitui a parcela de diagnóstico sobre
participação no protocolo.

## 7. Impactos fora da spec e critérios de aceite

| Área | Trabalho necessário |
| --- | --- |
| Backend §§2.1–2.4 | nomes de instância de função e fragmento; higiene; colisões e teto de nomes |
| Backend §§4.1–4.4 | headers por instância de função, posse fora de linha e camadas de dependências |
| Backend §5.4 | remover lowering especial de arena após migração para mecanismos gerais |
| Backend §5.5 | garantir que defer recebe o fluxo já expandido |
| Backend §5.16 | revisar apagamento de type e wrappers especializados |
| Backend §5.17 | preservar checks sem efeitos e independência entre checks e layout |
| Backend §§6–7 | origem dos tokens, determinismo com fragmentos e depfiles |
| Ferramenta §§4.3, 4.5, 5–6 | instâncias de função, dependências, identidade, atualização e escrita determinística |
| Rationale | justificar papéis, nominalidade, duas posições de keel_code e revisão do apagamento |
| Base | arena comum, wrappers, assinaturas com papéis e declarações de protocolo |
| Parser/AST | registrar contratos, espécies de parâmetros, corpos, origem e vínculos de substituição |
| Análise | grafo lexical, conformidade, especialização e consulta de retorno declarado |
| Emissão | expansão no escopo correto e função especializada sem captura acidental |
| Golden | revisar fontes e escrever esperados C11/C23 à mão; não derivar um perfil do outro |

Critérios de aceite para a implementação futura:

1. Mesmo diagnóstico de escape e invalidação em um módulo de usuário com outro
   nome, sem teste especial de `keel.arena`.
2. Inicialização de arena preservada em todas as formas atuais e ausência de
   inicialização inserida em campos/extern.
3. Variante sem geração sem campos ou instruções geracionais; variante
   verificada com custo declarado, invalidação de avô, cópia de handle,
   reuso de slot e política de restore exercitados.
4. `free` de uma entidade não invalida as demais; `reset` do pool invalida
   todas; nenhum check se apresenta como proteção de ponteiro cru escapado.
5. Protocolos usados de imediato pela Base e pelas construções do golden;
   falhas de verbo ausente, papel divergente, ciclo e uso fora de protocolo.
6. Mesma função especializada por tipos ou fragmentos diferentes; combinação
   idêntica reutilizada; nomes iguais em contextos diferentes não confundidos.
7. `consume(next(), mold())` com código executado no ponto de `C` dentro
   da função gerada, sem expansão da função comum no chamador.
8. Duas expansões de `from_stack` no mesmo bloco sem colisão, armazenamento
   vivo pelo bloco correto, tamanho constante e sem zeroing não solicitado.
9. Posições recusadas não alteram curto-circuito, frequência de avaliação nem
   duração de objetos. `defer` mantém os destinos de saída.
10. Headers determinísticos por identidade, dependências completas e nenhum
    novo `.c` sem regra explícita de posse e compilação.

Estes são critérios propostos; não foram executados testes do compilador para
este documento. A tarefa presente é redação e análise da especificação.

## 8. Decisões para aceite antes de editar os normativos

| Decisão | Direção recomendada neste documento |
| --- | --- |
| Papéis | adotar como contrato geral, com análise lexical e assinatura confiada |
| Alvo reutilizável versus recurso consumido | fechar a distinção antes de normatizar free individual |
| Geração | biblioteca opcional; nenhuma imposição de layout pelo papel |
| Cadeia de regiões | validar ancestrais; declarar custo por profundidade |
| Restore verificado | começar com invalidação ampla explicitamente contratada, ou adiar a variante precisa |
| Inicializador padrão e byref comum | necessários para retirar todo privilégio; grafia ainda a escolher |
| Protocolos | nominalidade também nas construções; binders por protocolo propostos |
| Requisitos condicionais | fechar ptr/get e failed/win sem aumentar obrigações silenciosamente |
| Type de função | especializar por padrão; migrar o apagamento atual de forma explícita |
| Keel_code em parâmetro | especializa o corpo; não implica expandir a função chamadora |
| Keel_code na função | expansão no chamador, com retorno final e posições iniciais restritas |
| Captura | nenhuma captura automática em função comum especializada |
| Fragmento e identidade | expressão inicialmente; contexto e codificação determinística obrigatórios |
| Emissão | respeitar pub/inline e os três artefatos atuais; posse fora de linha explícita |

A incorporação pode ser dividida, mas a retirada dos privilégios da arena deve
esperar o conjunto mínimo completo. Protocolos, papéis e substituição são
mecanismos gerais do núcleo; os contratos concretos e os algoritmos ficam na
Base. A redução é de casos especiais, não a eliminação do trabalho de
reconhecimento, instanciação e emissão do compilador.
