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
    char scratch[2048];        /* text that edits point to; reset when the edits are copied (a one-line parallel
                                * block takes a header of up to 1000 bytes, and its body and `}` after it) */
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
/* whether `n` is a function with a parameter typed by a protocol (backend §5.19) */
bool k_em_over_protocol(const KAst *ast, const KAstNode *n);

/* instance.c, for the instance mode of emit.c (e->inst set): the argument a
 * binder token stands for; the rank suffix `_<N>` of a vector accessor */
bool k_em_binder_arg(const Emit *e, KToken t, keel_slice_char *arg);
/* a type as C writes it, without its top-level `const`/`volatile` */
keel_slice_char k_em_unqualified(keel_slice_char s);
bool k_em_rank_suffix(const Emit *e, const KAstNode *fn, keel_slice_char *value);
/* how the modifier's fields hold type binder k: 0 not, 1 by pointer only, 2 by value */
int k_em_binder_use(const Emit *e, size_t k);
/* The symbol of the module instance when it is a unit of its own (KEmitInstance.unit_buf):
 * written into buf, 0 when the instance is the modifier's alone. verb_on_modifier: whether
 * the function takes the modifier itself as a parameter's type (`const slot *s`), and so
 * carries the modifier's symbol; any other verb of such an instance carries the unit's. */
size_t k_em_unit_symbol(const KAst *user, const KClosure *c, char *buf, size_t cap);
/* whether `d` is the typedef that names the instance's unit (backend §5.10, rule 1) */
bool k_em_unit_typedef(const Emit *e, const KAstNode *d);
bool k_em_verb_on_modifier(const Emit *e, const KAstNode *fn);
/* emit.c: the .h of the instance e->inst, written by the module emitter over the
 * generic's tree and the instance's islands (k_collect_instance_islands) */
void k_em_instance_h(Emit *e);
/* whether the file being written leaves the declaration `n` out (emit.c) */
bool k_em_skip(const Emit *e, const KAstNode *n);
/* whether `n` belongs to another modifier of the generic than the instance's: a
 * modifier of another name, or a function whose signature names one (the instance
 * of that modifier writes it). False for a module without modifiers, or a protocol instance. */
bool k_em_other_modifier(const Emit *e, const KAstNode *n);
void k_em_instance_c(Emit *e);

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
 * name (k_em_constblock_rename), a module declaration by its mangled name.
 * The comments of the gaps are left out (k_em_put_source). */
void k_em_put_range(Emit *e, size_t tok_a, size_t tok_b);
/* the source bytes [a, b) with their comments left out (backend §6, rule 3) */
void k_em_put_source(Emit *e, size_t a, size_t b);
/* the text written since n0 made the body of a `#define`: ` \` before each line break */
void k_em_continue_from(Emit *e, size_t n0);
void k_em_put_continued(Emit *e, size_t tok_a, size_t tok_b);
/* whether the node's token span intersects the source bytes [a, b) */
bool k_em_overlaps(const Emit *e, const KAstNode *n, size_t a, size_t b);

/* a kind that declares a module-level value: function, variable, constexpr */
bool k_em_declares_value(KAstKind k);
/* The module-level declaration that token `i` names, or NULL. */
const KAstNode *k_em_declared_at(const Emit *e, size_t i);
/* whether an island says token `i` is a name of the module (not a local) */
bool k_em_named_island_at(const KAst *ast, size_t i);

/* The <N> of `keel__c<N>` (backend §2.3, rule 2: a counter per function) at
 * token tok: the `defer [now ...]` captures and the `foreach` over a container
 * (its pointer and length, §5.7) of the body before it. */
size_t k_em_capture_ordinal(const KAst *ast, const KAstNode *fn, size_t tok);

/* Sorts ed[from..cnt) into the prefix ed[0..from), which must already be sorted
 * by `at` (insertion sort: few edits per line). */
void k_em_sort_edits(Edit *ed, size_t from, size_t cnt);

/* emit.c: the edits of every node that overlaps a line (the dispatcher of the constructions) */
void k_em_node_edits(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt);

/* lines.c: a source line with its edits (k_em_copy_edited), written with its .k number
 * (k_em_emit_line); a one-line parallel block in two parts (k_em_emit_split); the lines
 * [a, b) (k_em_emit_range); the comments left out (k_em_comment_edits); the edits a rewrite
 * swallowed (k_em_drop_nested). k_em_spanned_end is the offset the last k_em_copy_edited
 * consumed up to, past a line break an edit spans. k_em_emit_opaque_range is [a, b) with no
 * edits at all, comments included (extern_c content, linguagem §2.1: "atravessa intacto"). */
