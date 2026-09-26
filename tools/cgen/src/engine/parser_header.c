#include "engine/parser.h"

void k_scan_module_decl(KLexer *lexer, KToken module_kw, KModuleHeader *out,
                         KToken *next_out, TKPpKind *next_pp_kind_out) {
    KToken name_first = k_lexer_next(lexer, next_pp_kind_out);
    out->module_name = k_scan_qualified_name(lexer, name_first, next_out, next_pp_kind_out);
    out->dim_count = out->tag_count = out->type_count = 0;

    if (k_token_is_ident_named(*next_out, "dim")) {
        KToken first = k_lexer_next(lexer, next_pp_kind_out);
        k_scan_ident_list(lexer, first, out->dims, 8, &out->dim_count, next_out, next_pp_kind_out);
    }
    if (k_token_is_ident_named(*next_out, "tags")) {
        KToken first = k_lexer_next(lexer, next_pp_kind_out);
        k_scan_ident_list(lexer, first, out->tags, 8, &out->tag_count, next_out, next_pp_kind_out);
    }
    if (k_token_is_ident_named(*next_out, "type")) {
        KToken first = k_lexer_next(lexer, next_pp_kind_out);
        k_scan_ident_list(lexer, first, out->types, 8, &out->type_count, next_out, next_pp_kind_out);
    }

    *next_out = k_lexer_next(lexer, next_pp_kind_out);
}

void k_scan_import(KLexer *lexer, KToken import_kw, KImportDecl *out,
                    KToken *next_out, TKPpKind *next_pp_kind_out) {
    KToken name_first = k_lexer_next(lexer, next_pp_kind_out);
    out->module_name = k_scan_qualified_name(lexer, name_first, next_out, next_pp_kind_out);
    out->alias = (keel_slice_char){0};
    out->has_types = false;

    if (k_token_is_ident_named(*next_out, "as")) {
        out->alias = k_lexer_next(lexer, next_pp_kind_out);
        *next_out = k_lexer_next(lexer, next_pp_kind_out);
    }

    if (k_token_is_ident_named(*next_out, "types")) {
        out->has_types = true;
        *next_out = k_lexer_next(lexer, next_pp_kind_out);
    }

    *next_out = k_lexer_next(lexer, next_pp_kind_out);
}

void k_scan_import_c(KLexer *lexer, KToken import_c_kw, KImportCDecl *out,
                      KToken *next_out, TKPpKind *next_pp_kind_out) {
    KToken tok = k_lexer_next(lexer, next_pp_kind_out);

    if (k_token_is_string(tok)) {
        out->header = tok;
    } else {
        KToken first = tok;
        KToken last = tok;
        while (!k_token_is_punct(last, ">")) {
            last = k_lexer_next(lexer, next_pp_kind_out);
        }
        out->header = (keel_slice_char){ .ptr = first.ptr, .len = (size_t)((last.ptr + last.len) - first.ptr) };
    }

    KToken semicolon = k_lexer_next(lexer, next_pp_kind_out);
    (void)semicolon;
    *next_out = k_lexer_next(lexer, next_pp_kind_out);
}