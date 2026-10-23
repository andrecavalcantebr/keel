/* engine/emit/mangle.c — file-level names (mangle.h; backend §2.1, §4.1,
 * §4.2, §5.8; cgen design §5.3). Plain concatenation, no table. No I/O. */
#include <string.h>
#include "engine/emit/mangle.h"

typedef struct { keel_slice_char out; size_t n; } KOut;
static void put_c(KOut *o, char c) {
    if (o->n < o->out.len) o->out.ptr[o->n] = c;
    o->n++;
}
static void put_s(KOut *o, const char *s) { while (*s) put_c(o, *s++); }
static void put_slice(KOut *o, keel_slice_char s) {
    for (size_t i = 0; i < s.len; i++) put_c(o, s.ptr[i]);
}

/* the module symbol: "app.cfg" -> "app_cfg" (backend §2.1, rule 1) */
static void put_module_symbol(KOut *o, keel_slice_char module) {
    for (size_t i = 0; i < module.len; i++) put_c(o, module.ptr[i] == '.' ? '_' : module.ptr[i]);
}

/* the prelude module: the whole name is "keel" (backend §4.2) */
static bool is_prelude(keel_slice_char module) {
    return module.len == 4 && memcmp(module.ptr, "keel", 4) == 0;
}

/* where the last component of module starts: 0, or just after the last '.' */
static size_t last_component_start(keel_slice_char module) {
    size_t start = 0;
    for (size_t i = 0; i < module.len; i++)
        if (module.ptr[i] == '.') start = i + 1;
    return start;
}

/* the last component of module equals name (exact spelling) */
static bool is_last_component(keel_slice_char module, keel_slice_char name) {
    size_t start = last_component_start(module);
    size_t len = module.len - start;
    return len == name.len && memcmp(module.ptr + start, name.ptr, len) == 0;
}

size_t k_mangle_module(keel_slice_char module, keel_slice_char out) {
    KOut o = { out, 0 };
    put_module_symbol(&o, module);
    return o.n;
}

size_t k_mangle_decl(keel_slice_char module, keel_slice_char name, bool is_type, keel_slice_char out) {
    KOut o = { out, 0 };
    if (is_prelude(module)) {
        put_slice(&o, name);                       /* "i8" stays "i8" */
    } else if (is_type && is_last_component(module, name)) {
        put_module_symbol(&o, module);             /* "keel.arena", "arena" -> keel_arena */
    } else {
        put_module_symbol(&o, module);
        put_c(&o, '_');
        put_slice(&o, name);
    }
    return o.n;
}

size_t k_mangle_path(keel_slice_char module, const char *ext, keel_slice_char out) {
    KOut o = { out, 0 };
    size_t start = last_component_start(module);
    /* the parent components are directories: "net.http.server" -> "net/http/" */
    if (start != 0) {
        for (size_t i = 0; i < start - 1; i++) put_c(&o, module.ptr[i] == '.' ? '/' : module.ptr[i]);
        put_c(&o, '/');
    }
    put_module_symbol(&o, module);
    put_s(&o, ext);
    return o.n;
}

size_t k_mangle_guard(keel_slice_char path, keel_slice_char out) {
    KOut o = { out, 0 };
    for (size_t i = 0; i < path.len; i++) {
        char c = path.ptr[i];
        if (c >= 'a' && c <= 'z') put_c(&o, (char)(c - 'a' + 'A'));
        else if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) put_c(&o, c);
        else put_c(&o, '_');
    }
    return o.n;
}

size_t k_mangle_entry(keel_slice_char module, keel_slice_char out) {
    KOut o = { out, 0 };
    put_s(&o, "main_");
    put_module_symbol(&o, module);
    put_s(&o, ".c");
    return o.n;
}
