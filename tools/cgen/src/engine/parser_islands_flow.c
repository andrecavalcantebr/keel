/* Pass 3: defer, foreach, walk and match; parallel, win and fail; extent; and the else of a result. Split from parser_islands.c by T3; the code is unchanged. */

#include "engine/parser_islands_internal.h"

bool tags_decl(const KAst *where, keel_slice_char name, KTagsDecl *out);
size_t column_access(Ctx *c, size_t i, size_t open, bool *any);
/* ---- defer, foreach, walk, match (stages 4c, 4d, 4f) ----------------- */

void put_range(Text *t, const KAst *a, size_t first, size_t end) {
    for (size_t j = first; j < end; j++) {
        if (j > first) {
            bool tight = punct(a, j, ".") || punct(a, j - 1, ".") || punct(a, j, "..") || punct(a, j - 1, "..") ||
                         punct(a, j, "->") || punct(a, j - 1, "->") || punct(a, j, ",") || punct(a, j - 1, "(") ||
                         punct(a, j, ")") || punct(a, j, "[") || punct(a, j - 1, "[") || punct(a, j, "]") ||
                         punct(a, j - 1, "*") || punct(a, j - 1, "&");
            if (!tight) put_str(t, " ");
            else if (punct(a, j - 1, ",")) put_str(t, " ");
        }
        put(t, tok(a, j).ptr, tok(a, j).len);
    }
}

/* the '{' that opens the block holding token `i`, or SIZE_MAX */
size_t enclosing_brace(const KAst *a, size_t from, size_t i) {
    size_t depth = 0;
    for (size_t j = i; j-- > from; ) {
        if (punct(a, j, "}")) depth++;
        else if (punct(a, j, "{")) { if (!depth) return j; depth--; }
    }
    return SIZE_MAX;
}

/* the control word whose statement token `j` is the body of — `if`, `while`,
   `for`, `switch` before a ')', or `else` and `do' themselves — or NULL */
const char *control_before(const KAst *a, size_t from, size_t j) {
    if (j <= from) return NULL;
    if (word(a, j - 1, "else")) return "else";
    if (word(a, j - 1, "do")) return "do";
    if (!punct(a, j - 1, ")")) return NULL;
    size_t depth = 0;
    for (size_t k = j - 1; k-- > from; ) {
        if (punct(a, k, ")")) depth++;
        else if (punct(a, k, "(")) {
            if (depth) { depth--; continue; }
            static const char *const words[] = { "if", "while", "for", "switch" };
            for (size_t w = 0; w < 4; w++) if (k > from && word(a, k - 1, words[w])) return words[w];
            return NULL;
        }
    }
    return NULL;
}

/* defer [capture] body (spec §4.6, cgen-tool §5.2: the capture as written, or
   `later` when none is written) */
static void defer_island(Ctx *c, const KAstNode *n, size_t at) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    const char *ctl = control_before(a, n->first, at);
    if (ctl) diag3(c, K_DIAG_DEFER_WITHOUT_BRACES, tok(a, at), k_diag_text(ctl), none, none);
    size_t brace = enclosing_brace(a, n->first, at);
    const char *block = brace != SIZE_MAX ? control_before(a, n->first, brace) : NULL;
    if (block && (!strcmp(block, "if") || !strcmp(block, "else") || !strcmp(block, "switch")))
        diag3(c, K_DIAG_DEFER_IN_CONTROL_BLOCK, tok(a, at), k_diag_text(block), none, none);
    Text d = { .ok = true };
    if (punct(a, at + 1, "[")) {
        size_t close = close_bracket(a, at + 1, n->end);
        if (close == SIZE_MAX) return;
        if (word(a, at + 2, "later") && close > at + 3)
            diag3(c, K_DIAG_LATER_WITH_CAPTURE, tok(a, at + 3), range_text(a, (Range){ at + 3, close }), none, none);
        /* each entry of `[now ...]` ends in its name: keel rebuilds the copy
           from the tokens before it (spec §4.6, hidden-declarator) */
        if (word(a, at + 2, "now"))
            for (size_t j = at + 3, st = at + 3, depth = 0; j <= close; j++) {
                if (j < close && (punct(a, j, "(") || punct(a, j, "["))) depth++;
                else if (j < close && (punct(a, j, ")") || punct(a, j, "]"))) depth--;
                else if (j == close || (!depth && punct(a, j, ","))) {
                    if (j > st && !ident(a, j - 1)) {
                        size_t nm = j - 1;
                        while (nm > st && !ident(a, nm)) nm--;
                        diag3(c, K_DIAG_HIDDEN_DECLARATOR, tok(a, nm), tok(a, nm), none, none);
                    }
                    st = j + 1;
                }
            }
        put_range(&d, a, at + 2, close);
    } else {
        put_str(&d, "later");
    }
    emit(c, K_ISLAND_DEFER, at, &d);
    bool later = !punct(a, at + 1, "[") || word(a, at + 2, "later");
    if (later && c->later_count < sizeof c->later / sizeof *c->later) {
        size_t first = punct(a, at + 1, "[") ? close_bracket(a, at + 1, n->end) + 1 : at + 1;
        size_t end = first;
        for (size_t depth = 0; end < n->end; end++) {
            if (punct(a, end, "{") || punct(a, end, "(")) depth++;
            else if (punct(a, end, "}") || punct(a, end, ")")) { if (--depth == 0 && punct(a, end, "}")) break; }
            else if (!depth && punct(a, end, ";")) break;
        }
        c->later[c->later_count++] = (__typeof__(c->later[0])){ .at = at, .body_first = first, .body_end = end,
                                                                .depth = c->depth };
    }
}

/* the protocols of the base live in the prelude (spec §5.1, rule 9) */
static const KAst *protocols_ast(const Ctx *c) {
    const KModule *m = module_by_name(c->ast->symbols, "keel");
    return m && m->ast ? m->ast : NULL;
}

/* Whether the type of `r`, which has kind KT_MODIFIER or KT_NAMED, meets the
   base protocol `proto`; `missing` names the verbs it lacks. A type the pass
   cannot see is taken as meeting it: C decides. */
static bool meets(Ctx *c, Range r, const char *proto, Text *missing) {
    int t = type_of(c, r);
    if (t < 0) return true;
    const KType *k = kt(c, t);
    if (k->kind != KT_MODIFIER && k->kind != KT_NAMED && k->kind != KT_ARRAY) return true;
    const KAst *pa = protocols_ast(c);
    KProtocolDecl pd;
    if (!pa || !protocol_decl(c, pa, k_diag_text(proto), &pd, 0)) return true;
    const KAst *own;
    if (k->kind == KT_ARRAY) {
        /* an `array` meets a protocol by the verbs of keel.array (spec §5.1) */
        const KModule *am = module_by_name(c->ast->symbols, "keel.array");
        own = am && am->ast ? am->ast : NULL;
        if (!own) { put_str(missing, "its verbs"); return false; }
        return satisfies(c, own, (keel_slice_char){0}, &pd, missing, tok(c->ast, r.first));
    }
    own = k->module && k->module->ast ? k->module->ast : c->ast;
    return satisfies(c, own, k->name, &pd, missing, tok(c->ast, r.first));
}

/* the statement after token `at`: a block to its '}', or a statement to its ';' */
static size_t statement_end(const KAst *a, size_t at, size_t end) {
    size_t depth = 0;
    for (size_t j = at; j < end; j++) {
        if (punct(a, j, "{") || punct(a, j, "(") || punct(a, j, "[")) depth++;
        else if (punct(a, j, "}") || punct(a, j, ")") || punct(a, j, "]")) {
            if (depth && --depth == 0 && punct(a, j, "}") && punct(a, at, "{")) return j;
        }
        else if (!depth && punct(a, j, ";")) return j;
    }
    return end;
}

/* `push`, `pop` or `clear` over the container `box` in [first, end): the body
   of the construction that goes over it (spec §4.7, §4.8) */
