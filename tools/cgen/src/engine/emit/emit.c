/* engine/emit/emit.c — the module emitter (emit.h; backend §4.1, §4.3, §5.8,
 * §6; codegen-design §8). One call writes one generated file, in the slice
 * emit.h names. No I/O, no allocation: a source line that needs no change goes
 * to the writer as it is; a line that does (visibility dropped, a function
 * mangled, a static_assert spelled for C11, an import_c turned into #include)
 * is built in a fixed stack buffer. A buffer that is too small makes the call
 * UNSUPPORTED, never a truncated file. */
#include <string.h>
#include "engine/emit/emit.h"
#include "engine/emit/mangle.h"
#include "engine/emit/writer.h"

#define EM_DASH "\xE2\x80\x94"  /* U+2014: the dash of every header comment */
#define NAME_CAP 256            /* a generated file name, guard or symbol */
#define LINE_CAP 1024           /* one changed source line, or one prototype */
#define MAX_EDITS 32            /* the edits of one line */

typedef enum { EDIT_DROP, EDIT_RENAME, EDIT_ASSERT, EDIT_STATIC, EDIT_TAG } EditKind;

/* source bytes [at, at + skip) are replaced: DROP by nothing, RENAME by the
 * mangled name of the declaration whose name is token `name_tok`, ASSERT by
 * _Static_assert, STATIC by `static`; TAG inserts " <mangled name>" (skip is 0) */
typedef struct {
    EditKind kind;
    size_t at, skip;
    size_t name_tok;
    bool is_type;
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
} Emit;

static KToken tok(const KAst *ast, size_t i) {
    return keel_buffer_KLexeme_ptr(&ast->tokens, i)->token;
}
static size_t off_of(const KAst *ast, const char *p) { return (size_t)(p - ast->source.ptr); }
static size_t tok_off(const KAst *ast, size_t i) { return off_of(ast, tok(ast, i).ptr); }
static size_t tok_end(const KAst *ast, size_t i) { return tok_off(ast, i) + tok(ast, i).len; }
static size_t first_off(const Emit *e, const KAstNode *n) { return tok_off(e->ast, n->first); }
static size_t last_end(const Emit *e, const KAstNode *n) { return tok_end(e->ast, n->end - 1); }

static bool text_is(KToken t, const char *s) {
    size_t n = strlen(s);
    return t.len == n && memcmp(t.ptr, s, n) == 0;
}
/* `pub` or `priv`, the only visibility words a declaration starts with (spec §4.1) */
static bool is_visibility(KToken t) { return text_is(t, "pub") || text_is(t, "priv"); }
static bool is_keel(keel_slice_char m) { return m.len == 4 && memcmp(m.ptr, "keel", 4) == 0; }

/* the start of the line holding `off`, and just past the newline ending it */
static size_t line_start(const Emit *e, size_t off) {
    const char *s = e->ast->source.ptr;
    while (off > 0 && s[off - 1] != '\n') off--;
    return off;
}
static size_t line_end(const Emit *e, size_t off) {
    const char *s = e->ast->source.ptr;
    size_t len = e->ast->source.len;
    while (off < len && s[off] != '\n') off++;
    return off < len ? off + 1 : len;
}
/* the .k line of `off`; the scan starts from the last answer, so calls in
 * source order cost one pass over the file */
static size_t line_at(Emit *e, size_t off) {
    const char *s = e->ast->source.ptr;
    if (off < e->cache_off) { e->cache_off = 0; e->cache_line = 1; }
    for (size_t i = e->cache_off; i < off; i++)
        if (s[i] == '\n') e->cache_line++;
    e->cache_off = off;
    return e->cache_line;
}

static bool overlaps(const Emit *e, const KAstNode *n, size_t a, size_t b) {
    return first_off(e, n) < b && last_end(e, n) > a;
}

/* ---- the line builder ---- */

static void put_bytes(Emit *e, const char *p, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (e->n >= LINE_CAP) { e->ok = false; return; }
        e->buf[e->n++] = p[i];
    }
}

/* the mangled name of a declaration, written at the end of buf */
static void put_mangled(Emit *e, size_t name_tok, bool is_type) {
    size_t room = LINE_CAP - e->n;
    size_t need = k_mangle_decl(e->module, tok(e->ast, name_tok), is_type,
                                (keel_slice_char){ room, e->buf + e->n });
    if (need > room) { e->ok = false; return; }
    e->n += need;
}

