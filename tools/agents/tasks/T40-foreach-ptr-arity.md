<!--
   FILE: T40-foreach-ptr-arity.md
   DESC: cartão de tarefa para agente Haiku — conserto do sufixo de aridade
         em foreach de binder por ponteiro
   AUTHOR: Claude Sonnet 5 (orquestrador)
   DATE: Nov, 2026
-->

# T40 — `foreach` de binder por ponteiro: sufixo de aridade errado

## Aviso antes de começar

Esta tarefa **não é um swap de literal contido**. A investigação do
orquestrador já foi longe o bastante para saber que o conserto correto
precisa resolver a aridade real do verbo `ptr` **no módulo que declara o
contêiner**, não apenas trocar uma string fixa por outra. Pode ser que o
jeito certo de fazer isso exija tocar o parser (onde a ilha de `foreach` é
montada) e não só o emissor. **Se a investigação levar a essa conclusão,
pare e registre no relatório — não edite arquivo de parser, e não invente
uma segunda forma de resolver a mesma coisa.** Essa é a saída aceitável
desta tarefa, tanto quanto um conserto que passa na aceitação.

## Objetivo

Em `tools/cgen/src/engine/emit/foreach.c`, a função `container_value()`
grava o literal fixo `"_ptr1(keel__c"` para o binder por ponteiro, não
importa quantas aridades o verbo `ptr` tem no módulo que declara o tipo do
contêiner:

```c
// tools/cgen/src/engine/emit/foreach.c:332-347
static const char *container_value(Emit *e, size_t n, const Over *o, size_t *vl) {
    const KAst *ast = e->ast;
    e->n = 0;
    k_em_put_range(e, o->vtype_a, o->vtype_b);
    if (!word(ast, o->vtype_b - 1, "*")) k_em_put_bytes(e, " ", 1);
    put_tok(e, o->value);
    k_em_put_bytes(e, " = ", 3);
    k_em_put_bytes(e, o->sym, o->sym_len);
    if (o->ptr_binder) k_em_put_bytes(e, "_ptr1(keel__c", 13);   /* the element's address (rule 3) */
    else k_em_put_bytes(e, "_get(keel__c", 12);
    put_uint(e, n);
    k_em_put_bytes(e, ", ", 2); put_tok(e, o->index);
    k_em_put_bytes(e, ");", 2);
    return take(e, vl);
}
```

Pela regra 1 do backend §2.1.1 — "o sufixo existe só quando o verbo é
declarado em mais de uma aridade" —, o sufixo `1` só é correto quando o
módulo do contêiner declara **duas** aridades de `ptr` (`ptr(x)` e
`ptr(x,i)`, como `keel.buffer` e `keel.slice` fazem). Um módulo com `ptr`
numa aridade só (só `ptr(x,i)`) deveria gerar `_ptr`, sem sufixo — e hoje
gera `_ptr1`, que não existe, e o C falha.

`o->ptr_binder` é `true` quando o binder é `T *p` (ponteiro); o `else`
(binder por valor, chama `_get`) não tem o mesmo problema porque o
protocolo `IndexGet` só admite `get` numa aridade — não é força de
coincidência, é a razão de nunca ter sido notado.

## O que "correto" já parece em outro lugar do cgen

Duas peças do próprio cgen já resolvem o mesmo problema — sufixo de
aridade de um verbo, contra o módulo real que o declara — para outras
construções. Elas são a referência de correção, **não necessariamente o
código a chamar direto** (uma mora no parser, isolada por
`parser_islands_internal.h`; ver o aviso no topo do cartão sobre não
atravessar essa fronteira sem parar e perguntar primeiro):

**1. A indexação `x[i]`, no parser** —
`tools/cgen/src/engine/parser/parser_islands_index.c:86-99`:

```c
/* `<type symbol>_<verb><suffix>` for a container and a call of `arity` arguments,
   object included; `adapt` says the object takes an `&`. */
bool isl_container_verb(const KType *k, const KAst *home, keel_slice_char verb, size_t arity, Text *tx,
                           bool *adapt) {
    Sig sig = isl_find_verb(home, verb, arity);
    if (!sig.found || !*k->symbol) return false;
    isl_put_str(tx, k->symbol);
    isl_put_str(tx, "_");
    isl_put(tx, verb.ptr, verb.len);
    if (sig.rank_binder >= 0) return false;
    if (sig.overloaded && arity > 1) isl_put_uint(tx, arity - 1);
    if (sig.n && sig.p[0].pointer && k->pointers == 0 && k->lvalue) *adapt = true;
    return true;
}
```

