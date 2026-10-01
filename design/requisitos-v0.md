# Requisitos de arquitetura da implementação v0

Requisitos para que as construções v1 da spec não exijam reescrita:

1. **Origem por token.** Cada token guarda arquivo e linha de origem, e
   reserva espaço para a cadeia de expansão.
2. **Vínculo por identidade.** Cada uso de nome aponta para a declaração no
   seu escopo, não para a grafia.
3. **Passes em ordem fixa:** coleta, resolução, expansão (vazia na v0),
   papéis, `defer`, emissão.
4. **Análise sobre a AST.** Papéis e `defer` operam sobre a AST já
   transformada, sem caso especial para origem dos nós.
5. **Tabela de protocolos.** As construções resolvem verbos por
   protocolo → verbos, não por nomes fixos em cada construção.
6. **Grafo de procedência.** Arestas de dependência e estados por símbolo
   ([spec §4.12](../keel-spec.md#412-procedência-e-invalidação)), sem
   representação em execução.

A v0 reconhece as construções v1 e as recusa com `not-in-v0`. Estes requisitos
orientam a implementação; não afirmam que o compilador atual já os satisfaz.

## Decisões preservadas nesta revisão

- Nomes ingleses dos protocolos são a grafia usada nesta versão; uma eventual
  renomeação é decisão editorial posterior.
- A posição gramatical de `byref` em `typedef` está registrada; sua
  generalização semântica permanece pendente. A arena conserva sua restrição
  específica de passagem por valor.
- O `{0}` implícito da arena permanece até a decisão sobre seu mecanismo geral.

O argumento das decisões está no [rationale](../keel-rationale.md#papéis-e-núcleo-mínimo).