/* Adds the edits that `node` makes inside [a, b) (spec §4.1; backend §4.1).
 * Edits come in source order: a node's visibility precedes its tokens. */
static void push_edit(Emit *e, Edit *ed, size_t *cnt, Edit x) {
    if (*cnt >= MAX_EDITS) { e->ok = false; return; }
    ed[(*cnt)++] = x;
}

static bool declares_value(KAstKind k) {
    return k == K_AST_FUNCTION || k == K_AST_VARIABLE || k == K_AST_CONSTEXPR;
}

/* The module-level declaration that token `i` names, or NULL. A name after
 * `.` or `->` is a member, not the declaration. Scope is not tracked: a local
 * or parameter spelled like a module declaration is renamed too (a limit of
 * the slice, M5 brings the symbol table). */
static const KAstNode *declared_at(const Emit *e, size_t i) {
    const KAst *ast = e->ast;
    if (i > 0 && (text_is(tok(ast, i - 1), ".") || text_is(tok(ast, i - 1), "->"))) return NULL;
    KToken t = tok(ast, i);
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *d = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (d->kind != K_AST_TYPE && d->kind != K_AST_TAGS && !declares_value(d->kind)) continue;
        if (d->name_first >= d->name_end) continue;
        KToken n = tok(ast, d->name_first);
        if (n.len == t.len && memcmp(n.ptr, t.ptr, t.len) == 0) return d;
    }
    return NULL;
}

static void node_edits(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    bool value = declares_value(n->kind);
    if (n->kind == K_AST_TYPE || value) {
        size_t at = tok_off(ast, n->first);
        if (is_visibility(tok(ast, n->first)) && at >= a && at < b) {
            if (value && text_is(tok(ast, n->first), "priv")) {
                /* a priv function or variable is internal linkage in the .c */
                push_edit(e, ed, cnt, (Edit){ EDIT_STATIC, at, tok(ast, n->first).len, 0, false });
            } else {
                /* the keyword and the blanks after it */
                size_t skip = tok(ast, n->first).len;
                while (at + skip < b && (ast->source.ptr[at + skip] == ' ' || ast->source.ptr[at + skip] == '\t'))
                    skip++;
                push_edit(e, ed, cnt, (Edit){ EDIT_DROP, at, skip, 0, false });
            }
        }
    }
    if (!is_keel(e->module)) {   /* the prelude's names are not prefixed (backend §4.2) */
        if (n->kind == K_AST_TYPE) {
            /* only the declarator is a module name; the members are not */
            size_t k = n->first + (is_visibility(tok(ast, n->first)) ? 1 : 0);
            if (k + 2 < n->end && text_is(tok(ast, k), "typedef") && text_is(tok(ast, k + 1), "struct") &&
                text_is(tok(ast, k + 2), "{") && n->name_first < n->name_end) {
                size_t at = tok_end(ast, k + 1);
                if (at >= a && at < b)
                    push_edit(e, ed, cnt, (Edit){ EDIT_TAG, at, 0, n->name_first, true });
            }
            for (size_t i = n->first; i < n->end; i++) {
                /* the declarator, and the module types the members use; a member
                 * or a value of the module is not renamed here */
                size_t at = tok_off(ast, i);
                if (at < a || at >= b) continue;
                const KAstNode *d = declared_at(e, i);
                if (d && (i == n->name_first || d->kind == K_AST_TYPE || d->kind == K_AST_TAGS))
                    push_edit(e, ed, cnt, (Edit){ EDIT_RENAME, at, tok(ast, i).len, d->name_first, true });
            }
        } else if (value) {
            for (size_t i = n->first; i < n->end; i++) {
                size_t at = tok_off(ast, i);
                if (at < a || at >= b) continue;
                const KAstNode *d = declared_at(e, i);
                if (d) push_edit(e, ed, cnt, (Edit){ EDIT_RENAME, at, tok(ast, i).len, d->name_first, d->kind == K_AST_TYPE || d->kind == K_AST_TAGS });
            }
        }
    }
    /* the prelude's static_assert in C11 (backend §4.2, §9): only the keyword changes */
    if (n->kind == K_AST_OPAQUE && !e->opt.c23 && text_is(tok(ast, n->first), "static_assert")) {
        size_t at = tok_off(ast, n->first);
        if (at >= a && at < b)
            push_edit(e, ed, cnt, (Edit){ EDIT_ASSERT, at, tok(ast, n->first).len, 0, false });
    }
}

