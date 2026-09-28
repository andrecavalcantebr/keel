/* tool/tool.c — `carrega`/`processa` (spec da ferramenta §3; cgen-tool
   §3.1, §3.3). Esta é a metade do carregador que sabe o que é um
   arquivo. O motor nunca aprende: ele pede um módulo pelo nome, através
   do ponteiro de função do `KLoader`, e recebe símbolos.

   **O estado da carga mora aqui** (cgen-tool §3.1): por módulo, nunca
   visto, pendente, terminado ou falhou. Só pedir um módulo *pendente* é
   ciclo — pedir um terminado é acerto de memoização, ainda que um
   ancestral na pilha continue pendente. */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "keel/keel_buffer_char.type.h"
#include "engine/ast.h"
#include "engine/loader.h"

bool cgen_read_source(const char *path, keel_buffer_char *out);
bool cgen_module_path(const char *module_name, size_t name_len,
                      const char *root, char *out, size_t cap);

#define CGEN_MAX_MODULES  128
#define CGEN_MAX_ROOTS    130   /* main.c admite 128 `-I`, mais `.` e a base */
#define CGEN_MAX_EXPORTS  256
#define CGEN_MAX_NAME     512

typedef enum { CGEN_PENDING, CGEN_DONE, CGEN_FAILED } CgenModState;

typedef struct {
    char          name[CGEN_MAX_NAME];
    size_t        name_len;
    CgenModState  state;
    KModule       module;
    KSymbol       exports[CGEN_MAX_EXPORTS];
} CgenModuleEntry;

typedef struct {
    const char       *roots[CGEN_MAX_ROOTS];   /* na ordem; a base por último */
    int               root_count;
    KDiagnosticSink  *diag;
    CgenModuleEntry  *mods[CGEN_MAX_MODULES];
    int               mod_count;
} CgenTool;

/* Quantos IDENT há num intervalo de tokens — os binders do cabeçalho vêm
   separados por vírgula, e a aridade de todo modificador do módulo é a
   soma dos três (parser-design §4.3). */
static int ident_count(const KAst *ast, size_t first, size_t end) {
    int n = 0;
    for (size_t i = first; i < end && i < ast->token_count; i++)
        if (k_token_is_ident(ast->tokens[i].token)) n++;
    return n;
}

/* Os símbolos que um módulo exporta: os nós `pub` com nome. É o que o
   `types` de um import injeta e o que a qualificação alcança. */
static size_t collect_exports(const KAst *ast, KSymbol *out, size_t cap) {
    const KAstNode *mod = &ast->nodes[ast->module];
    int arity = ident_count(ast, mod->dim_first, mod->dim_end)
              + ident_count(ast, mod->tags_first, mod->tags_end)
              + ident_count(ast, mod->type_first, mod->type_end);

    size_t n = 0;
    for (size_t i = 0; i < ast->node_count && n < cap; i++) {
        const KAstNode *node = &ast->nodes[i];
        if (!node->is_public) continue;
        if (node->name_first == node->name_end) continue;
        KSymKind kind;
        int sym_arity = 0;
        switch (node->kind) {
            case K_AST_TYPE:      kind = K_SYM_TYPE; break;
            case K_AST_MODIFIER:  kind = K_SYM_MODIFIER; sym_arity = arity; break;
            case K_AST_TAGS:      kind = K_SYM_TAGS; break;
            case K_AST_CONSTEXPR: kind = K_SYM_CONSTANT; break;
            case K_AST_FUNCTION:  kind = K_SYM_FUNCTION; break;
            case K_AST_VARIABLE:  kind = K_SYM_VARIABLE; break;
            default: continue;
        }
        KToken lead = ast->tokens[node->name_first].token;
        KToken last = ast->tokens[node->name_end - 1].token;
        out[n++] = (KSymbol){
            .name = { (size_t)((last.ptr + last.len) - lead.ptr), lead.ptr },
            .kind = kind, .arity = sym_arity
        };
    }
    return n;
}

static CgenModuleEntry *find(CgenTool *t, keel_slice_char name) {
    for (int i = 0; i < t->mod_count; i++)
        if (t->mods[i]->name_len == name.len &&
            memcmp(t->mods[i]->name, name.ptr, name.len) == 0)
            return t->mods[i];
    return NULL;
}

/* O primeiro achado vence, e a base é a última raiz (§3.3 item 6). */
static bool locate(CgenTool *t, keel_slice_char name, char *out, size_t cap,
                   long long *mtime_out) {
    for (int i = 0; i < t->root_count; i++) {
        if (!cgen_module_path(name.ptr, name.len, t->roots[i], out, cap)) continue;
        struct stat st;
        if (stat(out, &st) == 0 && S_ISREG(st.st_mode)) {
            *mtime_out = (long long)st.st_mtime;
            return true;
        }
    }
    return false;
}