`home` é o AST do módulo que declara o tipo do contêiner (não o AST do
módulo sendo traduzido). `isl_find_verb` (`parser_islands.c:337`, assinatura
em `parser_islands_internal.h:185`) procura o verbo nesse AST e devolve
`Sig.overloaded` — `true` só quando há mais de uma aridade declarada. É
`sig.overloaded && arity > 1` que decide o sufixo, nunca um valor fixo.

```c
typedef struct {
    size_t n;
    bool found;
    bool overloaded;    /* declared in more than one arity (backend §2.1.1) */
    int rank_binder;
    bool producer;
    const KAstNode *fn;
} Sig;
Sig isl_find_verb(const KAst *a, keel_slice_char verb, size_t arity);
```

**2. A contagem de aridade dentro do próprio módulo, no emissor** —
`tools/cgen/src/engine/emit/roles.c:60-80`, `k_em_arity_suffix`: percorre
`ast->nodes` do **mesmo** AST em que `fn` está, procurando outra declaração
do mesmo nome com aridade diferente. Serve de modelo de "como contar
aridades a partir de um `KAstNode`", mas só alcança o módulo que está
sendo emitido agora — **não** o módulo que declara o contêiner de um
`foreach`, que normalmente é outro.

**3. Acesso a um AST de outro módulo, a partir do emissor** — já existe em
mais de um arquivo de `emit/`, pela tabela de símbolos do AST atual
(`ast->symbols`), por exemplo em `tools/cgen/src/engine/emit/uses.c:160-176`:

```c
static const KAst *callee_module(const KAst *ast, size_t anchor, size_t end, size_t *name) {
    if (!ast->symbols) return NULL;
    *name = end - 1;
    if (end - anchor == 3 && k_em_text_is(k_em_tok(ast, anchor + 1), ".")) {
        KToken m = k_em_tok(ast, anchor);
        for (size_t i = 0; i < ast->symbols->len; i++) {
            const KSymbol *s = keel_buffer_KSymbol_ptr(ast->symbols, i);
            if (s->kind == K_SYM_MODULE && s->origin && s->origin->ast &&
                k_symtab_same_name(s->name, (keel_slice_char){ m.len, (char *)m.ptr }))
                return s->origin->ast;
        }
    }
    ...
}
```

e em `tools/cgen/src/engine/emit/match.c:340-350` e
`tools/cgen/src/engine/emit/routine.c:75-90`, com a mesma forma: percorrer
`ast->symbols`, achar um `KSymbol` de `kind` `K_SYM_TYPE` ou
`K_SYM_MODIFIER` cujo nome escrito corresponda, e seguir `s->origin->ast`
até o AST que de fato declara o tipo/modificador.

O que falta decidir — e é aqui que a investigação pode achar que o jeito
correto está no parser, não no emissor — é **de onde, dentro de
`container_value()`/`analyze_over()` em `foreach.c`, vem o nome original
(não mangled) do modificador do contêiner** (`vec`, no caso do oráculo
desta tarefa), para procurá-lo em `ast->symbols` do jeito do item 3. `Over`
guarda `o->cont` (o token do uso da variável contêiner) e `o->sym`/`sym_len`
(o prefixo **já mangled**, como `fv_vec_i32`) — não guarda o nome do
modificador como escrito. As linhas 239-248 de `foreach.c` já procuram a
declaração da variável contêiner dentro da função atual (para decidir
`by_pointer`); talvez a mesma busca dê acesso aos tokens do tipo escrito
(`vec i32 v`), e dali ao nome `vec` a procurar em `ast->symbols`.

## Arquivos