/* Comments leave the generated text and their lines stay (backend §6, rule 3):
 * a comment is dropped, and so are the blanks before it when it ended the
 * line. Tokens do not include comments, so they sit in the gaps between
 * tokens, which hold nothing else; that is where they are found. The edits
 * are merged into `ed` in source order. */
static void comment_edits(Emit *e, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    const char *s = ast->source.ptr;
    size_t ntok = ast->tokens.len;
    size_t lo = 0, hi = ntok;
    while (lo < hi) {   /* the first token that ends after a */
        size_t mid = (lo + hi) / 2;
        if (tok_end(ast, mid) > a) hi = mid; else lo = mid + 1;
    }
    size_t before = *cnt;
    size_t g0 = lo > 0 ? tok_end(ast, lo - 1) : 0;
    for (size_t i = lo; g0 < b; i++) {
        size_t g1 = i < ntok ? tok_off(ast, i) : ast->source.len;
        for (size_t p = g0 < a ? a : g0; p < g1 && p < b; ) {
            size_t ce;
            if (s[p] == '/' && p + 1 < g1 && s[p + 1] == '/') {
                ce = p;
                while (ce < g1 && s[ce] != '\n') ce++;
            } else if (s[p] == '/' && p + 1 < g1 && s[p + 1] == '*') {
                ce = p + 2;
                while (ce + 1 < g1 && !(s[ce] == '*' && s[ce + 1] == '/')) ce++;
                ce = ce + 2 < g1 ? ce + 2 : g1;
            } else { p++; continue; }
            size_t cs = p, end = ce < b ? ce : b;
            if (end > cs && s[end - 1] == '\n') end--;     /* the break stays */
            size_t rest = end;
            while (rest < b && (s[rest] == ' ' || s[rest] == '\t')) rest++;
            if (rest >= b || s[rest] == '\n')              /* the comment ended the line */
                while (cs > g0 && cs > a && (s[cs - 1] == ' ' || s[cs - 1] == '\t')) cs--;
            if (end > cs) push_edit(e, ed, cnt, (Edit){ EDIT_DROP, cs, end - cs, 0, false });
            p = ce > p ? ce : p + 1;
        }
        if (i >= ntok) break;
        g0 = tok_end(ast, i);
    }
    for (size_t i = before; i < *cnt; i++)   /* insertion sort: few edits per line */
        for (size_t j = i; j > 0 && ed[j - 1].at > ed[j].at; j--) {
            Edit t = ed[j]; ed[j] = ed[j - 1]; ed[j - 1] = t;
        }
}

/* Copies source [a, b) into buf, applying the edits (sorted by position). */
static void copy_edited(Emit *e, size_t a, size_t b, const Edit *ed, size_t cnt) {
    const char *s = e->ast->source.ptr;
    size_t pos = a;
    for (size_t k = 0; k < cnt && e->ok; k++) {
        put_bytes(e, s + pos, ed[k].at - pos);
        if (ed[k].kind == EDIT_RENAME) put_mangled(e, ed[k].name_tok, ed[k].is_type);
        else if (ed[k].kind == EDIT_ASSERT) put_bytes(e, "_Static_assert", 14);
        else if (ed[k].kind == EDIT_STATIC) put_bytes(e, "static", 6);
        else if (ed[k].kind == EDIT_TAG) { put_bytes(e, " ", 1); put_mangled(e, ed[k].name_tok, true); }
        pos = ed[k].at + ed[k].skip;
    }
    if (e->ok) put_bytes(e, s + pos, b - pos);
}

/* Writes the source lines [a, b) as the .k line `ln` (one call: a changed
 * line is one text, and the writer owns its #line). */
static void emit_line(Emit *e, size_t ln, size_t a, size_t b) {
    Edit ed[MAX_EDITS];
    size_t cnt = 0;
    for (size_t i = 0; i < e->ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
        if (overlaps(e, n, a, b)) node_edits(e, n, a, b, ed, &cnt);
    }
    comment_edits(e, a, b, ed, &cnt);
    if (!e->ok) return;
    const char *s = e->ast->source.ptr;
    if (cnt == 0 && b > a && s[b - 1] == '\n') {
        if (!k_emit_at(&e->w, ln, (keel_slice_char){ b - a, (char *)s + a })) e->ok = false;
        return;
    }
    e->n = 0;
    copy_edited(e, a, b, ed, cnt);
    if (e->n == 0 || e->buf[e->n - 1] != '\n') put_bytes(e, "\n", 1);
    if (e->ok && !k_emit_at(&e->w, ln, (keel_slice_char){ e->n, e->buf })) e->ok = false;
}

