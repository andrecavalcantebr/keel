/* Oracle for k_resolve_imports (parser-design §3, passage 1). Expected
 * values are hand-derived from keel-spec §4.1 and the loader contract of
 * cgen-tool §3.1, not from running a solving implementation.
 * Not model-generated.
 *
 * The loader here is the fake the design asks for: "um teste entrega
 * bytes e um KLoader de mentira que devolve módulos de um vetor em
 * memória — nenhum diretório temporário, nenhum mkstemp, nenhuma
 * limpeza." No file is opened anywhere in this test.
 */
#include <stdio.h>
#include <string.h>
#include "engine/ast.h"
#include "engine/parser.h"

static int failures = 0;
#define S(lit) ((keel_slice_char){ sizeof(lit) - 1, (char *)(lit) })

/* ---- the fake loader ------------------------------------------------ */

/* `keel` carries the prelude's primitives; `coll` a modifier of arity 1
   plus a function, to prove `types` injects the first and not the
   second (keel-spec §4.1 item 6). `spin` is the module the tool
   reports as pending, i.e. a cycle. */
static KSymbol keel_syms[] = {
    { { 3, (char *)"i32" }, K_SYM_TYPE, 0 },
    { { 3, (char *)"f64" }, K_SYM_TYPE, 0 },
};
static KSymbol coll_syms[] = {
    { { 5, (char *)"stack" }, K_SYM_MODIFIER, 1 },
    { { 4, (char *)"Node" },  K_SYM_TYPE, 0 },
    { { 4, (char *)"push" },  K_SYM_FUNCTION, 2 },
    { { 5, (char *)"COUNT" }, K_SYM_CONSTANT, 0 },
};
static KModule mod_keel = { { 4, (char *)"keel" }, keel_syms, 2, 0 };
static KModule mod_coll = { { 4, (char *)"coll" }, coll_syms, 4, 0 };

typedef struct { int calls; } FakeTool;

static KLoadResult fake_load(void *tool, keel_slice_char name, KModule **out) {
    ((FakeTool *)tool)->calls++;
    if (k_symtab_same_name(name, S("keel"))) { *out = &mod_keel; return K_LOAD_OK; }
    if (k_symtab_same_name(name, S("coll"))) { *out = &mod_coll; return K_LOAD_OK; }
    if (k_symtab_same_name(name, S("spin"))) { *out = NULL; return K_LOAD_CYCLE; }
    *out = NULL;
    return K_LOAD_NOT_FOUND;
}

/* ---- the harness ---------------------------------------------------- */

/* `want` lists the symbols the table must end with, in order, as
   "name:kind:arity", joined by '|'. Kinds are the enum's own numbers. */
static void run(const char *label, const char *src, bool want_ok,
                const char *want, size_t want_diags) {
    keel_slice_char source = { strlen(src), (char *)src };
    KLexeme tokens[256];
    KDiagnostic diags[8];
    KDiagnosticSink sink;
    k_diag_init(&sink, diags, 8);
    size_t n = k_lexemes(source, tokens, 256, &sink);
    if (n > 256) { fprintf(stderr, "FAIL: %s — needs %zu tokens\n", label, n); failures++; return; }

    KAstNode nodes[64];
    KAst ast = {.source=source, .tokens=keel_buffer_KLexeme_of(tokens,n),
                .nodes=keel_buffer_KAstNode_from(nodes,64)};
    if (!k_parse_ast(&ast)) {
        fprintf(stderr, "FAIL: %s — k_parse_ast returned false\n", label);
        failures++;
        return;
    }

    KSymbol storage[32];
    KSymbolTable symtab;
    k_symtab_init(&symtab, storage, 32);
    FakeTool tool = { 0 };
    KLoader loader = { fake_load, &tool };
    k_diag_init(&sink, diags, 8);

    bool ok = k_resolve_imports(&ast, &loader, &symtab, &sink);
    if (ok != want_ok) {
        fprintf(stderr, "FAIL: %s — returned %d, want %d\n", label, (int)ok, (int)want_ok);
        failures++;
    }

    char got[400];
    size_t at = 0;
    for (size_t i = 0; i < symtab.len; i++) {
        if (i) got[at++] = '|';
        memcpy(got + at, storage[i].name.ptr, storage[i].name.len);
        at += storage[i].name.len;
        at += (size_t)snprintf(got + at, sizeof got - at, ":%d:%d",
                               (int)storage[i].kind, storage[i].arity);
    }
    got[at] = '\0';
    if (strcmp(got, want) != 0) {
        fprintf(stderr, "FAIL: %s — symtab \"%s\", want \"%s\"\n", label, got, want);
        failures++;
    }
    if (sink.len != want_diags) {
        fprintf(stderr, "FAIL: %s — %zu diagnostics, want %zu\n", label, sink.len, want_diags);
        failures++;
    }
}

int main(void) {
    /* K_SYM_TYPE = 0, K_SYM_MODIFIER = 1, K_SYM_MODULE = 6 */

    /* no import at all still gets the implicit `import keel types;` */
    run("implicit prelude", "module app;\n", true, "i32:0:0|f64:0:0|keel:6:0", 0);

    /* the prelude itself must not import itself */
    run("keel does not", "module keel;\n", true, "", 0);

    /* `types` injects types and modifiers, with arity, and nothing else:
       `push` (function) and `COUNT` (constant) must not appear */
    run("types injects", "module app;\nimport coll types;\n", true,
        "i32:0:0|f64:0:0|keel:6:0|stack:1:1|Node:0:0|coll:6:0", 0);

    /* the alias is a module symbol; without `types` nothing is injected */
    run("alias only", "module app;\nimport coll as c;\n", true,
        "i32:0:0|f64:0:0|keel:6:0|coll:6:0|c:6:0", 0);

    /* both together, in the order the file writes them */
    run("alias and types", "module app;\nimport coll as c types;\n", true,
        "i32:0:0|f64:0:0|keel:6:0|stack:1:1|Node:0:0|coll:6:0|c:6:0", 0);

    /* A module no root has is diagnosed here, at the import site. */
    run("not found", "module app;\nimport nope types;\n", false,
        "i32:0:0|f64:0:0|keel:6:0", 1);

    /* pending in the tool: this half knows the position, so it reports */
    run("cycle", "module app;\nimport spin;\n", false, "i32:0:0|f64:0:0|keel:6:0", 1);

    /* one bad import does not stop the others */
    run("bad then good", "module app;\nimport nope;\nimport coll types;\n", false,
        "i32:0:0|f64:0:0|keel:6:0|stack:1:1|Node:0:0|coll:6:0", 1);

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
