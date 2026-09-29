# keel — Análises

Documento **não normativo**. Registra análises críticas do projeto, datadas,
para que se possa comparar o que se pensava com o que aconteceu. Nada aqui é
decisão: uma recomendação só vira contrato pelo caminho de sempre — discussão,
texto proposto, aceite.

---

## 2026-09-29 — Utilidade, posicionamento e metaprogramação

**Premissa.** O cgen está terminado nos moldes da spec. A pergunta é se keel,
assim, é útil ou só um esforço acadêmico; como se compara a V e Nim, com Zig,
C++ e Rust como teto; e se vale pensar seriamente em metaprogramação.

### Veredito

Útil, num nicho estreito. O risco maior não é técnico, é de posicionamento: ser
visto como "C3 com menos recursos" ou "C com macros mais bonitas". O que o
sustenta de verdade é uma combinação que nenhum dos concorrentes tem junta:

- a entrada é C, e dá para adotar arquivo a arquivo;
- a saída é C legível, com `#line`;
- toda construção tem o custo e a tradução escritos.

Isso tem valor onde o artefato final tem de ser C: embarcado, toolchains de
fornecedor, código que passa por certificação ou auditoria, times que não podem
trocar de compilador. Fora desse público, quem aceita mudar de linguagem vai
para Zig ou Rust.

### Pontos fortes

- **A adoção incremental é real.** Um `.k` é C, e o C atravessa. Zig e Rust
  pedem FFI; Nim e V geram C que ninguém lê.
- **O C gerado é auditável.** Em ambiente regulado, poder ler e revisar o C é
  argumento de venda, e não detalhe.
- **Não há runtime escondido:** nem GC, nem VLA, nem `alloca`. O golden o
  verifica com `-Werror=vla`.
- **O custo é documentado.** O backend tem mais de 2000 linhas dizendo o que
  cada construção emite, coisa rara até em linguagens estabelecidas.
- **O foco é coerente:** DOD. `extent`, `buffer`/`slice`,
  `partition`/`parallel` e `tags`/`match` formam um conjunto com uma tese, e não
  uma coleção de recursos.
- **Os genéricos por módulo resolvem uma dor real de C.** Monomorfização com
  nome estável e o corte `.type.h`/`.h` substituem X-macros e `_Generic`, com
  erro legível.
- **O rigor de engenharia é incomum.** Spec, rationale, backend e oráculo
  executável em dois perfis estão acima do que a maioria dos projetos de
  linguagem tem nessa fase.

### Pontos fracos

- **A segurança é "C com asserts".** A discussão da arena (possibilidades,
  "Alocável e Hierarquizável") mostrou o limite: o uso depois do reset só é
  pego no mesmo escopo, e o dado derivado nunca. Rust dá garantia, Zig dá
  ferramenta; keel dá conveniência e verificação em debug. É honesto, mas não é
  diferencial.
- **Os erros vêm do compilador C.** Erro de tipo sai do gcc sobre
  `keel_buffer_i32_as_slice2`. O `#line` acerta a linha, mas não o vocabulário.
  Zig, Rust e Nim são bem melhores aqui.
- **O ecossistema é zero:** não há LSP, gerenciador de pacotes nem build
  próprio. No depurador, as variáveis aparecem com nomes mangled.
- **O parser de ilhas é frágil por natureza.** Macros do usuário podem esconder
  sintaxe, e nomes comuns colidem (o `apply` era um exemplo). O modelo impõe um
  teto ao que keel consegue verificar.
- **O tamanho da spec é um sinal.** São cerca de 7000 linhas normativas e 140
  diagnósticos para um pré-processador, e muito disso são casos especiais por
  construção. É o argumento mais forte a favor da metaprogramação.
- **Os genéricos não servem para escrever algoritmos.** `T` é opaco, então não
  se escreve `media` sobre qualquer Percorrível. Zig (comptime), Rust (traits),
  Nim (concepts) e C++ (concepts) fazem isso.
- **Há um autor só.** O risco de continuidade é alto.

### Comparação

