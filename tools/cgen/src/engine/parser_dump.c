/* engine/parser_dump.c — the module/import/import_c/decl lines of
 * --stop-after=parse's dump (cgen-tool.md §5.2), header+decl level only
 * (no inst/ilha yet — those need the instance closure and island
 * recognition, both later). This is a dump *projection*, not the AST
 * design proposed in parser-design.md §2.3: it parses and formats in the
 * same pass, the same way k_parser_keel does for the lex dump, because
 * nothing downstream needs a retained tree yet. Revisit when the emitter
 * does.
 *
 * `import`'s module is not actually loaded here — no KLoader exists yet
 * (parser-design §3.2, etapa 1's remaining piece). A top-decl whose
 * specifier names an imported symbol (not one declared earlier in the
 * same file) is unrecognized and falls through to the opaque-C branch,
 * silently, until the loader exists. */
#include <string.h>
#include "engine/parser.h"

typedef struct {
    keel_slice_char out;
    size_t n;
} KOut;

static void put_c(KOut *o, char c) {
    if (o->n < o->out.len) o->out.ptr[o->n] = c;
    o->n++;
}
static void put_s(KOut *o, const char *s) { while (*s) put_c(o, *s++); }
static void put_slice(KOut *o, keel_slice_char s) {
    for (size_t i = 0; i < s.len; i++) put_c(o, s.ptr[i]);
}
static void put_buf(KOut *o, const char *buf, size_t len) {
    for (size_t i = 0; i < len; i++) put_c(o, buf[i]);
}
static void put_uint(KOut *o, size_t v) {
    char digits[24];
    size_t k = 0;
    do { digits[k++] = (char)('0' + v % 10); v /= 10; } while (v > 0);
    while (k > 0) put_c(o, digits[--k]);
}

/* module.path + local name -> module_path_local_name (dots become '_').
 * Truncates safely if `cap` is too small; returns the length that would
 * be needed, same sizing convention as everything else. Promote this to
 * its own header once something besides this file needs it. */
static size_t mangle(keel_slice_char module_name, keel_slice_char local_name,
                      char *out, size_t cap) {
    size_t n = 0;
    for (size_t i = 0; i < module_name.len; i++) {
        char c = module_name.ptr[i];
        if (n < cap) out[n] = (c == '.') ? '_' : c;
        n++;
    }
    if (n < cap) out[n] = '_';
    n++;
    for (size_t i = 0; i < local_name.len; i++) {
        if (n < cap) out[n] = local_name.ptr[i];
        n++;
    }
    return n;
}

typedef struct { size_t line, col, at; } KPos;

/* Same physical-position accounting as parser_keel.c's lex dump (CRLF and
 * a lone CR are one line break); `pos` only moves forward, which is fine
 * because every anchor token is visited in source order. */
static void emit_pos(KOut *o, keel_slice_char input, KPos *pos, KToken tok) {
    size_t off = input.len == 0 ? 0 : (size_t)(tok.ptr - input.ptr);
    for (; pos->at < off; ++pos->at) {
        char c = input.ptr[pos->at];
        if (c == '\n' || (c == '\r' && !(pos->at + 1 < input.len && input.ptr[pos->at + 1] == '\n'))) {
            pos->line++;
            pos->col = 1;
        } else if (c != '\r') {
            pos->col++;
        }
    }
    put_uint(o, pos->line);
    put_c(o, ':');
    put_uint(o, pos->col);
}

static void put_decl_line(KOut *o, keel_slice_char input, KPos *pos,
                           const char *vis, const char *espece,
                           keel_slice_char name, keel_slice_char module_name,
                           KToken anchor) {
    char sym[128];
    size_t n = mangle(module_name, name, sym, sizeof sym);
    if (n > sizeof sym) n = sizeof sym; /* truncate rather than overrun */
    put_s(o, "decl\t");
    put_s(o, vis);
    put_c(o, ' ');
    put_s(o, espece);
    put_c(o, '\t');
    put_slice(o, name);
    put_c(o, '\t');
    put_buf(o, sym, n);
    put_c(o, '\t');
    emit_pos(o, input, pos, anchor);
    put_c(o, '\n');
}

