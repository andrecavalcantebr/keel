/* Hand-written driver for tools/harness/tasks/m1-token-predicates-shape.md.
 * Not model-generated. */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "keel/keel_slice_char.type.h"

typedef keel_slice_char KToken;

bool k_token_is_number(KToken t);
bool k_token_is_string(KToken t);
bool k_token_is_char(KToken t);
bool k_token_is_punct(KToken t, const char *spelling);

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
    CHECK(k_token_is_number(tok("42")), "42 is a number");
    CHECK(k_token_is_number(tok("0x1.fp3")), "hex float is a number");
    CHECK(k_token_is_number(tok(".5")), ".5 is a number");
    CHECK(k_token_is_number(tok("1e-6")), "1e-6 is a number");
    CHECK(!k_token_is_number(tok("x42")), "x42 is not a number");
    CHECK(!k_token_is_number(tok("..")), ".. is not a number (range, not a number)");

    CHECK(k_token_is_string(tok("\"hi\"")), "plain string literal");
    CHECK(k_token_is_string(tok("u8\"hi\"")), "u8-prefixed string literal");
    CHECK(k_token_is_string(tok("L\"hi\"")), "L-prefixed string literal");
    CHECK(!k_token_is_string(tok("'x'")), "a char literal is not a string");
    CHECK(!k_token_is_string(tok("hi")), "a bare ident is not a string");

    CHECK(k_token_is_char(tok("'x'")), "plain char literal");
    CHECK(k_token_is_char(tok("u'x'")), "u-prefixed char literal");
    CHECK(k_token_is_char(tok("U'x'")), "U-prefixed char literal");
    CHECK(k_token_is_char(tok("L'x'")), "L-prefixed char literal");
    CHECK(!k_token_is_char(tok("u8'x'")), "u8 is not a valid char prefix in C");
    CHECK(!k_token_is_char(tok("\"x\"")), "a string literal is not a char");

    CHECK(k_token_is_punct(tok("->"), "->"), "-> matches itself");
    CHECK(!k_token_is_punct(tok("->"), "-"), "-> does not match a shorter prefix");
    CHECK(k_token_is_punct(tok("..."), "..."), "... matches itself");
    CHECK(!k_token_is_punct(tok(".."), "..."), ".. does not match ...");

    KToken empty = { .len = 0, .ptr = "" };
    CHECK(!k_token_is_number(empty), "empty token is not a number");
    CHECK(!k_token_is_string(empty), "empty token is not a string");
    CHECK(!k_token_is_char(empty), "empty token is not a char");

    if (failures == 0) {
        puts("ok");
        return 0;
    }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
