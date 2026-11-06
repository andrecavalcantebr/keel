<!--
   FILE: T41-foreach-countable.md
   DESC: cartão de tarefa para agente Haiku — foreach de um binder sobre
         um tipo que só implementa Countable (first/limit)
   AUTHOR: Claude Sonnet 5 (orquestrador)
   DATE: Nov, 2026
-->

# T41 — `foreach` de um binder sobre `Countable`: forma geral, hoje recusada

## Aviso antes de começar

Esta tarefa é **nova emissão**, não um bug num caminho existente: hoje não
existe nenhuma análise, em `tools/cgen/src/engine/emit/foreach.c`, para um
`foreach (auto i : x)` onde `x` é uma variável de um tipo que implementa só
`Countable` (`first`/`limit`, spec §5.1) e não é `range`. O comentário de
topo do arquivo já documenta isso: "the countable form of another type...
stays refused." O cgen recusa com `implementation-limit`.

A boa notícia: a peça que falta é pequena, e **reaproveita três mecanismos
já prontos** no próprio arquivo — nenhum deles no parser. Este cartão quota
o código exato de cada um. Se, ainda assim, a investigação concluir que é
preciso tocar `tools/cgen/src/engine/parser/`, **pare e registre no
relatório** — não edite arquivo de parser, e não invente uma segunda forma
de resolver a mesma coisa.

## Objetivo

Em `foreach.c`, acrescentar um quarto caminho de análise/emissão para
`foreach (auto i : x)` (um binder, `x` uma variável — por ponteiro ou por
valor — de um tipo **diferente de `range`** que declara `first` e `limit`),
ao lado dos três que já existem:

1. `analyze_over`/`edits_over` — dois binders, `IndexGet`/`IndexPtr`.
2. `analyze_range_var`/`edits_range_var` — um binder, variável de `range`.
3. `analyze`/a forma literal `A..B` — um binder, literal.

## O que já existe e deve ser reaproveitado

**1. O detalhe da ilha já carrega o tipo escrito, para qualquer variável —
não só `range`.** `traversal()` (parser, fora do escopo desta tarefa, só
para contexto) grava `"<binder> : <contêiner> (<tipo escrito>)"` sempre que
o contêiner é uma variável local — `tools/cgen/src/engine/parser/parser_islands_flow.c:224-225`:

```c
const Local *l = box.end == box.first + 1 && isl_ident(a, box.first) ? isl_find_local(c, isl_tok(a, box.first)) : NULL;
if (l && l->spec.text.len) { isl_put_str(&d, " ("); isl_put(&d, l->spec.text.ptr, l->spec.text.len); isl_put_str(&d, ")"); }
```

Confirmado rodando o binário sobre um módulo equivalente ao oráculo desta
tarefa, com `--stop-after=parse`:

```
ilha	foreach	i : w (window)	.../win3.k:10:5
```

`analyze_range_var` (linha 304) já lê esse mesmo detalhe, mas só aceita
quando o texto entre parênteses é literalmente `"range"`:

```c
// tools/cgen/src/engine/emit/foreach.c:302-313
static bool analyze_range_var(const KAst *ast, const KIsland *is, size_t kw, const KAstNode *fn, RangeVar *r) {
    if (!word(ast, kw, "foreach") || !word(ast, kw + 1, "(")) return false;
    r->close = kw + 6;
    if (!word(ast, r->close, ")") || !word(ast, kw + 4, ":")) return false;
    if (!word_like(ast, kw + 2) || !word_like(ast, kw + 3) || !word_like(ast, kw + 5)) return false;
    r->type = kw + 2; r->binder = kw + 3; r->colon = kw + 4; r->cont = kw + 5;
    const char *d = keel_buffer_char_ptr(&ast->island_text, is->text);
    size_t len = is->text_len;
    if (len < 7 || memcmp(d + len - 7, "(range)", 7) != 0) return false;
```

A forma geral desta tarefa tem a MESMA estrutura de cabeçalho (`foreach (
<tipo-ou-auto> <binder> : <cont> )`), e deve ser tentada **depois** de
`analyze_range_var` falhar (ou seja: continua tratando `"(range)"` pelo
caminho de hoje, intocado — nenhum golden existente pode mudar de saída).