static void mutations(Ctx *c, size_t first, size_t end, Range box, const char *what) {
    KAst *a = c->ast;
    keel_slice_char text = range_text(a, box);
    static const char *const verbs[] = { "push", "pop", "clear" };
    for (size_t j = first; j < end; j++) {
        if (!punct(a, j + 1, "(")) continue;
        bool verb = false;
        for (size_t v = 0; v < 3; v++) if (word(a, j, verbs[v])) verb = true;
        if (!verb) continue;
        size_t arg = j + 2, stop = arg;
        for (size_t depth = 0; stop < end; stop++) {
            if (punct(a, stop, "(") || punct(a, stop, "[")) depth++;
            else if ((punct(a, stop, ")") || punct(a, stop, "]")) && depth-- == 0) break;
            else if (!depth && punct(a, stop, ",")) break;
        }
        while (arg < stop && punct(a, arg, "&")) arg++;
        keel_slice_char got = range_text(a, (Range){ arg, stop });
        if (stop > arg && got.len == text.len && !memcmp(got.ptr, text.ptr, text.len))
            diag3(c, K_DIAG_MUTATION_DURING_TRAVERSAL, tok(a, j), tok(a, j), text, k_diag_text(what));
    }
}

/* foreach (binder[, binder] : container) and walk (elem, cursor : container)
   (spec §4.7). The detail is the binders' names, the container as written,
   and its keel type when it is a declared symbol (cgen-tool §5.2). */
static void traversal(Ctx *c, const KAstNode *n, size_t at, bool cursor) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    size_t open = at + 1, close = close_paren(a, open, n->end);
    if (close == SIZE_MAX) return;
    size_t colon = SIZE_MAX, depth = 0;
    size_t comma[2]; size_t commas = 0;
    for (size_t j = open + 1; j < close; j++) {
        if (punct(a, j, "(") || punct(a, j, "[")) depth++;
        else if (punct(a, j, ")") || punct(a, j, "]")) depth--;
        else if (!depth && punct(a, j, ":")) { colon = j; break; }
        else if (!depth && punct(a, j, ",") && commas < 2) comma[commas++] = j;
    }
    if (colon == SIZE_MAX) return;
    size_t binders = commas + 1;
    Range b1 = { open + 1, commas ? comma[0] : colon }, b2 = { commas ? comma[0] + 1 : colon, colon };
    Range box = { colon + 1, close };
    bool literal = false, pointer = false;
    for (size_t j = box.first; j < box.end; j++) if (punct(a, j, "..")) literal = true;
    for (size_t j = b1.first; j < b1.end; j++) if (punct(a, j, "*")) pointer = true;

    Text d = { .ok = true };
    if (b1.end > b1.first) put(&d, tok(a, b1.end - 1).ptr, tok(a, b1.end - 1).len);
    if (binders > 1 && b2.end > b2.first) { put_str(&d, ", "); put(&d, tok(a, b2.end - 1).ptr, tok(a, b2.end - 1).len); }
    put_str(&d, " : ");
    put_range(&d, a, box.first, box.end);
    const Local *l = box.end == box.first + 1 && ident(a, box.first) ? find_local(c, tok(a, box.first)) : NULL;
    if (l && l->spec.text.len) { put_str(&d, " ("); put(&d, l->spec.text.ptr, l->spec.text.len); put_str(&d, ")"); }
    emit(c, cursor ? K_ISLAND_WALK : K_ISLAND_FOREACH, at, &d);
    /* an element binder by value that is an instance copies it (spec §4.7) */
    if (!pointer && b1.end > b1.first + 1) {
        KLexer lexer;
        TKPpKind pp;
        KSpecifier spec;
        KToken next = {0};
        KToken first = enter(a, b1.first, &lexer, &pp);
        if (k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp) && spec.kind == K_SPEC_MODIFIER)
            diag3(c, K_DIAG_BINDER_COPIES_CONTAINER, tok(a, b1.end - 1), tok(a, b1.end - 1), spec.text, none);
    }
    /* the cursor binder has the type `begin` produces (spec §4.7) */
    if (cursor && binders == 2 && b2.end > b2.first + 1) {
        int t = type_of(c, box);
        const KType *k = t >= 0 ? kt(c, t) : NULL;
        const KAst *home = k && k->kind == KT_MODIFIER && k->module ? ast_of(c, k->module) : NULL;
        Sig sig = home ? find_verb(home, k_diag_text("begin"), 1) : (Sig){0};
        int r = sig.found ? ret_type(c, home, sig.fn, t) : -1;
        size_t last = b2.end - 1;
        while (last > b2.first && punct(a, last - 1, "*")) last--;
        int b = last > b2.first ? kt_from_text(c, a->symbols, range_text(a, (Range){ b2.first, last })) : -1;
        if (r >= 0 && b >= 0 && *kt(c, r)->symbol && *kt(c, b)->symbol && strcmp(kt(c, r)->symbol, kt(c, b)->symbol)) {
            Text prod = { .ok = true };
            put_str(&prod, kt(c, r)->symbol);
            diag3(c, K_DIAG_CURSOR_TYPE_MISMATCH, tok(a, b2.first), range_text(a, (Range){ b2.first, last }), keep(c, &prod), none);
        }
    }
    if (!literal) mutations(c, close + 1, statement_end(a, close + 1, n->end), box, cursor ? "walk" : "foreach");
    /* the binder by pointer, and the cursor of `walk`, come from the container
       (spec §4.12, rules 14 and 15); a binder by value is a copy */
    int parent = literal ? -1 : local_of(c, box);
    if (parent >= 0) {
        if (pointer && b1.end > b1.first && c->pending_count < 8)
            c->pending[c->pending_count++] = (__typeof__(c->pending[0])){ tok(a, b1.end - 1), parent, close };
        if (cursor && binders > 1 && b2.end > b2.first && c->pending_count < 8)
            c->pending[c->pending_count++] = (__typeof__(c->pending[0])){ tok(a, b2.end - 1), parent, close };
    }

    Text missing = { .ok = true };
    if (cursor) {
        if (binders < 2) { diag3(c, K_DIAG_WALK_WITHOUT_CURSOR, tok(a, at), range_text(a, b1), none, none); return; }
        if (!literal && !meets(c, box, "Traversable", &missing))
            diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, tok(a, box.first), range_text(a, box), k_diag_text("Traversable"),
                  keep(c, &missing));
        return;
    }
    if (binders == 2 && literal)
        diag3(c, K_DIAG_FOREACH_TWO_BINDERS_ON_LITERAL, tok(a, box.first), range_text(a, box), none, none);
    if (binders == 1 && literal && pointer)
        diag3(c, K_DIAG_POINTER_BINDER_ON_RANGE, tok(a, b1.first), range_text(a, b1), none, none);
    if (binders == 2 && !(b2.end == b2.first + 2 && word(a, b2.first, "size_t")))
        diag3(c, K_DIAG_INDEX_NOT_SIZE_T, tok(a, b2.first), range_text(a, b2), none, none);
    if (literal) return;
    int t = type_of(c, box);
    if (t >= 0 && kt(c, t)->kind == KT_ARRAY) return;          /* the core's own traversal (spec §4.2) */
    const char *proto = binders == 1 ? "Countable" : pointer ? "IndexPtr" : "IndexGet";
    if (!meets(c, box, proto, &missing)) {
        Text other = { .ok = true };
        if (binders == 2 && !pointer && meets(c, box, "IndexPtr", &other)) return;
        diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, tok(a, box.first), range_text(a, box), k_diag_text(proto),
              keep(c, &missing));
    }
}

/* The tags declaration `name` in `where` (spec §4.9). */
bool tags_decl(const KAst *where, keel_slice_char name, KTagsDecl *out) {
    for (size_t i = 0; i < where->nodes.len; i++) {
        const KAstNode *n = node_at(where, i);
        if (n->kind != K_AST_TAGS || !k_symtab_same_name(tok(where, n->name_first), name)) continue;
        size_t kw = n->first;
        while (kw < n->name_first && !word(where, kw, "tags")) kw++;
        KToken t = tok(where, kw);
        KLexer lexer;
        TKPpKind pp;
        k_lexer_init(&lexer, (keel_slice_char){ (size_t)(where->source.ptr + where->source.len - t.ptr), (char *)t.ptr }, NULL);
        KToken first = k_lexer_next(&lexer, &pp), next;
        KSymbol scratch[2];
        KSymbolTable none;
        k_symtab_init(&none, scratch, 2);
        return k_scan_tags_decl(&lexer, first, &none, out, &next, &pp);
    }
    return false;
}

