# `array` como cidadão de primeira classe — verbos com corpo em `keel.k`

> **Decidido e aplicado em 2026-09-25**, com mudanças: o binder é
> `array T v[size_t N]` (explícito, por causa de `#define N`), só na dimensão 0;
> as constantes (`length`, `capacity`, `dim`) ficam em `keel`, e `get`, `set`,
> `ptr` e `at` vão para `module keel.array type T;`, selecionado pelo tipo do
> elemento (saída A2). O texto normativo está na spec §4.2, §5.3, no rationale
> ("O tamanho de um parâmetro `array`") e no backend §5.18. Este briefing fica
> como histórico.

Briefing de uma discussão de design (sessão remota, 2026-09-25), ainda **não
decidida** — nada foi escrito nos normativos. Traz pro Claude Code do PC pra
continuar dali.

## Contexto

Hoje `keel.length(v)`, `keel.ptr(v[,i])`, `keel.get/set(v,i)`, `keel.at(v,i)`,
`keel.capacity(v)`, `keel.dim(v,k)` (spec §4.2) são só prosa — nenhuma existe
como texto keel em `/base`, ao contrário de `buffer`/`slice`/`arena`/`range`,
que têm corpo real em `base/keel/*.k`. `array` é a única família de contêiner
cujos verbos "aparecem do nada e somem no nada" na tradução — sem nem uma
função geradas, ao contrário de `buffer`/`slice`/`extent`.

## Diagnóstico do porquê falta

`array` não tem runtime descriptor (`array i32 m[2,3]` **é** `i32 m[2][3]`,
spec §4.2 regra 10) — não dá pra virar `modifier`/módulo genérico convencional
sem quebrar essa invariante (e sem violar a premissa de parser de ilhas: quebraria
interop com C escrito por quem nunca pensou em keel, tipo os casos golden do
`list.h` do Linux, `022-024-linux-list-*`). A causa raiz real: a gramática hoje
trata o conteúdo dos colchetes de um parâmetro `array` como **opaco**
(`dimensions ::= '[' [ <opaque> {...} ] ']'`) — keel já sabe o tamanho de cada
`array` (inclusive em parâmetro, regra 12), só não empresta esse número pro
corpo de uma função.

## Proposta (three-part)

1. **Binder de dimensão em parâmetro `array`.** `param-array` ganha uma posição
   de binder: `array T v[N]` liga `N` ao valor real da dimensão a cada chamada
   — apagado na tradução, exatamente como `type T` já é apagado em
   `arena.alloc` (`pub T *alloc(arena *a, type T, size_t n); // erased`). Vira
   `size_t` comum passado pro C, não template, não gera declaração nova (não
   cai na recusa "família de declarações a partir de `dim`" da rationale).
   Diagnóstico de colisão reaproveita `parameter-name-reuse`.

2. **Binder de tipo no mesmo marcador.** `T` em `array T v[N]` também vira
   variável livre nessa posição (hoje `argument` exige tipo já conhecido) —
   sem precisar de `type T` como parâmetro separado. Isso é o que mantém
   `keel.length(v)` com **um argumento só**, como já documentado — um `type T`
   explícito obrigaria o call site a virar `keel.length(T, v)`, quebrando a
   tabela atual. `T` e `N` binders simétricos, ambos dentro do mesmo
   `[...]`/marcador.

3. **Os sete verbos escritos em `base/keel.k`**, ao lado de `keel_index`:

   ```keel
   size_t length(array T v[N])            { return N; }
   size_t capacity(array T v[N])           { return N; }
   T *ptr(array T v[N])                    { return v; }
   T *ptr(array T v[N], size_t i)          { return &v[keel_index(i, N)]; }
   T get(array T v[N], size_t i)           { return v[keel_index(i, N)]; }
   void set(array T v[N], size_t i, T x)   { v[keel_index(i, N)] = x; }
   outcome T at(array T v[N], size_t i)    { ... }
   ```

   `keel.dim(v,k)` e formas multidimensionais (`array T v[R,C]`) ficam
   **pendentes** — não cabem em aridade fixa, decidir depois.

## Dois problemas abertos, ainda sem decisão

### A. Loop de import

`at` precisa de `outcome`, mas `keel.outcome.k` já recebe `import keel types;`
implícito (§4.1 regra 15) — se `keel.k` importar `keel.outcome` explicitamente
pro `at`, fecha um `keel ↔ outcome` que a regra `circular-import` pega hoje, ao
pé da letra. Não é recursão de verdade (lados usam coisas diferentes: um
primitivos, o outro só um tipo de retorno), mas a spec não tem carve-out pra
isso. Duas saídas:

- **(A1)** isentar a aresta implícita da checagem de ciclo (regra nova,
  pontual);
- **(A2)** tirar `at` do prelúdio pra `module keel.array;` (arquivo próprio,
  mesmo padrão que já tirou `tagged` do núcleo) — `length/capacity/ptr/get/set`
  ficam em `keel.k` sem import; só `at` muda de `keel.at` pra `array.at`, com
  import explícito. Custo real baixo, porque quem chama `at` já precisa
  importar `outcome` pra nomear o retorno.

Inclinação: (A2), por seguir precedente existente, mas troca o nome da
chamada — decisão do André.

### B. Escopo do binder

Não precisa ficar restrito aos sete verbos do núcleo — é extensão geral de
`param-array`, então qualquer função do usuário ganha acesso ao tamanho de um
parâmetro `array`. Considerado feature, não risco.

## O que muda em cada normativo, se aprovado

- `keel-spec.md` §2.2 (gramática `param-array`/`dimensions`), §4.2 (regra nova
  de binder T+N, tabela de operações apontando pro corpo real), possivelmente
  §4.1 (a isenção do ciclo, se (A1)).
- `keel-rationale.md`: registrar a simetria `else` (outcome) / `a..b` (range) /
  `[N]` (array) como mesmo mecanismo — frase-chave do André: "keel é o truque
  de macro como linguagem".
- `base/keel.k` (ou novo `base/keel/array.k` se (A2)): os verbos.
- Nenhuma mudança de emissão/backend nova — o C gerado continua igual a hoje,
  só passa a ter fonte keel legível por trás.

**Nada foi escrito nos arquivos normativos ainda** — CLAUDE.md pede "sim" antes
de editar `keel-spec.md`/`keel-rationale.md`/`keel-c-backend.md`/
`cgen-tool-spec.md`, e ficaram as duas perguntas (A e confirmação de B) em
aberto.
