/* engine/lexer.c — k_lexer_next: one token per call, built from the
 * recognizers (lexer-design §1, §3, §4). */
#include "engine/lexer.h"

static bool is_letter(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(int c) {
    return c >= '0' && c <= '9';
}

void k_lexer_init(KLexer *lexer, keel_slice_char source, KDiagnosticSink *diagnostics) {
    lexer->source = source;
    lexer->cursor.i = 0;
    lexer->line_clean = true;
    lexer->directive = false;
    lexer->diagnostics = diagnostics;
    lexer->leading = 0;
    lexer->keel_module = false;
}

/* the contextual words of keel (lexer-design §4), sorted by strcmp */
static const char *const k_contextual_words[] = {
    "ALL", "ANY", "apply", "array", "as", "byref", "constexpr", "defer",
    "dim", "else", "extent", "extern_c", "fail", "foreach", "import",
    "import_c", "instance", "later", "match", "modifier", "module", "now",
    "parallel", "priv", "pub", "ref", "tags", "type", "type_h", "types",
    "walk", "win"
};

/* the identifier at pos in src, after spaces, tabs and comments */
static KToken word_at(keel_slice_char src, size_t *pos) {
    size_t first = k_lexer_skip_trivia(src, *pos);
    size_t end = k_lexer_scan_identifier(src, first);
    *pos = end;
    KToken t = { end - first, src.ptr + first };
    return t;
}

/* define-over-keel-name (lexer-design §4): the target of `#define` or
   `#undef` may not be a keel word nor start with `keel_` or `KEEL_`. The
   body is not read. The base's own modules own the KEEL_ names. */
static void check_define(KLexer *lexer, KToken directive) {
    if (lexer->diagnostics == NULL || lexer->keel_module) return;
    size_t pos, w;
    k_lexer_peek_at(directive, 0, &w);              /* the `#` */
    pos = w;
    KToken verb = word_at(directive, &pos);
    if (!k_token_spelled(verb, "define") && !k_token_spelled(verb, "undef")) return;
    KToken name = word_at(directive, &pos);
    if (name.len == 0) return;

    const char *why = NULL;
    if (k_token_starts_with(name, "keel_") || k_token_starts_with(name, "KEEL_")) {
        why = "names starting with `keel_` or `KEEL_` are reserved to keel";
    } else {
        size_t n = sizeof(k_contextual_words) / sizeof(k_contextual_words[0]);
        for (size_t i = 0; i < n && why == NULL; ++i) {
            if (k_token_spelled(name, k_contextual_words[i])) why = "it is a keel word";
        }
    }
    if (why == NULL) return;
    KDiagArgs args = { { verb, name, k_diag_text(why) } };
    k_diag_emit(lexer->diagnostics, K_DIAG_DEFINE_OVER_KEEL_NAME, name, args);
}

/* `module keel` or `module keel.<name>` as the first two tokens */
static void track_leading(KLexer *lexer, KToken t) {
    if (lexer->leading == 0) {
        lexer->leading = k_token_spelled(t, "module") ? 1 : 2;
    } else if (lexer->leading == 1) {
        lexer->keel_module = k_token_spelled(t, "keel");
        lexer->leading = 2;
    }
}

/* Whether a string or char literal starts at pos: a quote, or a u8/u/U/L
   prefix right before one (u8 before a char is C23). */
static bool starts_quoted(keel_slice_char src, size_t pos, int c, size_t w) {
    if (c == '"' || c == '\'') return true;
    if (c != 'u' && c != 'U' && c != 'L') return false;
    size_t w1;
    int c1 = k_lexer_peek_at(src, pos + w, &w1);
    if (c1 == '"' || c1 == '\'') return true;
    if (c == 'u' && c1 == '8') {
        int c2 = k_lexer_peek_at(src, pos + w + w1, NULL);
        return c2 == '"' || c2 == '\'';
    }
    return false;
}

KToken k_lexer_next(KLexer *lexer, TKPpKind *pp_kind) {
    keel_slice_char src = lexer->source;
    size_t pos = lexer->cursor.i;
    size_t w = 0;
    int c;

    if (pp_kind != NULL) *pp_kind = TK_PP_OTHER;
    lexer->directive = false;

    /* trivia: skip_trivia takes spaces, tabs and comments; newlines are
       counted here, because they make the line clean (lexer-design §3) */
    for (;;) {
        pos = k_lexer_skip_trivia(src, pos);
        c = k_lexer_peek_at(src, pos, &w);
        if (c == '\n') {
            pos += w;
            lexer->line_clean = true;
        } else if (c == '\r') {
            pos += w;
            size_t w2;
            if (k_lexer_peek_at(src, pos, &w2) == '\n') pos += w2;
            lexer->line_clean = true;
        } else if (c == '\v' || c == '\f') {
            pos += w;
        } else {
            break;
        }
    }

    if (c == -1) {                       /* EOF: an empty token at the end */
        lexer->cursor.i = src.len;
        KToken eof = { 0, src.ptr + src.len };
        return eof;
    }

    size_t first = pos;
    size_t end;

    if (c == '#' && lexer->line_clean) {
        TKPpKind kind = TK_PP_OTHER;
        end = k_lexer_scan_directive(src, pos, &kind);
        if (pp_kind != NULL) *pp_kind = kind;
        lexer->directive = true;
        lexer->line_clean = true;        /* the token took the newline */
        KToken d = { end - first, src.ptr + first };
        check_define(lexer, d);
    } else {
        lexer->line_clean = false;
        if (starts_quoted(src, pos, c, w)) {
            bool unterminated = false;
            end = k_lexer_scan_quoted(src, pos, &unterminated);
            int stop = k_lexer_peek_at(src, end, NULL);
            if (unterminated && (stop == '\n' || stop == '\r')) {
                KToken lit = { end - first, src.ptr + first };
                bool str = k_token_is_string(lit);
                KDiagArgs args = { { k_diag_text(str ? "string" : "character"),
                                     k_diag_text(str ? "\"" : "'") } };
                k_diag_emit(lexer->diagnostics, K_DIAG_LITERAL_WITH_NEWLINE, lit, args);
            }
        } else if (is_letter(c) ||
                   (c == '\\' && (k_lexer_peek_at(src, pos + w, NULL) == 'u' ||
                                  k_lexer_peek_at(src, pos + w, NULL) == 'U'))) {
            end = k_lexer_scan_identifier(src, pos);
        } else if (is_digit(c) ||
                   (c == '.' && is_digit(k_lexer_peek_at(src, pos + w, NULL)))) {
            end = k_lexer_scan_number(src, pos);
        } else {
            end = k_lexer_scan_punct(src, pos);
        }
    }

    if (end <= first) end = first + w;   /* every call makes progress */
    if (end > src.len) end = src.len;

    lexer->cursor.i = end;
    KToken token = { end - first, src.ptr + first };
    if (!lexer->directive) track_leading(lexer, token);
    return token;
}