/* The set a `match` operand's labels come from (spec §4.9): its declared type
   when that is a set, or else the set its module's `tag` returns — for an
   instance, the argument written for the module's `tags` parameter. */
static int match_set(Ctx *c, Range op, const KAst **where, keel_slice_char *set) {
    int t = type_of(c, op);
    if (t < 0) return -1;
    const KType *k = kt(c, t);
    const KAst *own = k->module && k->module->ast ? k->module->ast : c->ast;
    if (k->kind == KT_NAMED) {
        const KSymbol *s = k_symbol_resolve(c->ast->symbols, k->name);
        if (s && s->kind == K_SYM_TAGS) { *where = own; *set = k->name; return 1; }
    }
    if (k->kind != KT_MODIFIER && k->kind != KT_NAMED) return -1;
    Sig sig = find_verb(own, k_diag_text("tag"), 1);
    if (!sig.found || (!sig.p[0].is_array && !k_symtab_same_name(sig.p[0].base, k->name))) return 0;
    size_t s = sig.fn->first;
    while (s < sig.fn->name_first && (word(own, s, "pub") || word(own, s, "priv") || word(own, s, "inline")))
        s++;
    if (s >= sig.fn->name_first || !ident(own, s)) return -1;
    keel_slice_char ret = tok(own, s);
    if (k->kind == KT_MODIFIER) {
        const KAstNode *m = module_node(own);
        int idx = 0;
        for (size_t j = m->dim_first; j < m->dim_end; j++) if (ident(own, j)) idx++;
        for (size_t j = m->tags_first; j < m->tags_end; j++) {
            if (!ident(own, j)) continue;
            if (k_symtab_same_name(tok(own, j), ret) && idx < k->argc) {
                const KType *arg = kt(c, k->arg[idx]);
                *where = arg->module && arg->module->ast ? arg->module->ast : c->ast;
                *set = arg->name;
                return 1;
            }
            idx++;
        }
    }
    for (size_t i = 0; i < own->nodes.len; i++)
        if (node_at(own, i)->kind == K_AST_TAGS && k_symtab_same_name(tok(own, node_at(own, i)->name_first), ret)) {
            *where = own; *set = ret; return 1;
        }
    return 2;                                       /* `tag` returns no set */
}

/* match (operand) { LABEL: … } (spec §4.9). */
static void match_island(Ctx *c, const KAstNode *n, size_t at) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    size_t open = at + 1, close = close_paren(a, open, n->end);
    if (close == SIZE_MAX || !punct(a, close + 1, "{")) return;
    Range op = { open + 1, close };
    size_t body = close + 1, depth = 0, end = SIZE_MAX;
    for (size_t j = body; j < n->end; j++) {
        if (punct(a, j, "{")) depth++;
        else if (punct(a, j, "}") && --depth == 0) { end = j; break; }
    }
    if (end == SIZE_MAX) return;
    size_t labels[K_TAGS_MAX_ITEMS * 2], count = 0;
    depth = 0;
    for (size_t j = body + 1; j < end; j++) {
        if (punct(a, j, "{") || punct(a, j, "(") || punct(a, j, "[")) depth++;
        else if (punct(a, j, "}") || punct(a, j, ")") || punct(a, j, "]")) depth--;
        else if (!depth && ident(a, j) && punct(a, j + 1, ":") &&
                 (punct(a, j - 1, "{") || punct(a, j - 1, ";") || punct(a, j - 1, "}") || punct(a, j - 1, ":")) &&
                 count < sizeof labels / sizeof *labels)
            labels[count++] = j;
    }
    Text d = { .ok = true };
    put_range(&d, a, op.first, op.end);
    const Local *l = op.end == op.first + 1 && ident(a, op.first) ? find_local(c, tok(a, op.first)) : NULL;
    if (l && l->spec.text.len) { put_str(&d, " ("); put(&d, l->spec.text.ptr, l->spec.text.len); put_str(&d, ")"); }
    put_str(&d, ":");
    for (size_t k = 0; k < count; k++) { put_str(&d, " "); put(&d, tok(a, labels[k]).ptr, tok(a, labels[k]).len); }
    emit(c, K_ISLAND_MATCH, at, &d);

    const KAst *where = NULL;
    keel_slice_char set = {0};
    int found = match_set(c, op, &where, &set);
    if (found == 0) {
        diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, tok(a, op.first), range_text(a, op), k_diag_text("Taggable"), k_diag_text("tag"));
        return;
    }
    if (found == 2) { diag3(c, K_DIAG_MATCH_WITHOUT_TAGS, tok(a, op.first), range_text(a, op), none, none); return; }
    KTagsDecl tags;
    if (found != 1 || !tags_decl(where, set, &tags)) return;
    for (size_t k = 0; k < count; k++) {
        bool in = false;
        for (size_t m = 0; m < tags.item_count; m++) if (k_symtab_same_name(tok(a, labels[k]), tags.items[m])) in = true;
        if (!in) diag3(c, tag_of_some_set(a, tok(a, labels[k])) ? K_DIAG_TAG_FROM_OTHER_SET : K_DIAG_TAG_NOT_IN_SET,
                       tok(a, labels[k]), tok(a, labels[k]), set, none);
        for (size_t m = 0; m < k; m++)
            if (k_symtab_same_name(tok(a, labels[k]), tok(a, labels[m])))
                diag3(c, K_DIAG_DUPLICATE_TAG, tok(a, labels[k]), tok(a, labels[k]), set, none);
    }
    for (size_t m = 0; m < tags.item_count; m++) {
        bool labelled = false;
        for (size_t k = 0; k < count; k++) if (k_symtab_same_name(tok(a, labels[k]), tags.items[m])) labelled = true;
        if (!labelled) diag3(c, K_DIAG_TAG_WITHOUT_LABEL, tok(a, at), tags.items[m], set, none);
    }
}

/* ---- parallel, win, fail (stage 4e) ----------------------------------- */

static bool inside_parallel(const Ctx *c, size_t i) { return c->par_close && i > c->par_open && i < c->par_close; }