**2. `k_em_type_symbol` já resolve o nome escrito de um tipo para o símbolo
manglado, para qualquer tipo com uma ilha de tipo no AST — não só
instância genérica** (`tools/cgen/src/engine/emit/uses.c:510-517`):

```c
/* The symbol of a type spelled `spelled` ("buffer i32"): the one a type island
 * with that written name resolved to. */
bool k_em_type_symbol(const KAst *ast, const char *spelled, size_t len, const char **sym, size_t *sym_len) {
    for (size_t i = 0; i < ast->islands.len; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        Detail d;
        if (is->kind != K_ISLAND_TYPE || !detail_of(ast, is, &d)) continue;
        if (d.left_len == len && memcmp(d.left, spelled, len) == 0) { *sym = d.sym; *sym_len = d.sym_len; return true; }
    }
    return false;
}
```

Já é como `analyze_over` resolve `o->sym` para o `foreach` de dois binders
(`foreach.c:276`), a partir do mesmo texto entre parênteses do detalhe.

**3. `verb_symbol` já resolve um verbo pelo nome, tanto para instância
genérica quanto para função comum de um tipo não genérico — é como `walk`
chama `begin`/`has_next`/`next`** (`tools/cgen/src/engine/emit/foreach.c:522-544`):

```c
static bool verb_symbol(Emit *e, const char *sym, size_t sym_len, const char *verb, char *out, size_t cap, size_t *len) {
    const KAst *ast = e->ast;
    size_t vl = strlen(verb);
    for (size_t c = 0; c < ast->closure.len; c++) {
        const KClosure *k = keel_buffer_KClosure_ptr(&ast->closure, c);
        if (k->kind != K_CLOSURE_INSTANCE || k->symbol_len != sym_len || memcmp(k->symbol, sym, sym_len) != 0) continue;
        if (sym_len + 1 + vl > cap) return false;
        memcpy(out, sym, sym_len); out[sym_len] = '_'; memcpy(out + sym_len + 1, verb, vl);
        *len = sym_len + 1 + vl;
        return true;
    }
    for (size_t n = 0; n < ast->nodes.len; n++) {
        const KAstNode *d = keel_buffer_KAstNode_ptr(&ast->nodes, n);
        if (d->kind != K_AST_FUNCTION || d->name_first >= d->name_end) continue;
        KToken t = k_em_tok(ast, d->name_first);
        if (t.len != vl || memcmp(t.ptr, verb, vl) != 0) continue;
        size_t q = k_mangle_decl(e->module, (keel_slice_char){ vl, (char *)verb }, false, (keel_slice_char){ cap, out });
        if (q > cap) return false;
        *len = q;
        return true;
    }
    return false;
}
```

Chamado em `edits_walk` (`foreach.c:611-613`):

```c
if (!verb_symbol(e, o->sym, o->sym_len, "begin", vb[0], NAME_CAP, &vl3[0]) ||
    !verb_symbol(e, o->sym, o->sym_len, "has_next", vb[1], NAME_CAP, &vl3[1]) ||
    !verb_symbol(e, o->sym, o->sym_len, "next", vb[2], NAME_CAP, &vl3[2])) { e->ok = false; return; }
```

Para esta tarefa, chame igual, com `"first"` e `"limit"`.

**4. A decisão `&`/direto já existe, igual, em dois lugares** —
`edits_walk` (`foreach.c:614-618`):

```c
k_em_put_bytes(e, o->sym, o->sym_len);
/* a container that is a value (`slice`) is copied, and its verbs take the copy */
put_s(e, o->value_copy ? " keel__c" : " *keel__c"); put_uint(e, n);
put_s(e, " = ");
if (!o->by_pointer && !o->value_copy) put_s(e, "&");
k_em_put_range(e, o->cont, o->cont + 1);
```

e o próprio `analyze_over`, que descobre `by_pointer` varrendo a declaração
do contêiner entre `fn->first` e `kw` (`foreach.c:279-293`):