Pode editar:
- `tools/cgen/src/engine/emit/foreach.c`
- `tools/cgen/src/engine/emit/roles.c` e sua declaração em
  `tools/cgen/src/engine/emit/emit_internal.h`, **só se** precisar expor
  dali uma função pequena e genérica (ex.: "este AST declara este verbo em
  mais de uma aridade?") para reutilizar de `foreach.c` — não para
  duplicar a lógica de `roles.c` com outro nome.
- `test/cgen/gen/golden-cases.txt` — **só a linha do caso 063**, para
  remover o `|wip` depois que o caso passar a bater byte a byte (ver
  Aceitação).

Não edita, não move, não apaga:
- `golden/cases/063-foreach-ptr-arity/**` (o oráculo desta tarefa, escrito
  à mão pelo orquestrador — tocar nele invalidaria a verificação).
- qualquer outro caso de `golden/cases/`.
- qualquer arquivo de `tools/cgen/src/engine/parser/` (ver o aviso no
  topo: achar que precisa é motivo de parar e registrar, não de editar).
- `keel-spec.md`, `keel-rationale.md`, `keel-c-backend.md`,
  `cgen-tool-spec.md`.

## Contrato

Depois do conserto, para um módulo cujo `ptr` tem **uma aridade só**
(como `fv.k` do oráculo desta tarefa), `foreach (T *p, size_t i :
container)` deve chamar `<símbolo>_ptr(keel__cN, i)`, sem sufixo. Para um
módulo cujo `ptr` tem **duas** aridades (como `keel.buffer`/`keel.slice`),
deve continuar chamando `<símbolo>_ptr1(keel__cN, i)`, exatamente como
hoje — **nenhum caso golden existente pode mudar de saída** (eles todos
passam por `keel.buffer`/`keel.slice`, de duas aridades).

O oráculo que prova isso já está escrito e commitado:
`golden/cases/063-foreach-ptr-arity/` (ler o `NOTES` do caso primeiro — ele
explica a aridade única de `fv.k` e por que o caso está `|wip` hoje em
`test/cgen/gen/golden-cases.txt`).

## Passos sugeridos

1. Leia `golden/cases/063-foreach-ptr-arity/NOTES`, `fv.k` e `fv_use.k`, e
   o `expected/c23/fv_use.c` — é o C que o `cgen` deve passar a produzir.
2. Leia os três pontos de referência da seção anterior até entender como
   cada um resolve "qual é o AST que declara este tipo" e "quantas
   aridades este verbo tem nesse AST".
3. Em `foreach.c`, decida como `container_value()` (ou quem a chama) passa
   a saber a aridade real de `ptr` para o contêiner em questão. Teste a
   ideia com um build (`make -C tools/cgen -j8`) e com
   `cgen --stop-after=gen` manual sobre o `.k` do oráculo antes de rodar a
   suíte inteira — mais rápido para iterar.
4. Se, depois de tentar, a conclusão for que isso exige mudar a ilha de
   `foreach` no parser (o formato do `Detail` que ele grava), **pare**:
   registre no relatório o que tentou, por que não bastou ficar só no
   emissor, e deixe o código no último estado que passa
   `sh tools/agents/verify.sh`.

## Aceitação

```sh
sh tools/agents/verify.sh
```
deve terminar em `ALL OK`.

Além disso, confira que o caso 063 virou `ok` de verdade, não só que o
`|wip` foi removido sem checar:

```sh
sh test/cgen/gen_golden.sh 2>&1 | grep -E '063-foreach-ptr-arity|^[0-9]+ ok'
```
deve mostrar o caso **sem** a palavra `wip` ao lado (hoje mostra
`wip    063-foreach-ptr-arity (differs)` nos dois perfis), e a última linha
deve ter uma unidade mais em `ok` do que tinha antes (72 → 73) e **zero**
em `wip` para este caso. Só depois disso, tire o `|wip` da linha do 063 em
`test/cgen/gen/golden-cases.txt` e rode de novo para confirmar que
continua `ok` sem o flag.

Por fim:
```sh
sh test/run.sh
```
tem que terminar com código 0 e sem nenhum `FAIL`.

## Relatório

Escreva `tools/agents/reports/T40-foreach-ptr-arity.md`, em pt-BR:
- o que mudou, arquivo por arquivo, com o trecho relevante;
- por que a solução escolhida não precisa olhar a aridade de `ptr` de
  nenhum outro módulo além do real (ou seja: por que não é um valor fixo
  disfarçado de outro jeito);
- a saída exata de `sh tools/agents/verify.sh`, de
  `sh test/cgen/gen_golden.sh` (a linha do 063 e o total) e de
  `sh test/run.sh`;
- se parou no parser (seção "Passos sugeridos", item 4): o que tentou e
  por que concluiu que não bastava ficar no emissor — com evidência, não
  só a afirmação.
