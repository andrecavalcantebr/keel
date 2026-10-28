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

#define K_EMIT_INSTANCE_ARGS 4

/* One instance of a generic module (codegen design §7; backend §4.3, §4.4.1):
 * the C symbols the parser's closure already computed. */
typedef struct {
    keel_slice_char symbol;     /* the instance's own symbol: "coll_stack_i32" */
    keel_slice_char module;     /* the generic module's declared name: "coll" */
    keel_slice_char modifier;   /* the modifier it instances: "stack" */
    size_t argc;
    keel_slice_char arg[K_EMIT_INSTANCE_ARGS];   /* the C symbol of each type binder, in order: "i32" */
    size_t dimc;
    keel_slice_char dim[K_EMIT_INSTANCE_ARGS];   /* the value of each dimension binder, in decimal: "3" */
} KEmitInstance;

/* The instance a closure entry of `user` names (its arguments live in the
 * closure's text; the dimensions, in the symbol: `grids_grid_3_f32` is the
 * modifier's name, each dimension and each type argument, backend §2.2).
 * False when the entry is not an instance of a module that has a tree. */
bool k_emit_instance_of(const KAst *user, const KClosure *c, KEmitInstance *out);

/* `module` is the declared name with dots ("app.cfg"); `src_path` is the .k
 * path relative to its root, with the extension ("app/cfg.k"), the string the
 * `#line` and the header comment carry. */
size_t k_emit_file(const KAst *ast, keel_slice_char module, keel_slice_char src_path,
                   KEmitFile which, KEmitOptions opt, keel_slice_char out);

/* The .type.h (K_EMIT_TYPE_H) or the .h (K_EMIT_H) of an instance, from the
 * generic module's tree: the modifier, and every verb of the module (§7.2: the
 * instance comes out whole), with the binders replaced by the arguments and the
 * names by the instance's. The file's path is k_mangle_instance_path. The
 * non-inline `pub` bodies belong to the module that places the instance. */
size_t k_emit_instance(const KAst *generic, const KEmitInstance *inst, keel_slice_char src_path,
                       KEmitFile which, KEmitOptions opt, keel_slice_char out);

#endif /* CGEN_ENGINE_EMIT_EMIT_H */
