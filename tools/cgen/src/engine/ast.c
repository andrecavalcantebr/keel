#include <stdint.h>
#include "engine/ast.h"
#include "engine/parser.h"

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

static size_t balanced(const KAst *a, size_t i, const char *open, const char *close) {
    size_t depth = 0;
    for (size_t j = i; j < a->token_count; j++) {
        if (punct(a, j, open)) depth++;
        if (punct(a, j, close) && --depth == 0) return j + 1;
    }
    return a->token_count;
}

/* Stop at a file-level semicolon or after a braced definition. Only reached
   for function and variable declarations (below), and for a typedef that
   k_scan_decl_typedef turned down — every other kind has a complete
   recognizer in engine/parser.h now, and consumes its own terminator
   (k_scan_decl_keel does not yet handle a function's parameter group —
   parser.h's own note on it: "no trailing suffix ... yet" — so a declarator
   followed by '(' still has to be found this way). */
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

/* Re-enters the token stream at a->tokens[i], through a throwaway lexer, so
   the recognizers of engine/parser.h — each written against a live KLexer,
   one k_lexer_next call at a time — can be called from here without
   re-lexing the file from its start. Diagnostics are NOT re-emitted through
   it (NULL sink): the whole file was already lexed once, with the real
   sink, before k_parse_ast ever runs (tool/stop_parse.c) — this second,
   local lex, over the tail of the same source buffer, would just repeat
   them. Returns a->tokens[i].token itself, read as the recognizers expect
   ("first" already consumed), with `lexer` positioned right after it. */
static KToken reenter(const KAst *a, size_t i, KLexer *lexer, TKPpKind *pp) {
    KToken t = a->tokens[i].token;
    keel_slice_char rest = { (size_t)((a->source.ptr + a->source.len) - t.ptr), t.ptr };
    k_lexer_init(lexer, rest, NULL);
    return k_lexer_next(lexer, pp);
}

/* The index, searching from `from`, of the first token whose spelling
   begins at or after `ptr` — converts a recognizer's keel_slice_char result
   back into the token-index pairs every KAstNode field already uses. Also
   correct for an empty span (k_scan_braced_opaque's empty body): `ptr` then
   names a byte position between two tokens, not the start of any real one,
   and this returns the token right after it — the same index for both ends,
   exactly the empty-range convention this file already used. */
static size_t index_at(const KAst *a, size_t from, const char *ptr) {
    size_t k = from;
    while (k < a->token_count && a->tokens[k].token.ptr < ptr) k++;
    return k;
}