/* the whole lines [a, b), each with its own .k number */
static void emit_range(Emit *e, size_t a, size_t b) {
    size_t ln = line_at(e, a);
    while (a < b && e->ok) {
        size_t next = line_end(e, a);
        emit_line(e, ln, a, next);
        a = next;
        ln++;
    }
}

/* import_c <x>; -> #include <x> (backend §4.1; the prelude and hello use only
 * the <...> form, supported() refuses the rest). The header is one slice of
 * the source: its tokens are contiguous. */
static void emit_import(Emit *e, const KAstNode *n, size_t a) {
    const KAst *ast = e->ast;
    size_t hs = tok_off(ast, n->first + 1), he = tok_end(ast, n->end - 2);
    e->n = 0;
    put_bytes(e, "#include ", 9);
    put_bytes(e, ast->source.ptr + hs, he - hs);
    put_bytes(e, "\n", 1);
    if (e->ok && !k_emit_at(&e->w, line_at(e, a), (keel_slice_char){ e->n, e->buf })) e->ok = false;
}

/* A public function's prototype (backend §4.1): the declaration up to the `)`
 * that closes the parameters, without the visibility, with the mangled name. */
static void emit_proto(Emit *e, const KAstNode *n) {
    size_t a = first_off(e, n);
    size_t b = tok_end(e->ast, n->body_first - 2);
    Edit ed[MAX_EDITS];
    size_t cnt = 0;
    node_edits(e, n, a, b, ed, &cnt);
    comment_edits(e, a, b, ed, &cnt);
    if (!e->ok) return;
    e->n = 0;
    copy_edited(e, a, b, ed, cnt);
    put_bytes(e, ";\n", 2);
    if (e->ok && !k_emit_at(&e->w, line_at(e, a), (keel_slice_char){ e->n, e->buf })) e->ok = false;
}

/* A public variable's declaration for the .h (backend §4.1, rule 13): `extern`,
 * the declaration without the initializer; the definition stays in the .c. */
static void emit_extern(Emit *e, const KAstNode *n) {
    const KAst *ast = e->ast;
    size_t a = first_off(e, n), b = last_end(e, n);
    for (size_t i = n->first, depth = 0; i < n->end; i++) {   /* the top-level `=` */
        KToken t = tok(ast, i);
        if (text_is(t, "(") || text_is(t, "[") || text_is(t, "{")) depth++;
        else if (text_is(t, ")") || text_is(t, "]") || text_is(t, "}")) depth--;
        else if (depth == 0 && (text_is(t, "=") || text_is(t, ";"))) { b = tok_off(ast, i); break; }
    }
    while (b > a && (ast->source.ptr[b - 1] == ' ' || ast->source.ptr[b - 1] == '\t')) b--;
    Edit ed[MAX_EDITS];
    size_t cnt = 0;
    node_edits(e, n, a, b, ed, &cnt);
    comment_edits(e, a, b, ed, &cnt);
    if (!e->ok) return;
    e->n = 0;
    put_bytes(e, "extern ", 7);
    copy_edited(e, a, b, ed, cnt);
    put_bytes(e, ";\n", 2);
    if (e->ok && !k_emit_at(&e->w, line_at(e, a), (keel_slice_char){ e->n, e->buf })) e->ok = false;
}

/* A module-level constexpr under C11 (backend §9.2, rules 1 and 2): the macro
 * and the checking object, both on the line of the constexpr. The cast binds
 * tighter than most operators, so an initializer of more than one token is
 * parenthesized; a single token (`4`) is not, as in golden 008. */