```c
KToken c = k_em_tok(ast, o->cont);
bool found = false;
for (size_t i = fn->first + 1; i < kw && !found; i++) {
    KToken t = k_em_tok(ast, i);
    if (t.len != c.len || memcmp(t.ptr, c.ptr, c.len) != 0) continue;
    if (word(ast, i - 1, "*")) { o->by_pointer = true; found = true; }
    else if (word_like(ast, i - 1) && (word(ast, i + 1, "=") || word(ast, i + 1, ";") || word(ast, i + 1, ",") || word(ast, i + 1, ")") ||
                                      word(ast, i + 1, ":"))) {   /* the last, a `parallel` binder */
        o->by_pointer = false; found = true;
    }
}
```

**Esta tarefa não precisa de `value_copy`** (o protocolo `Countable`
declara `first`/`limit` tomando `C *x`, spec §5.1 — a forma canônica por
ponteiro; só `range`, já tratado pelo caminho antigo, intocado, declara por
valor). Reaproveite só a parte do `&`/direto: quando o contêiner **não** é
`by_pointer`, o argumento é `&<cont>`; quando é, `<cont>` direto — igual à
condição `!o->by_pointer && !o->value_copy` acima, com `value_copy` sempre
falso aqui.

## O oráculo

`golden/cases/064-foreach-countable/` (escrito e já verificado por mim:
`golden/run.sh` passa, 128 ok, 0 fail, incluindo `layout-check` e
`line-check`). Leia `NOTES` primeiro.

`win.k` declara `window` (um `typedef struct` comum, nem genérico nem
`byref`) com `first(window *w)`/`limit(window *w)`, e duas funções:
`sum_param` (contêiner já por ponteiro, parâmetro `window *w`) e
`sum_local` (contêiner local por valor, `window w = {...}`) — as duas
formas de `by_pointer` que a regra 4 acima decide.

O C esperado (`expected/c23/win.c`, trecho):

```c
size_t win_sum_param(win_window *w) {
    size_t total = 0;
    { size_t keel__f0 = win_first(w); size_t keel__l0 = win_limit(w); for (size_t i = keel__f0; i < keel__l0; i++) {
        total += i;
    } }
    return total;
}

size_t win_sum_local(void) {
    win_window w = { 2, 5 };
    size_t total = 0;
    { size_t keel__f0 = win_first(&w); size_t keel__l0 = win_limit(&w); for (size_t i = keel__f0; i < keel__l0; i++) {
        total += i;
    } }
    return total;
}
```

Note: **sem** temporário para o próprio contêiner (`keel__c<N>`) — `w`/`&w`
é escrito direto nas duas chamadas, igual ao exemplo da própria regra 6 do
backend §5.7 para `range` nomeado (`keel_range_first(indices);
keel_range_limit(indices);`). Só `first`/`limit` ganham temporário
(`keel__f<N>`/`keel__l<N>`), exatamente como em `edits_range_var`.

## Arquivos

Pode editar:
- `tools/cgen/src/engine/emit/foreach.c`
- `test/cgen/gen/golden-cases.txt` — **só a linha do caso 064**, para
  remover o `|wip` depois que o caso passar a bater byte a byte.

Não edita, não move, não apaga:
- `golden/cases/064-foreach-countable/**` (o oráculo, escrito à mão por
  mim).
- qualquer outro caso de `golden/cases/`.
- qualquer arquivo de `tools/cgen/src/engine/parser/` (ver o aviso no
  topo).
- `keel-spec.md`, `keel-rationale.md`, `keel-c-backend.md`,
  `cgen-tool-spec.md`.

## Contrato

Depois do conserto: `foreach (auto i : x)` sobre uma variável `x` de um tipo
que declara `first(T *)`/`limit(T *)` e não é `range` deve emitir
`{ size_t keel__f<N> = <mangled>_first(<arg>); size_t keel__l<N> =
<mangled>_limit(<arg>); for (size_t i = keel__f<N>; i < keel__l<N>; i++)
body }`, com `<arg>` igual a `&x` quando `x` é declarado por valor, e `x`
direto quando `x` já é ponteiro. `auto` vira `size_t` (regra já existente,
igual à de `edits_range_var`). Nenhum golden existente pode mudar de saída
— em especial, nenhum caso com `range` nomeado.