bool k_parse_ast(KAst *a, KAstNode *nodes, size_t cap) {
    a->nodes = nodes;
    a->node_count = 0;
    a->module = 0;
    size_t i = 0, n = a->token_count;
    while (i < n && a->tokens[i].directive) i++;
    if (i >= n || !named(a, i, "module")) return false;

    KLexer lexer;
    TKPpKind pp;
    KToken module_kw = reenter(a, i, &lexer, &pp);
    KModuleHeader header;
    KToken next;
    TKPpKind next_pp;
    k_scan_module_decl(&lexer, module_kw, &header, &next, &next_pp);
    int module_arity = (int)(header.dim_count + header.tag_count + header.type_count);

    KAstNode mod = { .kind = K_AST_MODULE, .first = i, .anchor = i,
                     .alias = SIZE_MAX, .body_first = SIZE_MAX, .body_end = SIZE_MAX };
    mod.name_first = index_at(a, i, header.module_name.ptr);
    mod.name_end = index_at(a, mod.name_first, header.module_name.ptr + header.module_name.len);
    mod.dim_first = mod.dim_end = mod.tags_first = mod.tags_end =
        mod.type_first = mod.type_end = mod.name_end;
    if (header.dim_count > 0) {
        mod.dim_first = index_at(a, mod.name_end, header.dims[0].ptr);
        KToken last = header.dims[header.dim_count - 1];
        mod.dim_end = index_at(a, mod.dim_first, last.ptr + last.len);
    }
    if (header.tag_count > 0) {
        mod.tags_first = index_at(a, mod.name_end, header.tags[0].ptr);
        KToken last = header.tags[header.tag_count - 1];
        mod.tags_end = index_at(a, mod.tags_first, last.ptr + last.len);
    }
    if (header.type_count > 0) {
        mod.type_first = index_at(a, mod.name_end, header.types[0].ptr);
        KToken last = header.types[header.type_count - 1];
        mod.type_end = index_at(a, mod.type_first, last.ptr + last.len);
    }
    mod.end = index_at(a, mod.name_end, next.ptr);
    if (mod.end <= i) return false;
    if (!append(a, cap, mod)) return false;
    i = mod.end;

    /* The declaring module's own symbols (locally-declared types, modifiers,
       tags and constants) — engine/symtab.h: "the caller gives the
       storage", same as `nodes` above. 256 is an arbitrary limit, well past
       any keel module seen so far. */
    KSymbol symtab_storage[256];
    KSymbolTable symtab;
    k_symtab_init(&symtab, symtab_storage, 256);

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
            KToken kw = reenter(a, i, &lexer, &pp);
            if (c) {
                KImportCDecl imp;
                k_scan_import_c(&lexer, kw, &imp, &next, &next_pp);
                node.kind = K_AST_IMPORT_C;
                node.name_first = index_at(a, i, imp.header.ptr);
                node.name_end = index_at(a, node.name_first, imp.header.ptr + imp.header.len);
            } else {
                KImportDecl imp;
                k_scan_import(&lexer, kw, &imp, &next, &next_pp);
                node.kind = K_AST_IMPORT;
                node.name_first = index_at(a, i, imp.module_name.ptr);
                node.name_end = index_at(a, node.name_first, imp.module_name.ptr + imp.module_name.len);
                node.has_types = imp.has_types;
                node.alias = imp.alias.len ? index_at(a, node.name_end, imp.alias.ptr) : SIZE_MAX;
            }
            node.end = index_at(a, node.name_end, next.ptr);
        } else if (named(a, i, "extern_c")) {
            KToken kw = reenter(a, i, &lexer, &pp);
            KExternCDecl ext;
            k_scan_extern_c(&lexer, kw, &ext, &next, &next_pp);
            node.kind = K_AST_EXTERN_C;
            node.body_first = index_at(a, i, ext.body.ptr);
            node.body_end = index_at(a, node.body_first, ext.body.ptr + ext.body.len);
            node.end = index_at(a, node.body_end, next.ptr);
        } else if (named(a, i, "modifier")) {
            KToken kw = reenter(a, i, &lexer, &pp);
            KModifierDecl mdef;
            if (!k_scan_modifier_decl(&lexer, kw, module_arity, &symtab, &mdef, &next, &next_pp))
                return false;
            node.kind = K_AST_MODIFIER;
            node.name_first = index_at(a, i, mdef.name.ptr);
            node.name_end = index_at(a, node.name_first, mdef.name.ptr + mdef.name.len);
            node.body_first = index_at(a, node.name_end, mdef.body.ptr);
            node.body_end = index_at(a, node.body_first, mdef.body.ptr + mdef.body.len);
            node.end = index_at(a, node.body_end, next.ptr);
        } else if (named(a, i, "tags")) {
            KToken kw = reenter(a, i, &lexer, &pp);
            KTagsDecl tdef;
            if (!k_scan_tags_decl(&lexer, kw, &symtab, &tdef, &next, &next_pp)) return false;
            node.kind = K_AST_TAGS;
            node.name_first = index_at(a, i, tdef.name.ptr);
            node.name_end = index_at(a, node.name_first, tdef.name.ptr + tdef.name.len);
            node.end = index_at(a, node.name_end, next.ptr);
        } else if (named(a, i, "struct") || named(a, i, "union")) {
            KToken kw = reenter(a, i, &lexer, &pp);
            KStructDecl sdef;
            if (!k_scan_struct_decl(&lexer, kw, &symtab, &sdef, &next, &next_pp)) return false;
            size_t after_name = i;
            if (sdef.tag_name.len != 0) {
                node.kind = K_AST_TYPE;
                node.name_first = index_at(a, i, sdef.tag_name.ptr);
                node.name_end = index_at(a, node.name_first, sdef.tag_name.ptr + sdef.tag_name.len);
                after_name = node.name_end;
            } else {
                node.kind = K_AST_OPAQUE;
            }
            node.body_first = index_at(a, after_name, sdef.body.ptr);
            node.body_end = index_at(a, node.body_first, sdef.body.ptr + sdef.body.len);
            node.end = index_at(a, node.body_end, next.ptr);
        } else if (named(a, i, "constexpr")) {
            KToken kw = reenter(a, i, &lexer, &pp);
            KConstexprDecl cdef;
            if (!k_scan_decl_constexpr(&lexer, kw, &symtab, &cdef, &next, &next_pp))
                return false;
            node.kind = K_AST_CONSTEXPR;
            if (cdef.has_name) {
                node.name_first = index_at(a, i, cdef.name.ptr);
                node.name_end = index_at(a, node.name_first, cdef.name.ptr + cdef.name.len);
            }
            node.end = index_at(a, i, next.ptr);
        } else if (named(a, i, "typedef")) {
            KToken kw = reenter(a, i, &lexer, &pp);
            KTypedefDecl tdef;
            if (k_scan_decl_typedef(&lexer, kw, &symtab, &tdef, &next, &next_pp) &&
                tdef.name_count > 0) {
                node.kind = K_AST_TYPE;
                node.end = index_at(a, i, next.ptr);
                /* One declaration, several declared types (keel-spec §2.2's
                   comma list): a node each, in declaration order, sharing
                   the span and the anchor — they are one declaration. The
                   last name is the node the loop appends below. */
                for (size_t k = 0; k + 1 < tdef.name_count; k++) {
                    KAstNode extra = node;
                    extra.name_first = index_at(a, i, tdef.names[k].ptr);
                    extra.name_end = index_at(a, extra.name_first,
                                              tdef.names[k].ptr + tdef.names[k].len);
                    if (!append(a, cap, extra)) return false;
                }
                keel_slice_char last = tdef.names[tdef.name_count - 1];
                node.name_first = index_at(a, i, last.ptr);
                node.name_end = index_at(a, node.name_first, last.ptr + last.len);
            } else {
                /* top-decl's own `<opaque>` alternative: a typedef that does
                   not fit the production is preserved, not rejected. */
                node.end = declaration_end(a, i, &node.body_first, &node.body_end);
                if (node.end <= start || node.end > n) return false;
                node.kind = K_AST_OPAQUE;
            }
        } else {
            node.end = declaration_end(a, i, &node.body_first, &node.body_end);
            if (node.end <= start || node.end > n) return false;
            node.kind = K_AST_OPAQUE;
            size_t j;
            {
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
        if (node.end <= start) return false;
        if (!append(a, cap, node)) return false;
        i = node.end;
    }
    return true;
}
