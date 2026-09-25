#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "keel/keel_slice_char.type.h"

extern int k_lexer_peek_at(keel_slice_char source, size_t pos, size_t *width_out);

typedef enum {
    TK_PP_OTHER,
    TK_PP_IF,     /* #if, #ifdef, #ifndef */
    TK_PP_ELSE,   /* #elif, #elifdef, #elifndef, #else */
    TK_PP_ENDIF   /* #endif */
} TKPpKind;

static const struct { const char *word; TKPpKind kind; } k_pp_words[] = {
    { "if",       TK_PP_IF    },
    { "ifdef",    TK_PP_IF    },
    { "ifndef",   TK_PP_IF    },
    { "elif",     TK_PP_ELSE  },
    { "elifdef",  TK_PP_ELSE  },
    { "elifndef", TK_PP_ELSE  },
    { "else",     TK_PP_ELSE  },
    { "endif",    TK_PP_ENDIF },
};

size_t k_lexer_scan_directive(keel_slice_char source, size_t pos, TKPpKind *pp_kind_out) {
    size_t start = pos;
    size_t offset = pos;

    // Skip the '#' itself
    offset += 1;

    // Skip horizontal whitespace (space or tab)
    size_t width;
    int ch;
    while ((ch = k_lexer_peek_at(source, offset, &width)) != -1) {
        if (ch == ' ' || ch == '\t') {
            offset += width;
        } else {
            break;
        }
    }

    // Read the keyword into a local buffer
    char keyword[9]; // 8 chars + null terminator
    size_t keyword_len = 0;
    size_t keyword_offset = offset;

    while (keyword_len < 8) {
        int c = k_lexer_peek_at(source, keyword_offset, &width);
        if (c == -1 || (!(c >= 'A' && c <= 'Z') && !(c >= 'a' && c <= 'z'))) {
            break;
        }
        keyword[keyword_len++] = (char)c;
        keyword_offset += width;
    }

    // Null terminate for comparison
    keyword[keyword_len] = '\0';

    // Classify the directive
    TKPpKind kind = TK_PP_OTHER;
    if (pp_kind_out != NULL) {
        for (size_t i = 0; i < sizeof(k_pp_words)/sizeof(k_pp_words[0]); ++i) {
            const char *word = k_pp_words[i].word;
            size_t word_len = strlen(word);
            if (word_len == keyword_len && memcmp(word, keyword, keyword_len) == 0) {
                kind = k_pp_words[i].kind;
                break;
            }
        }
        *pp_kind_out = kind;
    }

    // Find the end of the token: scan to the next newline or end of input
    offset = start;
    while (1) {
        int c = k_lexer_peek_at(source, offset, &width);
        if (c == -1) {
            return offset;  // End of input
        }
        offset += width;
        if (c == '\n' || c == '\r') {
            // Include the newline in the token
            if (c == '\r') {
                int next = k_lexer_peek_at(source, offset, &width);
                if (next == '\n') {
                    offset += width;  // Skip \n after \r
                }
            }
            return offset;
        }
    }

    // Unreachable
    return offset;
}