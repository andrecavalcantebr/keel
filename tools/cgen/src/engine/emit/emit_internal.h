/* engine/emit/emit_internal.h — internal to engine/emit/: the state of one
 * k_emit_file call, the edits of a line, the helpers that emit.c and the
 * construction files (array.c, foreach.c, constexpr_block.c, index.c) share.
 * Nothing outside engine/emit/ includes it (emit.h is the API). */
#ifndef CGEN_ENGINE_EMIT_EMIT_INTERNAL_H
#define CGEN_ENGINE_EMIT_EMIT_INTERNAL_H
#include <stdbool.h>
#include <stddef.h>
#include "engine/emit/emit.h"
#include "engine/emit/mangle.h"
#include "engine/emit/writer.h"

#define NAME_CAP 256            /* a generated file name, guard or symbol */
#define LINE_CAP 1024           /* one changed source line, or one prototype */
#define MAX_EDITS 32            /* the edits of one line */

typedef enum {
    EDIT_DROP, EDIT_RENAME, EDIT_ASSERT, EDIT_STATIC, EDIT_TAG,
    EDIT_TEXT, EDIT_ENUMERATOR, EDIT_ENUM_CLOSE, EDIT_TOKS, EDIT_LINES
} EditKind;

/* source bytes [at, at + skip) are replaced: DROP by nothing, RENAME by the
 * mangled name of the declaration whose name is token `name_tok`, ASSERT by
 * _Static_assert, STATIC by `static`; TAG inserts " <mangled name>" (skip is 0);
 * TEXT by `text`; ENUMERATOR (a tag of a `tags`) by <mangled set>_<the tag>,
 * and ENUM_CLOSE by "} <mangled set>", both naming the set by `name_tok`;
 * TOKS by the source text of tokens [tok_a, tok_b) with every name renamed as
 * in the generated C (k_em_put_range), for a copy of source text that a
 * construction moves or repeats (the dimension in `keel_index(i, N)`);
 * LINES splits the output line at `at` around lines of their own, each with the
 * .k line it maps to (backend §5.5, rule 4: the cleanup of a `defer`). `text` is
 * the payload, one record per line of it:
 *     H|<text>         appended to the first half (e.g. "{"); optional, first
 *     L<n>|<text>      an output line of its own, mapped to .k line <n>
 *     T|<text>         starts the second half (its indentation); last
 * The first half is written as its own line unless it is only blanks; the
 * second half continues with the source after the edit and is written by the
 * caller as the line itself. The payload lives in k_em_scratch. */
typedef struct {
    EditKind kind;
    size_t at, skip;
    size_t name_tok;
    bool is_type;
    const char *text;           /* TEXT: a string literal */
    size_t text_len;
    size_t tok_a, tok_b;        /* TOKS */
} Edit;

typedef struct {
    const KAst *ast;
    keel_slice_char module;     /* the declared name, dotted */
    keel_slice_char src_path;   /* the .k path, with extension */
    KEmitOptions opt;
    const KEmitInstance *inst;  /* the instance being written (instance.c), else NULL */
    KWriter w;
    size_t done;               /* source offset: the lines before it are written */
    size_t cache_off, cache_line;   /* line_at's last answer */
    bool ok;                   /* false: a limit was hit, the call is UNSUPPORTED */
    char buf[LINE_CAP];        /* the line being built */
    size_t n;                  /* bytes in buf */
    size_t block_ordinal;      /* unused: the ordinal is computed from the tokens (constexpr_block.c) */
    char scratch[1024];        /* text that edits point to; reset when the edits are copied */
    size_t scratch_n;
} Emit;

/* ---- helpers of emit_core.c (shared inside engine/emit/) ---- */

KToken k_em_tok(const KAst *ast, size_t i);
size_t k_em_tok_off(const KAst *ast, size_t i);
size_t k_em_tok_end(const KAst *ast, size_t i);
size_t k_em_first_off(const Emit *e, const KAstNode *n);
size_t k_em_last_end(const Emit *e, const KAstNode *n);
bool k_em_text_is(KToken t, const char *s);
/* `pub` or `priv`, the only visibility words a declaration starts with (spec §4.1) */
bool k_em_is_visibility(KToken t);
bool k_em_is_keel(keel_slice_char m);

/* A generic module (`module coll type T;`) keeps in its own headers what names no
 * binder and no modifier; the rest belongs to the instances (backend §4.4.1). */
bool k_em_module_is_generic(const KAst *ast);
bool k_em_generic_out(const KAst *ast, const KAstNode *n);

