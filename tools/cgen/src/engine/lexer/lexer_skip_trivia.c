#include <stddef.h>
#include "keel/keel_slice_char.type.h"

extern int k_lexer_peek_at(keel_slice_char source, size_t pos, size_t *width_out);

/* Spaces, tabs and comments (lexer-design §4). Newlines are the caller's,
   because they make the line clean. Every byte is peeked with its own width:
   a splice may sit before any of them, so `/` + splice + `*` still opens a
   comment and `*` + splice + `/` still closes one. */
size_t k_lexer_skip_trivia(keel_slice_char source, size_t pos) {
    while (pos < source.len) {
        size_t w1, w2;
        int c = k_lexer_peek_at(source, pos, &w1);
        if (c < 0) {
            break;
        }

        if (c == ' ' || c == '\t') {
            pos += w1;
            continue;
        }

        if (c != '/') {
            break;
        }
        int next = k_lexer_peek_at(source, pos + w1, &w2);

        /* line comment: up to, not including, the logical newline */
        if (next == '/') {
            pos += w1 + w2;
            for (;;) {
                int ch = k_lexer_peek_at(source, pos, &w1);
                if (ch < 0 || ch == '\n' || ch == '\r') {
                    break;
                }
                pos += w1;
            }
            continue;
        }

        /* block comment: up to the first `*` `/`; unclosed, up to EOF */
        if (next == '*') {
            pos += w1 + w2;
            for (;;) {
                int ch = k_lexer_peek_at(source, pos, &w1);
                if (ch < 0) {
                    return source.len;
                }
                if (ch == '*' && k_lexer_peek_at(source, pos + w1, &w2) == '/') {
                    pos += w1 + w2;
                    break;
                }
                pos += w1;
            }
            continue;
        }

        break;
    }

    return pos;
}