| | keel | V | Nim | C3 | Zig | C++ | Rust |
|---|---|---|---|---|---|---|---|
| Aceita C cru no fonte | **sim** | não | não | não | via `@cImport` | quase | não |
| Saída C legível | **sim** | razoável | não | não gera C | não | — | — |
| Genéricos úteis para algoritmos | não | limitado | sim | sim | sim | sim | sim |
| Segurança de memória | asserts de debug | fraca | ARC/ORC | contratos | checks e ferramentas | RAII | **garantida** |
| Metaprogramação | substituição por módulo | limitada | macros de AST | macros | comptime | templates | macros e traits |
| Maturidade | spec | irregular | madura | jovem | pré-1.0, forte | total | total |

- **V** é o oposto em disciplina: promete antes de entregar, e keel especifica
  antes de implementar. V tem mais usuários; keel tem mais previsibilidade.
- **Nim** é o precedente mais instrutivo para a metaprogramação: mostrou que
  macros sobre um backend C funcionam, e também o custo delas em legibilidade e
  em ferramentas.
- **C3** e **Cake** são os concorrentes diretos.
  - C3 tem módulos genéricos quase iguais aos de keel, mais `defer`, slices,
    resultados falíveis e contratos. É uma linguagem completa, com compilador
    LLVM próprio, e não aceita C cru.
  - Cake transpila C23 para C antigo, com `defer` e análise de ownership.
  - keel precisa saber responder "por que não C3?". A resposta honesta é o mar
    de C e a saída legível.
- **Zig, C++ e Rust** são o teto. keel não compete em garantias nem em
  ferramentas; compete em "o resultado é C que você entrega e revisa".

### Metaprogramação

Vale pensar seriamente, com restrições fortes, e não agora.

**A favor**

- keel já faz metaprogramação: instanciar um módulo genérico é substituição
  textual de `T`. `construct` e `protocol` estendem o mesmo mecanismo, sem
  paradigma novo.
- O núcleo ficaria com `module`, `modifier`, `construct` e `protocol`, e boa
  parte da spec e do catálogo sairia do núcleo para a biblioteca, com
  diagnósticos vindos da conformidade ao protocolo, e não escritos um a um.
- `protocol` com `bound` resolve o ponto fraco mais sério, que é não poder
  escrever algoritmos genéricos.

**Contra**

- **Conflito de princípio.** keel não avalia expressões C. Metaprogramação com
  computação em tempo de compilação, como o comptime do Zig ou as macros do Nim,
  exigiria justamente isso. Só a forma declarativa — substituição, sem avaliação
  nem reflexão — é compatível com a tese.
- **O diferencial está em jogo.** "Toda construção tem uma tradução escrita e
  legível" deixa de valer se o usuário puder escrever construções arbitrárias.
  Continua valendo só se o molde for a tradução, e o C emitido for exatamente o
  molde preenchido.
- **Nim e C++ mostram o custo** em mensagens de erro, em ferramentas e no "o que
  esse código faz?".
- **Não resolve tudo.** `parallel`, `match` e `else` não cabem em substituição,
  então o núcleo não encolhe tanto quanto parece.

**Ordem proposta**

1. **Congelar a v0 e terminar o cgen.** Sem ele nada disso é útil, e reabrir o
   núcleo antes do primeiro compilador é a armadilha clássica de projeto de
   linguagem.
2. **Escrever um programa real médio em keel** (5 a 10 mil linhas, algo DOD).
   Essa experiência diz se faltam algoritmos genéricos ou construções novas,
   melhor do que qualquer especulação.
3. **Protocolo nominal com `bound` primeiro.** É o maior ganho de expressividade
   pelo menor custo.
4. **`construct` depois**, com um critério de aceite objetivo: reescrever o
   `foreach` e o `walk` da base como moldes e obter emissão idêntica ao golden de
   hoje. Se não der, a metaprogramação não paga o que custa.
5. **Nunca** macros com computação ou reflexão sobre campos, pelo princípio de
   não analisar C.

Se a v0 sair sólida e os passos 3 e 4 passarem, keel vira um "C3 que aceita C"
com núcleo pequeno, e aí o argumento de posicionamento fica bem mais forte.

Ver também: [possibilidades](possibilidades.md), entradas "Protocolo nominal",
"Alocável e Hierarquizável" e "Construções definidas por módulo".
