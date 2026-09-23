#include <stdbool.h>
#include <string.h>
#include "keel/keel_slice_char.type.h"

typedef keel_slice_char KToken;

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

bool k_token_is_c_word(KToken t) {
    size_t n = sizeof(k_c_words) / sizeof(k_c_words[0]);
    for (size_t i = 0; i < n; ++i) {
        const char *word = k_c_words[i];
        size_t len = strlen(word);
        if (t.len == len && memcmp(t.ptr, word, len) == 0) {
            return true;
        }
    }
    return false;
}

bool k_token_is_ident(KToken t) {
    if (t.len == 0) return false;

    /* Check if first character is valid start */
    char c = t.ptr[0];
    if (!((c >= 'a' && c <= 'z') ||
          (c >= 'A' && c <= 'Z') ||
          c == '_')) {
        return false;
    }

    /* Check remaining characters */
    for (size_t i = 1; i < t.len; ++i) {
        c = t.ptr[i];
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '_')) {
            return false;
        }
    }

    /* Check if it's a keyword */
    size_t n = sizeof(k_c_words) / sizeof(k_c_words[0]);
    for (size_t i = 0; i < n; ++i) {
        const char *word = k_c_words[i];
        size_t len = strlen(word);
        if (t.len == len && memcmp(t.ptr, word, len) == 0) {
            return false;
        }
    }

    return true;
}

bool k_token_is_ident_named(KToken t, const char *name) {
    if (!k_token_is_ident(t)) return false;
    size_t len = strlen(name);
    if (t.len != len) return false;
    return memcmp(t.ptr, name, len) == 0;
}

bool k_token_is_c_word_named(KToken t, const char *name) {
    if (!k_token_is_c_word(t)) return false;
    size_t len = strlen(name);
    if (t.len != len) return false;
    return memcmp(t.ptr, name, len) == 0;
}