static void emit_constexpr_c11(Emit *e, const KAstNode *n) {
    const KAst *ast = e->ast;
    size_t k = n->first + (is_visibility(tok(ast, n->first)) ? 1 : 0) + 1;   /* past `constexpr` */
    size_t ln = line_at(e, first_off(e, n));
    size_t ts = tok_off(ast, k), te = tok_off(ast, n->name_first);
    size_t is = tok_off(ast, n->name_first + 2), ie = tok_end(ast, n->end - 2);
    while (te > ts && (ast->source.ptr[te - 1] == ' ' || ast->source.ptr[te - 1] == '\t')) te--;
    bool paren = n->end - 2 > n->name_first + 2;   /* more than one token */
    const char *s = ast->source.ptr;
    e->n = 0;
    put_bytes(e, "#define ", 8);
    put_mangled(e, n->name_first, false);
    put_bytes(e, " ((", 3);
    put_bytes(e, s + ts, te - ts);
    put_bytes(e, ")", 1);
    if (paren) put_bytes(e, "(", 1);
    put_bytes(e, s + is, ie - is);
    if (paren) put_bytes(e, ")", 1);
    put_bytes(e, ")\n", 2);
    if (e->ok && !k_emit_at(&e->w, ln, (keel_slice_char){ e->n, e->buf })) e->ok = false;
    e->n = 0;
    put_bytes(e, "static const ", 13);
    put_bytes(e, s + ts, te - ts);
    put_bytes(e, " ", 1);
    put_mangled(e, n->name_first, false);
    put_bytes(e, "__chk = ", 8);
    put_bytes(e, s + is, ie - is);
    put_bytes(e, ";\n", 2);
    if (e->ok && !k_emit_at(&e->w, ln, (keel_slice_char){ e->n, e->buf })) e->ok = false;
}

/* `tags N [A, B = 3];` -> the enum (backend §5.6, rule 1): one line when no
 * tag carries a value, else one tag per line, four blanks of indent. */
static void emit_tags(Emit *e, const KAstNode *n) {
    const KAst *ast = e->ast;
    size_t open = n->name_first + 1, close = n->end - 2;   /* `[` and `]` */
    bool values = false;
    for (size_t i = open; i < close; i++) if (text_is(tok(ast, i), "=")) values = true;
    e->n = 0;
    put_bytes(e, "typedef enum ", 13);
    put_mangled(e, n->name_first, true);
    put_bytes(e, values ? " {\n" : " { ", 3);
    for (size_t i = open + 1; i < close && e->ok; ) {
        if (values) put_bytes(e, "    ", 4);
        put_mangled(e, n->name_first, true);
        put_bytes(e, "_", 1);
        put_bytes(e, tok(ast, i).ptr, tok(ast, i).len);
        i++;
        if (text_is(tok(ast, i), "=")) {
            put_bytes(e, " = ", 3);
            for (i++; i < close && !text_is(tok(ast, i), ","); i++)
                put_bytes(e, tok(ast, i).ptr, tok(ast, i).len);
        }
        if (i < close) { put_bytes(e, values ? ",\n" : ", ", 2); i++; }
    }
    put_bytes(e, values ? "\n} " : " } ", 3);
    put_mangled(e, n->name_first, true);
    put_bytes(e, ";\n", 2);
    if (e->ok && !k_emit_at(&e->w, line_at(e, first_off(e, n)), (keel_slice_char){ e->n, e->buf })) e->ok = false;
}

/* ---- the header bodies ---- */

/* .type.h: the typedefs, the opaque C, the import_c lines and the body of
 * `extern_c [type_h]`, each with its own .k line (cgen design §8.3). A line
 * shared by two constructions (two typedefs) is written once. */
static void type_h_body(Emit *e) {
    const KAst *ast = e->ast;
    for (size_t i = 0; i < ast->nodes.len && e->ok; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        size_t a, b;
        switch (n->kind) {
        case K_AST_IMPORT_C:
            a = line_start(e, first_off(e, n));
            if (a >= e->done) {
                emit_import(e, n, a);
                e->done = line_end(e, last_end(e, n));
            }
            break;
        case K_AST_TAGS:
            emit_tags(e, n);
            e->done = line_end(e, last_end(e, n));
            break;
        case K_AST_CONSTEXPR:
            a = line_start(e, first_off(e, n));
            b = line_end(e, last_end(e, n));
            if (e->opt.c23) emit_range(e, a, b);
            else emit_constexpr_c11(e, n);
            e->done = b;
            break;
        case K_AST_TYPE:
        case K_AST_OPAQUE:
            a = line_start(e, first_off(e, n));
            if (a < e->done) a = e->done;
            b = line_end(e, last_end(e, n));
            if (a < b) { emit_range(e, a, b); e->done = b; }
            break;
        case K_AST_EXTERN_C:
            /* the lines after the `{` line, up to the `}` line (not included) */
            a = line_end(e, tok_off(ast, n->body_first - 1));
            b = line_start(e, tok_off(ast, n->body_end));
            if (a < e->done) a = e->done;
            if (a < b) { emit_range(e, a, b); e->done = b; }
            break;
        default:
            break;
        }
    }
}