/* the ranges a `;` at depth 0 splits [first, end) into, up to `cap` */
static size_t split_semis(const KAst *a, size_t first, size_t end, Range *out, size_t cap) {
    size_t n = 0, depth = 0, st = first;
    for (size_t j = first; j <= end; j++) {
        if (j < end && (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{"))) depth++;
        else if (j < end && (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}"))) depth--;
        else if (j == end || (!depth && punct(a, j, ";"))) {
            if (n < cap) out[n] = (Range){ st, j };
            n++;
            st = j + 1;
        }
    }
    return n;
}

static size_t colon_in(const KAst *a, Range r) {
    for (size_t j = r.first, depth = 0; j < r.end; j++) {
        if (punct(a, j, "(") || punct(a, j, "[")) depth++;
        else if (punct(a, j, ")") || punct(a, j, "]")) depth--;
        else if (!depth && punct(a, j, ":")) return j;
    }
    return SIZE_MAX;
}

/* parallel NAME POLICY (w : 0..k; part : x [; (captures)]) { body } (spec §4.8).
   The detail is the name, the policy, the binders and the container as
   written, the container's keel type, the captures, and the `partition` the
   distribution calls (cgen-tool §5.2). The body's tokens are walked as usual:
   this only records where it is, for `win`, `fail`, `return` and the captures. */
static void parallel_island(Ctx *c, const KAstNode *n, size_t at) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    if (!ident(a, at + 1) || punct(a, at + 2, "(") || word(a, at + 1, "ALL") || word(a, at + 1, "ANY")) {
        diag3(c, K_DIAG_UNNAMED_PARALLEL, tok(a, at), none, none, none);
        return;
    }
    KToken name = tok(a, at + 1);
    size_t open = at + 2;
    while (open < n->end && open < at + 6 && !punct(a, open, "(")) open++;
    if (!punct(a, open, "(")) return;
    Range policy = { at + 2, open };
    size_t close = close_paren(a, open, n->end);
    if (close == SIZE_MAX || !punct(a, close + 1, "{")) return;
    size_t body = close + 1, end = SIZE_MAX;
    for (size_t j = body, depth = 0; j < n->end; j++) {
        if (punct(a, j, "{")) depth++;
        else if (punct(a, j, "}") && --depth == 0) { end = j; break; }
    }
    if (end == SIZE_MAX) return;
    Range part[3];
    size_t parts = split_semis(a, open + 1, close, part, 3);
    if (parts < 2) return;
    size_t c1 = colon_in(a, part[0]), c2 = colon_in(a, part[1]);
    if (c1 == SIZE_MAX || c2 == SIZE_MAX || c1 == part[0].first || c2 == part[1].first) return;
    Range workers = { c1 + 1, part[0].end }, box = { c2 + 1, part[1].end };
    Range binder = { part[1].first, c2 };
    Range capture = { 0, 0 };
    if (parts > 2 && punct(a, part[2].first, "(") && part[2].end > part[2].first &&
        punct(a, part[2].end - 1, ")"))
        capture = (Range){ part[2].first + 1, part[2].end - 1 };

    Text d = { .ok = true };
    put(&d, name.ptr, name.len);
    put_str(&d, " ");
    put_range(&d, a, policy.first, policy.end);
    put_str(&d, " (");
    put(&d, tok(a, c1 - 1).ptr, tok(a, c1 - 1).len);
    put_str(&d, " : ");
    put_range(&d, a, workers.first, workers.end);
    put_str(&d, "; ");
    put(&d, tok(a, c2 - 1).ptr, tok(a, c2 - 1).len);
    put_str(&d, " : ");
    put_range(&d, a, box.first, box.end);
    const Local *l = box.end == box.first + 1 && ident(a, box.first) ? find_local(c, tok(a, box.first)) : NULL;
    if (l && l->spec.text.len) { put_str(&d, " ("); put(&d, l->spec.text.ptr, l->spec.text.len); put_str(&d, ")"); }
    if (capture.end > capture.first) { put_str(&d, "; ("); put_range(&d, a, capture.first, capture.end); put_str(&d, ")"); }
    put_str(&d, ")");

    resolution_begin(c);
    int t = type_of(c, box);
    const KType *k = t >= 0 ? kt(c, t) : NULL;
    if (k && k->kind == KT_MODIFIER && k->module && ast_of(c, k->module)) {
        const KAst *home = ast_of(c, k->module);
        Text sym = { .ok = true };
        bool adapt = false;
        if (container_verb(k, home, k_diag_text("partition"), 3, &sym, &adapt)) {
            put_str(&d, " \xe2\x86\x92 ");
            put(&d, sym.s, sym.n);
            if (adapt) put_str(&d, " &1");
            /* the binder's written type against the product of `partition` (§4.8,
               item 5): a `byref` product comes by pointer, so the stars go */
            Sig sig = find_verb(home, k_diag_text("partition"), 3);
            int r = sig.found ? ret_type(c, home, sig.fn, t) : -1;
            size_t last = binder.end - 1;
            while (last > binder.first && punct(a, last - 1, "*")) last--;
            int b = last > binder.first ? kt_from_text(c, a->symbols, range_text(a, (Range){ binder.first, last })) : -1;
            if (r >= 0 && b >= 0 && *kt(c, r)->symbol && *kt(c, b)->symbol &&
                strcmp(kt(c, r)->symbol, kt(c, b)->symbol) != 0)
            {
                Text prod = { .ok = true };
                put_str(&prod, kt(c, r)->symbol);
                diag3(c, K_DIAG_PARTITION_TYPE_MISMATCH, tok(a, binder.first),
                      range_text(a, (Range){ binder.first, last }), range_text(a, box), keep(c, &prod));
            }
        }
    }
    Text missing = { .ok = true };
    if (!meets(c, box, "Partitionable", &missing))
        diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, tok(a, box.first), range_text(a, box), k_diag_text("Partitionable"),
              keep(c, &missing));
    resolution_end(c);
    emit(c, K_ISLAND_PARALLEL, at, &d);
    mutations(c, body, end, box, "parallel");
    {
        int parent = local_of(c, box);
        if (parent >= 0 && c->pending_count < 8)
            c->pending[c->pending_count++] = (__typeof__(c->pending[0])){ tok(a, c2 - 1), parent, close };
    }

    /* the count of workers and the policy are constants (§4.8) */
    size_t dots = SIZE_MAX;
    for (size_t j = workers.first; j < workers.end; j++) if (punct(a, j, "..")) dots = j;
    long v;
    if (dots != SIZE_MAX && !decimal_of(c, (Range){ dots + 1, workers.end }, &v))
        diag3(c, K_DIAG_NONCONSTANT_PARALLEL, tok(a, dots + 1), range_text(a, (Range){ dots + 1, workers.end }), none, none);
    bool qualified = policy.end == policy.first + 3 && punct(a, policy.first + 1, ".");
    if (!word(a, policy.first, "ALL") && !word(a, policy.first, "ANY") && !qualified && !decimal_of(c, policy, &v))
        diag3(c, K_DIAG_NONCONSTANT_PARALLEL, tok(a, policy.first), range_text(a, policy), none, none);

    /* the name declares a `parallel.control` in the enclosing scope (§4.8), which
       the queries of keel.parallel read when that module is imported */
    const KModule *pm = module_by_name(a->symbols, "keel.parallel");
    if (pm && !c->quiet) {
        Text ty = { .ok = true };
        keel_slice_char alias = alias_of(a->symbols, pm);
        put(&ty, alias.ptr, alias.len);
        put_str(&ty, ".control");
        keel_slice_char text = keep(c, &ty);
        KLexer lexer;
        TKPpKind pp;
        KSpecifier spec;
        KToken next = {0};
        KToken first = enter_text(text, &lexer, &pp);
        if (k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp) && spec.kind != K_SPEC_NONE)
            add_local(c, (Local){ .name = name, .spec = spec, .decl_at = at + 1 });
    }
    for (size_t k2 = 0; k2 < c->par_count; k2++)
        if (k_symtab_same_name(c->par_names[k2], name))
            diag3(c, K_DIAG_DUPLICATE_PARALLEL_NAME, name, name, none, none);
    if (c->par_count < sizeof c->par_names / sizeof *c->par_names) c->par_names[c->par_count++] = name;
    if (inside_parallel(c, at)) {
        diag3(c, K_DIAG_NESTED_PARALLEL, tok(a, at), name, c->par_name, none);
        return;
    }
    c->par_open = body;
    c->par_close = end;
    c->par_name = name;
    c->cap_first = capture.first;
    c->cap_end = capture.end;
}

/* win; fail; return; and a write to a capture, in or out of a worker body */
static bool worker_flow(Ctx *c, size_t i) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    bool inside = inside_parallel(c, i);
    if ((word(a, i, "win") || word(a, i, "fail")) && punct(a, i + 1, ";")) {
        if (!inside) {
            diag3(c, K_DIAG_FLOW_VERB_OUTSIDE_PARALLEL, tok(a, i), tok(a, i), none, none);
            return true;
        }
        Text d = { .ok = true };
        put(&d, tok(a, i).ptr, tok(a, i).len);
        put_str(&d, " \xe2\x86\x92 ");
        put(&d, c->par_name.ptr, c->par_name.len);
        emit(c, K_ISLAND_WORKER_EXIT, i, &d);
        return true;
    }
    if (!inside) return false;
    bool write = assignment_op(a, i + 1) || (i > 0 && (punct(a, i - 1, "++") || punct(a, i - 1, "--")));
    bool through = i > 0 && punct(a, i - 1, "*") && !after_operand(a, i - 1);   /* `*out = v` writes the target */
    if (!write || through) return false;
    for (size_t j = c->cap_first; j < c->cap_end; j++) {
        if (!ident(a, j) || !k_symtab_same_name(tok(a, j), tok(a, i))) continue;
        const Local *l = find_local(c, tok(a, i));
        if (l && l->spec.kind == K_SPEC_MODIFIER && l->pointers == 0) return false;   /* a byref instance */
        diag3(c, K_DIAG_CAPTURED_WRITE, tok(a, i), tok(a, i), c->par_name, none);
        return false;
    }
    return false;
}

/* ---- extent (stage 4h) ------------------------------------------------ */

#define K_EXTENT_MAX 16

