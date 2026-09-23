#include <stddef.h>
#include "keel/keel_slice_char.type.h"

extern int k_lexer_peek_at(keel_slice_char source, size_t pos, size_t *width_out);

size_t k_lexer_skip_trivia(keel_slice_char source, size_t pos) {
    while (pos < source.len) {
        size_t width;
        int c = k_lexer_peek_at(source, pos, &width);
        if (c < 0) {
            break;
        }

        // Skip horizontal whitespace
        if (c == ' ' || c == '\t') {
            pos += width;
            continue;
        }

        // Skip line comment
        if (c == '/') {
            int next = k_lexer_peek_at(source, pos + width, &width);
            if (next == '/') {
                pos += width + 1; // Skip the second '/'
                while (pos < source.len) {
                    int ch = k_lexer_peek_at(source, pos, &width);
                    if (ch < 0 || ch == '\n' || ch == '\r') {
                        break;
                    }
                    pos += width;
                }
                continue;
            }

            // Skip block comment
            if (next == '*') {
                pos += width + 1; // Skip the '*'
                while (pos < source.len) {
                    int ch = k_lexer_peek_at(source, pos, &width);
                    if (ch < 0) {
                        break;
                    }
                    if (ch == '*' && pos + width < source.len) {
                        int next_ch = k_lexer_peek_at(source, pos + width, &width);
                        if (next_ch == '/') {
                            pos += width + 1; // Skip the '/'
                            break;
                        }
                    }
                    pos += width;
                }
                continue;
            }
        }

        // If none of the above, stop skipping
        break;
    }

    return pos;
}