/* the start of the line holding `off`, and just past the newline ending it */
size_t k_em_line_start(const Emit *e, size_t off);
size_t k_em_line_end(const Emit *e, size_t off);
/* the .k line of `off`; the scan starts from the last answer */
size_t k_em_line_at(Emit *e, size_t off);

/* the line builder: bytes and mangled names are written at the end of buf */
void k_em_put_bytes(Emit *e, const char *p, size_t len);
void k_em_put_mangled(Emit *e, size_t name_tok, bool is_type);
void k_em_push_edit(Emit *e, Edit *ed, size_t *cnt, Edit x);
/* `n` bytes of scratch for an edit's text (valid until the edits are copied);
 * NULL and e->ok = false when the line needs more than the scratch holds */
char *k_em_scratch(Emit *e, size_t n);
/* The source text of tokens [tok_a, tok_b), with the gaps between them, and
 * every name renamed as in the generated C: a block constexpr by its macro
 * name (k_em_constblock_rename), a module declaration by its mangled name. */
void k_em_put_range(Emit *e, size_t tok_a, size_t tok_b);
/* whether the node's token span intersects the source bytes [a, b) */
bool k_em_overlaps(const Emit *e, const KAstNode *n, size_t a, size_t b);

/* a kind that declares a module-level value: function, variable, constexpr */
bool k_em_declares_value(KAstKind k);
/* The module-level declaration that token `i` names, or NULL. */
const KAstNode *k_em_declared_at(const Emit *e, size_t i);

/* Sorts ed[from..cnt) into the prefix ed[0..from), which must already be sorted
 * by `at` (insertion sort: few edits per line). */
void k_em_sort_edits(Edit *ed, size_t from, size_t cnt);

/* ---- the construction files: one file each (codegen design E5) ----
 * edits_X: the edits this construction makes inside the function `fn`, in the
 * source [a, b), appended to `ed` (cnt counts them).
 * handles_X: whether this construction lowers this island (its kind AND its
 * detail: a `foreach` over a container is not the one over `a..b`), so that
 * islands_supported() accepts it. */

void k_em_edits_array(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_array(const KAst *ast, const KIsland *is);

void k_em_edits_foreach(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_foreach(const KAst *ast, const KIsland *is);

void k_em_edits_index(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_index(const KAst *ast, const KIsland *is);

/* defer.c: `defer` (backend §5.5) */
void k_em_edits_defer(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_defer(const KAst *ast, const KIsland *is);

/* constexpr_block.c: a block-scope constexpr. Its edits, and the whole lines
 * written before / after the source line `ln` (bytes [a, b)). */
void k_em_edits_constblock(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_constblock(const KAst *ast, const KIsland *is);
void k_em_pre_lines_constblock(Emit *e, size_t ln, size_t a, size_t b);
void k_em_post_lines_constblock(Emit *e, size_t ln, size_t a, size_t b);
/* The name a use of a block constexpr takes in C11 (`keel__N_0`), written to
 * out[0..cap) with its length in *len; false when token `tok` is not a use of
 * a block constexpr (or the profile is C23, which takes it verbatim). */
bool k_em_constblock_rename(const Emit *e, size_t tok, char *out, size_t cap, size_t *len);
/* false when `fn` has a block constexpr this profile cannot emit yet */
bool k_em_supported_constblock(const Emit *e, const KAstNode *fn);

/* instance.c: a module that places instances with `instance` (linguagem §4.9).
 * places_instances: it has an `instance`; supported_placement: it has nothing
 * else, and each instance it places is one instance.c writes; placement_c:
 * the rest of its .c, after the own include. */
bool k_em_places_instances(const KAst *ast);
bool k_em_supported_placement(const KAst *ast);
void k_em_placement_c(Emit *e);

/* uses.c: the uses of another module's names (backend §4.3.2, rule 4).
 * foreign_island: a type, name or call island whose name this module does not
 * declare; handles_use: one of those that a rename lowers. edits_uses: the
 * renames in [a, b) of node n; use_includes: the #include of the .h of each
 * module and instance the module uses, for its .c. */
bool k_em_foreign_island(const KAst *ast, const KIsland *is);
bool k_em_handles_use(const KAst *ast, const KIsland *is);
bool k_em_handles_implicit_init(const KAst *ast, const KIsland *is);
void k_em_edits_uses(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt);
void k_em_use_includes(Emit *e);
/* the .h: `typedef struct X X;` for another module's type a public signature
 * takes by pointer */
void k_em_h_forward_decls(Emit *e);

#endif /* CGEN_ENGINE_EMIT_EMIT_INTERNAL_H */
