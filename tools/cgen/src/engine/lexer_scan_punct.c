#include <stddef.h>
#include <stdbool.h>
#include "keel/keel_slice_char.type.h"

extern int k_lexer_peek_at(keel_slice_char source, size_t pos, size_t *width_out);

static const char *const k_punct_3[] = { "...", "<<=", ">>=" };

static const char *const k_punct_2[] = {
    "->", "++", "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||",
    "*=", "/=", "%=", "+=", "-=", "&=", "^=", "|=", "##", ".."
};

static const char *const k_punct_1[] = {
    "[", "]", "(", ")", "{", "}", ".", "&", "*", "+", "-", "~", "!", "/",
    "%", "<", ">", "^", "|", "?", ":", ";", "=", ",", "#"
};

size_t k_lexer_scan_punct(keel_slice_char source, size_t pos) {
    size_t width_out;
    int b1 = k_lexer_peek_at(source, pos, &width_out);
    if (b1 == -1) return pos;

    size_t p1 = width_out;
    int b2 = k_lexer_peek_at(source, pos + p1, &width_out);
    if (b2 == -1) {
        // Only one byte available
        for (size_t i = 0; i < sizeof(k_punct_1)/sizeof(*k_punct_1); ++i) {
            if (k_punct_1[i][0] == b1) {
                return pos + p1;
            }
        }
        return pos + p1;
    }

    size_t p2 = p1 + width_out;
    int b3 = k_lexer_peek_at(source, pos + p2, &width_out);
    if (b3 == -1) {
        // Only two bytes available
        for (size_t i = 0; i < sizeof(k_punct_2)/sizeof(*k_punct_2); ++i) {
            if (k_punct_2[i][0] == b1 && k_punct_2[i][1] == b2) {
                return pos + p2;
            }
        }
        for (size_t i = 0; i < sizeof(k_punct_1)/sizeof(*k_punct_1); ++i) {
            if (k_punct_1[i][0] == b1) {
                return pos + p1;
            }
        }
        return pos + p1;
    }

    // Three bytes available
    for (size_t i = 0; i < sizeof(k_punct_3)/sizeof(*k_punct_3); ++i) {
        if (k_punct_3[i][0] == b1 && k_punct_3[i][1] == b2 && k_punct_3[i][2] == b3) {
            return pos + p2 + width_out;
        }
    }

    for (size_t i = 0; i < sizeof(k_punct_2)/sizeof(*k_punct_2); ++i) {
        if (k_punct_2[i][0] == b1 && k_punct_2[i][1] == b2) {
            return pos + p2;
        }
    }

    for (size_t i = 0; i < sizeof(k_punct_1)/sizeof(*k_punct_1); ++i) {
        if (k_punct_1[i][0] == b1) {
            return pos + p1;
        }
    }

    // Fallback: consume one byte
    return pos + p1;
}