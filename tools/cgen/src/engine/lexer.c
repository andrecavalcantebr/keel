/* engine/lexer.c — k_lexer_next: one token per call, built from the
 * recognizers (lexer-design §1, §3, §4). */
#include "engine/lexer.h"

static bool is_letter(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(int c) {
    return c >= '0' && c <= '9';
}

void k_lexer_init(KLexer *lexer, keel_slice_char source) {
    lexer->source = source;
    lexer->cursor.i = 0;
    lexer->line_clean = true;
    lexer->directive = false;
}

/* Whether a string or char literal starts at pos: a quote, or a u8/u/U/L
   prefix right before one. u8 only before '"', which is what
   k_lexer_scan_quoted accepts. */
static bool starts_quoted(keel_slice_char src, size_t pos, int c, size_t w) {
    if (c == '"' || c == '\'') return true;
    if (c != 'u' && c != 'U' && c != 'L') return false;
    size_t w1;
    int c1 = k_lexer_peek_at(src, pos + w, &w1);
    if (c1 == '"' || c1 == '\'') return true;
    if (c == 'u' && c1 == '8') {
        int c2 = k_lexer_peek_at(src, pos + w + w1, NULL);
        return c2 == '"';
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
    } else {
        lexer->line_clean = false;
        if (starts_quoted(src, pos, c, w)) {
            bool unterminated = false;   /* literal-with-newline: with diag.c */
            end = k_lexer_scan_quoted(src, pos, &unterminated);
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
    return token;
}
