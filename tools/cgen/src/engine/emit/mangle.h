/* engine/emit/mangle.h — file-level names (backend §2.1, §4.1; cgen design
 * §5.3): the module's symbol, a declaration's symbol, the generated file's
 * path, its include guard, and the entry unit's name. Instance names (type
 * arguments, arity suffixes, `_Atomic`) are M5 and are not here.
 *
 * Every function writes into `out` and returns the length the result needs;
 * bytes past out.len are dropped (the convention of `k_dump_ast`). Nothing is
 * NUL-terminated. `module` is the declared name with dots: "app.cfg". */
#ifndef CGEN_ENGINE_EMIT_MANGLE_H
#define CGEN_ENGINE_EMIT_MANGLE_H
#include <stdbool.h>
#include <stddef.h>
#include "keel/keel_slice_char.type.h"

/* "app.cfg" -> "app_cfg"; "keel" -> "keel"; "net.http.server" -> "net_http_server". */
size_t k_mangle_module(keel_slice_char module, keel_slice_char out);

/* The C name of a declaration of the module: "<module symbol>_<name>".
 * "app.cfg", "MAX" -> "app_cfg_MAX"; "hello", "main" -> "hello_main".
 * For a type or a modifier (`is_type`), a name equal to the module's last
 * component is not repeated (backend §2.1, rule 2): "keel.arena", "arena" ->
 * "keel_arena". A function or variable of that name is concatenated as usual:
 * "app.cfg", "cfg" -> "app_cfg_cfg".
 * The prelude keeps the spelling (backend §4.2, rule 1): "keel", "i8" -> "i8". */
size_t k_mangle_decl(keel_slice_char module, keel_slice_char name, bool is_type, keel_slice_char out);

/* The path of a generated file, relative to --dest-dir (backend §4.1, rules 5
 * and 6): the parent components are directories, the file is the symbol.
 * `ext` is ".type.h", ".h" or ".c".
 * "app.cfg", ".type.h" -> "app/app_cfg.type.h"; "keel", ".h" -> "keel.h";
 * "net.http.server", ".c" -> "net/http/net_http_server.c". */
size_t k_mangle_path(keel_slice_char module, const char *ext, keel_slice_char out);

/* The path of an instance's file: the generic module's directory and the
 * instance's own symbol. "coll", "coll_stack_i32", ".h" -> "coll_stack_i32.h";
 * "keel.buffer", "keel_buffer_i32", ".type.h" -> "keel/keel_buffer_i32.type.h". */
size_t k_mangle_instance_path(keel_slice_char module, keel_slice_char symbol, const char *ext, keel_slice_char out);

/* The include guard of a path (cgen design §5.3): upper case, and every
 * byte outside [A-Z0-9] becomes '_'.
 * "app/app_cfg.type.h" -> "APP_APP_CFG_TYPE_H"; "keel.h" -> "KEEL_H". */
size_t k_mangle_guard(keel_slice_char path, keel_slice_char out);

/* The entry unit of --main (backend §5.8), at the root of --dest-dir:
 * "app.cx" -> "main_app_cx.c"; "hello" -> "main_hello.c". */
size_t k_mangle_entry(keel_slice_char module, keel_slice_char out);

#endif /* CGEN_ENGINE_EMIT_MANGLE_H */
