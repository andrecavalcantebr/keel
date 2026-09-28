#include <stdint.h>
#include "engine/ast.h"
#include "engine/parser.h"

static bool named(const KAst *a, size_t i, const char *s) {
    return i < a->tokens.len && k_token_spelled(keel_buffer_KLexeme_ptr(&a->tokens, i)->token, s);
}

static bool punct(const KAst *a, size_t i, const char *s) {
    return i < a->tokens.len && k_token_is_punct(keel_buffer_KLexeme_ptr(&a->tokens, i)->token, s);
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
    for (size_t j = i; j < a->tokens.len; j++) {
        if (punct(a, j, open)) depth++;
        if (punct(a, j, close) && --depth == 0) return j + 1;
    }
    return SIZE_MAX;
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
    for (size_t j = i; j < a->tokens.len; j++) {
        if (punct(a, j, "(")) paren++;
        else if (punct(a, j, ")") && paren) paren--;
        else if (punct(a, j, "[")) bracket++;
        else if (punct(a, j, "]") && bracket) bracket--;
        else if (!paren && !bracket && punct(a, j, "=")) initializer = true;
        else if (!paren && !bracket && punct(a, j, "{")) {
            size_t after = balanced(a, j, "{", "}");
            if (after == SIZE_MAX) return SIZE_MAX;
            *body_first = j + 1;
            *body_end = after > j ? after - 1 : j;
            if (named(a, i, "typedef") || initializer) {
                j = after - 1;
                continue;
            }
            return punct(a, after, ";") ? after + 1 : after;
        } else if (!paren && !bracket && punct(a, j, ";")) return j + 1;
    }
    return SIZE_MAX;
}

static bool append(KAst *a, KAstNode n) {
    return keel_buffer_KAstNode_push_1(&a->nodes, n) != NULL;
}

/* Re-enters the token stream at (*keel_buffer_KLexeme_ptr(&a->tokens, i)), through a throwaway lexer, so
   the recognizers of engine/parser.h — each written against a live KLexer,
   one k_lexer_next call at a time — can be called from here without
   re-lexing the file from its start. Diagnostics are NOT re-emitted through
   it (NULL sink): the whole file was already lexed once, with the real
   sink, before k_parse_ast ever runs (tool/stop_parse.c) — this second,
   local lex, over the tail of the same source buffer, would just repeat
   them. Returns keel_buffer_KLexeme_ptr(&a->tokens, i)->token itself, read as the recognizers expect
   ("first" already consumed), with `lexer` positioned right after it. */
static KToken reenter(const KAst *a, size_t i, KLexer *lexer, TKPpKind *pp) {
    KToken t = keel_buffer_KLexeme_ptr(&a->tokens, i)->token;
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
    while (k < a->tokens.len && keel_buffer_KLexeme_ptr(&a->tokens, k)->token.ptr < ptr) k++;
    return k;
}