typedef struct {
    const KAst *home;
    size_t at;                              /* the word `extent` */
    keel_slice_char name;
    size_t groups;
    size_t count[8], cap[8];                /* token indexes of each group's names */
    size_t columns;
    size_t col[K_EXTENT_MAX];               /* the column's name */
    bool embedded[K_EXTENT_MAX];
    keel_slice_char dims[K_EXTENT_MAX];     /* an embedded column's dimensions, as written */
    size_t fields;
    keel_slice_char field[32];              /* every field's name, columns included */
} Extent;

/* `extent struct NAME [count, cap]... { fields }` in [n->first, n->end) of `home` (spec §4.11) */
static bool extent_scan(const KAst *home, const KAstNode *n, Extent *e) {
    memset(e, 0, sizeof *e);
    e->home = home;
    size_t j = n->first;
    while (j < n->end && (word(home, j, "pub") || word(home, j, "priv"))) j++;
    if (!word(home, j, "extent") || !word(home, j + 1, "struct") || !ident(home, j + 2)) return false;
    e->at = j;
    e->name = tok(home, j + 2);
    j += 3;
    while (punct(home, j, "[") && e->groups < 8) {
        size_t cl = close_bracket(home, j, n->end);
        if (cl == SIZE_MAX || cl != j + 4 || !punct(home, j + 2, ",")) return false;
        e->count[e->groups] = j + 1;
        e->cap[e->groups++] = j + 3;
        j = cl + 1;
    }
    if (!punct(home, j, "{")) return false;
    size_t close = j, depth = 0;
    for (; close < n->end; close++) {
        if (punct(home, close, "{")) depth++;
        else if (punct(home, close, "}") && --depth == 0) break;
    }
    for (size_t f = j + 1; f < close; ) {
        size_t end = f;
        depth = 0;
        while (end < close && !(depth == 0 && punct(home, end, ";"))) {
            if (punct(home, end, "(") || punct(home, end, "[") || punct(home, end, "{")) depth++;
            else if (punct(home, end, ")") || punct(home, end, "]") || punct(home, end, "}")) depth--;
            end++;
        }
        bool column = word(home, f, "array");
        /* each declarator: the last name before a ',' or a '[' at depth 0 */
        size_t name = SIZE_MAX, dims = SIZE_MAX;
        depth = 0;
        for (size_t k = f; k <= end; k++) {
            bool stop = k == end || (!depth && punct(home, k, ","));
            if (!stop) {
                if (punct(home, k, "[") && !depth && dims == SIZE_MAX) dims = k;
                if (punct(home, k, "(") || punct(home, k, "[") || punct(home, k, "{")) depth++;
                else if (punct(home, k, ")") || punct(home, k, "]") || punct(home, k, "}")) depth--;
                else if (!depth && dims == SIZE_MAX && ident(home, k)) name = k;
                continue;
            }
            if (name != SIZE_MAX) {
                if (e->fields < 32) e->field[e->fields++] = tok(home, name);
                if (column && e->columns < K_EXTENT_MAX) {
                    e->col[e->columns] = name;
                    e->embedded[e->columns] = dims != SIZE_MAX;
                    if (dims != SIZE_MAX)
                        e->dims[e->columns] = range_text(home, (Range){ dims, k });
                    e->columns++;
                }
            }
            name = dims = SIZE_MAX;
        }
        f = end + 1;
    }
    return true;
}

static bool extent_field(const Extent *e, keel_slice_char name) {
    for (size_t k = 0; k < e->fields; k++) if (k_symtab_same_name(e->field[k], name)) return true;
    return false;
}

/* the extent declared as `name` in `home`, if it is one */
static bool extent_named(const KAst *home, keel_slice_char name, Extent *e) {
    for (size_t i = 0; i < home->nodes.len; i++) {
        const KAstNode *n = node_at(home, i);
        if (n->kind == K_AST_TYPE && k_symtab_same_name(tok(home, n->name_first), name)) return extent_scan(home, n, e);
    }
    return false;
}

/* the declaration: its groups, its columns and their checks */
void extent_island(Ctx *c, const KAstNode *n) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    Extent e;
    if (!extent_scan(a, n, &e)) return;
    bool any_embedded = false, any_pointer = false;
    for (size_t k = 0; k < e.columns; k++) {
        if (e.embedded[k]) any_embedded = true;
        else any_pointer = true;
    }
    Text d = { .ok = true };
    put(&d, e.name.ptr, e.name.len);
    for (size_t g = 0; g < e.groups; g++) {
        put_str(&d, " [");
        put(&d, tok(a, e.count[g]).ptr, tok(a, e.count[g]).len);
        put_str(&d, ", ");
        put(&d, tok(a, e.cap[g]).ptr, tok(a, e.cap[g]).len);
        put_str(&d, "]");
    }
    put_str(&d, any_embedded ? " embedded:" : " pointer:");
    for (size_t k = 0; k < e.columns; k++) { put_str(&d, " "); put(&d, tok(a, e.col[k]).ptr, tok(a, e.col[k]).len); }
    emit(c, K_ISLAND_EXTENT, e.at, &d);

    bool field_cap = false;
    for (size_t g = 0; g < e.groups; g++) {
        KToken count = tok(a, e.count[g]), cap = tok(a, e.cap[g]);
        if (!extent_field(&e, count)) diag3(c, K_DIAG_EXTENT_COUNT_NOT_FIELD, count, count, e.name, none);
        long v;
        if (extent_field(&e, cap)) field_cap = true;
        else if (!decimal_of_token(c, cap, &v)) diag3(c, K_DIAG_EXTENT_UNKNOWN_CAPACITY, cap, cap, e.name, none);
    }
    if (!e.columns) { diag3(c, K_DIAG_EXTENT_WITHOUT_COLUMN, tok(a, e.at), e.name, none, none); return; }
    if (any_embedded && any_pointer) diag3(c, K_DIAG_EXTENT_MIXED_COLUMNS, tok(a, e.at), e.name, none, none);
    Text caps = { .ok = true };
    put_str(&caps, "[");
    for (size_t g = 0; g < e.groups; g++) {
        if (g) put_str(&caps, ", ");
        put(&caps, tok(a, e.cap[g]).ptr, tok(a, e.cap[g]).len);
    }
    put_str(&caps, "]");
    for (size_t k = 0; k < e.columns; k++) {
        if (!e.embedded[k]) continue;
        KToken col = tok(a, e.col[k]);
        if (field_cap) {
            for (size_t g = 0; g < e.groups; g++)
                if (extent_field(&e, tok(a, e.cap[g]))) {
                    diag3(c, K_DIAG_EXTENT_EMBEDDED_FIELD_CAPACITY, col, col, tok(a, e.cap[g]), none);
                    break;
                }
            continue;
        }
        /* the dimensions, one per group and named as its capacity */
        KLexer lexer;
        TKPpKind pp;
        k_lexer_init(&lexer, e.dims[k], NULL);
        size_t g = 0, depth = 0, tokens = 0;
        bool same = true;
        KToken only = {0};
        for (KToken t = k_lexer_next(&lexer, &pp); t.len; t = k_lexer_next(&lexer, &pp)) {
            bool open = k_token_is_punct(t, "["), close = k_token_is_punct(t, "]");
            if (open && depth++ == 0) { tokens = 0; continue; }
            if ((close && --depth == 0) || (depth == 1 && k_token_is_punct(t, ","))) {
                if (g >= e.groups || tokens != 1 || !k_symtab_same_name(only, tok(a, e.cap[g]))) same = false;
                g++;
                tokens = 0;
                continue;
            }
            if (depth >= 1) { tokens++; only = t; }
        }
        if (!same || g != e.groups)
            diag3(c, K_DIAG_EXTENT_DIMENSION_MISMATCH, col, col, e.dims[k], keep(c, &caps));
    }
}

/* `P.col[i, ...]` or `P->col[...]`, with `col` a column of an extent: the
   path [i, open) ends in `. col` or `-> col` (spec §4.11). Returns the token
   after the brackets, or 0 when the path is not a column. */
