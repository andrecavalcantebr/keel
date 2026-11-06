<!--
   FILE: T40-foreach-ptr-arity.md
   DESC: relatório do conserto do sufixo de aridade em foreach de binder por
         ponteiro (T40)
   AUTHOR: Claude Sonnet 5 (orquestrador, revisão e fechamento)
   DATE: Nov, 2026
-->

# T40 — relatório

## O que mudou

**`tools/cgen/src/engine/emit/roles.c`**: a busca de aridades duplicadas que
já existia dentro de `k_em_arity_suffix` foi extraída para uma função própria,
genérica em qualquer AST e qualquer nome:

```c
bool k_em_name_overloaded(const KAst *ast, const char *name, size_t len) {
    bool seen = false;
    size_t first = 0;
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *o = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (!has_params(ast, o)) continue;
        KToken t = k_em_tok(ast, o->name_first);
        if (t.len != len || memcmp(t.ptr, name, len) != 0) continue;
        size_t c = param_count(ast, o);
        if (!seen) { first = c; seen = true; }
        else if (c != first) return true;
    }
    return false;
}
```

`k_em_arity_suffix` passou a chamar essa função em vez de repetir a busca
inline — sem mudança de comportamento (ela ainda ignora `fn` ao comparar
contra o próprio nó, porque ao olhar o nó atual `c == mine` e isso não conta
como segunda aridade distinta; a nova função generaliza isso ao mesmo
resultado, verificado por `test/run.sh` idêntico antes e depois).

Declarada em `tools/cgen/src/engine/emit/emit_internal.h`.

**`tools/cgen/src/engine/emit/foreach.c`**: `container_value()` escrevia o
literal fixo `"_ptr1(keel__c"` para todo binder por ponteiro. Agora o sufixo
vem de `o->ptr_overloaded`, calculado no AST que **declara o modificador do
contêiner** — não no AST que está sendo emitido:

```c
if (o->ptr_binder) {
    k_em_put_bytes(e, "_ptr", 4);
    if (o->ptr_overloaded) k_em_put_bytes(e, "1", 1);
    k_em_put_bytes(e, "(keel__c", 8);
} else k_em_put_bytes(e, "_get(keel__c", 12);
```

Duas novas funções locais calculam `o->ptr_overloaded`:

- `written_name()` lê o nome do modificador como escrito no `.k` (`vec`, ou
  qualificado `fv.vec`), a partir do token do tipo da declaração do binder;
- `ptr_overloaded_of()` resolve esse nome em `ast->symbols`, confirma que é
  um `K_SYM_MODIFIER` com `origin->ast` (o AST do módulo que o declara), e
  chama `k_em_name_overloaded(s->origin->ast, "ptr", 3)` nesse AST — nunca no
  AST local.

Essa resolução roda nos dois lugares que descobrem `o->ptr_binder`:
`analyze_call_over` (contêiner vindo de uma chamada que devolve ponteiro) e
`analyze_over` (contêiner declarado localmente). Em ambos, se o nome não
resolve a um modificador conhecido (`known == false`), a análise do
`foreach` falha (`return false`) em vez de arriscar um palpite — mantém a
disciplina de recusar o que não se sabe decidir, em vez de inventar um
`else` que nunca foi exercitado.

## Por que isto não é um valor fixo disfarçado

O valor fixo antigo (`"_ptr1"`) foi substituído por uma pergunta ao AST real:
"o módulo que declara este modificador declara `ptr` em mais de uma
aridade?" — a mesma pergunta que `isl_find_verb`/`Sig.overloaded` já fazem no
parser para `x.ptr`/`x[i]`, e que `k_em_arity_suffix` já fazia, mas restrita
ao módulo local. Não há nenhum caminho de código que devolva `1` sem antes
consultar `ast->symbols` e o AST de origem do modificador; o caso que não
resolve (`known == false`) recusa a análise, não cai num padrão.

A prova empírica: com o módulo `fv.k` do oráculo (`ptr` numa aridade só), o
cgen agora emite `fv_vec_i32_ptr(keel__c0, i)`, sem sufixo; com
`keel.buffer`/`keel.slice` (duas aridades), continua emitindo
`..._ptr1(keel__c0, i)`, idêntico a antes — nenhum golden existente mudou de
saída (confirmado por `test/run.sh` sem nenhuma diferença fora do caso 063).

## Um achado fora do escopo do cartão, corrigido por mim

Depois do conserto em `foreach.c`, `fv_use.c` (o arquivo que o `foreach`
lowera) já batia byte a byte nos dois perfis. O único arquivo que ainda
divergia era `fv_vec_i32.h` — faltava `[[nodiscard]]` na declaração e na
definição de `ptr`:

```diff
-static inline i32 *fv_vec_i32_ptr(fv_vec_i32 *v, size_t i);
+[[nodiscard]] static inline i32 *fv_vec_i32_ptr(fv_vec_i32 *v, size_t i);
```

Essa marca não vem de `foreach.c`: vem de `k_em_nodiscard` (`roles.c`,
commit `5ad9800`), regra já existente desde M3 — "`[[nodiscard]]` on a
function that returns a `child` pointer" (backend §5.2, rule 5; C23 only).
`fv.k` declara `pub inline child T *ptr(parent vec *v, size_t i)`, então a
regra se aplica, e sempre se aplicou — é independente da aridade. O perfil
c11 do mesmo oráculo já batia sem diferença (`[[nodiscard]]` é só C23), o
que confirma que a regra está certa e o que faltava era no **oráculo**
escrito à mão, não na emissão.

O cartão (corretamente) proibiu o agente Haiku de tocar em
`golden/cases/063-foreach-ptr-arity/**`. A correção dessas duas linhas em
`expected/c23/fv_vec_i32.h` foi feita por mim (orquestrador), que escrevi o
oráculo originalmente e deixei essa marca de fora por descuido.

## Saída dos oráculos

```
$ sh tools/agents/verify.sh
ok   build (0 warnings, baseline 0)
ok   make check (54 lines ok)
ok   golden: 126 ok, 0 fail, 0 xfail, 0 xpass, 0 wip
ALL OK
```

```
$ sh test/cgen/gen_golden.sh
73 ok, 0 fail, 0 wip
```

(72 → 73, como o cartão previa; nenhum `wip` para o caso 063, e nenhum outro
caso mudou de estado.)

```
$ sh test/run.sh
...
ok     063-foreach-ptr-arity    c23  (run)
ok     063-foreach-ptr-arity    c11  (run)
...
126 ok, 0 fail, 0 xfail, 0 xpass, 0 wip
docs: 45 ok, 0 fail
```

código de saída 0, sem nenhum `FAIL`.

## Não parou no parser

A investigação do agente não precisou tocar
`tools/cgen/src/engine/parser/`: o nome escrito do modificador já estava
alcançável a partir dos tokens do binder (`written_name`), e a resolução em
`ast->symbols` + `K_SYM_MODIFIER.origin->ast` seguiu o padrão já usado em
`uses.c`/`match.c`/`routine.c`. Nenhum arquivo de parser foi editado.
