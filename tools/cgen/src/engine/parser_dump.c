/* The parse dump is a projection of the retained file-level AST. */
#include "engine/ast.h"
#include "engine/instances.h"

typedef struct { keel_slice_char out; size_t n; } KOut;
static void put_c(KOut *o, char c) {
    if (o->n < o->out.len) o->out.ptr[o->n] = c;
    o->n++;
}
static void put_s(KOut *o, const char *s) { while (*s) put_c(o, *s++); }
static void put_token(KOut *o, KToken t) {
    for (size_t i = 0; i < t.len; i++) put_c(o, t.ptr[i]);
}
static void put_uint(KOut *o, size_t n) {
    char digits[24]; size_t len = 0;
    do { digits[len++] = (char)('0' + n % 10); n /= 10; } while (n);
    while (len) put_c(o, digits[--len]);
}
static void put_name(KOut *o, const KAst *a, size_t first, size_t end) {
    for (size_t i = first; i < end; i++) put_token(o, keel_buffer_KLexeme_ptr(&a->tokens, i)->token);
}
/* a type as written, a space between words, none around `.` or the
   parentheses of a `dim` argument: `blk.blk(4) Color i32` */
static void put_spelled(KOut *o, const KAst *a, size_t first, size_t end) {
    for (size_t j = first; j < end; j++) {
        KToken token = keel_buffer_KLexeme_ptr(&a->tokens, j)->token;
        if (j > first) {
            KToken previous = keel_buffer_KLexeme_ptr(&a->tokens, j - 1)->token;
            bool dot = k_token_is_punct(token, ".") || k_token_is_punct(previous, ".");
            bool bracket = k_token_is_punct(token, "(") || k_token_is_punct(token, ")") ||
                           k_token_is_punct(token, ",") || k_token_is_punct(previous, "(");
            if (!dot && !bracket) put_c(o, ' ');
        }
        put_token(o, token);
    }
}
static void put_symbol(KOut *o, const KAst *a, const KAstNode *node) {
    const KAstNode *module = keel_buffer_KAstNode_ptr(&a->nodes, a->module);
    for (size_t i = module->name_first; i < module->name_end; i++) {
        KToken t = keel_buffer_KLexeme_ptr(&a->tokens, i)->token;
        if (k_token_is_punct(t, ".")) put_c(o, '_');
        else put_token(o, t);
    }
    put_c(o, '_');
    put_name(o, a, node->name_first, node->name_end);
}
static void put_pos(KOut *o, const KAst *a, const char *path, size_t at) {
    KToken token = keel_buffer_KLexeme_ptr(&a->tokens, at)->token;
    size_t off = (size_t)(token.ptr - a->source.ptr);
    size_t line = 1, col = 1;
    for (size_t i = 0; i < off; i++) {
        char c = a->source.ptr[i];
        if (c == '\n' || (c == '\r' && !(i + 1 < a->source.len && a->source.ptr[i + 1] == '\n'))) {
            line++; col = 1;
        } else if (c != '\r') col++;
    }
    if (path && *path) { put_s(o, path); put_c(o, ':'); }
    put_uint(o, line); put_c(o, ':'); put_uint(o, col); put_c(o, '\n');
}
size_t k_dump_ast(const KAst *a, const char *path, keel_slice_char output) {
    KOut o = {output, 0};
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&a->nodes, i);
        switch (n->kind) {
            case K_AST_MODULE:
                put_s(&o, "module\t"); put_name(&o, a, n->name_first, n->name_end);
                break;
            case K_AST_IMPORT:
                put_s(&o, "import\t"); put_name(&o, a, n->name_first, n->name_end);
                if (n->alias != (size_t)-1) {
                    put_s(&o, " as "); put_token(&o, keel_buffer_KLexeme_ptr(&a->tokens, n->alias)->token);
                }
                if (n->has_types) put_s(&o, " types");
                break;
            case K_AST_IMPORT_C:
                put_s(&o, "import_c\t"); put_name(&o, a, n->name_first, n->name_end);
                break;
            case K_AST_INSTANCE: {
                /* names no symbol (spec §4.3, rule 19): the type as written and
                   the instance whose bodies this module holds */
                put_s(&o, "decl\tinstance\t"); put_spelled(&o, a, n->name_first, n->name_end);
                put_c(&o, '\t');
                keel_slice_char text = { (size_t)(keel_buffer_KLexeme_ptr(&a->tokens, n->name_end - 1)->token.ptr +
                                                   keel_buffer_KLexeme_ptr(&a->tokens, n->name_end - 1)->token.len -
                                                   keel_buffer_KLexeme_ptr(&a->tokens, n->name_first)->token.ptr),
                                          (char *)keel_buffer_KLexeme_ptr(&a->tokens, n->name_first)->token.ptr };
                char sym[256];
                size_t len = a->symbols ? k_spec_symbol(text, a->symbols, sym, sizeof sym) : SIZE_MAX;
                if (len == SIZE_MAX) put_c(&o, '-');
                else for (size_t j = 0; j < len; j++) put_c(&o, sym[j]);
                break;
            }
            case K_AST_MODIFIER: case K_AST_TAGS: case K_AST_TYPE:
            case K_AST_CONSTEXPR: case K_AST_FUNCTION: case K_AST_VARIABLE:
            case K_AST_PROTOCOL:
                if (n->name_first == n->name_end) continue;
                put_s(&o, "decl\t"); put_s(&o, n->is_public ? "pub" : "priv");
                if (n->is_inline) put_s(&o, " inline");
                switch (n->kind) {
                    case K_AST_MODIFIER: put_s(&o, " modifier"); break;
                    case K_AST_TAGS: put_s(&o, " tags"); break;
                    case K_AST_TYPE: put_s(&o, " type"); break;
                    case K_AST_CONSTEXPR: put_s(&o, " constexpr"); break;
                    case K_AST_FUNCTION: put_s(&o, " func"); break;
                    case K_AST_PROTOCOL: put_s(&o, " protocol"); break;
                    default: put_s(&o, " var"); break;
                }
                put_c(&o, '\t');
                put_name(&o, a, n->name_first, n->name_end);
                put_c(&o, '\t');
                if (n->kind == K_AST_PROTOCOL) put_c(&o, '-');   /* a protocol emits no C */
                else put_symbol(&o, a, n);
                break;
            default: continue;
        }
        put_c(&o, '\t'); put_pos(&o, a, path, n->anchor);
    }
    for(size_t i=0;i<a->instances.len;i++) {
        const KInstanceUse *use=keel_buffer_KInstanceUse_ptr(&a->instances, i);
        put_s(&o,"inst\t");
        for(size_t j=use->first;j<use->end;j++) {
            KToken token=keel_buffer_KLexeme_ptr(&a->tokens, j)->token;
            if(j>use->first) {
                KToken previous=keel_buffer_KLexeme_ptr(&a->tokens, j-1)->token;
                bool dot=k_token_is_punct(token,".")||k_token_is_punct(previous,".");
                bool bracket=k_token_is_punct(token,"(")||k_token_is_punct(token,")")||k_token_is_punct(token,",")||k_token_is_punct(previous,"(");
                if(!dot&&!bracket)put_c(&o,' ');
            }
            put_token(&o,token);
        }
        put_c(&o,'\t');
        for(size_t j=0;j<use->symbol_len;j++)put_c(&o,use->symbol[j]);
        put_c(&o,'\t');put_pos(&o,a,path,use->first);
    }
    for(size_t i=0;i<a->closure_text.len;i++)put_c(&o,a->closure_text.ptr[i]);
    static const char *const island_names[]={"type","name","call","ref","defer",
        "implicit-init","foreach","match","index","range-index","array","array-index","walk",
        "parallel","worker-exit","else","extent","column"};
    for(size_t i=0;i<a->islands.len;i++) {
        const KIsland *island=keel_buffer_KIsland_ptr(&a->islands, i);
        put_s(&o,"ilha\t");put_s(&o,island_names[island->kind]);put_c(&o,'\t');
        for(size_t j=0;j<island->text_len;j++)put_c(&o,a->island_text.ptr[island->text+j]);
        put_c(&o,'\t');put_pos(&o,a,path,island->anchor);
    }
    return o.n;
}