size_t k_dump_module(keel_slice_char source, KSymbol *symtab_storage, size_t symtab_cap,
                      keel_slice_char output) {
    KOut o = { output, 0 };
    KPos pos = { 1, 1, 0 };
    KSymbolTable symtab;
    k_symtab_init(&symtab, symtab_storage, symtab_cap);

    KLexer lexer;
    k_lexer_init(&lexer, source, NULL);
    TKPpKind pp;

    /* header: module */
    KToken module_kw = k_lexer_next(&lexer, &pp);
    KModuleHeader mh;
    KToken next;
    TKPpKind npp;
    k_scan_module_decl(&lexer, module_kw, &mh, &next, &npp);
    put_s(&o, "module\t");
    put_slice(&o, mh.module_name);
    put_c(&o, '\t');
    emit_pos(&o, source, &pos, module_kw);
    put_c(&o, '\n');

    /* header: import / import_c (extern_c is recognized, per keel-spec
       §2.2, but has no dump line of its own — cgen-tool.md §5.2's table
       does not list one). */
    for (;;) {
        if (k_token_is_ident_named(next, "import")) {
            KToken kw = next;
            KImportDecl imp;
            k_scan_import(&lexer, kw, &imp, &next, &npp);
            put_s(&o, "import\t");
            put_slice(&o, imp.module_name);
            if (imp.alias.len) { put_s(&o, " as "); put_slice(&o, imp.alias); }
            if (imp.has_types) put_s(&o, " types");
            put_c(&o, '\t');
            emit_pos(&o, source, &pos, kw);
            put_c(&o, '\n');
        } else if (k_token_is_ident_named(next, "import_c")) {
            KToken kw = next;
            KImportCDecl ic;
            k_scan_import_c(&lexer, kw, &ic, &next, &npp);
            put_s(&o, "import_c\t");
            put_slice(&o, ic.header);
            put_c(&o, '\t');
            emit_pos(&o, source, &pos, kw);
            put_c(&o, '\n');
        } else if (k_token_is_ident_named(next, "extern_c")) {
            KToken kw = next;
            KExternCDecl ec;
            k_scan_extern_c(&lexer, kw, &ec, &next, &npp);
        } else {
            break;
        }
    }

    /* top-decls (parser-design §4's dispatch, minus decl-function and
       opaque C, both later): pub/priv, then modifier/tags/struct/union/
       constexpr by keyword, else decl-keel; anything else is skipped as
       opaque up to the next top-level ';'. */
    for (;;) {
        if (next.len == 0) break; /* EOF */
        KToken first = next;
        const char *vis = "pub";
        if (k_token_is_ident_named(first, "pub") || k_token_is_ident_named(first, "priv")) {
            vis = k_token_is_ident_named(first, "priv") ? "priv" : "pub";
            first = k_lexer_next(&lexer, &npp);
        }

        if (k_token_is_ident_named(first, "modifier")) {
            KModifierDecl md;
            int arity = (int)(mh.dim_count + mh.tag_count + mh.type_count);
            k_scan_modifier_decl(&lexer, first, arity, &symtab, &md, &next, &npp);
            put_decl_line(&o, source, &pos, vis, "modifier", md.name, mh.module_name, first);
        } else if (k_token_is_ident_named(first, "tags")) {
            KTagsDecl td;
            k_scan_tags_decl(&lexer, first, &symtab, &td, &next, &npp);
            put_decl_line(&o, source, &pos, vis, "tags", td.name, mh.module_name, first);
        } else if (k_token_is_c_word_named(first, "struct") || k_token_is_c_word_named(first, "union")) {
            KStructDecl sd;
            k_scan_struct_decl(&lexer, first, &symtab, &sd, &next, &npp);
            if (sd.tag_name.len != 0)
                put_decl_line(&o, source, &pos, vis, "type", sd.tag_name, mh.module_name, first);
        } else if (k_token_is_c_word_named(first, "constexpr")) {
            KConstexprDecl cd;
            bool ok = k_scan_decl_constexpr(&lexer, first, &symtab, &cd, &next, &npp);
            if (ok && cd.has_name)
                put_decl_line(&o, source, &pos, vis, "constexpr", cd.name, mh.module_name, first);
            else if (!ok) {
                static const char *terms[] = { ";" };
                size_t idx;
                k_scan_opaque_until(&lexer, first, terms, 1, &idx, &next, &npp);
                next = k_lexer_next(&lexer, &npp);
            }
        } else {
            KKeelDecl kd;
            bool ok = k_scan_decl_keel(&lexer, first, &symtab, &kd, &next, &npp);
            if (ok) {
                for (size_t i = 0; i < kd.name_count; i++)
                    put_decl_line(&o, source, &pos, vis, "var", kd.names[i].name, mh.module_name, first);
            } else {
                static const char *terms[] = { ";" };
                size_t idx;
                k_scan_opaque_until(&lexer, first, terms, 1, &idx, &next, &npp);
                next = k_lexer_next(&lexer, &npp);
            }
        }
    }

    return o.n;
}
