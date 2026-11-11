/* engine/emit/emit.h — the module emitter of M3 (codegen-design §2; backend
 * §4.1, §4.3, §5.8, §6). One call produces one generated file from the parsed
 * module. It covers the language the golden cases and the base exercise (see
 * tools/agents/levantamento-2026-11-02.md for the forms still refused). Any
 * form it does not lower is K_EMIT_UNSUPPORTED, so a module that needs more
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
    K_EMIT_ENTRY     /* main_<symbol>.c, for --main (backend §5.8); `ast` is the module's, for the signature of its main */
} KEmitFile;

typedef struct {
    bool c23;        /* the profile: C23 when true, C11 when false (backend §9) */
    bool line_on;    /* false under --line=off: no #line at all */
    bool omp;        /* the `parallel` lowering: the OpenMP directive when true, series when false (backend §5.9.1) */
} KEmitOptions;

#define K_EMIT_UNSUPPORTED ((size_t)-1)

/* A capacity of the emitter ran out (cgen-tool-spec §7.1): the call is refused, and the
 * code names the limit, so the message can give its name and its value. One code per
 * limit; each is a #define of emit_internal.h, whose text k_emit_limit_text gives. */
#define K_EMIT_LIMIT_LINE     ((size_t)-2)   /* LINE_CAP: one generated line, or one prototype */
#define K_EMIT_LIMIT_EDITS    ((size_t)-3)   /* MAX_EDITS: the edits of one source line */
#define K_EMIT_LIMIT_SCRATCH  ((size_t)-4)   /* SCRATCH_CAP: the text the edits point to */
#define K_EMIT_LIMIT_NAME     ((size_t)-5)   /* NAME_CAP: a generated name or path */
#define K_EMIT_LIMIT_INCLUDES ((size_t)-6)   /* MAX_INCLUDES: the includes of one file */
#define K_EMIT_LIMIT_HEADER   ((size_t)-7)   /* PAR_HEADER_CAP: the header of a parallel block */
/* any refusal of the emitter: K_EMIT_UNSUPPORTED or one of the limits above */
#define K_EMIT_IS_FAILURE(n) ((n) >= (size_t)-7)
/* The message of a limit code (its name and value, from the defines): NULL for a code
 * that is not a limit. */
const char *k_emit_limit_text(size_t code);

#define K_EMIT_INSTANCE_ARGS 4

/* One instance of a generic module (codegen design §7; backend §4.3, §4.4.1):
 * the C symbols the parser's closure already computed. */
typedef struct {
    keel_slice_char symbol;     /* the instance's own symbol: "coll_stack_i32" */
    keel_slice_char module;     /* the generic module's declared name: "coll" */
    keel_slice_char modifier;   /* the modifier it instances: "stack" */
    size_t argc;
    keel_slice_char arg[K_EMIT_INSTANCE_ARGS];   /* the C symbol of each type binder, in order: "i32" */
    keel_slice_char arg_module[K_EMIT_INSTANCE_ARGS];   /* an argument that is an instance: its generic module */
    keel_slice_char arg_spell[K_EMIT_INSTANCE_ARGS];    /* how C writes each argument: `const char` (the symbol is `const_char`) */
    size_t dimc;
    keel_slice_char dim[K_EMIT_INSTANCE_ARGS];   /* the value of each dimension binder, in decimal: "3" */
    const KUnavailable *unavailable;   /* the verbs the instance loses (spec §4.3, rules 11 to 13) */
    size_t unavailable_count;
    /* the instance of a function over a protocol on one concrete type (backend
     * §5.19): `symbol` is the file's (keel_slice_of_keel_buffer_i32), `module`
     * the protocol function's; the concrete type, its module, and how its value
     * is passed (by pointer when byref; `T x[], size_t keel__N` over an array,
     * with `proto_elem` the element) */
    keel_slice_char proto_fn, proto_concrete, proto_module, proto_elem;
    bool proto_byref, proto_array;
    /* the module instance as a unit of its own (backend §5.10, rule 1), when the
     * modifier is not named like its module and the module declares a typedef:
     * `keel_routine_ag2_Ag` beside the modifier's `keel_routine_slot_ag2_Ag`.
     * It names the files, the typedef and the verbs whose object is not the
     * modifier; empty when the instance is the modifier's alone. */
    char unit_buf[256];
    size_t unit_len;
    const KAst *user;           /* the tree that asked for the instance: where its arguments are declared */
    char arg_unit_buf[K_EMIT_INSTANCE_ARGS][256];   /* the unit of an argument that is a unit of its own */
    size_t arg_unit_len[K_EMIT_INSTANCE_ARGS];
} KEmitInstance;

/* the symbol that names the instance's files: the unit, else the modifier's */
static inline keel_slice_char k_emit_unit(const KEmitInstance *i) {
    return i->unit_len ? (keel_slice_char){ i->unit_len, (char *)i->unit_buf } : i->symbol;
}

/* the symbol that names the files of the instance argument k */
static inline keel_slice_char k_emit_arg_file(const KEmitInstance *i, size_t k) {
    return i->arg_unit_len[k] ? (keel_slice_char){ i->arg_unit_len[k], (char *)i->arg_unit_buf[k] } : i->arg[k];
}

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

/* Whether every function of the generic's instances is `pub inline`: the
 * instance's own .c has no body (redundant-instance, cgen design §2.8). */
bool k_emit_instance_redundant(const KAst *generic);

/* Whether `generic` declares no modifier (`keel.array`): its instance is the
 * family of verbs of its arguments. `verb` is one of them with a single
 * parameter, the container, which a call can name the instance by. */
bool k_emit_generic_without_modifier(const KAst *generic, keel_slice_char *verb);

/* Whether `ast` declares a function `name` over a protocol (backend §5.19). */
bool k_emit_protocol_function(const KAst *ast, keel_slice_char name);

/* Whether the module has binders on its `module` line. */
bool k_emit_module_is_generic(const KAst *ast);

#endif /* CGEN_ENGINE_EMIT_EMIT_H */
