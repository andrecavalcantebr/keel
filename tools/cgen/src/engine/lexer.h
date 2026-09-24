/* engine/lexer.h — the lexer's types and the recognizers it is built from
 * (lexer-design §1, §3). engine/: no I/O, no allocation. */
#ifndef CGEN_ENGINE_LEXER_H
#define CGEN_ENGINE_LEXER_H

#include <stdbool.h>
#include <stddef.h>
#include "keel/keel_slice_char.type.h"

/* a token is only a view of the source (lexer-design §1) */
typedef keel_slice_char KToken;

typedef enum {
    TK_PP_OTHER,
    TK_PP_IF,     /* #if, #ifdef, #ifndef */
    TK_PP_ELSE,   /* #elif, #elifdef, #elifndef, #else */
    TK_PP_ENDIF   /* #endif */
} TKPpKind;

/* The diagnostics sink of lexer-design §1 is not here yet: it arrives with
 * diag.c (diag-design). `directive` tells the caller that the last token
 * returned was a whole directive line, so `TK_PP_OTHER` can be told apart
 * from an ordinary token. */
typedef struct {
    keel_slice_char source;
    keel_slice_cursor cursor;
    bool line_clean;
    bool directive;
} KLexer;

void   k_lexer_init(KLexer *lexer, keel_slice_char source);
/* an empty token is EOF; pp_kind is TK_PP_OTHER outside a structural directive */
KToken k_lexer_next(KLexer *lexer, TKPpKind *pp_kind);

/* recognizers (lexer-design §3), one file each */
size_t k_lexer_splice_width(keel_slice_char source, size_t pos);
int    k_lexer_peek_at(keel_slice_char source, size_t pos, size_t *width_out);
size_t k_lexer_skip_trivia(keel_slice_char source, size_t pos);
size_t k_lexer_scan_directive(keel_slice_char source, size_t pos, TKPpKind *pp_kind_out);
size_t k_lexer_scan_identifier(keel_slice_char source, size_t pos);
size_t k_lexer_scan_number(keel_slice_char source, size_t pos);
size_t k_lexer_scan_quoted(keel_slice_char source, size_t pos, bool *unterminated_out);
size_t k_lexer_scan_punct(keel_slice_char source, size_t pos);

/* predicates (lexer-design §5) */
bool k_token_is_ident(KToken t);
bool k_token_is_c_word(KToken t);
bool k_token_is_number(KToken t);
bool k_token_is_string(KToken t);
bool k_token_is_char(KToken t);

/* The engine's entry point. For now it runs up to `--stop-after=lex`: it
 * writes every token of `input` to `output`, one per line, as
 * "<line>:<col>: <class> \"<spelling>\"\n" and a last "<line>:<col>: eof\n"
 * (cgen design §5.1, without the file name, which is the tool's). It writes
 * at most output.len bytes and returns how many the whole dump needs — call
 * it with an empty output to size the buffer. */
size_t k_parser_keel(keel_slice_char input, keel_slice_char output);

#endif /* CGEN_ENGINE_LEXER_H */