static void h_body(Emit *e) {
    for (size_t i = 0; i < e->ast->nodes.len && e->ok; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
        if (n->kind == K_AST_FUNCTION && n->is_public) emit_proto(e, n);
        else if (n->kind == K_AST_VARIABLE && n->is_public) emit_extern(e, n);
    }
}

static bool type_h_has_content(const Emit *e) {
    for (size_t i = 0; i < e->ast->nodes.len; i++) {
        KAstKind k = keel_buffer_KAstNode_ptr(&e->ast->nodes, i)->kind;
        if (k == K_AST_IMPORT_C || k == K_AST_TYPE || k == K_AST_OPAQUE || k == K_AST_EXTERN_C ||
            k == K_AST_CONSTEXPR || k == K_AST_TAGS) return true;
    }
    return false;
}
static bool h_has_content(const Emit *e) {
    for (size_t i = 0; i < e->ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
        if ((n->kind == K_AST_FUNCTION || n->kind == K_AST_VARIABLE) && n->is_public) return true;
    }
    return false;
}

/* ---- raw text and names ---- */

static void raw(Emit *e, const char *s) { k_emit_raw(&e->w, (keel_slice_char){ strlen(s), (char *)s }); }
static void raw_slice(Emit *e, keel_slice_char s) { k_emit_raw(&e->w, s); }

/* A name a mangle function wrote into buf; ok drops when it did not fit. */
static keel_slice_char fit(Emit *e, char *buf, size_t need) {
    if (need > NAME_CAP) e->ok = false;
    return (keel_slice_char){ need <= NAME_CAP ? need : 0, buf };
}

/* ---- the files ---- */

/* The frame of a header (backend §4.1; cgen design §8.3):
 *   R0 always a blank line after #define; R1 only after a block and before
 *   content; R5 before #endif unless the file is empty (no block, no content). */
static void emit_header(Emit *e, KEmitFile which) {
    bool type_h = which == K_EMIT_TYPE_H;
    bool c23 = e->opt.c23;
    bool keel_mod = is_keel(e->module);
    char path_buf[NAME_CAP], guard_buf[NAME_CAP], type_buf[NAME_CAP];
    keel_slice_char path = fit(e, path_buf,
        k_mangle_path(e->module, type_h ? ".type.h" : ".h", (keel_slice_char){ NAME_CAP, path_buf }));
    keel_slice_char guard = fit(e, guard_buf, k_mangle_guard(path, (keel_slice_char){ NAME_CAP, guard_buf }));
    keel_slice_char type_path = fit(e, type_buf,
        k_mangle_path(e->module, ".type.h", (keel_slice_char){ NAME_CAP, type_buf }));
    if (!e->ok) return;

    bool has_content = type_h ? type_h_has_content(e) : h_has_content(e);
    /* the synthetic block (backend §4.1): under C11 the prelude's type header
     * includes stdbool and assert; the other modules include keel.type.h; the
     * C23 prelude has none */
    bool block = !(type_h && keel_mod) || !c23;

    raw(e, "/* ");
    raw_slice(e, path);
    raw(e, " " EM_DASH " generated from ");
    raw_slice(e, e->src_path);
    raw(e, c23 ? " by cgen, C23 profile. */\n" : " by cgen, C11 profile. */\n");
    raw(e, "#ifndef "); raw_slice(e, guard);
    raw(e, "\n#define "); raw_slice(e, guard);
    raw(e, "\n\n");
    if (type_h && keel_mod) {
        if (!c23) raw(e, "#include <stdbool.h>\n#include <assert.h>\n");
    } else if (type_h) {
        raw(e, "#include \"keel.type.h\"\n");
    } else {
        raw(e, "#include \""); raw_slice(e, type_path); raw(e, "\"\n");
    }
    if (block && has_content) raw(e, "\n");
    if (type_h) type_h_body(e);
    else h_body(e);
    if (block || has_content) raw(e, "\n");
    raw(e, "#endif /* "); raw_slice(e, guard); raw(e, " */\n");
}

/* The .c: every source line, in order. A line of a construction that does not
 * go to C is empty (backend §6, rule 3: the breaks stay); the rest is copied,
 * with the function's edits. */