## Passos sugeridos

1. Leia `golden/cases/064-foreach-countable/NOTES`, `win.k` e os três
   arquivos de `expected/c23/`.
2. Releia as quatro peças citadas acima (`analyze_range_var`,
   `k_em_type_symbol`, `verb_symbol`, o laço de `by_pointer` em
   `analyze_over`) até entender como cada uma encaixa.
3. Escreva uma struct nova (ex.: `Countable`) e um par
   `analyze_countable_var`/`edits_countable_var`, no molde de
   `RangeVar`/`analyze_range_var`/`edits_range_var`:
   - mesmo reconhecimento de cabeçalho de `analyze_range_var` (`foreach (
     <tipo> <binder> : <cont> )`, um binder);
   - em vez de checar `"(range)"` fixo, extraia o texto entre parênteses
     (igual a `analyze_over`, `foreach.c:273-276`) e resolva com
     `k_em_type_symbol`; se não resolver, ou se o texto for exatamente
     `"range"`, devolva `false` (deixa para `analyze_range_var`, que já
     corre antes no despacho — ver passo 5);
     resolva `by_pointer` com o mesmo laço de declaração de
     `analyze_over` (`foreach.c:279-293`), adaptado ao único binder.
   - na emissão, troque `keel_range_first`/`keel_range_limit` fixos por
     `verb_symbol(e, o->sym, o->sym_len, "first"/"limit", ...)`, e o
     `*r->cont`/`r->cont` de `edits_range_var` pela decisão `&`/direto da
     regra 4 acima (sem `value_copy`).
4. Em `k_em_edits_foreach` (`foreach.c:752`), chame o par novo depois de
   `analyze_range_var` e antes da forma literal (`Foreach f`), na mesma
   forma dos dois `if` já ali (linhas 762-764).
5. Construa (`make -C tools/cgen -j8`) e teste manualmente antes da suíte
   inteira:
   ```sh
   ./tools/cgen/cgen -std=c23 --base-dir "$PWD/base" --stop-after=gen \
       -I golden/cases/064-foreach-countable --dest-dir /tmp/t41out \
       golden/cases/064-foreach-countable/win.k
   diff golden/cases/064-foreach-countable/expected/c23/win.c /tmp/t41out/win.c
   ```
6. Se, depois de tentar, a conclusão for que isso exige mudar a ilha de
   `foreach` no parser, **pare**: registre no relatório o que tentou, por
   que não bastou ficar só no emissor, e deixe o código no último estado
   que passa `sh tools/agents/verify.sh`.

## Aceitação

```sh
sh tools/agents/verify.sh
```
deve terminar em `ALL OK`.

Além disso, confira que o caso 064 virou `ok` de verdade:

```sh
sh test/cgen/gen_golden.sh 2>&1 | grep -E '064-foreach-countable|^[0-9]+ ok'
```
deve mostrar o caso **sem** a palavra `wip` ao lado (hoje mostra `wip
064-foreach-countable`), e a última linha deve ter uma unidade mais em `ok`
do que tinha antes (73 → 74) e **zero** em `wip` para este caso. Só depois
disso, tire o `|wip` da linha do 064 em `test/cgen/gen/golden-cases.txt` e
rode de novo para confirmar que continua `ok` sem o flag.

Por fim:
```sh
sh test/run.sh
```
tem que terminar com código 0 e sem nenhum `FAIL`.

## Relatório

Escreva `tools/agents/reports/T41-foreach-countable.md`, em pt-BR:
- o que mudou, arquivo por arquivo, com o trecho relevante;
- por que a solução não precisa de nenhum valor fixo (nem `"first"`/
  `"limit"` manglados à mão, nem a suposição de que o contêiner é sempre
  `window`) — aponte onde `k_em_type_symbol`/`verb_symbol` fazem essa
  resolução de verdade;
- a saída exata de `sh tools/agents/verify.sh`, de
  `sh test/cgen/gen_golden.sh` (a linha do 064 e o total) e de
  `sh test/run.sh`;
- se parou no parser (passo 6): o que tentou e por que concluiu que não
  bastava ficar no emissor — com evidência, não só a afirmação.