KLoadResult cgen_load(void *tool_v, keel_slice_char module_name, KModule **out) {
    CgenTool *t = (CgenTool *)tool_v;
    *out = NULL;
    if (module_name.len == 0 || module_name.len >= CGEN_MAX_NAME) return K_LOAD_ERROR;

    CgenModuleEntry *entry = find(t, module_name);
    if (entry != NULL) {
        switch (entry->state) {
            /* o único caso de ciclo: a carga dele está em andamento num
               nível acima desta pilha */
            case CGEN_PENDING: return K_LOAD_CYCLE;
            case CGEN_DONE:    *out = &entry->module; return K_LOAD_ALREADY;
            default:           return K_LOAD_ERROR;
        }
    }

    char path[4096];
    long long mtime = 0;
    if (!locate(t, module_name, path, sizeof path, &mtime)) return K_LOAD_NOT_FOUND;
    if (t->mod_count >= CGEN_MAX_MODULES) return K_LOAD_ERROR;

    entry = calloc(1, sizeof *entry);
    if (entry == NULL) return K_LOAD_ERROR;
    memcpy(entry->name, module_name.ptr, module_name.len);
    entry->name_len = module_name.len;
    entry->state = CGEN_PENDING;          /* pendente a partir daqui */
    t->mods[t->mod_count++] = entry;

    keel_buffer_char source;
    if (!cgen_read_source(path, &source)) { entry->state = CGEN_FAILED; return K_LOAD_ERROR; }
    keel_slice_char text = { source.len, source.ptr };

    /* o fonte nunca é liberado: as fatias dos símbolos apontam para ele
       (cgen-tool §3.2, "nada é liberado antes do fim do processo") */
    size_t count = k_lexemes(text, NULL, 0, t->diag);
    if (count > SIZE_MAX / sizeof(KLexeme) - 1) { entry->state = CGEN_FAILED; return K_LOAD_ERROR; }
    KLexeme *tokens = calloc(count + 1, sizeof *tokens);
    KAstNode *nodes = calloc(count + 1, sizeof *nodes);
    if (tokens == NULL || nodes == NULL) { entry->state = CGEN_FAILED; return K_LOAD_ERROR; }
    k_lexemes(text, tokens, count, t->diag);

    KAst ast = { text, tokens, count, NULL, 0, 0 };
    if (!k_parse_ast(&ast, nodes, count + 1)) { entry->state = CGEN_FAILED; return K_LOAD_ERROR; }

    /* a recursão: os imports deste módulo, pela mesma porta, o que faz a
       pilha de carga crescer e o ciclo aparecer */
    KSymbol *scratch = calloc(CGEN_MAX_EXPORTS, sizeof *scratch);
    if (scratch == NULL) { entry->state = CGEN_FAILED; return K_LOAD_ERROR; }
    KSymbolTable symtab;
    k_symtab_init(&symtab, scratch, CGEN_MAX_EXPORTS);
    KLoader self = { cgen_load, t };
    bool imports_ok = k_resolve_imports(&ast, &self, &symtab, t->diag);

    /* o mtime do fecho é o maior entre o próprio e os dos importados
       (§3.3 item 7) */
    long long closure = mtime;
    for (int i = 0; i < t->mod_count; i++)
        if (t->mods[i]->state == CGEN_DONE && t->mods[i]->module.closure_mtime > closure)
            closure = t->mods[i]->module.closure_mtime;

    entry->module.name = (keel_slice_char){ entry->name_len, entry->name };
    entry->module.symbol_count = collect_exports(&ast, entry->exports, CGEN_MAX_EXPORTS);
    entry->module.symbols = entry->exports;
    entry->module.closure_mtime = closure;

    if (!imports_ok) { entry->state = CGEN_FAILED; return K_LOAD_ERROR; }
    entry->state = CGEN_DONE;             /* terminado: sai da pendência */
    *out = &entry->module;
    return K_LOAD_OK;
}

/* Monta o carregador de uma invocação. `roots` já vem na ordem de busca,
   com a base por último (§2.7). */
void cgen_loader_init(void *tool_v, const char *const *roots, int root_count,
                      KDiagnosticSink *diag, KLoader *out) {
    CgenTool *t = (CgenTool *)tool_v;
    t->root_count = root_count < CGEN_MAX_ROOTS ? root_count : CGEN_MAX_ROOTS;
    for (int i = 0; i < t->root_count; i++) t->roots[i] = roots[i];
    t->diag = diag;
    t->mod_count = 0;
    out->load = cgen_load;
    out->tool = t;
}

size_t cgen_loader_size(void) { return sizeof(CgenTool); }