static void emit_c(Emit *e) {
    char c_buf[NAME_CAP], h_buf[NAME_CAP];
    keel_slice_char c_path = fit(e, c_buf, k_mangle_path(e->module, ".c", (keel_slice_char){ NAME_CAP, c_buf }));
    keel_slice_char h_path = fit(e, h_buf, k_mangle_path(e->module, ".h", (keel_slice_char){ NAME_CAP, h_buf }));
    if (!e->ok) return;
    raw(e, "/* ");
    raw_slice(e, c_path);
    raw(e, " " EM_DASH " generated from ");
    raw_slice(e, e->src_path);
    raw(e, e->opt.c23 ? " by cgen, C23 profile. */\n" : " by cgen, C11 profile. */\n");
    raw(e, "#include \""); raw_slice(e, h_path); raw(e, "\"\n");

    size_t len = e->ast->source.len;
    char nl[] = "\n";
    for (size_t a = 0; a < len && e->ok; ) {
        size_t b = line_end(e, a);
        bool empty = false, fn = false;
        for (size_t i = 0; i < e->ast->nodes.len; i++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
            if (!overlaps(e, n, a, b)) continue;
            if (n->kind == K_AST_FUNCTION || n->kind == K_AST_VARIABLE) fn = true;
            else empty = true;   /* every other kind in the slice has no C in the .c */
        }
        if (fn && empty) { e->ok = false; return; }
        if (empty) {
            if (!k_emit_at(&e->w, line_at(e, a), (keel_slice_char){ 1, nl })) e->ok = false;
        } else {
            emit_line(e, line_at(e, a), a, b);
        }
        a = b;
    }
}

/* main_<symbol>.c (backend §5.8): a fixed unit that calls the module's main */
static void emit_entry(Emit *e) {
    char entry_buf[NAME_CAP], h_buf[NAME_CAP], sym_buf[NAME_CAP];
    keel_slice_char entry = fit(e, entry_buf, k_mangle_entry(e->module, (keel_slice_char){ NAME_CAP, entry_buf }));
    keel_slice_char header = fit(e, h_buf, k_mangle_path(e->module, ".h", (keel_slice_char){ NAME_CAP, h_buf }));
    keel_slice_char main_name = { 4, (char *)"main" };
    keel_slice_char sym = fit(e, sym_buf, k_mangle_decl(e->module, main_name, false, (keel_slice_char){ NAME_CAP, sym_buf }));
    if (!e->ok) return;
    raw(e, "/* "); raw_slice(e, entry); raw(e, " " EM_DASH " entry unit, generated by --main ");
    raw_slice(e, e->module); raw(e, ". */\n");
    raw(e, "#include \""); raw_slice(e, header); raw(e, "\"\n");
    raw(e, "int main(int argc, char **argv) { return "); raw_slice(e, sym);
    raw(e, "(argc, argv); }\n");
}

/* Whether the islands of the module are the ones a rename covers: a type,
 * a name or a call whose detail is `<name>[/arity] → <symbol>` and whose name
 * is declared by this module. Any other island is a construction that needs
 * lowering (backend §5), which the slice does not have. */
static bool islands_supported(const KAst *ast) {
    for (size_t i = 0; i < ast->islands.len; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        if (is->kind != K_ISLAND_TYPE && is->kind != K_ISLAND_NAME && is->kind != K_ISLAND_CALL) return false;
        const char *d = keel_buffer_char_ptr(&ast->island_text, is->text);
        size_t len = is->text_len, nm = 0;
        while (nm < len && d[nm] != '/' && d[nm] != ' ') nm++;
        bool arrow = false;
        for (size_t k = nm; k + 3 <= len; k++)
            if (memcmp(d + k, "\xE2\x86\x92", 3) == 0) { arrow = true; break; }
        if (!arrow) return false;
        bool declared = false;
        for (size_t k = 0; k < ast->nodes.len && !declared; k++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
            if (n->name_first >= n->name_end) continue;
            KToken t = tok(ast, n->name_first);
            declared = t.len == nm && memcmp(t.ptr, d, nm) == 0;
        }
        if (!declared) return false;
    }
    return true;
}

