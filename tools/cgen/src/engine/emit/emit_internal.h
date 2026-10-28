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
    EDIT_TEXT, EDIT_ENUMERATOR, EDIT_ENUM_CLOSE
} EditKind;

/* source bytes [at, at + skip) are replaced: DROP by nothing, RENAME by the
 * mangled name of the declaration whose name is token `name_tok`, ASSERT by
 * _Static_assert, STATIC by `static`; TAG inserts " <mangled name>" (skip is 0);
 * TEXT by `text`; ENUMERATOR (a tag of a `tags`) by <mangled set>_<the tag>,
 * and ENUM_CLOSE by "} <mangled set>", both naming the set by `name_tok` */
typedef struct {
    EditKind kind;
    size_t at, skip;
    size_t name_tok;
    bool is_type;
    const char *text;           /* TEXT: a string literal */
    size_t text_len;
} Edit;

typedef struct {
    const KAst *ast;
    keel_slice_char module;     /* the declared name, dotted */
    keel_slice_char src_path;   /* the .k path, with extension */
    KEmitOptions opt;
    KWriter w;
    size_t done;               /* source offset: the lines before it are written */
    size_t cache_off, cache_line;   /* line_at's last answer */
    bool ok;                   /* false: a limit was hit, the call is UNSUPPORTED */
    char buf[LINE_CAP];        /* the line being built */
    size_t n;                  /* bytes in buf */
    size_t block_ordinal;      /* the constexpr blocks of the module so far (constexpr_block.c) */
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

/* the start of the line holding `off`, and just past the newline ending it */
size_t k_em_line_start(const Emit *e, size_t off);
size_t k_em_line_end(const Emit *e, size_t off);
/* the .k line of `off`; the scan starts from the last answer */
size_t k_em_line_at(Emit *e, size_t off);

/* the line builder: bytes and mangled names are written at the end of buf */
void k_em_put_bytes(Emit *e, const char *p, size_t len);
void k_em_put_mangled(Emit *e, size_t name_tok, bool is_type);
void k_em_push_edit(Emit *e, Edit *ed, size_t *cnt, Edit x);

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
 * handles_X: whether this construction lowers the island kind, so that
 * islands_supported() accepts it. */

void k_em_edits_array(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_array(KIslandKind kind);

void k_em_edits_foreach(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_foreach(KIslandKind kind);

void k_em_edits_index(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_index(KIslandKind kind);

/* constexpr_block.c: a block-scope constexpr. Its edits, and the whole lines
 * written before / after the source line `ln` (bytes [a, b)). */
void k_em_edits_constblock(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_constblock(KIslandKind kind);
void k_em_pre_lines_constblock(Emit *e, size_t ln, size_t a, size_t b);
void k_em_post_lines_constblock(Emit *e, size_t ln, size_t a, size_t b);
/* false when `fn` has a block constexpr this profile cannot emit yet */
bool k_em_supported_constblock(const Emit *e, const KAstNode *fn);

#endif /* CGEN_ENGINE_EMIT_EMIT_INTERNAL_H */