static bool parse(KAst *a,
                  KSymbolTable *symbols, bool headers_only) {
    a->nodes.len = 0;
    a->module = 0;
    a->error_token = 0;
    size_t i = 0, n = a->tokens.len;
    while (i < n && keel_buffer_KLexeme_ptr(&a->tokens, i)->directive) i++;
    if (i >= n || !named(a, i, "module")) return false;

    KLexer lexer;
    TKPpKind pp;
    KToken module_kw = reenter(a, i, &lexer, &pp);
    KModuleHeader header;
    KToken next;
    TKPpKind next_pp;
    if (!k_scan_module_decl(&lexer, module_kw, &header, &next, &next_pp)) return false;
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
    if (!append(a, mod)) return false;
    i = mod.end;

    /* The declaring module's own symbols (locally-declared types, modifiers,
       tags and constants) — engine/symtab.h: "the caller gives the
       storage", same as `nodes` above. 256 is an arbitrary limit, well past
       any keel module seen so far. */
    KSymbol symtab_storage[256];
    KSymbolTable local;
    k_symtab_init(&local, symtab_storage, 256);
    KSymbolTable *symtab = symbols ? symbols : &local;

    while (i < n) {
        if (keel_buffer_KLexeme_ptr(&a->tokens, i)->directive) { i++; continue; }
        size_t start = i;
        a->error_token = i;
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
                if (!k_scan_import_c(&lexer, kw, &imp, &next, &next_pp)) return false;
                node.kind = K_AST_IMPORT_C;
                node.name_first = index_at(a, i, imp.header.ptr);
                node.name_end = index_at(a, node.name_first, imp.header.ptr + imp.header.len);
            } else {
                KImportDecl imp;
                if (!k_scan_import(&lexer, kw, &imp, &next, &next_pp)) return false;
                node.kind = K_AST_IMPORT;
                node.name_first = index_at(a, i, imp.module_name.ptr);
                node.name_end = index_at(a, node.name_first, imp.module_name.ptr + imp.module_name.len);
                node.has_types = imp.has_types;
                node.alias = imp.alias.len ? index_at(a, node.name_end, imp.alias.ptr) : SIZE_MAX;
            }
            node.end = index_at(a, node.name_end, next.ptr);
        } else if (headers_only) {
            node.kind = K_AST_OPAQUE;
            node.end = declaration_end(a, i, &node.body_first, &node.body_end);
            if (node.end > n) return false;
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
            if (!k_scan_modifier_decl(&lexer, kw, module_arity, symtab, &mdef, &next, &next_pp))
                return false;
            keel_buffer_KSymbol_ptr(symtab, symtab->len-1)->dim_arity=(int)header.dim_count;
            node.kind = K_AST_MODIFIER;
            node.name_first = index_at(a, i, mdef.name.ptr);
            node.name_end = index_at(a, node.name_first, mdef.name.ptr + mdef.name.len);
            node.body_first = index_at(a, node.name_end, mdef.body.ptr);
            node.body_end = index_at(a, node.body_first, mdef.body.ptr + mdef.body.len);
            node.end = index_at(a, node.body_end, next.ptr);
        } else if (named(a, i, "tags")) {
            KToken kw = reenter(a, i, &lexer, &pp);
            KTagsDecl tdef;
            if (!k_scan_tags_decl(&lexer, kw, symtab, &tdef, &next, &next_pp)) return false;
            node.kind = K_AST_TAGS;
            node.name_first = index_at(a, i, tdef.name.ptr);
            node.name_end = index_at(a, node.name_first, tdef.name.ptr + tdef.name.len);
            node.end = index_at(a, node.name_end, next.ptr);
        } else if ((named(a, i, "struct") || named(a, i, "union")) &&
                   (punct(a,i+1,"{") || punct(a,i+2,"{"))) {
            KToken kw = reenter(a, i, &lexer, &pp);
            KStructDecl sdef;
            if (!k_scan_struct_decl(&lexer, kw, symtab, &sdef, &next, &next_pp)) return false;
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
            for(size_t k=0;k<sdef.name_count;k++) {
                if(!append(a,node))return false;
                node.kind=K_AST_VARIABLE;
                node.name_first=index_at(a,i,sdef.names[k].ptr);
                node.name_end=index_at(a,node.name_first,sdef.names[k].ptr+sdef.names[k].len);
            }
        } else if (named(a, i, "array") && symbols) {
            KToken kw=reenter(a,i,&lexer,&pp);
            KArrayDecl decl;
            if(!k_scan_decl_array(&lexer,kw,symtab,&decl,&next,&next_pp))return false;
            node.kind=K_AST_VARIABLE;
            node.end=index_at(a,i,next.ptr);
            for(size_t k=0;k<decl.name_count;k++) {
                node.name_first=index_at(a,i,decl.names[k].ptr);
                node.name_end=index_at(a,node.name_first,decl.names[k].ptr+decl.names[k].len);
                node.dim_first=index_at(a,node.name_end,decl.dims[k].ptr);
                node.dim_end=index_at(a,node.dim_first,decl.dims[k].ptr+decl.dims[k].len);
                if(k+1<decl.name_count&&!append(a,node))return false;
            }
        } else if (named(a, i, "constexpr")) {
            KToken kw = reenter(a, i, &lexer, &pp);
            KConstexprDecl cdef;
            if (!k_scan_decl_constexpr(&lexer, kw, symtab, &cdef, &next, &next_pp))
                return false;
            node.kind = K_AST_CONSTEXPR;
            if (cdef.has_name) {
                node.name_first = index_at(a, i, cdef.name.ptr);
                node.name_end = index_at(a, node.name_first, cdef.name.ptr + cdef.name.len);
            }
            node.end = index_at(a, i, next.ptr);
        } else if (named(a, i, "extent")) {
            KToken kw = reenter(a, i, &lexer, &pp);
            KExtentDecl edef;
            if (k_scan_extent_decl(&lexer, kw, symtab, &edef, &next, &next_pp)) {
                /* `extent struct X [...] {...};` declares the type X
                   (keel-spec §4.11); without this branch it fell to the
                   generic path and came out as a variable. */
                node.kind = K_AST_TYPE;
                node.dim_first=index_at(a,i,edef.dim_names[0].ptr);
                node.dim_end=index_at(a,node.dim_first,edef.body.ptr);
                node.name_first = index_at(a, i, edef.name.ptr);
                node.name_end = index_at(a, node.name_first,
                                         edef.name.ptr + edef.name.len);
                node.body_first = index_at(a, node.name_end, edef.body.ptr);
                node.body_end = index_at(a, node.body_first,
                                         edef.body.ptr + edef.body.len);
                node.end = index_at(a, node.body_end, next.ptr);
            } else {
                node.end = declaration_end(a, i, &node.body_first, &node.body_end);
                if (node.end <= start || node.end > n) return false;
                node.kind = K_AST_OPAQUE;
            }
        } else if (named(a, i, "typedef")) {
            KToken kw = reenter(a, i, &lexer, &pp);
            KTypedefDecl tdef;
            if (k_scan_decl_typedef(&lexer, kw, symtab, &tdef, &next, &next_pp) &&
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
                    if (!append(a, extra)) return false;
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

            /* decl-function and decl-keel (keel-spec §2.2) share a shape:
               a specifier, then a declarator. Only the declarator's own
               suffixes say which — a parameter group straight on the name
               is a function, anything else declares an object. Reading
               the last IDENT before the ';' instead dropped every name
               ending in a suffix: `const char *NAMES[3]` and
               `array Person people[4]` named nothing at all, and a node
               with no name is skipped by the dump. */
            KToken kw = reenter(a, i, &lexer, &pp);
            const char *limit = node.end < n ? keel_buffer_KLexeme_ptr(&a->tokens, node.end)->token.ptr
                                             : a->source.ptr + a->source.len;
            KToken begin = k_find_declarator_start(&lexer, kw, limit);
            /* Prefer the imported/local signature over the C fallback:
               a modifier may consume several arguments before its declarator. */
            KLexer typed=lexer; KToken type_first=kw,type_next={0};
            while(k_token_spelled(type_first,"const")||k_token_spelled(type_first,"volatile")||
                  k_token_spelled(type_first,"static")||k_token_spelled(type_first,"extern"))
                type_first=k_lexer_next(&typed,&pp);
            KSpecifier spec;
            bool type_ok=k_scan_known_type(&typed,type_first,symtab,&spec,&type_next,&pp);
            /* Inside a generic, its own modifier can omit its arguments;
               the declarator fallback preserves that template signature. */
            if(type_ok && spec.kind!=K_SPEC_NONE)begin=type_next;
            if (begin.len != 0) {
                KToken tok = kw;
                while (tok.len != 0 && tok.ptr < begin.ptr)
                    tok = k_lexer_next(&lexer, &pp);
                KDeclarator declarator;
                while (tok.len != 0 &&
                    k_scan_declarator(&lexer, tok, &declarator, &next, &next_pp) &&
                    declarator.name.len != 0) {
                    size_t name_first = index_at(a, i, declarator.name.ptr);
                    size_t name_end = index_at(a, name_first,
                                               declarator.name.ptr + declarator.name.len);
                    if (name_end <= node.end) {
                        node.kind = declarator.has_function_suffix &&
                                    !declarator.parenthesized
                                    ? K_AST_FUNCTION : K_AST_VARIABLE;
                        node.name_first = name_first;
                        node.name_end = name_end;
                    }
                    if(node.kind==K_AST_FUNCTION)break;
                    if(k_token_is_punct(next,"=")) {
                        static const char *const terms[]={",",";"}; size_t which;
                        KToken value=k_lexer_next(&lexer,&next_pp);
                        k_scan_opaque_until(&lexer,value,terms,2,&which,&next,&next_pp);
                    }
                    if(!k_token_is_punct(next,","))break;
                    if(!append(a,node))return false;
                    tok=k_lexer_next(&lexer,&next_pp);
                }
            }
        }
        if (node.end <= start) return false;
        if (!append(a, node)) return false;
        i = node.end;
    }
    return true;
}

bool k_parse_headers(KAst *a) {
    return parse(a, NULL, true);
}
bool k_collect_ast(KAst *a, KSymbolTable *symbols) {
    a->symbols = symbols;
    return parse(a, symbols, false);
}
bool k_parse_ast(KAst *a) {
    return parse(a, NULL, false);
}
