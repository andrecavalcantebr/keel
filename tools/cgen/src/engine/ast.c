#include <stdint.h>
#include "engine/ast.h"

static bool named(const KAst *a, size_t i, const char *s) {
    return i < a->token_count && k_token_spelled(a->tokens[i].token, s);
}

static bool punct(const KAst *a, size_t i, const char *s) {
    return i < a->token_count && k_token_is_punct(a->tokens[i].token, s);
}

size_t k_lexemes(keel_slice_char source, KLexeme *out, size_t cap,
                 KDiagnosticSink *diagnostics) {
    KLexer lexer;
    k_lexer_init(&lexer, source, diagnostics);
    size_t n = 0;
    for (;;) {
        TKPpKind pp;
        KToken token = k_lexer_next(&lexer, &pp);
        if (token.len == 0) break;
        if (n < cap) out[n] = (KLexeme){token, pp, lexer.directive};
        n++;
    }
    return n;
}

static size_t qualified_end(const KAst *a, size_t i) {
    size_t j = i + 1;
    while (j + 1 < a->token_count && punct(a, j, ".")) j += 2;
    return j;
}

static size_t balanced(const KAst *a, size_t i, const char *open, const char *close) {
    size_t depth = 0;
    for (size_t j = i; j < a->token_count; j++) {
        if (punct(a, j, open)) depth++;
        if (punct(a, j, close) && --depth == 0) return j + 1;
    }
    return a->token_count;
}

/* Stop at a file-level semicolon or after a braced definition. */
static size_t declaration_end(const KAst *a, size_t i, size_t *body_first,
                              size_t *body_end) {
    size_t paren = 0, bracket = 0;
    bool initializer = false;
    *body_first = *body_end = SIZE_MAX;
    for (size_t j = i; j < a->token_count; j++) {
        if (punct(a, j, "(")) paren++;
        else if (punct(a, j, ")") && paren) paren--;
        else if (punct(a, j, "[")) bracket++;
        else if (punct(a, j, "]") && bracket) bracket--;
        else if (!paren && !bracket && punct(a, j, "=")) initializer = true;
        else if (!paren && !bracket && punct(a, j, "{")) {
            size_t after = balanced(a, j, "{", "}");
            *body_first = j + 1;
            *body_end = after > j ? after - 1 : j;
            if (named(a, i, "typedef") || initializer) {
                j = after - 1;
                continue;
            }
            return punct(a, after, ";") ? after + 1 : after;
        } else if (!paren && !bracket && punct(a, j, ";")) return j + 1;
    }
    return a->token_count;
}

static bool append(KAst *a, size_t cap, KAstNode n) {
    if (a->node_count >= cap) return false;
    a->nodes[a->node_count++] = n;
    return true;
}

