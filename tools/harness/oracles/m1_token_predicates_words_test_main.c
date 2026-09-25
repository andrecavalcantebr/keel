/* Hand-written driver for tools/harness/tasks/m1-token-predicates-words.md.
 * Not model-generated. */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "keel/keel_slice_char.type.h"

typedef keel_slice_char KToken;

bool k_token_is_c_word(KToken t);
bool k_token_is_ident(KToken t);
bool k_token_is_ident_named(KToken t, const char *name);
bool k_token_is_c_word_named(KToken t, const char *name);

static int failures = 0;

static KToken tok(const char *s) {
    KToken t;
    t.len = strlen(s);
    t.ptr = (char *)s;
    return t;
}

#define CHECK(cond, msg) do { \
        if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); failures++; } \
    } while (0)

int main(void) {
    CHECK(k_token_is_c_word(tok("return")), "return is a c word");
    CHECK(!k_token_is_ident(tok("return")), "return is not an ident");
    CHECK(k_token_is_ident(tok("myVar")), "myVar is an ident");
    CHECK(!k_token_is_c_word(tok("myVar")), "myVar is not a c word");
    CHECK(k_token_is_ident(tok("returnx")), "returnx is an ident, not a c word");
    CHECK(k_token_is_c_word_named(tok("int"), "int"), "int named int");
    CHECK(!k_token_is_c_word_named(tok("int"), "long"), "int is not long");
    CHECK(k_token_is_ident_named(tok("foo"), "foo"), "foo named foo");
    CHECK(!k_token_is_ident_named(tok("foo"), "bar"), "foo is not bar");
    CHECK(!k_token_is_ident(tok("3abc")), "3abc does not start with a digit-free ident");
    CHECK(k_token_is_c_word(tok("_Atomic")), "_Atomic is a c word");
    CHECK(!k_token_is_ident(tok("_Atomic")), "_Atomic is not an ident");
    CHECK(k_token_is_ident(tok("_private")), "_private is an ident");

    /* the logical spelling: splices are skipped (lexer-design §5, [D7]) */
    CHECK(k_token_is_c_word(tok("ret\\\nurn")), "ret-splice-urn is return");
    CHECK(!k_token_is_ident(tok("ret\\\r\nurn")), "ret-splice(CRLF)-urn is not an ident");
    CHECK(k_token_is_ident_named(tok("fo\\\no"), "foo"), "fo-splice-o is named foo");
    CHECK(k_token_is_c_word_named(tok("in\\\nt"), "int"), "in-splice-t is the c word int");

    /* universal character names (lexer-design §5) */
    CHECK(k_token_is_ident(tok("caf\\u00e9")), "caf\\u00e9 is an ident");
    CHECK(k_token_is_ident(tok("\\U0001F600x")), "an ident may start with \\U");
    CHECK(!k_token_is_ident(tok("caf\\u00e")), "\\u needs 4 hex digits");
    CHECK(!k_token_is_ident(tok("caf\\u00eg")), "\\u takes hex digits only");
    CHECK(!k_token_is_ident(tok("a\\x41")), "\\x is not a universal character name");

    const char *words[] = {"int", "return", "struct", "_Bool", "while", "volatile"};
    for (size_t i = 0; i < sizeof(words) / sizeof(*words); i++) {
        KToken t = tok(words[i]);
        CHECK(k_token_is_c_word(t) && !k_token_is_ident(t), words[i]);
    }

    if (failures == 0) {
        puts("ok");
        return 0;
    }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
