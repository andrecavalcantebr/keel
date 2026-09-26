/* Oracle for the m2-parser-extern-c task. Checks next_out on every case —
 * the exact contract that broke m2-parser-header's first version. Not
 * model-generated. */

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

static void check_next(const char *label, KToken next) {
    if (!EQ(next, "ok")) {
        fprintf(stderr, "FAIL: %s — next_out = \"%.*s\", want \"ok\"\n", label, (int)next.len, next.ptr);
        failures++;
    }
}

int main(void) {
    KLexer lexer;
    TKPpKind pp;
    KToken next;

    /* { } ok  -- braced_opaque directly, empty body */
    KToken open1 = first_token("{ } ok\n", &lexer);
    keel_slice_char body1 = k_scan_braced_opaque(&lexer, open1, &next, &pp);
    if (body1.len != 0) { fprintf(stderr, "FAIL: empty body — len = %zu, want 0\n", body1.len); failures++; }
    check_next("empty body", next);

    /* { x ; } ok  -- one level of content */
    KToken open2 = first_token("{ x ; } ok\n", &lexer);
    keel_slice_char body2 = k_scan_braced_opaque(&lexer, open2, &next, &pp);
    if (!EQ(body2, "x ;")) { fprintf(stderr, "FAIL: flat body = \"%.*s\", want \"x ;\"\n", (int)body2.len, body2.ptr); failures++; }
    check_next("flat body", next);

    /* { a { b } c } ok  -- nested braces must not stop the scan early */
    KToken open3 = first_token("{ a { b } c } ok\n", &lexer);
    keel_slice_char body3 = k_scan_braced_opaque(&lexer, open3, &next, &pp);
    if (!EQ(body3, "a { b } c")) { fprintf(stderr, "FAIL: nested body = \"%.*s\", want \"a { b } c\"\n", (int)body3.len, body3.ptr); failures++; }
    check_next("nested body", next);

    /* extern_c { fclose(fp); } ok  -- no type_h */
    KToken ekw1 = first_token("extern_c { fclose(fp); } ok\n", &lexer);
    KExternCDecl ec1;
    k_scan_extern_c(&lexer, ekw1, &ec1, &next, &pp);
    if (ec1.has_type_h) { fprintf(stderr, "FAIL: no type_h — has_type_h = true\n"); failures++; }
    if (!EQ(ec1.body, "fclose(fp);")) { fprintf(stderr, "FAIL: extern_c body = \"%.*s\"\n", (int)ec1.body.len, ec1.body.ptr); failures++; }
    check_next("extern_c, no type_h", next);

    /* extern_c [type_h] { typedef int t; } ok  -- with type_h */
    KToken ekw2 = first_token("extern_c [type_h] { typedef int t; } ok\n", &lexer);
    KExternCDecl ec2;
    k_scan_extern_c(&lexer, ekw2, &ec2, &next, &pp);
    if (!ec2.has_type_h) { fprintf(stderr, "FAIL: with type_h — has_type_h = false\n"); failures++; }
    if (!EQ(ec2.body, "typedef int t;")) { fprintf(stderr, "FAIL: extern_c[type_h] body = \"%.*s\"\n", (int)ec2.body.len, ec2.body.ptr); failures++; }
    check_next("extern_c, with type_h", next);

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
