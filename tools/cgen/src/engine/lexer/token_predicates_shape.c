#include <stdbool.h>
#include <string.h>
#include "keel/keel_slice_char.type.h"

typedef keel_slice_char KToken;

extern int k_lexer_peek_at(keel_slice_char source, size_t pos, size_t *width_out);

/* The predicates read the logical spelling: splices are skipped (lexer-design
   §5, [D7]). `c` is the n-th logical byte of t, or -1 past the end. */
static int logical_at(KToken t, size_t n) {
    size_t pos = 0, w;
    int c = k_lexer_peek_at(t, pos, &w);
    while (c >= 0 && n > 0) {
        pos += w;
        c = k_lexer_peek_at(t, pos, &w);
        n--;
    }
    return c;
}

static bool is_digit(int c) {
    return c >= '0' && c <= '9';
}

/* how many logical bytes the u8, u, U or L prefix takes, if the literal
   has one */
static size_t prefix_len(KToken t) {
    int c0 = logical_at(t, 0);
    if (c0 == 'u' && logical_at(t, 1) == '8') return 2;
    if (c0 == 'u' || c0 == 'U' || c0 == 'L') return 1;
    return 0;
}

bool k_token_is_number(KToken t) {
    int c0 = logical_at(t, 0);
    return is_digit(c0) || (c0 == '.' && is_digit(logical_at(t, 1)));
}

bool k_token_is_string(KToken t) {
    return logical_at(t, prefix_len(t)) == '"';
}

/* u8 is a char prefix since C23 */
bool k_token_is_char(KToken t) {
    return logical_at(t, prefix_len(t)) == '\'';
}

bool k_token_is_punct(KToken t, const char *spelling) {
    if (spelling == NULL) return false;
    size_t pos = 0, w;
    for (; *spelling; spelling++) {
        if (k_lexer_peek_at(t, pos, &w) != (unsigned char)*spelling) return false;
        pos += w;
    }
    return k_lexer_peek_at(t, pos, NULL) < 0;
}
