/* Oracle of engine/emit/mangle.c (file-level names: backend §2.1, §4.1, §4.2,
 * §5.8; cgen design §5.3). Expected values derived by hand from mangle.h.
 * Not model-generated. */
#include <stdio.h>
#include <string.h>
#include "engine/emit/mangle.h"

static int failures = 0;
static keel_slice_char S(const char *s) { return (keel_slice_char){ strlen(s), (char *)s }; }

static void check(const char *name, size_t n, const char *got, const char *want) {
    if (n != strlen(want) || memcmp(got, want, n) != 0) {
        fprintf(stderr, "FAIL: %s: got \"%.*s\" (%zu), want \"%s\"\n", name, (int)(n < 128 ? n : 128), got, n, want);
        failures++;
    }
}

#define T1(fn, a, want) do { char b[128]; memset(b, '#', sizeof b); \
    size_t n = fn(S(a), (keel_slice_char){ sizeof b, b }); check(#fn "(" a ")", n, b, want); } while (0)
#define T2(fn, a, x, want) do { char b[128]; memset(b, '#', sizeof b); \
    size_t n = fn(S(a), x, (keel_slice_char){ sizeof b, b }); check(#fn "(" a ")", n, b, want); } while (0)

int main(void) {
    T1(k_mangle_module, "app.cfg", "app_cfg");
    T1(k_mangle_module, "keel", "keel");
    T1(k_mangle_module, "keel.arena", "keel_arena");
    T1(k_mangle_module, "net.http.server", "net_http_server");

#define TD(a, name, is_type, want) do { char b[128]; memset(b, '#', sizeof b); \
    size_t n = k_mangle_decl(S(a), S(name), is_type, (keel_slice_char){ sizeof b, b }); \
    check("k_mangle_decl(" a ", " name ")", n, b, want); } while (0)
    TD("app.cfg", "MAX", false, "app_cfg_MAX");
    TD("hello", "main", false, "hello_main");
    TD("keel", "i8", true, "i8");                    /* the prelude keeps the spelling */
    TD("keel.arena", "arena", true, "keel_arena");   /* §2.1 rule 2: a type is not repeated */
    TD("keel.arena", "mark", false, "keel_arena_mark");
    TD("app.cfg", "Cfg", true, "app_cfg_Cfg");       /* the rule compares the exact spelling */
    TD("app.cfg", "cfg", false, "app_cfg_cfg");      /* a function is not a type: concatenated */

    T2(k_mangle_path, "app.cfg", ".type.h", "app/app_cfg.type.h");
    T2(k_mangle_path, "app.cfg", ".c", "app/app_cfg.c");
    T2(k_mangle_path, "keel", ".h", "keel.h");
    T2(k_mangle_path, "keel.arena", ".h", "keel/keel_arena.h");
    T2(k_mangle_path, "net.http.server", ".c", "net/http/net_http_server.c");
    T2(k_mangle_path, "hello", ".type.h", "hello.type.h");

    T1(k_mangle_guard, "app/app_cfg.type.h", "APP_APP_CFG_TYPE_H");
    T1(k_mangle_guard, "keel.h", "KEEL_H");
    T1(k_mangle_guard, "keel/keel_arena.type.h", "KEEL_KEEL_ARENA_TYPE_H");
    T1(k_mangle_guard, "a-b/x9.h", "A_B_X9_H");

    T1(k_mangle_entry, "app.cx", "main_app_cx.c");
    T1(k_mangle_entry, "hello", "main_hello.c");

    /* measuring: an empty slice returns the length and writes nothing */
    size_t n = k_mangle_path(S("app.cfg"), ".type.h", (keel_slice_char){ 0, NULL });
    if (n != strlen("app/app_cfg.type.h")) { fprintf(stderr, "FAIL: measure returned %zu\n", n); failures++; }
    /* truncation: 4 bytes hold "app/", the length is still the full one */
    char small[8]; memset(small, '#', sizeof small);
    n = k_mangle_path(S("app.cfg"), ".type.h", (keel_slice_char){ 4, small });
    if (n != strlen("app/app_cfg.type.h") || memcmp(small, "app/", 4) != 0 || small[4] != '#') {
        fprintf(stderr, "FAIL: truncation\n"); failures++; }

    if (failures == 0) { puts("ok"); return 0; }
    return 1;
}
