#include <stdbool.h>
#include <string.h>
#include "keel/keel_slice_char.type.h"

typedef keel_slice_char KToken;

extern int k_lexer_peek_at(keel_slice_char source, size_t pos, size_t *width_out);

static const char *const k_c_words[] = {
    "_Alignas", "_Alignof", "_Atomic", "_BitInt", "_Bool", "_Complex",
    "_Decimal128", "_Decimal32", "_Decimal64", "_Generic", "_Imaginary",
    "_Noreturn", "_Static_assert", "_Thread_local",
    "alignas", "alignof", "auto", "bool", "break", "case", "char", "const",
    "constexpr", "continue", "default", "do", "double", "else", "enum",
    "extern", "false", "float", "for", "goto", "if", "inline", "int", "long",
    "nullptr", "register", "restrict", "return", "short", "signed", "sizeof",
    "static", "static_assert", "struct", "switch", "thread_local", "true",
    "typedef", "typeof", "typeof_unqual", "union", "unsigned", "void",
    "volatile", "while"
};

/* Whether the logical spelling of t — splices skipped (lexer-design §5,
   [D7]) — begins with `prefix`; and the position just past it. */
static bool spelled_prefix(KToken t, const char *prefix, size_t *end) {
    size_t pos = 0, w;
    for (; *prefix; prefix++) {
        if (k_lexer_peek_at(t, pos, &w) != (unsigned char)*prefix) return false;
        pos += w;
    }
    *end = pos;
    return true;
}

bool k_token_starts_with(KToken t, const char *prefix) {
    size_t end;
    return spelled_prefix(t, prefix, &end);
}

bool k_token_spelled(KToken t, const char *s) {
    size_t end;
    return spelled_prefix(t, s, &end) && k_lexer_peek_at(t, end, NULL) < 0;
}

static bool is_ident_byte(int c, bool first) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ||
           (!first && c >= '0' && c <= '9');
}

static bool is_hex(int c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool k_token_is_c_word(KToken t) {
    size_t n = sizeof(k_c_words) / sizeof(k_c_words[0]);
    for (size_t i = 0; i < n; ++i) {
        if (k_token_spelled(t, k_c_words[i])) return true;
    }
    return false;
}

/* The C form of an identifier: ASCII letters, digits and `_`, plus the
   universal character names `\uXXXX` and `\UXXXXXXXX` (lexer-design §5); and
   not a C word. */
bool k_token_is_ident(KToken t) {
    size_t pos = 0, w;
    bool first = true;
    int c;
    while ((c = k_lexer_peek_at(t, pos, &w)) >= 0) {
        pos += w;
        if (c == '\\') {
            int u = k_lexer_peek_at(t, pos, &w);
            if (u != 'u' && u != 'U') return false;
            pos += w;
            for (int k = u == 'u' ? 4 : 8; k > 0; k--) {
                if (!is_hex(k_lexer_peek_at(t, pos, &w))) return false;
                pos += w;
            }
        } else if (!is_ident_byte(c, first)) {
            return false;
        }
        first = false;
    }
    return !first && !k_token_is_c_word(t);
}

bool k_token_is_ident_named(KToken t, const char *name) {
    return k_token_is_ident(t) && k_token_spelled(t, name);
}

bool k_token_is_c_word_named(KToken t, const char *name) {
    return k_token_is_c_word(t) && k_token_spelled(t, name);
}