/* Whether the slice of emit.h covers every node of the module. */
static bool supported(const KAst *ast) {
    if (!islands_supported(ast)) return false;
    for (size_t i = 0; i < ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        if (n->first >= n->end) return false;
        switch (n->kind) {
        case K_AST_MODULE:
        case K_AST_PROTOCOL:
            break;
        case K_AST_OPAQUE:
            break;
        case K_AST_IMPORT_C:
            /* import_c <x>; with the <...> form only */
            if (n->end - n->first < 5) return false;
            if (tok(ast, n->first + 1).ptr[0] != '<') return false;
            if (!text_is(tok(ast, n->end - 2), ">") || !text_is(tok(ast, n->end - 1), ";")) return false;
            break;
        case K_AST_TYPE: {
            size_t k = n->first + (is_visibility(tok(ast, n->first)) ? 1 : 0);
            if (k >= n->end || !text_is(tok(ast, k), "typedef")) return false;
            /* one declarator per typedef: a list shares the span of its nodes */
            if (i > 0 && keel_buffer_KAstNode_ptr(&ast->nodes, i - 1)->first == n->first) return false;
            break;
        }
        case K_AST_TAGS: {
            /* pub tags N [A, B = 3];  priv sends the enum to the .c, which is not in the slice;
             * a value is a number, optionally negative */
            size_t k = n->first + (is_visibility(tok(ast, n->first)) ? 1 : 0);
            if (!n->is_public || n->name_first >= n->name_end || n->name_first != k + 1) return false;
            if (n->end - n->name_first < 4 || !text_is(tok(ast, n->name_first + 1), "[")) return false;
            for (size_t i = n->name_first + 2; i + 2 < n->end; i++) {
                if (!text_is(tok(ast, i), "=")) continue;
                size_t v = i + 1;
                if (text_is(tok(ast, v), "-")) v++;
                char c = tok(ast, v).ptr[0];
                if (c < '0' || c > '9') return false;
            }
            for (size_t p = tok_off(ast, n->first); p < tok_end(ast, n->end - 1); p++)
                if (ast->source.ptr[p] == '\n') return false;
            break;
        }
        case K_AST_CONSTEXPR: {
            /* [pub|priv] constexpr T NAME = INIT; on one line, INIT not naming the module */
            size_t k = n->first + (is_visibility(tok(ast, n->first)) ? 1 : 0);
            if (n->name_first >= n->name_end || n->name_first + 2 >= n->end - 1 || n->name_first <= k + 1) return false;
            if (!text_is(tok(ast, k), "constexpr") || !text_is(tok(ast, n->name_first + 1), "=")) return false;
            for (size_t p = tok_off(ast, n->first); p < tok_end(ast, n->end - 1); p++)
                if (ast->source.ptr[p] == '\n') return false;
            break;
        }
        case K_AST_VARIABLE: {
            /* one declarator: no top-level comma */
            size_t depth = 0;
            for (size_t p = n->first; p < n->end; p++) {
                KToken t = tok(ast, p);
                if (text_is(t, "(") || text_is(t, "[") || text_is(t, "{")) depth++;
                else if (text_is(t, ")") || text_is(t, "]") || text_is(t, "}")) depth--;
                else if (depth == 0 && text_is(t, ",")) return false;
            }
            break;
        }
        case K_AST_EXTERN_C: {
            size_t k = n->first + (is_visibility(tok(ast, n->first)) ? 1 : 0);
            if (k + 4 >= n->end) return false;
            if (!text_is(tok(ast, k), "extern_c") || !text_is(tok(ast, k + 1), "[") ||
                !text_is(tok(ast, k + 2), "type_h") || !text_is(tok(ast, k + 3), "]") ||
                !text_is(tok(ast, k + 4), "{")) return false;
            if (n->body_first == (size_t)-1 || n->body_end == (size_t)-1) return false;
            break;
        }
        case K_AST_FUNCTION:
            if (n->body_first == (size_t)-1 || n->body_first < 2) return false;
            if (!text_is(tok(ast, n->body_first - 1), "{") || !text_is(tok(ast, n->body_first - 2), ")"))
                return false;
            break;
        default:
            return false;
        }
    }
    return true;
}

size_t k_emit_file(const KAst *ast, keel_slice_char module, keel_slice_char src_path,
                   KEmitFile which, KEmitOptions opt, keel_slice_char out) {
    Emit e;
    e.ast = ast;
    e.module = module;
    e.src_path = src_path;
    e.opt = opt;
    e.done = 0;
    e.cache_off = 0;
    e.cache_line = 1;
    e.ok = true;
    e.n = 0;
    k_writer_init(&e.w, out, src_path, opt.line_on);

    if (which == K_EMIT_ENTRY) {
        emit_entry(&e);
    } else {
        if (!supported(ast)) return K_EMIT_UNSUPPORTED;
        if (which == K_EMIT_C) emit_c(&e);
        else emit_header(&e, which);
    }
    if (!e.ok) return K_EMIT_UNSUPPORTED;
    return k_writer_len(&e.w);
}