size_t column_access(Ctx *c, size_t i, size_t open, bool *any) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    if (open < i + 3 || !ident(a, open - 1) || !(punct(a, open - 2, ".") || punct(a, open - 2, "->"))) return 0;
    int t = type_of(c, (Range){ i, open - 2 });
    if (t < 0) return 0;
    const KType *k = kt(c, t);
    if (k->kind != KT_NAMED || !*k->symbol) return 0;
    const KAst *home = ast_of(c, k->module);
    Extent e;
    if (!home || !extent_named(home, k->name, &e)) return 0;
    size_t which = SIZE_MAX;
    for (size_t m = 0; m < e.columns; m++) if (k_symtab_same_name(tok(home, e.col[m]), tok(a, open - 1))) which = m;
    if (which == SIZE_MAX) return 0;
    size_t close = close_bracket(a, open, a->tokens.len);
    if (close == SIZE_MAX) return 0;
    Range idx[8];
    size_t indices = 0, st = open + 1, depth = 0;
    for (size_t m = open + 1; m <= close; m++) {
        if (m < close && (punct(a, m, "(") || punct(a, m, "[") || punct(a, m, "{"))) depth++;
        else if (m < close && (punct(a, m, ")") || punct(a, m, "]") || punct(a, m, "}"))) depth--;
        else if (m == close || (!depth && punct(a, m, ","))) {
            if (indices < 8) idx[indices] = (Range){ st, m };
            indices++;
            st = m + 1;
        }
    }
    keel_slice_char path = { (size_t)(tok(a, open - 1).ptr + tok(a, open - 1).len - tok(a, i).ptr), tok(a, i).ptr };
    Text d = { .ok = true };
    put(&d, path.ptr, path.len);
    put_str(&d, " rank ");
    put_uint(&d, indices);
    put_str(&d, " \xe2\x86\x92 ");
    put_str(&d, k->symbol);
    put_str(&d, "_");
    put(&d, tok(a, open - 1).ptr, tok(a, open - 1).len);
    put_str(&d, "_ptr");
    if (punct(a, open - 2, ".")) put_str(&d, " &1");
    *any = emit(c, K_ISLAND_COLUMN, i, &d) || *any;
    if (indices != e.groups) {
        Text r = { .ok = true }, w = { .ok = true };
        put_uint(&r, e.groups);
        put_uint(&w, indices);
        diag3(c, K_DIAG_EXTENT_INDEX_ARITY, tok(a, i), path, keep(c, &r), keep(c, &w));
        return close + 1;
    }
    for (size_t g = 0; g < indices && g < 8; g++) {
        long v, cap;
        if (decimal_of(c, idx[g], &v) && decimal_of_token(c, tok(home, e.cap[g]), &cap) && v >= cap)
            diag3(c, K_DIAG_EXTENT_INDEX_ABOVE_CAPACITY, tok(a, idx[g].first), range_text(a, idx[g]),
                  tok(home, e.cap[g]), none);
    }
    return close + 1;
}

/* `f(x)->col[i]` or `f(x).col[i]`: a column reached through a call, where `col`
   is a column of an extent of the file (spec §4.11) */
static void column_through_call(Ctx *c, const KAstNode *n, size_t i) {
    KAst *a = c->ast;
    if (!punct(a, i, ")") || !(punct(a, i + 1, ".") || punct(a, i + 1, "->")) || !ident(a, i + 2) ||
        !punct(a, i + 3, "["))
        return;
    for (size_t m = 0; m < a->nodes.len; m++) {
        const KAstNode *t = node_at(a, m);
        Extent e;
        if (t->kind != K_AST_TYPE || !extent_scan(a, t, &e)) continue;
        for (size_t k = 0; k < e.columns; k++)
            if (k_symtab_same_name(tok(a, e.col[k]), tok(a, i + 2))) {
                size_t start = i, depth = 0;
                while (start > n->first) {
                    if (punct(a, start, ")")) depth++;
                    else if (punct(a, start, "(") && --depth == 0) break;
                    start--;
                }
                if (start > n->first && ident(a, start - 1)) start--;
                keel_slice_char path = { (size_t)(tok(a, i + 2).ptr + tok(a, i + 2).len - tok(a, start).ptr),
                                         tok(a, start).ptr };
                diag3(c, K_DIAG_EXTENT_PATH_WITH_CALL, tok(a, start), path, (keel_slice_char){0}, (keel_slice_char){0});
                return;
            }
    }
}

/* ---- the else of a result (stage 4g) ----------------------------------- */

/* `T x = e else …;` and `x = e else …;` (spec §4.10). The `else` of an `if`
   follows a statement, so a ';' or a '}' stands before it; the `else` of a
   result follows an expression. The detail is the target, its keel type, the
   form, and the verbs the test calls: `failed`, and `win` for a default
   (cgen-tool §5.2). */
static void else_island(Ctx *c, const KAstNode *n, size_t at) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    size_t s = at, depth = 0;
    while (s > n->first) {
        size_t j = s - 1;
        if (punct(a, j, ")") || punct(a, j, "]")) depth++;
        else if (punct(a, j, "(") || punct(a, j, "[")) { if (!depth) break; depth--; }
        else if (!depth && (punct(a, j, ";") || punct(a, j, "{") || punct(a, j, "}"))) break;
        s = j;
    }
    size_t eq = SIZE_MAX, commas = 0;
    depth = 0;
    for (size_t j = s; j < at; j++) {
        if (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{")) depth++;
        else if (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}")) depth--;
        else if (!depth && punct(a, j, "=") && eq == SIZE_MAX) eq = j;
        else if (!depth && punct(a, j, ",")) commas++;
    }
    KLexer lexer;
    TKPpKind pp;
    KSpecifier spec;
    KToken next = {0};
    size_t head = s;
    while (head < at && (word(a, head, "const") || word(a, head, "volatile") || word(a, head, "static") ||
                         word(a, head, "register")))
        head++;
    KToken first = enter(a, head, &lexer, &pp);
    bool keel_decl = k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp) && spec.kind != K_SPEC_NONE;
    if (keel_decl && !(spec.kind == K_SPEC_NAMED_TYPE && k_primitive_name(spec.type_name))) {
        if (commas) { diag3(c, K_DIAG_ELSE_MULTIPLE_DECLARATORS, tok(a, at), none, none, none); return; }
        if (eq == SIZE_MAX) { diag3(c, K_DIAG_ELSE_WITHOUT_INITIALIZER, tok(a, at), none, none, none); return; }
    } else if (eq == SIZE_MAX) {
        diag3(c, K_DIAG_ELSE_WITHOUT_INITIALIZER, tok(a, at), none, none, none);
        return;
    } else if (eq != s + 1 || !ident(a, s)) {
        /* a C declaration — two names before the '=' — takes no `else`: its type
           is not keel's to test; anything else is a target that is not a name */
        bool c_decl = eq >= s + 2 && (ident(a, s) || k_token_is_c_word(tok(a, s))) && ident(a, eq - 1) &&
                      (ident(a, eq - 2) || k_token_is_c_word(tok(a, eq - 2)) ||
                                                            punct(a, eq - 2, "*"));
        for (size_t j = s; c_decl && j < eq; j++)
            if (punct(a, j, ".") || punct(a, j, "->") || punct(a, j, "[") || punct(a, j, "(")) c_decl = false;
        if (c_decl) diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, tok(a, eq - 1), range_text(a, (Range){ head, eq - 1 }),
                          k_diag_text("Failable"), k_diag_text("failed"));
        else diag3(c, K_DIAG_ELSE_ON_COMPLEX_TARGET, tok(a, s), range_text(a, (Range){ s, eq }), none, none);
        return;
    }
    if (!ident(a, eq - 1)) return;
    Range target = { eq - 1, eq };
    const Local *l = find_local(c, tok(a, eq - 1));
    if (!keel_decl && (!l || !l->spec.text.len)) {
        diag3(c, K_DIAG_ELSE_ON_COMPLEX_TARGET, tok(a, s), range_text(a, target), none, none);
        return;
    }
    bool exit = punct(a, at + 1, "{") || word(a, at + 1, "return") || word(a, at + 1, "break") ||
                word(a, at + 1, "continue") || word(a, at + 1, "goto");

    Text d = { .ok = true };
    put(&d, tok(a, eq - 1).ptr, tok(a, eq - 1).len);
    if (l && l->spec.text.len) { put_str(&d, " ("); put(&d, l->spec.text.ptr, l->spec.text.len); put_str(&d, ")"); }
    put_str(&d, exit ? " exit" : " default");
    Text missing = { .ok = true };
    resolution_begin(c);
    int t = type_of(c, target);
    const KType *k = t >= 0 ? kt(c, t) : NULL;
    if (!meets(c, target, "Failable", &missing))
        diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, tok(a, eq - 1), range_text(a, target), k_diag_text("Failable"),
              keep(c, &missing));
    else if (!exit && (missing = (Text){ .ok = true }, !meets(c, target, "Winnable", &missing)))
        diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, tok(a, eq - 1), range_text(a, target), k_diag_text("Winnable"),
              keep(c, &missing));
    else if (k && (k->kind == KT_MODIFIER || k->kind == KT_NAMED) && k->module && ast_of(c, k->module)) {
        const KAst *home = ast_of(c, k->module);
        Text sym = { .ok = true };
        bool adapt = false, win_adapt = false;
        if (container_verb(k, home, k_diag_text("failed"), 1, &sym, &adapt) &&
            (exit || (put_str(&sym, " "), container_verb(k, home, k_diag_text("win"), 2, &sym, &win_adapt)))) {
            put_str(&d, " \xe2\x86\x92 ");
            put(&d, sym.s, sym.n);
            if (win_adapt) put_str(&d, " &1");
        }
    }
    resolution_end(c);
    emit(c, K_ISLAND_ELSE, at, &d);
}