void k_em_comment_edits(Emit *e, size_t a, size_t b, Edit *ed, size_t *cnt);
void k_em_drop_nested(Edit *ed, size_t *cnt);
void k_em_copy_edited(Emit *e, size_t ln, size_t a, size_t b, const Edit *ed, size_t cnt);
void k_em_emit_line(Emit *e, size_t ln, size_t a, size_t b);
void k_em_emit_split(Emit *e, size_t ln, size_t a, size_t b);
void k_em_emit_range(Emit *e, size_t a, size_t b);
void k_em_emit_opaque_range(Emit *e, size_t a, size_t b);
size_t k_em_spanned_end(void);

/* decls.c: the module-level declarations, each as its file form (k_em_emit_import,
 * k_em_emit_proto, k_em_emit_extern, k_em_emit_constexpr_c11), a `pub inline` function
 * (k_em_is_pub_inline), the edits of a declaration (k_em_decl_edits), and the three
 * forms of extern_c with their text (k_em_extern_c_*) */
bool k_em_is_pub_inline(const KAst *ast, const KAstNode *n);
bool k_em_decl_edits(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt);
void k_em_emit_import(Emit *e, const KAstNode *n, size_t a);
void k_em_emit_proto(Emit *e, const KAstNode *n);
void k_em_emit_extern(Emit *e, size_t i);
void k_em_emit_constexpr_c11(Emit *e, const KAstNode *n);
bool k_em_extern_c_type_h(const KAst *ast, const KAstNode *n);
bool k_em_extern_c_priv(const KAst *ast, const KAstNode *n);
void k_em_extern_c_lines(const Emit *e, const KAstNode *n, size_t *a, size_t *b);
bool k_em_extern_c_inline(const KAst *ast, const KAstNode *n);
void k_em_extern_c_text(Emit *e, const KAstNode *n);

/* generic.c: whether an instance's .h includes the module's .type.h (backend §4.4.1, rule 4) */
bool k_em_uses_share(const Emit *e);

/* supported.c: whether the slice of emit.h covers every node of the module */
bool k_em_supported(const Emit *e);

/* decls.c: whether k_em_supported's own switch, one function per K_AST_* kind,
 * named like the k_em_handles_* of an island (below) */
bool k_em_handles_import_c(const KAst *ast, const KAstNode *n);
bool k_em_handles_type(const KAst *ast, const KAstNode *n, const KAstNode *prev);
bool k_em_handles_tags(const KAst *ast, const KAstNode *n);
bool k_em_handles_constexpr(const KAst *ast, const KAstNode *n);
bool k_em_handles_variable(const KAst *ast, const KAstNode *n);
bool k_em_handles_extern_c(const Emit *e, const KAstNode *n);
bool k_em_handles_function(const Emit *e, const KAstNode *n);

/* use_paths.c: the header a symbol another module owns lives in (backend §4.3.2,
 * rules 2, 3 and 4), and the small library around it; shared with uses.c */
#define MAX_INCLUDES 32
typedef struct { char path[MAX_INCLUDES][NAME_CAP]; size_t len[MAX_INCLUDES]; size_t n; } Includes;
bool k_em_plain_arg(const Emit *e, const char *sym, size_t len);
size_t k_em_arg_header(const Emit *e, const char *sym, size_t len, const char *ext, char *path, size_t cap);
bool k_em_own_instance(const Emit *e, const char *sym, size_t len);
bool k_em_sym_is_tags_named(const KAst *ast, const char *sym, size_t len, const char *written, size_t wlen);
size_t k_em_header_of(const KAst *ast, const char *sym, size_t len, const char *ext, char *path, size_t cap);
void k_em_raw_include(Emit *e, const char *path, size_t len);
bool k_em_add_include(Includes *s, const char *path, size_t len);
size_t k_em_own_type_path(const Emit *e, char *path, size_t cap);
size_t k_em_import_rank(const KAst *ast, const char *sym, size_t len);
/* uses.c: a type or constant of the generic the instance declares (backend §4.3,
 * rule 1), shared with use_paths.c's k_em_own_instance */
bool k_em_own_declaration(const Emit *e, const char *sym, size_t len);

