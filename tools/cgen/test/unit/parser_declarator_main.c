/* Oracle for k_scan_declarator — the whole of keel-spec §2.2's
 * `declarator`. Expected values are hand-derived from the grammar, not
 * from a solving implementation. Not model-generated.
 *
 * The cases that matter are the ones where the declared name is NOT the
 * last IDENT before the terminator: `Vec[TAM]` names Vec, not TAM, and
 * `(*cmp)(void *a, void *b)` names cmp, not b. */
#include <stdio.h>
#include <string.h>
#include "engine/parser.h"

static int failures = 0;
#define EQ(s, want) (strlen(want) == (s).len && memcmp((s).ptr, want, (s).len) == 0)

static KToken first_token(const char *src, KLexer *lexer) {
    keel_slice_char source = { strlen(src), (char *)src };
    TKPpKind pp;
    k_lexer_init(lexer, source, NULL);
    return k_lexer_next(lexer, &pp);
}

/* One accepted declarator: every field checked against the grammar. */
static void ok_case(const char *label, const char *src, const char *want_name,
                    int want_depth, bool want_paren, bool want_array,
                    bool want_func, const char *want_next) {
    KLexer lexer;
    TKPpKind pp;
    KToken next;
    KDeclarator d;
    KToken first = first_token(src, &lexer);
    if (!k_scan_declarator(&lexer, first, &d, &next, &pp)) {
        fprintf(stderr, "FAIL: %s — returned false\n", label);
        failures++;
        return;
    }
    if (!EQ(d.name, want_name)) {
        fprintf(stderr, "FAIL: %s — name = \"%.*s\", want \"%s\"\n",
                label, (int)d.name.len, d.name.ptr, want_name);
        failures++;
    }
    if (d.pointer_depth != want_depth) {
        fprintf(stderr, "FAIL: %s — pointer_depth = %d, want %d\n",
                label, d.pointer_depth, want_depth);
        failures++;
    }
    if (d.parenthesized != want_paren) {
        fprintf(stderr, "FAIL: %s — parenthesized = %d, want %d\n",
                label, (int)d.parenthesized, (int)want_paren);
        failures++;
    }
    if (d.has_array_suffix != want_array) {
        fprintf(stderr, "FAIL: %s — has_array_suffix = %d, want %d\n",
                label, (int)d.has_array_suffix, (int)want_array);
        failures++;
    }
    if (d.has_function_suffix != want_func) {
        fprintf(stderr, "FAIL: %s — has_function_suffix = %d, want %d\n",
                label, (int)d.has_function_suffix, (int)want_func);
        failures++;
    }
    if (!EQ(next, want_next)) {
        fprintf(stderr, "FAIL: %s — next_out = \"%.*s\", want \"%s\"\n",
                label, (int)next.len, next.ptr, want_next);
        failures++;
    }
}

static void reject_case(const char *label, const char *src) {
    KLexer lexer;
    TKPpKind pp;
    KToken next;
    KDeclarator d;
    KToken first = first_token(src, &lexer);
    if (k_scan_declarator(&lexer, first, &d, &next, &pp)) {
        fprintf(stderr, "FAIL: %s — returned true, want false (name \"%.*s\")\n",
                label, (int)d.name.len, d.name.ptr);
        failures++;
    }
}

int main(void) {
    /*        label          source                          name  depth paren array func  next */
    ok_case("plain",         "p rest1\n",                     "p",  0, false, false, false, "rest1");
    ok_case("two stars",     "* * name rest2\n",              "name", 2, false, false, false, "rest2");
    ok_case("qual-c",        "* const * restrict p ;\n",      "p",  2, false, false, false, ";");
    ok_case("ref qual",      "* ref p ;\n",                   "p",  1, false, false, false, ";");

    /* arrays: the name is before the brackets, never inside them */
    ok_case("array num",     "Vec [ 8 ] ;\n",                 "Vec", 0, false, true,  false, ";");
    ok_case("array named",   "Vec [ TAM ] ;\n",               "Vec", 0, false, true,  false, ";");
    ok_case("array 2d",      "Mat [ TAM ] [ OUTRO ] ;\n",     "Mat", 0, false, true,  false, ";");
    ok_case("array nested",  "m [ N [ 2 ] ] ;\n",             "m",   0, false, true,  false, ";");
    ok_case("array empty",   "v [ ] ;\n",                     "v",   0, false, true,  false, ";");

    /* functions and pointers to functions */
    ok_case("function",      "f ( void ) ;\n",                "f",  0, false, false, true,  ";");
    ok_case("fn empty",      "f ( ) ;\n",                     "f",  0, false, false, true,  ";");
    ok_case("fn pointer",    "( * fp ) ( void ) ;\n",         "fp", 0, true,  false, true,  ";");
    ok_case("fn ptr params", "( * cmp ) ( void *a , void *b ) ;\n",
                                                              "cmp", 0, true, false, true,  ";");
    /* pointer to function returning pointer to function */
    ok_case("fn ptr twice",  "( * ( * f ) ( void ) ) ( int ) ;\n",
                                                              "f",   0, true, false, true,  ";");
    ok_case("parens only",   "( x ) ;\n",                     "x",   0, true, false, false, ";");

    /* a comma list stops at the ',' — the caller drives the repetition */
    ok_case("before comma",  "a [ 4 ] , * b ;\n",             "a",   0, false, true,  false, ",");

    /* no IDENT to find */
    reject_case("abstract star", "* ;\n");
    reject_case("abstract array", "[ 4 ] ;\n");
    reject_case("empty source",   "");

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