void walk(Ctx *c, const KAstNode *n) {
    KAst *a = c->ast;
    memcpy(c->locals, c->globals, c->global_count * sizeof *c->globals);
    c->local_count = c->global_count;
    c->depth = 0;
    c->has_target = false;
    c->par_open = c->par_close = 0;
    c->par_count = 0;
    c->later_count = 0;
    c->fn = n;
    c->pending_count = 0;
    c->cap_first = c->cap_end = 0;
    for (size_t i = n->first; i < n->end && !c->failed; i++) {
        if (!live(a, i)) continue;
        if (punct(a, i, "{")) { c->depth++; continue; }
        if (punct(a, i, "}")) {
            while (c->local_count && c->locals[c->local_count - 1].depth >= c->depth) c->local_count--;
            c->depth--;
            while (c->later_count && c->later[c->later_count - 1].depth > c->depth) c->later_count--;
            for (size_t k = 0; k < c->later_count; k++)
                if (c->later[k].shadow.len && c->later[k].shadow_depth > c->depth) c->later[k].shadow = (keel_slice_char){0};
            continue;
        }
        if (!signature_of(n, i) && (word(a, i, "return") || word(a, i, "break") || word(a, i, "continue") ||
                                    word(a, i, "goto")))
            for (size_t k = 0; k < c->later_count; k++)
                if (c->later[k].shadow.len && !c->later[k].told) {
                    diag3(c, K_DIAG_DEFER_LATER_SHADOWED, tok(a, i), c->later[k].shadow, tok(a, i), (keel_slice_char){0});
                    c->later[k].told = true;
                }
        if (punct(a, i, ";")) { c->has_target = false; continue; }
        if (!signature_of(n, i) && punct(a, i, ")")) { column_through_call(c, n, i); continue; }
        if (word(a, i, "constexpr") && !signature_of(n, i)) {
            check_constexpr(c, i, n->end);
            block_constexpr(c, i);
            continue;
        }
        if (!signature_of(n, i) && word(a, i, "else") && i > n->first && !punct(a, i - 1, ";") &&
            !punct(a, i - 1, "}")) {
            else_island(c, n, i);
            continue;
        }
        /* returning a symbol whose storage is local to the function (rule 5) */
        if (!signature_of(n, i) && word(a, i, "return") && ident(a, i + 1) && punct(a, i + 2, ";")) {
            int x = local_of(c, (Range){ i + 1, i + 2 });
            if (x >= 0 && c->locals[x].local_origin)
                diag3(c, K_DIAG_REGION_ESCAPE, tok(a, i + 1), tok(a, i + 1), (keel_slice_char){0}, (keel_slice_char){0});
        }
        if (word(a, i, "return") && inside_parallel(c, i)) {      /* a C word, not an identifier */
            diag3(c, K_DIAG_RETURN_IN_PARALLEL, tok(a, i), c->par_name, (keel_slice_char){0}, (keel_slice_char){0});
            continue;
        }
        if (!signature_of(n, i) && punct(a, i, "..")) {
            /* `a..`, `..b`, `..` belong in an index (spec §4.7) */
            bool open_left = punct(a, i - 1, "(") || punct(a, i - 1, ",") || punct(a, i - 1, ":") ||
                             punct(a, i - 1, "=") || punct(a, i - 1, "[");
            bool open_right = punct(a, i + 1, ")") || punct(a, i + 1, ",") || punct(a, i + 1, ";") ||
                              punct(a, i + 1, "]");
            if (open_left || open_right) {
                size_t j = i;
                for (size_t depth = 0; j-- > n->first; ) {
                    if (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}")) depth++;
                    else if (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{")) { if (!depth) break; depth--; }
                }
                if (!punct(a, j, "["))
                    diag3(c, K_DIAG_OPEN_RANGE_OUTSIDE_INDEX, tok(a, i), tok(a, i), (keel_slice_char){0}, (keel_slice_char){0});
            }
            continue;
        }
        if (i == n->name_first || !ident(a, i)) continue;
        if (!signature_of(n, i) && !(i > n->first && (punct(a, i - 1, ".") || punct(a, i - 1, "->")))) {
            int x = local_of(c, (Range){ i, i + 1 });
            bool assigned = punct(a, i + 1, "=");
            if (x >= 0 && c->locals[x].decl_at != i) {
                if (assigned) {
                    /* an assignment without a role leaves it with no guarantee (rule 10) */
                    c->locals[x].ndeps = 0;
                    c->locals[x].local_origin = c->locals[x].invalid = false;
                } else if (c->locals[x].invalid && !c->locals[x].told) {
                    diag3(c, K_DIAG_CHILD_REGION_AFTER_INVALIDATION, tok(a, i), tok(a, i), c->locals[x].invalid_by,
                          (keel_slice_char){0});
                    c->locals[x].told = true;
                }
            }
            /* `s = x[a..b]` and `T s = x[a..b];`: `s` is the product of `x` (rule 15) */
            if (x >= 0 && i >= 2 && punct(a, i - 1, "=") && ident(a, i - 2) && punct(a, i + 1, "[")) {
                size_t close = close_bracket(a, i + 1, n->end);
                bool range = false;
                for (size_t j = i + 2; close != SIZE_MAX && j < close; j++) if (punct(a, j, "..")) range = true;
                if (range && close != SIZE_MAX && punct(a, close + 1, ";")) {
                    int parent = x;
                    produce(c, local_of(c, (Range){ i - 2, i - 1 }), &parent, 1);
                }
            }
        }
        /* `corot.fault(r, c)` with a code known to be zero or negative (spec §5.5) */
        if (word(a, i, "fault") && punct(a, i + 1, "(") && i >= 2 && punct(a, i - 1, ".") && ident(a, i - 2)) {
            const KModule *m = module_alias(a->symbols, tok(a, i - 2));
            size_t close = close_paren(a, i + 1, n->end);
            if (m && k_symtab_same_name(m->name, k_diag_text("keel.corot")) && close != SIZE_MAX) {
                size_t comma = i + 2;
                for (size_t depth = 0; comma < close; comma++) {
                    if (punct(a, comma, "(") || punct(a, comma, "[")) depth++;
                    else if (punct(a, comma, ")") || punct(a, comma, "]")) depth--;
                    else if (!depth && punct(a, comma, ",")) break;
                }
                long v;
                bool negative = punct(a, comma + 1, "-");
                Range code = { comma + 1 + negative, close };
                if (comma < close && decimal_of(c, code, &v) && (negative || v == 0))
                    diag3(c, K_DIAG_INVALID_FAULT_CODE, tok(a, comma + 1), range_text(a, (Range){ comma + 1, close }),
                          (keel_slice_char){0}, (keel_slice_char){0});
            }
        }
        if (!signature_of(n, i) && c_declares(a, i, n->body_first - 1)) {
            check_name(c, tok(a, i), false, false);                 /* a C declaration in the body */
            if (c->later_count) note_shadow(c, tok(a, i));
        }
        if (i > n->first && (punct(a, i - 1, ".") || punct(a, i - 1, "->"))) continue;

        KToken t = tok(a, i);
        bool signature = signature_of(n, i);
        if (!signature && word(a, i, "defer")) { defer_island(c, n, i); continue; }
        if (!signature && word(a, i, "extern_c") && (punct(a, i + 1, "{") || punct(a, i + 1, "["))) {
            /* recognized only to be refused (spec §2.2, §4.1): its body is C */
            diag3(c, K_DIAG_NESTED_EXTERN_C, tok(a, i), (keel_slice_char){0}, (keel_slice_char){0}, (keel_slice_char){0});
            size_t b = i + 1;
            while (b < n->end && !punct(a, b, "{")) b++;
            for (size_t depth = 0; b < n->end; b++) {
                if (punct(a, b, "{")) depth++;
                else if (punct(a, b, "}") && --depth == 0) break;
            }
            i = b;
            continue;
        }
        if (!signature && word(a, i, "instance") && ident(a, i + 1) && !punct(a, i + 1, "=")) {
            size_t e = i + 1;
            while (e < n->end && !punct(a, e, ";") && !punct(a, e, "=") && !punct(a, e, "{")) e++;
            if (punct(a, e, ";")) {
                diag3(c, K_DIAG_INSTANCE_OUTSIDE_FILE_SCOPE, tok(a, i), range_text(a, (Range){ i + 1, e }),
                      (keel_slice_char){0}, (keel_slice_char){0});
                i = e;
                continue;
            }
        }
        if (!signature && punct(a, i + 1, "(") && (word(a, i, "foreach") || word(a, i, "walk"))) {
            resolution_begin(c);
            traversal(c, n, i, word(a, i, "walk"));
            resolution_end(c);
            continue;
        }
        if (!signature && word(a, i, "match") && punct(a, i + 1, "(")) {
            resolution_begin(c);
            match_island(c, n, i);
            resolution_end(c);
            continue;
        }
        /* `parallel` opens the construction only where it can: before a name, a
           policy or the parenthesis (spec §4.8); elsewhere it is a C name */
        if (!signature && word(a, i, "parallel") &&
            (ident(a, i + 1) || punct(a, i + 1, "(") || k_token_is_number(tok(a, i + 1)))) {
            parallel_island(c, n, i);
            continue;
        }
        if (!signature && worker_flow(c, i)) continue;
        check_uses(c, i);

        /* the marker, unless a qualifier: `array.get(`, with `array` an alias */
        if (k_token_spelled(t, "array") && !punct(a, i + 1, ".")) {
            i = array_decl(c, i, signature);
            continue;
        }
        if (k_token_spelled(t, "ref") && ident(a, i + 1) &&
            (punct(a, i - 1, "*") || word(a, i - 1, "const") || word(a, i - 1, "volatile"))) {
            Text d = { .ok = true };
            put(&d, tok(a, i + 1).ptr, tok(a, i + 1).len);
            emit(c, K_ISLAND_REF, i, &d);
            if (!signature && !punct(a, i + 2, "="))
                diag3(c, K_DIAG_REF_WITHOUT_INITIALIZER, tok(a, i + 1), tok(a, i + 1), (keel_slice_char){0},
                      (keel_slice_char){0});
            add_local(c, (Local){ .name = tok(a, i + 1), .pointers = 1, .decl_at = i + 1, .is_ref = true });
            continue;
        }

        /* a keel type, qualified or not */
        KLexer lexer;
        TKPpKind pp;
        KSpecifier spec;
        KToken next = {0};
        KToken first = enter(a, i, &lexer, &pp);
        bool scanned = k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp);
        if (scanned && spec.kind != K_SPEC_NONE) {
            if (spec.kind == K_SPEC_MODIFIER && i > n->first && word(a, i - 1, "restrict"))
                diag3(c, K_DIAG_RESTRICT_ON_CONTAINER, tok(a, i - 1), spec.text, (keel_slice_char){0},
                      (keel_slice_char){0});
            i = declaration(c, i, &spec, &lexer, next, signature, true);
            continue;
        }
        if (!scanned && punct(a, i + 1, "(") == false) {
            /* a modifier that reads a C arithmetic keyword as its argument (spec §4.2, item 6) */
            const KSymbol *m = k_symbol_resolve(a->symbols, t);
            static const char *const arithmetic[] = { "int", "long", "short", "unsigned", "signed", "float", "double" };
            if (m && m->kind == K_SYM_MODIFIER)
                for (size_t k = 0; k < sizeof arithmetic / sizeof *arithmetic; k++)
                    if (word(a, i + 1, arithmetic[k])) {
                        diag3(c, K_DIAG_C_TYPE_AS_ARGUMENT, tok(a, i + 1), tok(a, i), tok(a, i + 1),
                              (keel_slice_char){0});
                        break;
                    }
        }

        bool stmt_start = i == n->body_first + 1 || punct(a, i - 1, ";") || punct(a, i - 1, "{") || punct(a, i - 1, "}");
        /* `IDENT IDENT` or `IDENT * IDENT` opening a statement, the second a
           known keel symbol: a possible redeclaration, refused (spec §2.5) */
        if (!signature && stmt_start) {
            size_t second = punct(a, i + 1, "*") ? i + 2 : i + 1;
            if (ident(a, second) && (punct(a, second + 1, ";") || punct(a, second + 1, "=") ||
                                     punct(a, second + 1, ",") || punct(a, second + 1, "["))) {
                KToken nm = tok(a, second);
                const Local *l = find_local(c, nm);
                const KSymbol *sym = l ? NULL : k_symbol_resolve(a->symbols, nm);
                bool known = (l && l->spec.kind != K_SPEC_NONE) ||
                             (sym && sym->origin && (sym->kind == K_SYM_VARIABLE || sym->kind == K_SYM_FUNCTION ||
                                                     sym->kind == K_SYM_CONSTANT));
                if (known) diag3(c, K_DIAG_SYMBOL_REDECLARATION, nm, nm, tok(a, i), nm);
            }
        }
        /* `a = b;` between two `byref` instances by value: one storage, two
           names (spec §4.3, rule 17) */
        if (!signature && stmt_start && punct(a, i + 1, "=") && ident(a, i + 2) && punct(a, i + 3, ";")) {
            const Local *l = find_local(c, tok(a, i)), *r = find_local(c, tok(a, i + 2));
            if (l && r && l->pointers == 0 && r->pointers == 0 && l->spec.kind != K_SPEC_NONE &&
                r->spec.kind != K_SPEC_NONE && spec_byref(a, &l->spec) && spec_byref(a, &r->spec))
                diag3(c, K_DIAG_BYREF_ASSIGNMENT, tok(a, i), tok(a, i), tok(a, i + 2), (keel_slice_char){0});
        }

        /* `alias.verb(`, `verb(x, ...)` over a container, and a function of the file */
        if (call_at(c, i)) continue;
        if (punct(a, i + 1, ".") && ident(a, i + 2) && !punct(a, i + 3, "(")) {
            const KModule *module = module_alias(a->symbols, t);
            if (module) { qualified_name(c, i, module); i += 2; continue; }
        }
        if (punct(a, i + 1, "(")) continue;             /* a C function: not ours */
        /* a label is not a use */
        if (punct(a, i + 1, ":") && (i == n->first || punct(a, i - 1, "{") || punct(a, i - 1, ";") ||
                                     punct(a, i - 1, "}") || punct(a, i - 1, ":")))
            continue;
        /* `s.len` on an instance of a modifier: the fields belong to its module
           (instance-field-access, a warning) */
        if (!signature && (punct(a, i + 1, ".") || punct(a, i + 1, "->")) && ident(a, i + 2) && !punct(a, i + 3, "(")) {
            const Local *l = find_local(c, t);
            if (l && l->spec.kind == K_SPEC_MODIFIER && !l->rank)
                diag3(c, K_DIAG_INSTANCE_FIELD_ACCESS, tok(a, i + 2), tok(a, i + 2), t, l->spec.text);
        }
        if ((punct(a, i + 1, "[") || punct(a, i + 1, ".") || punct(a, i + 1, "->")) && path_forms(c, i)) continue;
        name_island(c, i);
    }
}