bool k_parse_ast(KAst *a, KAstNode *nodes, size_t cap) {
    a->nodes = nodes;
    a->node_count = 0;
    a->module = 0;
    size_t i = 0, n = a->token_count;
    while (i < n && a->tokens[i].directive) i++;
    if (i >= n || !named(a, i, "module")) return false;

    KAstNode mod = { .kind = K_AST_MODULE, .first = i, .anchor = i,
                     .name_first = i + 1, .alias = SIZE_MAX,
                     .body_first = SIZE_MAX, .body_end = SIZE_MAX };
    mod.name_end = qualified_end(a, mod.name_first);
    size_t j = mod.name_end;
    while (j < n && !punct(a, j, ";")) j++;
    if (j == n) return false;
    mod.dim_first = mod.dim_end = mod.name_end;
    mod.tags_first = mod.tags_end = mod.name_end;
    mod.type_first = mod.type_end = mod.name_end;
    for (size_t k = mod.name_end; k < j; k++) {
        size_t *first = NULL, *end = NULL;
        if (named(a, k, "dim")) { first = &mod.dim_first; end = &mod.dim_end; }
        else if (named(a, k, "tags")) { first = &mod.tags_first; end = &mod.tags_end; }
        else if (named(a, k, "type")) { first = &mod.type_first; end = &mod.type_end; }
        if (first) {
            *first = k + 1;
            size_t limit = k + 1;
            while (limit < j && !named(a, limit, "dim") &&
                   !named(a, limit, "tags") && !named(a, limit, "type")) limit++;
            *end = limit;
            k = limit - 1;
        }
    }
    mod.end = j + 1;
    if (!append(a, cap, mod)) return false;
    i = mod.end;

    while (i < n) {
        if (a->tokens[i].directive) { i++; continue; }
        size_t start = i;
        KAstNode node = { .first = start, .anchor = start, .alias = SIZE_MAX,
                          .body_first = SIZE_MAX, .body_end = SIZE_MAX,
                          .is_public = true };
        if (named(a, i, "pub") || named(a, i, "priv")) {
            node.is_public = named(a, i, "pub");
            i++;
        }
        if (named(a, i, "inline")) { node.is_inline = true; i++; }
        if (i >= n) return false;
        node.anchor = start;

        if (named(a, i, "import") || named(a, i, "import_c")) {
            bool c = named(a, i, "import_c");
            node.kind = c ? K_AST_IMPORT_C : K_AST_IMPORT;
            node.name_first = i + 1;
            node.name_end = c ? node.name_first : qualified_end(a, node.name_first);
            j = node.name_end;
            while (j < n && !punct(a, j, ";")) {
                if (!c && named(a, j, "as") && j + 1 < n) node.alias = j + 1;
                if (!c && named(a, j, "types")) node.has_types = true;
                if (c) node.name_end = j + 1;
                j++;
            }
            if (j == n) return false;
            node.end = j + 1;
        } else {
            node.end = declaration_end(a, i, &node.body_first, &node.body_end);
            if (node.end <= start || node.end > n) return false;
            node.kind = K_AST_OPAQUE;
            if (named(a, i, "extern_c")) node.kind = K_AST_EXTERN_C;
            else if (named(a, i, "modifier")) {
                node.kind = K_AST_MODIFIER;
                node.name_first = i + 1; node.name_end = i + 2;
            } else if (named(a, i, "tags")) {
                node.kind = K_AST_TAGS;
                node.name_first = i + 1; node.name_end = i + 2;
            } else if (named(a, i, "typedef")) {
                node.kind = K_AST_TYPE;
                for (j = node.end; j > i + 1; j--)
                    if (k_token_is_ident(a->tokens[j - 1].token)) {
                        node.name_first = j - 1; node.name_end = j; break;
                    }
            } else if (named(a, i, "struct") || named(a, i, "union")) {
                if (i + 1 < n && k_token_is_ident(a->tokens[i + 1].token)) {
                    node.kind = K_AST_TYPE;
                    node.name_first = i + 1; node.name_end = i + 2;
                }
            } else if (named(a, i, "constexpr")) {
                node.kind = K_AST_CONSTEXPR;
                size_t depth = 0;
                for (j = i + 1; j < node.end; j++) {
                    if (punct(a, j, "(") || punct(a, j, "[")) depth++;
                    else if ((punct(a, j, ")") || punct(a, j, "]")) && depth) depth--;
                    else if (!depth && punct(a, j, "=") && j > i + 1) {
                        node.name_first = j - 1; node.name_end = j; break;
                    }
                }
            } else {
                /* A final top-level parameter group preceded by IDENT is a
                   function declarator, provided no top-level '=' precedes it. */
                size_t paren = 0, bracket = 0;
                bool assigned = false;
                for (j = i; j < node.end; j++) {
                    if (!paren && !bracket && punct(a, j, "=")) assigned = true;
                    if (!paren && !bracket && punct(a, j, "(") && j > i &&
                        !assigned && k_token_is_ident(a->tokens[j - 1].token)) {
                        size_t after = balanced(a, j, "(", ")");
                        if (after < node.end &&
                            (punct(a, after, "{") || punct(a, after, ";"))) {
                            node.kind = K_AST_FUNCTION;
                            node.name_first = j - 1; node.name_end = j;
                            break;
                        }
                    }
                    if (punct(a, j, "(")) paren++;
                    else if (punct(a, j, ")") && paren) paren--;
                    else if (punct(a, j, "[")) bracket++;
                    else if (punct(a, j, "]") && bracket) bracket--;
                }
                if (node.kind == K_AST_OPAQUE && node.end > i + 1) {
                    /* File-level variables are resolved more precisely by the
                       symbol pass; this captures the simple declarator form. */
                    for (j = i + 1; j + 1 < node.end; j++)
                        if (k_token_is_ident(a->tokens[j].token) &&
                            (punct(a, j + 1, ";") || punct(a, j + 1, "="))) {
                            node.kind = K_AST_VARIABLE;
                            node.name_first = j; node.name_end = j + 1; break;
                        }
                }
            }
        }
        if (!append(a, cap, node)) return false;
        i = node.end;
    }
    return true;
}