/* defer_scope.c: the scope and jump target of a token inside a function's
 * body (backend §5.5, §4.6 rule 9); shared with defer.c */
bool k_em_word_like(const KAst *ast, size_t i);
bool k_em_same_text(KToken a, KToken b);
size_t k_em_close_square(const KAst *ast, size_t open);
const KAstNode *k_em_fn_of(const KAst *ast, size_t tok);
size_t k_em_block_open(const KAst *ast, size_t from, size_t lo);
size_t k_em_block_close(const KAst *ast, size_t open);
size_t k_em_exit_target(const KAst *ast, const KAstNode *fn, size_t p, bool want_switch);
bool k_em_at_fn_scope(const KAst *ast, const KAstNode *fn, size_t j);
size_t k_em_goto_target(const KAst *ast, const KAstNode *fn, size_t g);

/* ---- the construction files: one file each (codegen design E5) ----
 * edits_X: the edits this construction makes inside the function `fn`, in the
 * source [a, b), appended to `ed` (cnt counts them).
 * handles_X: whether this construction lowers this island (its kind AND its
 * detail: a `foreach` over a container is not the one over `a..b`), so that
 * islands_supported() accepts it. */

void k_em_edits_array(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_array(const KAst *ast, const KIsland *is);
/* the `array` fields of a struct type n (no island): lowered like a declaration, and whether
 * every one is a form the file lowers (supported() refuses the module when one is not) */
void k_em_edits_struct_arrays(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_supported_struct_arrays(const KAst *ast, const KAstNode *n);

void k_em_edits_foreach(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_foreach(const KAst *ast, const KIsland *is);

void k_em_edits_index(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_index(const KAst *ast, const KIsland *is);
/* the `array` declaration a plain name refers to in function fn (its own, a parameter's,
 * or the module's), for the verbs of the core: the token spans of its dimensions. */
#define K_ARRAY_RANK 8
typedef struct {
    size_t arr;                                         /* the word `array` */
    size_t lbr, rbr;                                    /* the brackets of its own declarator */
    size_t type_b;                                      /* the element type is [arr + 1, type_b): the first name */
    size_t dim_first[K_ARRAY_RANK], dim_end[K_ARRAY_RANK];
    size_t ndims;
    bool binder;                                        /* dimension 0 is `T n` of a parameter (§5.18) */
} KArrayRef;
bool k_em_array_of(const KAst *ast, const KAstNode *fn, size_t name, KArrayRef *out);
/* dimension k of the array, as a decimal (a literal, or a constexpr of one): *v is set */
bool k_em_array_dim(const KAst *ast, const KAstNode *fn, const KArrayRef *r, size_t k, long *v);
/* the name at token t as a constexpr of a decimal literal, seen from token `at` (index.c): *v is set */
bool k_em_constexpr_value(const KAst *ast, const KAstNode *fn, size_t at, size_t t, long *v);

/* roles.c: the roles, the erased `type T`, the arity suffix of a function's signature.
 * k_em_arity_suffix writes the digits (or nothing) into out[8] and returns their count. */
void k_em_edits_roles(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
size_t k_em_arity_suffix(const KAst *ast, const KAstNode *fn, char *out);
/* whether the tree declares the function `name` in more than one arity (backend §2.1.1, rule 1) */
bool k_em_name_overloaded(const KAst *ast, const char *name, size_t len);
/* the parameters written in keel: `type T` is one (backend §2.1.1, rule 5) */
size_t k_em_param_count(const KAst *ast, const KAstNode *fn);
bool k_em_nodiscard(const KAst *ast, const KAstNode *fn);
void k_em_edits_range_check(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);

/* else.c: the `else` of a result (spec §4.10, backend §5.12) */
void k_em_edits_else(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
/* the W(&x, of a default whose value is on a later line: after uses.c, see else.c */
void k_em_edits_else_tail(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_else(const KAst *ast, const KIsland *is);
/* parallel.c: the `parallel` block, the `win;`/`fail;` of its worker and the interrupted
 * query (backend §5.9). k_em_parallel_interrupted writes the edit for a call island and
 * answers true, or answers false and leaves the island to uses.c. */
void k_em_edits_parallel(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
void k_em_edits_worker_exit(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_parallel(const KAst *ast, const KIsland *is);
bool k_em_handles_worker_exit(const KAst *ast, const KIsland *is);
/* the offset where the body of a one-line parallel block begins, when its header lies in [a, b);
 * 0 otherwise. emit_range writes the line in two parts there, so the body gets its own #line. */
size_t k_em_parallel_cut(const KAst *ast, size_t a, size_t b);
bool k_em_parallel_interrupted(Emit *e, const KIsland *is, size_t a, size_t b, Edit *ed, size_t *cnt);
/* match.c: the `match` of a tagged label (backend §5.6) */
void k_em_edits_match(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_match(const KAst *ast, const KIsland *is);

/* subscript.c: `x[i]` and `x[a..b]` over a modifier instance (backend §5.3) */
void k_em_edits_subscript(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_subscript(const KAst *ast, const KIsland *is);
void k_em_edits_rangelit(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);

/* defer.c: `defer` (backend §5.5) */
void k_em_edits_defer(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_defer(const KAst *ast, const KIsland *is);

/* core.c: the verbs of the core, which are not calls to a module
 * (`keel.length` over an `array`, backend §5.3 rule 7). */
void k_em_edits_core(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_core(const KAst *ast, const KIsland *is);

/* extent.c: `extent` (backend §5.15). The declaration's edits; whether a node is
 * one; and the column accessors, written into the .h (pub) or the .c (priv):
 * the prototypes in declaration order, the bodies in the section of the
 * `pub inline` ones. */
void k_em_edits_extent(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_is_extent(const KAst *ast, const KAstNode *n);
void k_em_extent_protos(Emit *e, const KAstNode *n);
void k_em_extent_bodies(Emit *e, const KAstNode *n);
/* the use point: `P.col[…]` -> `*f(&P, …)`, `P->col[…]` -> `*f(P, …)` (rule 5) */
void k_em_edits_column(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt);
bool k_em_handles_column(const KAst *ast, const KIsland *is);
bool k_em_column_set(const KAst *ast, const KIsland *is, char *out, size_t cap, size_t *len, size_t *close_tok);

/* routine.c: the explicit zero of a modifier's field in a brace initializer
 * (backend §5.10, rule 2). It adds text to a line and refuses nothing, so there
 * is no handles_X: the island it reads is a type island, already accepted. */
void k_em_edits_routine(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt);

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
bool k_em_handles_ref(const KAst *ast, const KIsland *is);
void k_em_edits_uses(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt);
void k_em_use_includes(Emit *e);
/* the .h, section 1: the .type.h of another module's type a public signature
 * takes by value */
void k_em_h_type_includes(Emit *e);
/* the .h: `typedef struct X X;` for another module's type a public signature
 * takes by pointer */
void k_em_h_forward_decls(Emit *e);
/* .h, section 3: the .h of each module and instance the `pub inline` bodies
 * call, in the order of the path; whether any was written */
bool k_em_h_body_includes(Emit *e);
/* .type.h: the .type.h of the types of other modules the struct fields hold by value */
void k_em_type_h_includes(Emit *e);
/* .type.h of an instance: the .type.h of what its typedefs and modifier name (another
 * module's type, a type argument that is a type of the program), in order of first use */
void k_em_instance_type_includes(Emit *e);
typedef struct {
    const char *sym; size_t sym_len;   /* NULL: the tokens stay (an own call) */
    size_t end;                        /* the token after the written name */
    size_t amp[16]; size_t namp;       /* the arguments' first tokens that take `&` */
    size_t star[16]; size_t nstar;     /* the arguments' first tokens that take `*` (spec §4.4, rule 5) */
} KRangeUse;
int k_em_range_use(const KAst *ast, size_t i, KRangeUse *r);
/* the symbol a type island resolved the written type `spelled` to */
/* whether the instance with this symbol is of a modifier that is not `byref`: a value, copied into a temporary */
bool k_em_type_by_value(const KAst *ast, const char *sym, size_t len);
/* the symbol of the type island anchored at token `at`, if one starts there */
bool k_em_type_island_at(const KAst *ast, size_t at, const char **sym, size_t *sym_len);
/* the dimension 0 of the `array` variable at token tok, as written (a decimal), or its binder (uses.c) */
bool k_em_array_dim0(const KAst *ast, size_t tok, const char **p, size_t *len, bool *binder);
bool k_em_type_symbol(const KAst *ast, const char *spelled, size_t len, const char **sym, size_t *sym_len);

#endif /* CGEN_ENGINE_EMIT_EMIT_INTERNAL_H */
