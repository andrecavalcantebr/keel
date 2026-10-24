/* engine/emit/emit.h — the module emitter of M3 (codegen-design §2; backend
 * §4.1, §4.3, §5.8, §6). One call produces one generated file from the parsed
 * module. The slice of the language it covers today is the one the hello
 * oracle and the prelude exercise: `module`, `import_c`, `typedef`, opaque
 * top-level C, `extern_c [type_h]`, protocols (which emit no C) and plain
 * functions. Anything else is K_EMIT_UNSUPPORTED, so a module that needs more
 * is refused instead of mistranslated.
 *
 * Memory: the convention of `k_dump_ast` and the KWriter — the caller hands an
 * output slice, bytes past its length are counted and dropped, and the return
 * value is the length the file needs (so a first pass with { 0, NULL }
 * measures). Nothing is NUL-terminated. engine/: no I/O, no allocation. */
#ifndef CGEN_ENGINE_EMIT_EMIT_H
#define CGEN_ENGINE_EMIT_EMIT_H
#include <stdbool.h>
#include <stddef.h>
#include "engine/ast.h"

typedef enum {
    K_EMIT_TYPE_H,   /* <symbol>.type.h */
    K_EMIT_H,        /* <symbol>.h */
    K_EMIT_C,        /* <symbol>.c, the compiled module */
    K_EMIT_ENTRY     /* main_<symbol>.c, for --main (backend §5.8); `ast` unused */
} KEmitFile;

typedef struct {
    bool c23;        /* the profile: C23 when true, C11 when false (backend §9) */
    bool line_on;    /* false under --line=off: no #line at all */
} KEmitOptions;

#define K_EMIT_UNSUPPORTED ((size_t)-1)

/* `module` is the declared name with dots ("app.cfg"); `src_path` is the .k
 * path relative to its root, with the extension ("app/cfg.k"), the string the
 * `#line` and the header comment carry. */
size_t k_emit_file(const KAst *ast, keel_slice_char module, keel_slice_char src_path,
                   KEmitFile which, KEmitOptions opt, keel_slice_char out);

#endif /* CGEN_ENGINE_EMIT_EMIT_H */
