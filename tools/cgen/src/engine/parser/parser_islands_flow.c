/* Pass 3: defer, foreach, walk and match; parallel, win and fail; extent; and the else of a result. Split from parser_islands.c by T3; the code is unchanged. */

#include "engine/parser/parser_islands_internal.h"

bool isl_tags_decl(const KAst *where, keel_slice_char name, KTagsDecl *out);
size_t isl_column_access(Ctx *c, size_t i, size_t open, bool *any);
/* ---- defer, foreach, walk, match (stages 4c, 4d, 4f) ----------------- */

void isl_put_range(Text *t, const KAst *a, size_t first, size_t end) {
    for (size_t j = first; j < end; j++) {
        if (j > first) {
            bool tight = isl_punct(a, j, ".") || isl_punct(a, j - 1, ".") || isl_punct(a, j, "..") || isl_punct(a, j - 1, "..") ||
                         isl_punct(a, j, "->") || isl_punct(a, j - 1, "->") || isl_punct(a, j, ",") || isl_punct(a, j - 1, "(") ||
                         isl_punct(a, j, ")") || isl_punct(a, j, "[") || isl_punct(a, j - 1, "[") || isl_punct(a, j, "]") ||
                         isl_punct(a, j - 1, "*") || isl_punct(a, j - 1, "&");
            if (!tight) isl_put_str(t, " ");
            else if (isl_punct(a, j - 1, ",")) isl_put_str(t, " ");
        }
        isl_put(t, isl_tok(a, j).ptr, isl_tok(a, j).len);
    }
}

/* the '{' that opens the block holding token `i`, or SIZE_MAX */
size_t isl_enclosing_brace(const KAst *a, size_t from, size_t i) {
    size_t depth = 0;
    for (size_t j = i; j-- > from; ) {
        if (isl_punct(a, j, "}")) depth++;
        else if (isl_punct(a, j, "{")) { if (!depth) return j; depth--; }
    }
    return SIZE_MAX;
}

/* the control word whose statement token `j` is the body of — `if`, `while`,
   `for`, `switch` before a ')', or `else` and `do' themselves — or NULL */
const char *isl_control_before(const KAst *a, size_t from, size_t j) {
    if (j <= from) return NULL;
    if (isl_word(a, j - 1, "else")) return "else";
    if (isl_word(a, j - 1, "do")) return "do";
    if (!isl_punct(a, j - 1, ")")) return NULL;
    size_t depth = 0;
    for (size_t k = j - 1; k-- > from; ) {
        if (isl_punct(a, k, ")")) depth++;
        else if (isl_punct(a, k, "(")) {
            if (depth) { depth--; continue; }
            static const char *const words[] = { "if", "while", "for", "switch" };
            for (size_t w = 0; w < 4; w++) if (k > from && isl_word(a, k - 1, words[w])) return words[w];
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
    const char *ctl = isl_control_before(a, n->first, at);
    if (ctl) isl_diag3(c, K_DIAG_DEFER_WITHOUT_BRACES, isl_tok(a, at), k_diag_text(ctl), none, none);
    size_t brace = isl_enclosing_brace(a, n->first, at);
    const char *block = brace != SIZE_MAX ? isl_control_before(a, n->first, brace) : NULL;
    if (block && (!strcmp(block, "if") || !strcmp(block, "else") || !strcmp(block, "switch")))
        isl_diag3(c, K_DIAG_DEFER_IN_CONTROL_BLOCK, isl_tok(a, at), k_diag_text(block), none, none);
    Text d = { .ok = true };
    if (isl_punct(a, at + 1, "[")) {
        size_t close = isl_close_bracket(a, at + 1, n->end);
        if (close == SIZE_MAX) return;
        if (isl_word(a, at + 2, "later") && close > at + 3)
            isl_diag3(c, K_DIAG_LATER_WITH_CAPTURE, isl_tok(a, at + 3), isl_range_text(a, (Range){ at + 3, close }), none, none);
        /* each entry of `[now ...]` ends in its name: keel rebuilds the copy
           from the tokens before it (spec §4.6, hidden-declarator) */
        if (isl_word(a, at + 2, "now"))
            for (size_t j = at + 3, st = at + 3, depth = 0; j <= close; j++) {
                if (j < close && (isl_punct(a, j, "(") || isl_punct(a, j, "["))) depth++;
                else if (j < close && (isl_punct(a, j, ")") || isl_punct(a, j, "]"))) depth--;
                else if (j == close || (!depth && isl_punct(a, j, ","))) {
                    if (j > st && !isl_ident(a, j - 1)) {
                        size_t nm = j - 1;
                        while (nm > st && !isl_ident(a, nm)) nm--;
                        isl_diag3(c, K_DIAG_HIDDEN_DECLARATOR, isl_tok(a, nm), isl_tok(a, nm), none, none);
                    }
                    st = j + 1;
                }
            }
        isl_put_range(&d, a, at + 2, close);
    } else {
        isl_put_str(&d, "later");
    }
    isl_emit(c, K_ISLAND_DEFER, at, &d);
    bool later = !isl_punct(a, at + 1, "[") || isl_word(a, at + 2, "later");
    if (later && c->later_count < sizeof c->later / sizeof *c->later) {
        size_t first = isl_punct(a, at + 1, "[") ? isl_close_bracket(a, at + 1, n->end) + 1 : at + 1;
        size_t end = first;
        for (size_t depth = 0; end < n->end; end++) {
            if (isl_punct(a, end, "{") || isl_punct(a, end, "(")) depth++;
            else if (isl_punct(a, end, "}") || isl_punct(a, end, ")")) { if (--depth == 0 && isl_punct(a, end, "}")) break; }
            else if (!depth && isl_punct(a, end, ";")) break;
        }
        c->later[c->later_count++] = (__typeof__(c->later[0])){ .at = at, .body_first = first, .body_end = end,
                                                                .depth = c->depth };
    }
}

/* the protocols of the base live in the prelude (spec §5.1, rule 9) */
static const KAst *protocols_ast(const Ctx *c) {
    const KModule *m = isl_module_by_name(c->ast->symbols, "keel");
    return m && m->ast ? m->ast : NULL;
}

/* Whether the type of `r`, which has kind KT_MODIFIER or KT_NAMED, meets the
   base protocol `proto`; `missing` names the verbs it lacks. A type the pass
   cannot see is taken as meeting it: C decides. */
static bool meets(Ctx *c, Range r, const char *proto, Text *missing) {
    int t = isl_type_of(c, r);
    if (t < 0) return true;
    const KType *k = isl_kt(c, t);
    if (k->kind != KT_MODIFIER && k->kind != KT_NAMED && k->kind != KT_ARRAY) return true;
    const KAst *pa = protocols_ast(c);
    KProtocolDecl pd;
    if (!pa || !isl_protocol_decl(c, pa, k_diag_text(proto), &pd, 0)) return true;
    const KAst *own;
    if (k->kind == KT_ARRAY) {
        /* an `array` meets a protocol by the verbs of keel.array (spec §5.1) */
        const KModule *am = isl_module_by_name(c->ast->symbols, "keel.array");
        own = am && am->ast ? am->ast : NULL;
        if (!own) { isl_put_str(missing, "its verbs"); return false; }
        return isl_satisfies(c, own, (keel_slice_char){0}, &pd, missing, isl_tok(c->ast, r.first));
    }
    own = k->module && k->module->ast ? k->module->ast : c->ast;
    return isl_satisfies(c, own, k->name, &pd, missing, isl_tok(c->ast, r.first));
}

/* the statement after token `at`: a block to its '}', or a statement to its ';' */
static size_t statement_end(const KAst *a, size_t at, size_t end) {
    size_t depth = 0;
    for (size_t j = at; j < end; j++) {
        if (isl_punct(a, j, "{") || isl_punct(a, j, "(") || isl_punct(a, j, "[")) depth++;
        else if (isl_punct(a, j, "}") || isl_punct(a, j, ")") || isl_punct(a, j, "]")) {
            if (depth && --depth == 0 && isl_punct(a, j, "}") && isl_punct(a, at, "{")) return j;
        }
        else if (!depth && isl_punct(a, j, ";")) return j;
    }
    return end;
}

/* `push`, `pop` or `clear` over the container `box` in [first, end): the body
   of the construction that goes over it (spec §4.7, §4.8) */
static void mutations(Ctx *c, size_t first, size_t end, Range box, const char *what) {
    KAst *a = c->ast;
    keel_slice_char text = isl_range_text(a, box);
    static const char *const verbs[] = { "push", "pop", "clear" };
    for (size_t j = first; j < end; j++) {
        if (!isl_punct(a, j + 1, "(")) continue;
        bool verb = false;
        for (size_t v = 0; v < 3; v++) if (isl_word(a, j, verbs[v])) verb = true;
        if (!verb) continue;
        size_t arg = j + 2, stop = arg;
        for (size_t depth = 0; stop < end; stop++) {
            if (isl_punct(a, stop, "(") || isl_punct(a, stop, "[")) depth++;
            else if ((isl_punct(a, stop, ")") || isl_punct(a, stop, "]")) && depth-- == 0) break;
            else if (!depth && isl_punct(a, stop, ",")) break;
        }
        while (arg < stop && isl_punct(a, arg, "&")) arg++;
        keel_slice_char got = isl_range_text(a, (Range){ arg, stop });
        if (stop > arg && got.len == text.len && !memcmp(got.ptr, text.ptr, text.len))
            isl_diag3(c, K_DIAG_MUTATION_DURING_TRAVERSAL, isl_tok(a, j), isl_tok(a, j), text, k_diag_text(what));
    }
}

/* foreach (binder[, binder] : container) and walk (elem, cursor : container)
   (spec §4.7). The detail is the binders' names, the container as written,
   and its keel type when it is a declared symbol (cgen-tool §5.2). */
static void traversal(Ctx *c, const KAstNode *n, size_t at, bool cursor) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    size_t open = at + 1, close = isl_close_paren(a, open, n->end);
    if (close == SIZE_MAX) return;
    size_t colon = SIZE_MAX, depth = 0;
    size_t comma[2]; size_t commas = 0;
    for (size_t j = open + 1; j < close; j++) {
        if (isl_punct(a, j, "(") || isl_punct(a, j, "[")) depth++;
        else if (isl_punct(a, j, ")") || isl_punct(a, j, "]")) depth--;
        else if (!depth && isl_punct(a, j, ":")) { colon = j; break; }
        else if (!depth && isl_punct(a, j, ",") && commas < 2) comma[commas++] = j;
    }
    if (colon == SIZE_MAX) return;
    size_t binders = commas + 1;
    Range b1 = { open + 1, commas ? comma[0] : colon }, b2 = { commas ? comma[0] + 1 : colon, colon };
    Range box = { colon + 1, close };
    bool literal = false, pointer = false;
    for (size_t j = box.first; j < box.end; j++) if (isl_punct(a, j, "..")) literal = true;
    for (size_t j = b1.first; j < b1.end; j++) if (isl_punct(a, j, "*")) pointer = true;

    Text d = { .ok = true };
    if (b1.end > b1.first) isl_put(&d, isl_tok(a, b1.end - 1).ptr, isl_tok(a, b1.end - 1).len);
    if (binders > 1 && b2.end > b2.first) { isl_put_str(&d, ", "); isl_put(&d, isl_tok(a, b2.end - 1).ptr, isl_tok(a, b2.end - 1).len); }
    isl_put_str(&d, " : ");
    isl_put_range(&d, a, box.first, box.end);
    const Local *l = box.end == box.first + 1 && isl_ident(a, box.first) ? isl_find_local(c, isl_tok(a, box.first)) : NULL;
    if (l && l->spec.text.len) { isl_put_str(&d, " ("); isl_put(&d, l->spec.text.ptr, l->spec.text.len); isl_put_str(&d, ")"); }
    isl_emit(c, cursor ? K_ISLAND_WALK : K_ISLAND_FOREACH, at, &d);
    /* an element binder by value that is an instance copies it (spec §4.7) */
    if (!pointer && b1.end > b1.first + 1) {
        KLexer lexer;
        TKPpKind pp;
        KSpecifier spec;
        KToken next = {0};
        KToken first = isl_enter(a, b1.first, &lexer, &pp);
        if (k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp) && spec.kind == K_SPEC_MODIFIER)
            isl_diag3(c, K_DIAG_BINDER_COPIES_CONTAINER, isl_tok(a, b1.end - 1), isl_tok(a, b1.end - 1), spec.text, none);
    }
    /* the cursor binder has the type `begin` produces (spec §4.7) */
    if (cursor && binders == 2 && b2.end > b2.first + 1) {
        int t = isl_type_of(c, box);
        const KType *k = t >= 0 ? isl_kt(c, t) : NULL;
        const KAst *home = k && k->kind == KT_MODIFIER && k->module ? isl_ast_of(c, k->module) : NULL;
        Sig sig = home ? isl_find_verb(home, k_diag_text("begin"), 1) : (Sig){0};
        int r = sig.found ? isl_ret_type(c, home, sig.fn, t) : -1;
        size_t last = b2.end - 1;
        while (last > b2.first && isl_punct(a, last - 1, "*")) last--;
        int b = last > b2.first ? isl_kt_from_text(c, a->symbols, isl_range_text(a, (Range){ b2.first, last })) : -1;
        if (r >= 0 && b >= 0 && *isl_kt(c, r)->symbol && *isl_kt(c, b)->symbol && strcmp(isl_kt(c, r)->symbol, isl_kt(c, b)->symbol)) {
            Text prod = { .ok = true };
            isl_put_str(&prod, isl_kt(c, r)->symbol);
            isl_diag3(c, K_DIAG_CURSOR_TYPE_MISMATCH, isl_tok(a, b2.first), isl_range_text(a, (Range){ b2.first, last }), isl_keep(c, &prod), none);
        }
    }
    if (!literal) mutations(c, close + 1, statement_end(a, close + 1, n->end), box, cursor ? "walk" : "foreach");
    /* the binder by pointer, and the cursor of `walk`, come from the container
       (spec §4.12, rules 14 and 15); a binder by value is a copy */
    int parent = literal ? -1 : isl_local_of(c, box);
    if (parent >= 0) {
        if (pointer && b1.end > b1.first && c->pending_count < 8)
            c->pending[c->pending_count++] = (__typeof__(c->pending[0])){ isl_tok(a, b1.end - 1), parent, close };
        if (cursor && binders > 1 && b2.end > b2.first && c->pending_count < 8)
            c->pending[c->pending_count++] = (__typeof__(c->pending[0])){ isl_tok(a, b2.end - 1), parent, close };
    }

    Text missing = { .ok = true };
    if (cursor) {
        if (binders < 2) { isl_diag3(c, K_DIAG_WALK_WITHOUT_CURSOR, isl_tok(a, at), isl_range_text(a, b1), none, none); return; }
        if (!literal && !meets(c, box, "Traversable", &missing))
            isl_diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, isl_tok(a, box.first), isl_range_text(a, box), k_diag_text("Traversable"),
                  isl_keep(c, &missing));
        return;
    }
    if (binders == 2 && literal)
        isl_diag3(c, K_DIAG_FOREACH_TWO_BINDERS_ON_LITERAL, isl_tok(a, box.first), isl_range_text(a, box), none, none);
    if (binders == 1 && literal && pointer)
        isl_diag3(c, K_DIAG_POINTER_BINDER_ON_RANGE, isl_tok(a, b1.first), isl_range_text(a, b1), none, none);
    if (binders == 2 && !(b2.end == b2.first + 2 && isl_word(a, b2.first, "size_t")))
        isl_diag3(c, K_DIAG_INDEX_NOT_SIZE_T, isl_tok(a, b2.first), isl_range_text(a, b2), none, none);
    if (literal) return;
    int t = isl_type_of(c, box);
    if (t >= 0 && isl_kt(c, t)->kind == KT_ARRAY) return;          /* the core's own traversal (spec §4.2) */
    const char *proto = binders == 1 ? "Countable" : pointer ? "IndexPtr" : "IndexGet";
    if (!meets(c, box, proto, &missing)) {
        Text other = { .ok = true };
        if (binders == 2 && !pointer && meets(c, box, "IndexPtr", &other)) return;
        isl_diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, isl_tok(a, box.first), isl_range_text(a, box), k_diag_text(proto),
              isl_keep(c, &missing));
    }
}

/* The tags declaration `name` in `where` (spec §4.9). */
bool isl_tags_decl(const KAst *where, keel_slice_char name, KTagsDecl *out) {
    for (size_t i = 0; i < where->nodes.len; i++) {
        const KAstNode *n = isl_node_at(where, i);
        if (n->kind != K_AST_TAGS || !k_symtab_same_name(isl_tok(where, n->name_first), name)) continue;
        size_t kw = n->first;
        while (kw < n->name_first && !isl_word(where, kw, "tags")) kw++;
        KToken t = isl_tok(where, kw);
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
    int t = isl_type_of(c, op);
    if (t < 0) return -1;
    const KType *k = isl_kt(c, t);
    const KAst *own = k->module && k->module->ast ? k->module->ast : c->ast;
    if (k->kind == KT_NAMED) {
        const KSymbol *s = k_symbol_resolve(c->ast->symbols, k->name);
        if (s && s->kind == K_SYM_TAGS) { *where = own; *set = k->name; return 1; }
    }
    if (k->kind != KT_MODIFIER && k->kind != KT_NAMED) return -1;
    Sig sig = isl_find_verb(own, k_diag_text("tag"), 1);
    if (!sig.found || (!sig.p[0].is_array && !k_symtab_same_name(sig.p[0].base, k->name))) return 0;
    size_t s = sig.fn->first;
    while (s < sig.fn->name_first && (isl_word(own, s, "pub") || isl_word(own, s, "priv") || isl_word(own, s, "inline")))
        s++;
    if (s >= sig.fn->name_first || !isl_ident(own, s)) return -1;
    keel_slice_char ret = isl_tok(own, s);
    if (k->kind == KT_MODIFIER) {
        const KAstNode *m = isl_module_node(own);
        int idx = 0;
        for (size_t j = m->dim_first; j < m->dim_end; j++) if (isl_ident(own, j)) idx++;
        for (size_t j = m->tags_first; j < m->tags_end; j++) {
            if (!isl_ident(own, j)) continue;
            if (k_symtab_same_name(isl_tok(own, j), ret) && idx < k->argc) {
                const KType *arg = isl_kt(c, k->arg[idx]);
                *where = arg->module && arg->module->ast ? arg->module->ast : c->ast;
                *set = arg->name;
                return 1;
            }
            idx++;
        }
    }
    for (size_t i = 0; i < own->nodes.len; i++)
        if (isl_node_at(own, i)->kind == K_AST_TAGS && k_symtab_same_name(isl_tok(own, isl_node_at(own, i)->name_first), ret)) {
            *where = own; *set = ret; return 1;
        }
    return 2;                                       /* `tag` returns no set */
}

/* match (operand) { LABEL: … } (spec §4.9). */
static void match_island(Ctx *c, const KAstNode *n, size_t at) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    size_t open = at + 1, close = isl_close_paren(a, open, n->end);
    if (close == SIZE_MAX || !isl_punct(a, close + 1, "{")) return;
    Range op = { open + 1, close };
    size_t body = close + 1, depth = 0, end = SIZE_MAX;
    for (size_t j = body; j < n->end; j++) {
        if (isl_punct(a, j, "{")) depth++;
        else if (isl_punct(a, j, "}") && --depth == 0) { end = j; break; }
    }
    if (end == SIZE_MAX) return;
    size_t labels[K_TAGS_MAX_ITEMS * 2], count = 0;
    depth = 0;
    for (size_t j = body + 1; j < end; j++) {
        if (isl_punct(a, j, "{") || isl_punct(a, j, "(") || isl_punct(a, j, "[")) depth++;
        else if (isl_punct(a, j, "}") || isl_punct(a, j, ")") || isl_punct(a, j, "]")) depth--;
        else if (!depth && isl_ident(a, j) && isl_punct(a, j + 1, ":") &&
                 (isl_punct(a, j - 1, "{") || isl_punct(a, j - 1, ";") || isl_punct(a, j - 1, "}") || isl_punct(a, j - 1, ":")) &&
                 count < sizeof labels / sizeof *labels)
            labels[count++] = j;
    }
    Text d = { .ok = true };
    isl_put_range(&d, a, op.first, op.end);
    const Local *l = op.end == op.first + 1 && isl_ident(a, op.first) ? isl_find_local(c, isl_tok(a, op.first)) : NULL;
    if (l && l->spec.text.len) { isl_put_str(&d, " ("); isl_put(&d, l->spec.text.ptr, l->spec.text.len); isl_put_str(&d, ")"); }
    isl_put_str(&d, ":");
    for (size_t k = 0; k < count; k++) { isl_put_str(&d, " "); isl_put(&d, isl_tok(a, labels[k]).ptr, isl_tok(a, labels[k]).len); }
    isl_emit(c, K_ISLAND_MATCH, at, &d);

    const KAst *where = NULL;
    keel_slice_char set = {0};
    int found = match_set(c, op, &where, &set);
    if (found == 0) {
        isl_diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, isl_tok(a, op.first), isl_range_text(a, op), k_diag_text("Taggable"), k_diag_text("tag"));
        return;
    }
    if (found == 2) { isl_diag3(c, K_DIAG_MATCH_WITHOUT_TAGS, isl_tok(a, op.first), isl_range_text(a, op), none, none); return; }
    KTagsDecl tags;
    if (found != 1 || !isl_tags_decl(where, set, &tags)) return;
    for (size_t k = 0; k < count; k++) {
        bool in = false;
        for (size_t m = 0; m < tags.item_count; m++) if (k_symtab_same_name(isl_tok(a, labels[k]), tags.items[m])) in = true;
        if (!in) isl_diag3(c, isl_tag_of_some_set(a, isl_tok(a, labels[k])) ? K_DIAG_TAG_FROM_OTHER_SET : K_DIAG_TAG_NOT_IN_SET,
                       isl_tok(a, labels[k]), isl_tok(a, labels[k]), set, none);
        for (size_t m = 0; m < k; m++)
            if (k_symtab_same_name(isl_tok(a, labels[k]), isl_tok(a, labels[m])))
                isl_diag3(c, K_DIAG_DUPLICATE_TAG, isl_tok(a, labels[k]), isl_tok(a, labels[k]), set, none);
    }
    for (size_t m = 0; m < tags.item_count; m++) {
        bool labelled = false;
        for (size_t k = 0; k < count; k++) if (k_symtab_same_name(isl_tok(a, labels[k]), tags.items[m])) labelled = true;
        if (!labelled) isl_diag3(c, K_DIAG_TAG_WITHOUT_LABEL, isl_tok(a, at), tags.items[m], set, none);
    }
}

/* ---- parallel, win, fail (stage 4e) ----------------------------------- */

static bool inside_parallel(const Ctx *c, size_t i) { return c->par_close && i > c->par_open && i < c->par_close; }

/* the ranges a `;` at depth 0 splits [first, end) into, up to `cap` */
static size_t split_semis(const KAst *a, size_t first, size_t end, Range *out, size_t cap) {
    size_t n = 0, depth = 0, st = first;
    for (size_t j = first; j <= end; j++) {
        if (j < end && (isl_punct(a, j, "(") || isl_punct(a, j, "[") || isl_punct(a, j, "{"))) depth++;
        else if (j < end && (isl_punct(a, j, ")") || isl_punct(a, j, "]") || isl_punct(a, j, "}"))) depth--;
        else if (j == end || (!depth && isl_punct(a, j, ";"))) {
            if (n < cap) out[n] = (Range){ st, j };
            n++;
            st = j + 1;
        }
    }
    return n;
}

static size_t colon_in(const KAst *a, Range r) {
    for (size_t j = r.first, depth = 0; j < r.end; j++) {
        if (isl_punct(a, j, "(") || isl_punct(a, j, "[")) depth++;
        else if (isl_punct(a, j, ")") || isl_punct(a, j, "]")) depth--;
        else if (!depth && isl_punct(a, j, ":")) return j;
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
    if (!isl_ident(a, at + 1) || isl_punct(a, at + 2, "(") || isl_word(a, at + 1, "ALL") || isl_word(a, at + 1, "ANY")) {
        isl_diag3(c, K_DIAG_UNNAMED_PARALLEL, isl_tok(a, at), none, none, none);
        return;
    }
    KToken name = isl_tok(a, at + 1);
    size_t open = at + 2;
    while (open < n->end && open < at + 6 && !isl_punct(a, open, "(")) open++;
    if (!isl_punct(a, open, "(")) return;
    Range policy = { at + 2, open };
    size_t close = isl_close_paren(a, open, n->end);
    if (close == SIZE_MAX || !isl_punct(a, close + 1, "{")) return;
    size_t body = close + 1, end = SIZE_MAX;
    for (size_t j = body, depth = 0; j < n->end; j++) {
        if (isl_punct(a, j, "{")) depth++;
        else if (isl_punct(a, j, "}") && --depth == 0) { end = j; break; }
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
    if (parts > 2 && isl_punct(a, part[2].first, "(") && part[2].end > part[2].first &&
        isl_punct(a, part[2].end - 1, ")"))
        capture = (Range){ part[2].first + 1, part[2].end - 1 };

    Text d = { .ok = true };
    isl_put(&d, name.ptr, name.len);
    isl_put_str(&d, " ");
    isl_put_range(&d, a, policy.first, policy.end);
    isl_put_str(&d, " (");
    isl_put(&d, isl_tok(a, c1 - 1).ptr, isl_tok(a, c1 - 1).len);
    isl_put_str(&d, " : ");
    isl_put_range(&d, a, workers.first, workers.end);
    isl_put_str(&d, "; ");
    isl_put(&d, isl_tok(a, c2 - 1).ptr, isl_tok(a, c2 - 1).len);
    isl_put_str(&d, " : ");
    isl_put_range(&d, a, box.first, box.end);
    const Local *l = box.end == box.first + 1 && isl_ident(a, box.first) ? isl_find_local(c, isl_tok(a, box.first)) : NULL;
    if (l && l->spec.text.len) { isl_put_str(&d, " ("); isl_put(&d, l->spec.text.ptr, l->spec.text.len); isl_put_str(&d, ")"); }
    if (capture.end > capture.first) { isl_put_str(&d, "; ("); isl_put_range(&d, a, capture.first, capture.end); isl_put_str(&d, ")"); }
    isl_put_str(&d, ")");

    isl_resolution_begin(c);
    int t = isl_type_of(c, box);
    const KType *k = t >= 0 ? isl_kt(c, t) : NULL;
    if (k && k->kind == KT_MODIFIER && k->module && isl_ast_of(c, k->module)) {
        const KAst *home = isl_ast_of(c, k->module);
        Text sym = { .ok = true };
        bool adapt = false;
        if (isl_container_verb(k, home, k_diag_text("partition"), 3, &sym, &adapt)) {
            isl_put_str(&d, " \xe2\x86\x92 ");
            isl_put(&d, sym.s, sym.n);
            if (adapt) isl_put_str(&d, " &1");
            /* the binder's written type against the product of `partition` (§4.8,
               item 5): a `byref` product comes by pointer, so the stars go */
            Sig sig = isl_find_verb(home, k_diag_text("partition"), 3);
            int r = sig.found ? isl_ret_type(c, home, sig.fn, t) : -1;
            size_t last = binder.end - 1;
            while (last > binder.first && isl_punct(a, last - 1, "*")) last--;
            int b = last > binder.first ? isl_kt_from_text(c, a->symbols, isl_range_text(a, (Range){ binder.first, last })) : -1;
            if (r >= 0 && b >= 0 && *isl_kt(c, r)->symbol && *isl_kt(c, b)->symbol &&
                strcmp(isl_kt(c, r)->symbol, isl_kt(c, b)->symbol) != 0)
            {
                Text prod = { .ok = true };
                isl_put_str(&prod, isl_kt(c, r)->symbol);
                isl_diag3(c, K_DIAG_PARTITION_TYPE_MISMATCH, isl_tok(a, binder.first),
                      isl_range_text(a, (Range){ binder.first, last }), isl_range_text(a, box), isl_keep(c, &prod));
            }
        }
    }
    Text missing = { .ok = true };
    if (!meets(c, box, "Partitionable", &missing))
        isl_diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, isl_tok(a, box.first), isl_range_text(a, box), k_diag_text("Partitionable"),
              isl_keep(c, &missing));
    isl_resolution_end(c);
    isl_emit(c, K_ISLAND_PARALLEL, at, &d);
    mutations(c, body, end, box, "parallel");
    {
        int parent = isl_local_of(c, box);
        if (parent >= 0 && c->pending_count < 8)
            c->pending[c->pending_count++] = (__typeof__(c->pending[0])){ isl_tok(a, c2 - 1), parent, close };
    }

    /* the count of workers and the policy are constants (§4.8) */
    size_t dots = SIZE_MAX;
    for (size_t j = workers.first; j < workers.end; j++) if (isl_punct(a, j, "..")) dots = j;
    long v;
    if (dots != SIZE_MAX && !isl_decimal_of(c, (Range){ dots + 1, workers.end }, &v))
        isl_diag3(c, K_DIAG_NONCONSTANT_PARALLEL, isl_tok(a, dots + 1), isl_range_text(a, (Range){ dots + 1, workers.end }), none, none);
    bool qualified = policy.end == policy.first + 3 && isl_punct(a, policy.first + 1, ".");
    if (!isl_word(a, policy.first, "ALL") && !isl_word(a, policy.first, "ANY") && !qualified && !isl_decimal_of(c, policy, &v))
        isl_diag3(c, K_DIAG_NONCONSTANT_PARALLEL, isl_tok(a, policy.first), isl_range_text(a, policy), none, none);

    /* the name declares a `parallel.control` in the enclosing scope (§4.8), which
       the queries of keel.parallel read when that module is imported */
    const KModule *pm = isl_module_by_name(a->symbols, "keel.parallel");
    if (pm && !c->quiet) {
        Text ty = { .ok = true };
        keel_slice_char alias = isl_alias_of(a->symbols, pm);
        isl_put(&ty, alias.ptr, alias.len);
        isl_put_str(&ty, ".control");
        keel_slice_char text = isl_keep(c, &ty);
        KLexer lexer;
        TKPpKind pp;
        KSpecifier spec;
        KToken next = {0};
        KToken first = isl_enter_text(text, &lexer, &pp);
        if (k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp) && spec.kind != K_SPEC_NONE)
            isl_add_local(c, (Local){ .name = name, .spec = spec, .decl_at = at + 1 });
    }
    for (size_t k2 = 0; k2 < c->par_count; k2++)
        if (k_symtab_same_name(c->par_names[k2], name))
            isl_diag3(c, K_DIAG_DUPLICATE_PARALLEL_NAME, name, name, none, none);
    if (c->par_count < sizeof c->par_names / sizeof *c->par_names) c->par_names[c->par_count++] = name;
    if (inside_parallel(c, at)) {
        isl_diag3(c, K_DIAG_NESTED_PARALLEL, isl_tok(a, at), name, c->par_name, none);
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
    if ((isl_word(a, i, "win") || isl_word(a, i, "fail")) && isl_punct(a, i + 1, ";")) {
        if (!inside) {
            isl_diag3(c, K_DIAG_FLOW_VERB_OUTSIDE_PARALLEL, isl_tok(a, i), isl_tok(a, i), none, none);
            return true;
        }
        Text d = { .ok = true };
        isl_put(&d, isl_tok(a, i).ptr, isl_tok(a, i).len);
        isl_put_str(&d, " \xe2\x86\x92 ");
        isl_put(&d, c->par_name.ptr, c->par_name.len);
        isl_emit(c, K_ISLAND_WORKER_EXIT, i, &d);
        return true;
    }
    if (!inside) return false;
    bool write = isl_assignment_op(a, i + 1) || (i > 0 && (isl_punct(a, i - 1, "++") || isl_punct(a, i - 1, "--")));
    bool through = i > 0 && isl_punct(a, i - 1, "*") && !isl_after_operand(a, i - 1);   /* `*out = v` writes the target */
    if (!write || through) return false;
    for (size_t j = c->cap_first; j < c->cap_end; j++) {
        if (!isl_ident(a, j) || !k_symtab_same_name(isl_tok(a, j), isl_tok(a, i))) continue;
        const Local *l = isl_find_local(c, isl_tok(a, i));
        if (l && l->spec.kind == K_SPEC_MODIFIER && l->pointers == 0) return false;   /* a byref instance */
        isl_diag3(c, K_DIAG_CAPTURED_WRITE, isl_tok(a, i), isl_tok(a, i), c->par_name, none);
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
    while (j < n->end && (isl_word(home, j, "pub") || isl_word(home, j, "priv"))) j++;
    if (!isl_word(home, j, "extent") || !isl_word(home, j + 1, "struct") || !isl_ident(home, j + 2)) return false;
    e->at = j;
    e->name = isl_tok(home, j + 2);
    j += 3;
    while (isl_punct(home, j, "[") && e->groups < 8) {
        size_t cl = isl_close_bracket(home, j, n->end);
        if (cl == SIZE_MAX || cl != j + 4 || !isl_punct(home, j + 2, ",")) return false;
        e->count[e->groups] = j + 1;
        e->cap[e->groups++] = j + 3;
        j = cl + 1;
    }
    if (!isl_punct(home, j, "{")) return false;
    size_t close = j, depth = 0;
    for (; close < n->end; close++) {
        if (isl_punct(home, close, "{")) depth++;
        else if (isl_punct(home, close, "}") && --depth == 0) break;
    }
    for (size_t f = j + 1; f < close; ) {
        size_t end = f;
        depth = 0;
        while (end < close && !(depth == 0 && isl_punct(home, end, ";"))) {
            if (isl_punct(home, end, "(") || isl_punct(home, end, "[") || isl_punct(home, end, "{")) depth++;
            else if (isl_punct(home, end, ")") || isl_punct(home, end, "]") || isl_punct(home, end, "}")) depth--;
            end++;
        }
        bool column = isl_word(home, f, "array");
        /* each declarator: the last name before a ',' or a '[' at depth 0 */
        size_t name = SIZE_MAX, dims = SIZE_MAX;
        depth = 0;
        for (size_t k = f; k <= end; k++) {
            bool stop = k == end || (!depth && isl_punct(home, k, ","));
            if (!stop) {
                if (isl_punct(home, k, "[") && !depth && dims == SIZE_MAX) dims = k;
                if (isl_punct(home, k, "(") || isl_punct(home, k, "[") || isl_punct(home, k, "{")) depth++;
                else if (isl_punct(home, k, ")") || isl_punct(home, k, "]") || isl_punct(home, k, "}")) depth--;
                else if (!depth && dims == SIZE_MAX && isl_ident(home, k)) name = k;
                continue;
            }
            if (name != SIZE_MAX) {
                if (e->fields < 32) e->field[e->fields++] = isl_tok(home, name);
                if (column && e->columns < K_EXTENT_MAX) {
                    e->col[e->columns] = name;
                    e->embedded[e->columns] = dims != SIZE_MAX;
                    if (dims != SIZE_MAX)
                        e->dims[e->columns] = isl_range_text(home, (Range){ dims, k });
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
        const KAstNode *n = isl_node_at(home, i);
        if (n->kind == K_AST_TYPE && k_symtab_same_name(isl_tok(home, n->name_first), name)) return extent_scan(home, n, e);
    }
    return false;
}

/* the declaration: its groups, its columns and their checks */
void isl_extent_island(Ctx *c, const KAstNode *n) {
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
    isl_put(&d, e.name.ptr, e.name.len);
    for (size_t g = 0; g < e.groups; g++) {
        isl_put_str(&d, " [");
        isl_put(&d, isl_tok(a, e.count[g]).ptr, isl_tok(a, e.count[g]).len);
        isl_put_str(&d, ", ");
        isl_put(&d, isl_tok(a, e.cap[g]).ptr, isl_tok(a, e.cap[g]).len);
        isl_put_str(&d, "]");
    }
    isl_put_str(&d, any_embedded ? " embedded:" : " pointer:");
    for (size_t k = 0; k < e.columns; k++) { isl_put_str(&d, " "); isl_put(&d, isl_tok(a, e.col[k]).ptr, isl_tok(a, e.col[k]).len); }
    isl_emit(c, K_ISLAND_EXTENT, e.at, &d);

    bool field_cap = false;
    for (size_t g = 0; g < e.groups; g++) {
        KToken count = isl_tok(a, e.count[g]), cap = isl_tok(a, e.cap[g]);
        if (!extent_field(&e, count)) isl_diag3(c, K_DIAG_EXTENT_COUNT_NOT_FIELD, count, count, e.name, none);
        long v;
        if (extent_field(&e, cap)) field_cap = true;
        else if (!isl_decimal_of_token(c, cap, &v)) isl_diag3(c, K_DIAG_EXTENT_UNKNOWN_CAPACITY, cap, cap, e.name, none);
    }
    if (!e.columns) { isl_diag3(c, K_DIAG_EXTENT_WITHOUT_COLUMN, isl_tok(a, e.at), e.name, none, none); return; }
    if (any_embedded && any_pointer) isl_diag3(c, K_DIAG_EXTENT_MIXED_COLUMNS, isl_tok(a, e.at), e.name, none, none);
    Text caps = { .ok = true };
    isl_put_str(&caps, "[");
    for (size_t g = 0; g < e.groups; g++) {
        if (g) isl_put_str(&caps, ", ");
        isl_put(&caps, isl_tok(a, e.cap[g]).ptr, isl_tok(a, e.cap[g]).len);
    }
    isl_put_str(&caps, "]");
    for (size_t k = 0; k < e.columns; k++) {
        if (!e.embedded[k]) continue;
        KToken col = isl_tok(a, e.col[k]);
        if (field_cap) {
            for (size_t g = 0; g < e.groups; g++)
                if (extent_field(&e, isl_tok(a, e.cap[g]))) {
                    isl_diag3(c, K_DIAG_EXTENT_EMBEDDED_FIELD_CAPACITY, col, col, isl_tok(a, e.cap[g]), none);
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
                if (g >= e.groups || tokens != 1 || !k_symtab_same_name(only, isl_tok(a, e.cap[g]))) same = false;
                g++;
                tokens = 0;
                continue;
            }
            if (depth >= 1) { tokens++; only = t; }
        }
        if (!same || g != e.groups)
            isl_diag3(c, K_DIAG_EXTENT_DIMENSION_MISMATCH, col, col, e.dims[k], isl_keep(c, &caps));
    }
}

/* `P.col[i, ...]` or `P->col[...]`, with `col` a column of an extent: the
   path [i, open) ends in `. col` or `-> col` (spec §4.11). Returns the token
   after the brackets, or 0 when the path is not a column. */
size_t isl_column_access(Ctx *c, size_t i, size_t open, bool *any) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    if (open < i + 3 || !isl_ident(a, open - 1) || !(isl_punct(a, open - 2, ".") || isl_punct(a, open - 2, "->"))) return 0;
    int t = isl_type_of(c, (Range){ i, open - 2 });
    if (t < 0) return 0;
    const KType *k = isl_kt(c, t);
    if (k->kind != KT_NAMED || !*k->symbol) return 0;
    const KAst *home = isl_ast_of(c, k->module);
    Extent e;
    if (!home || !extent_named(home, k->name, &e)) return 0;
    size_t which = SIZE_MAX;
    for (size_t m = 0; m < e.columns; m++) if (k_symtab_same_name(isl_tok(home, e.col[m]), isl_tok(a, open - 1))) which = m;
    if (which == SIZE_MAX) return 0;
    size_t close = isl_close_bracket(a, open, a->tokens.len);
    if (close == SIZE_MAX) return 0;
    Range idx[8];
    size_t indices = 0, st = open + 1, depth = 0;
    for (size_t m = open + 1; m <= close; m++) {
        if (m < close && (isl_punct(a, m, "(") || isl_punct(a, m, "[") || isl_punct(a, m, "{"))) depth++;
        else if (m < close && (isl_punct(a, m, ")") || isl_punct(a, m, "]") || isl_punct(a, m, "}"))) depth--;
        else if (m == close || (!depth && isl_punct(a, m, ","))) {
            if (indices < 8) idx[indices] = (Range){ st, m };
            indices++;
            st = m + 1;
        }
    }
    keel_slice_char path = { (size_t)(isl_tok(a, open - 1).ptr + isl_tok(a, open - 1).len - isl_tok(a, i).ptr), isl_tok(a, i).ptr };
    Text d = { .ok = true };
    isl_put(&d, path.ptr, path.len);
    isl_put_str(&d, " rank ");
    isl_put_uint(&d, indices);
    isl_put_str(&d, " \xe2\x86\x92 ");
    isl_put_str(&d, k->symbol);
    isl_put_str(&d, "_");
    isl_put(&d, isl_tok(a, open - 1).ptr, isl_tok(a, open - 1).len);
    isl_put_str(&d, "_ptr");
    if (isl_punct(a, open - 2, ".")) isl_put_str(&d, " &1");
    *any = isl_emit(c, K_ISLAND_COLUMN, i, &d) || *any;
    if (indices != e.groups) {
        Text r = { .ok = true }, w = { .ok = true };
        isl_put_uint(&r, e.groups);
        isl_put_uint(&w, indices);
        isl_diag3(c, K_DIAG_EXTENT_INDEX_ARITY, isl_tok(a, i), path, isl_keep(c, &r), isl_keep(c, &w));
        return close + 1;
    }
    for (size_t g = 0; g < indices && g < 8; g++) {
        long v, cap;
        if (isl_decimal_of(c, idx[g], &v) && isl_decimal_of_token(c, isl_tok(home, e.cap[g]), &cap) && v >= cap)
            isl_diag3(c, K_DIAG_EXTENT_INDEX_ABOVE_CAPACITY, isl_tok(a, idx[g].first), isl_range_text(a, idx[g]),
                  isl_tok(home, e.cap[g]), none);
    }
    return close + 1;
}

/* `f(x)->col[i]` or `f(x).col[i]`: a column reached through a call, where `col`
   is a column of an extent of the file (spec §4.11) */
static void column_through_call(Ctx *c, const KAstNode *n, size_t i) {
    KAst *a = c->ast;
    if (!isl_punct(a, i, ")") || !(isl_punct(a, i + 1, ".") || isl_punct(a, i + 1, "->")) || !isl_ident(a, i + 2) ||
        !isl_punct(a, i + 3, "["))
        return;
    for (size_t m = 0; m < a->nodes.len; m++) {
        const KAstNode *t = isl_node_at(a, m);
        Extent e;
        if (t->kind != K_AST_TYPE || !extent_scan(a, t, &e)) continue;
        for (size_t k = 0; k < e.columns; k++)
            if (k_symtab_same_name(isl_tok(a, e.col[k]), isl_tok(a, i + 2))) {
                size_t start = i, depth = 0;
                while (start > n->first) {
                    if (isl_punct(a, start, ")")) depth++;
                    else if (isl_punct(a, start, "(") && --depth == 0) break;
                    start--;
                }
                if (start > n->first && isl_ident(a, start - 1)) start--;
                keel_slice_char path = { (size_t)(isl_tok(a, i + 2).ptr + isl_tok(a, i + 2).len - isl_tok(a, start).ptr),
                                         isl_tok(a, start).ptr };
                isl_diag3(c, K_DIAG_EXTENT_PATH_WITH_CALL, isl_tok(a, start), path, (keel_slice_char){0}, (keel_slice_char){0});
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
        if (isl_punct(a, j, ")") || isl_punct(a, j, "]")) depth++;
        else if (isl_punct(a, j, "(") || isl_punct(a, j, "[")) { if (!depth) break; depth--; }
        else if (!depth && (isl_punct(a, j, ";") || isl_punct(a, j, "{") || isl_punct(a, j, "}"))) break;
        s = j;
    }
    size_t eq = SIZE_MAX, commas = 0;
    depth = 0;
    for (size_t j = s; j < at; j++) {
        if (isl_punct(a, j, "(") || isl_punct(a, j, "[") || isl_punct(a, j, "{")) depth++;
        else if (isl_punct(a, j, ")") || isl_punct(a, j, "]") || isl_punct(a, j, "}")) depth--;
        else if (!depth && isl_punct(a, j, "=") && eq == SIZE_MAX) eq = j;
        else if (!depth && isl_punct(a, j, ",")) commas++;
    }
    KLexer lexer;
    TKPpKind pp;
    KSpecifier spec;
    KToken next = {0};
    size_t head = s;
    while (head < at && (isl_word(a, head, "const") || isl_word(a, head, "volatile") || isl_word(a, head, "static") ||
                         isl_word(a, head, "register")))
        head++;
    KToken first = isl_enter(a, head, &lexer, &pp);
    bool keel_decl = k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp) && spec.kind != K_SPEC_NONE;
    if (keel_decl && !(spec.kind == K_SPEC_NAMED_TYPE && k_primitive_name(spec.type_name))) {
        if (commas) { isl_diag3(c, K_DIAG_ELSE_MULTIPLE_DECLARATORS, isl_tok(a, at), none, none, none); return; }
        if (eq == SIZE_MAX) { isl_diag3(c, K_DIAG_ELSE_WITHOUT_INITIALIZER, isl_tok(a, at), none, none, none); return; }
    } else if (eq == SIZE_MAX) {
        isl_diag3(c, K_DIAG_ELSE_WITHOUT_INITIALIZER, isl_tok(a, at), none, none, none);
        return;
    } else if (eq != s + 1 || !isl_ident(a, s)) {
        /* a C declaration — two names before the '=' — takes no `else`: its type
           is not keel's to test; anything else is a target that is not a name */
        bool c_decl = eq >= s + 2 && (isl_ident(a, s) || k_token_is_c_word(isl_tok(a, s))) && isl_ident(a, eq - 1) &&
                      (isl_ident(a, eq - 2) || k_token_is_c_word(isl_tok(a, eq - 2)) ||
                                                            isl_punct(a, eq - 2, "*"));
        for (size_t j = s; c_decl && j < eq; j++)
            if (isl_punct(a, j, ".") || isl_punct(a, j, "->") || isl_punct(a, j, "[") || isl_punct(a, j, "(")) c_decl = false;
        if (c_decl) isl_diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, isl_tok(a, eq - 1), isl_range_text(a, (Range){ head, eq - 1 }),
                          k_diag_text("Failable"), k_diag_text("failed"));
        else isl_diag3(c, K_DIAG_ELSE_ON_COMPLEX_TARGET, isl_tok(a, s), isl_range_text(a, (Range){ s, eq }), none, none);
        return;
    }
    if (!isl_ident(a, eq - 1)) return;
    Range target = { eq - 1, eq };
    const Local *l = isl_find_local(c, isl_tok(a, eq - 1));
    if (!keel_decl && (!l || !l->spec.text.len)) {
        isl_diag3(c, K_DIAG_ELSE_ON_COMPLEX_TARGET, isl_tok(a, s), isl_range_text(a, target), none, none);
        return;
    }
    bool exit = isl_punct(a, at + 1, "{") || isl_word(a, at + 1, "return") || isl_word(a, at + 1, "break") ||
                isl_word(a, at + 1, "continue") || isl_word(a, at + 1, "goto");

    Text d = { .ok = true };
    isl_put(&d, isl_tok(a, eq - 1).ptr, isl_tok(a, eq - 1).len);
    if (l && l->spec.text.len) { isl_put_str(&d, " ("); isl_put(&d, l->spec.text.ptr, l->spec.text.len); isl_put_str(&d, ")"); }
    isl_put_str(&d, exit ? " exit" : " default");
    Text missing = { .ok = true };
    isl_resolution_begin(c);
    int t = isl_type_of(c, target);
    const KType *k = t >= 0 ? isl_kt(c, t) : NULL;
    if (!meets(c, target, "Failable", &missing))
        isl_diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, isl_tok(a, eq - 1), isl_range_text(a, target), k_diag_text("Failable"),
              isl_keep(c, &missing));
    else if (!exit && (missing = (Text){ .ok = true }, !meets(c, target, "Winnable", &missing)))
        isl_diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, isl_tok(a, eq - 1), isl_range_text(a, target), k_diag_text("Winnable"),
              isl_keep(c, &missing));
    else if (k && (k->kind == KT_MODIFIER || k->kind == KT_NAMED) && k->module && isl_ast_of(c, k->module)) {
        const KAst *home = isl_ast_of(c, k->module);
        Text sym = { .ok = true };
        bool adapt = false, win_adapt = false;
        if (isl_container_verb(k, home, k_diag_text("failed"), 1, &sym, &adapt) &&
            (exit || (isl_put_str(&sym, " "), isl_container_verb(k, home, k_diag_text("win"), 2, &sym, &win_adapt)))) {
            isl_put_str(&d, " \xe2\x86\x92 ");
            isl_put(&d, sym.s, sym.n);
            if (win_adapt) isl_put_str(&d, " &1");
        }
    }
    isl_resolution_end(c);
    isl_emit(c, K_ISLAND_ELSE, at, &d);
}

void isl_walk(Ctx *c, const KAstNode *n) {
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
        if (!isl_live(a, i)) continue;
        if (isl_punct(a, i, "{")) { c->depth++; continue; }
        if (isl_punct(a, i, "}")) {
            while (c->local_count && c->locals[c->local_count - 1].depth >= c->depth) c->local_count--;
            c->depth--;
            while (c->later_count && c->later[c->later_count - 1].depth > c->depth) c->later_count--;
            for (size_t k = 0; k < c->later_count; k++)
                if (c->later[k].shadow.len && c->later[k].shadow_depth > c->depth) c->later[k].shadow = (keel_slice_char){0};
            continue;
        }
        if (!isl_signature_of(n, i) && (isl_word(a, i, "return") || isl_word(a, i, "break") || isl_word(a, i, "continue") ||
                                    isl_word(a, i, "goto")))
            for (size_t k = 0; k < c->later_count; k++)
                if (c->later[k].shadow.len && !c->later[k].told) {
                    isl_diag3(c, K_DIAG_DEFER_LATER_SHADOWED, isl_tok(a, i), c->later[k].shadow, isl_tok(a, i), (keel_slice_char){0});
                    c->later[k].told = true;
                }
        if (isl_punct(a, i, ";")) { c->has_target = false; continue; }
        if (!isl_signature_of(n, i) && isl_punct(a, i, ")")) { column_through_call(c, n, i); continue; }
        if (isl_word(a, i, "constexpr") && !isl_signature_of(n, i)) {
            isl_check_constexpr(c, i, n->end);
            isl_block_constexpr(c, i);
            continue;
        }
        if (!isl_signature_of(n, i) && isl_word(a, i, "else") && i > n->first && !isl_punct(a, i - 1, ";") &&
            !isl_punct(a, i - 1, "}")) {
            else_island(c, n, i);
            continue;
        }
        /* returning a symbol whose storage is local to the function (rule 5) */
        if (!isl_signature_of(n, i) && isl_word(a, i, "return") && isl_ident(a, i + 1) && isl_punct(a, i + 2, ";")) {
            int x = isl_local_of(c, (Range){ i + 1, i + 2 });
            if (x >= 0 && c->locals[x].local_origin)
                isl_diag3(c, K_DIAG_REGION_ESCAPE, isl_tok(a, i + 1), isl_tok(a, i + 1), (keel_slice_char){0}, (keel_slice_char){0});
        }
        if (isl_word(a, i, "return") && inside_parallel(c, i)) {      /* a C word, not an identifier */
            isl_diag3(c, K_DIAG_RETURN_IN_PARALLEL, isl_tok(a, i), c->par_name, (keel_slice_char){0}, (keel_slice_char){0});
            continue;
        }
        if (!isl_signature_of(n, i) && isl_punct(a, i, "..")) {
            /* `a..`, `..b`, `..` belong in an index (spec §4.7) */
            bool open_left = isl_punct(a, i - 1, "(") || isl_punct(a, i - 1, ",") || isl_punct(a, i - 1, ":") ||
                             isl_punct(a, i - 1, "=") || isl_punct(a, i - 1, "[");
            bool open_right = isl_punct(a, i + 1, ")") || isl_punct(a, i + 1, ",") || isl_punct(a, i + 1, ";") ||
                              isl_punct(a, i + 1, "]");
            if (open_left || open_right) {
                size_t j = i;
                for (size_t depth = 0; j-- > n->first; ) {
                    if (isl_punct(a, j, ")") || isl_punct(a, j, "]") || isl_punct(a, j, "}")) depth++;
                    else if (isl_punct(a, j, "(") || isl_punct(a, j, "[") || isl_punct(a, j, "{")) { if (!depth) break; depth--; }
                }
                if (!isl_punct(a, j, "["))
                    isl_diag3(c, K_DIAG_OPEN_RANGE_OUTSIDE_INDEX, isl_tok(a, i), isl_tok(a, i), (keel_slice_char){0}, (keel_slice_char){0});
            }
            continue;
        }
        if (i == n->name_first || !isl_ident(a, i)) continue;
        if (!isl_signature_of(n, i) && !(i > n->first && (isl_punct(a, i - 1, ".") || isl_punct(a, i - 1, "->")))) {
            int x = isl_local_of(c, (Range){ i, i + 1 });
            bool assigned = isl_punct(a, i + 1, "=");
            if (x >= 0 && c->locals[x].decl_at != i) {
                if (assigned) {
                    /* an assignment without a role leaves it with no guarantee (rule 10) */
                    c->locals[x].ndeps = 0;
                    c->locals[x].local_origin = c->locals[x].invalid = false;
                } else if (c->locals[x].invalid && !c->locals[x].told) {
                    isl_diag3(c, K_DIAG_CHILD_REGION_AFTER_INVALIDATION, isl_tok(a, i), isl_tok(a, i), c->locals[x].invalid_by,
                          (keel_slice_char){0});
                    c->locals[x].told = true;
                }
            }
            /* `s = x[a..b]` and `T s = x[a..b];`: `s` is the product of `x` (rule 15) */
            if (x >= 0 && i >= 2 && isl_punct(a, i - 1, "=") && isl_ident(a, i - 2) && isl_punct(a, i + 1, "[")) {
                size_t close = isl_close_bracket(a, i + 1, n->end);
                bool range = false;
                for (size_t j = i + 2; close != SIZE_MAX && j < close; j++) if (isl_punct(a, j, "..")) range = true;
                if (range && close != SIZE_MAX && isl_punct(a, close + 1, ";")) {
                    int parent = x;
                    isl_produce(c, isl_local_of(c, (Range){ i - 2, i - 1 }), &parent, 1);
                }
            }
        }
        /* `corot.fault(r, c)` with a code known to be zero or negative (spec §5.5) */
        if (isl_word(a, i, "fault") && isl_punct(a, i + 1, "(") && i >= 2 && isl_punct(a, i - 1, ".") && isl_ident(a, i - 2)) {
            const KModule *m = isl_module_alias(a->symbols, isl_tok(a, i - 2));
            size_t close = isl_close_paren(a, i + 1, n->end);
            if (m && k_symtab_same_name(m->name, k_diag_text("keel.corot")) && close != SIZE_MAX) {
                size_t comma = i + 2;
                for (size_t depth = 0; comma < close; comma++) {
                    if (isl_punct(a, comma, "(") || isl_punct(a, comma, "[")) depth++;
                    else if (isl_punct(a, comma, ")") || isl_punct(a, comma, "]")) depth--;
                    else if (!depth && isl_punct(a, comma, ",")) break;
                }
                long v;
                bool negative = isl_punct(a, comma + 1, "-");
                Range code = { comma + 1 + negative, close };
                if (comma < close && isl_decimal_of(c, code, &v) && (negative || v == 0))
                    isl_diag3(c, K_DIAG_INVALID_FAULT_CODE, isl_tok(a, comma + 1), isl_range_text(a, (Range){ comma + 1, close }),
                          (keel_slice_char){0}, (keel_slice_char){0});
            }
        }
        if (!isl_signature_of(n, i) && isl_c_declares(a, i, n->body_first - 1)) {
            isl_check_name(c, isl_tok(a, i), false, false);                 /* a C declaration in the body */
            if (c->later_count) isl_note_shadow(c, isl_tok(a, i));
        }
        if (i > n->first && (isl_punct(a, i - 1, ".") || isl_punct(a, i - 1, "->"))) continue;

        KToken t = isl_tok(a, i);
        bool signature = isl_signature_of(n, i);
        if (!signature && isl_word(a, i, "defer")) { defer_island(c, n, i); continue; }
        if (!signature && isl_word(a, i, "extern_c") && (isl_punct(a, i + 1, "{") || isl_punct(a, i + 1, "["))) {
            /* recognized only to be refused (spec §2.2, §4.1): its body is C */
            isl_diag3(c, K_DIAG_NESTED_EXTERN_C, isl_tok(a, i), (keel_slice_char){0}, (keel_slice_char){0}, (keel_slice_char){0});
            size_t b = i + 1;
            while (b < n->end && !isl_punct(a, b, "{")) b++;
            for (size_t depth = 0; b < n->end; b++) {
                if (isl_punct(a, b, "{")) depth++;
                else if (isl_punct(a, b, "}") && --depth == 0) break;
            }
            i = b;
            continue;
        }
        if (!signature && isl_word(a, i, "instance") && isl_ident(a, i + 1) && !isl_punct(a, i + 1, "=")) {
            size_t e = i + 1;
            while (e < n->end && !isl_punct(a, e, ";") && !isl_punct(a, e, "=") && !isl_punct(a, e, "{")) e++;
            if (isl_punct(a, e, ";")) {
                isl_diag3(c, K_DIAG_INSTANCE_OUTSIDE_FILE_SCOPE, isl_tok(a, i), isl_range_text(a, (Range){ i + 1, e }),
                      (keel_slice_char){0}, (keel_slice_char){0});
                i = e;
                continue;
            }
        }
        if (!signature && isl_punct(a, i + 1, "(") && (isl_word(a, i, "foreach") || isl_word(a, i, "walk"))) {
            isl_resolution_begin(c);
            traversal(c, n, i, isl_word(a, i, "walk"));
            isl_resolution_end(c);
            continue;
        }
        if (!signature && isl_word(a, i, "match") && isl_punct(a, i + 1, "(")) {
            isl_resolution_begin(c);
            match_island(c, n, i);
            isl_resolution_end(c);
            continue;
        }
        /* `parallel` opens the construction only where it can: before a name, a
           policy or the parenthesis (spec §4.8); elsewhere it is a C name */
        if (!signature && isl_word(a, i, "parallel") &&
            (isl_ident(a, i + 1) || isl_punct(a, i + 1, "(") || k_token_is_number(isl_tok(a, i + 1)))) {
            parallel_island(c, n, i);
            continue;
        }
        if (!signature && worker_flow(c, i)) continue;
        isl_check_uses(c, i);

        /* the marker, unless a qualifier: `array.get(`, with `array` an alias */
        if (k_token_spelled(t, "array") && !isl_punct(a, i + 1, ".")) {
            i = isl_array_decl(c, i, signature);
            continue;
        }
        if (k_token_spelled(t, "ref") && isl_ident(a, i + 1) &&
            (isl_punct(a, i - 1, "*") || isl_word(a, i - 1, "const") || isl_word(a, i - 1, "volatile"))) {
            Text d = { .ok = true };
            isl_put(&d, isl_tok(a, i + 1).ptr, isl_tok(a, i + 1).len);
            isl_emit(c, K_ISLAND_REF, i, &d);
            if (!signature && !isl_punct(a, i + 2, "="))
                isl_diag3(c, K_DIAG_REF_WITHOUT_INITIALIZER, isl_tok(a, i + 1), isl_tok(a, i + 1), (keel_slice_char){0},
                      (keel_slice_char){0});
            isl_add_local(c, (Local){ .name = isl_tok(a, i + 1), .pointers = 1, .decl_at = i + 1, .is_ref = true });
            continue;
        }

        /* a keel type, qualified or not */
        KLexer lexer;
        TKPpKind pp;
        KSpecifier spec;
        KToken next = {0};
        KToken first = isl_enter(a, i, &lexer, &pp);
        bool scanned = k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp);
        if (scanned && spec.kind != K_SPEC_NONE) {
            if (spec.kind == K_SPEC_MODIFIER && i > n->first && isl_word(a, i - 1, "restrict"))
                isl_diag3(c, K_DIAG_RESTRICT_ON_CONTAINER, isl_tok(a, i - 1), spec.text, (keel_slice_char){0},
                      (keel_slice_char){0});
            i = isl_declaration(c, i, &spec, &lexer, next, signature, true);
            continue;
        }
        if (!scanned && isl_punct(a, i + 1, "(") == false) {
            /* a modifier that reads a C arithmetic keyword as its argument (spec §4.2, item 6) */
            const KSymbol *m = k_symbol_resolve(a->symbols, t);
            static const char *const arithmetic[] = { "int", "long", "short", "unsigned", "signed", "float", "double" };
            if (m && m->kind == K_SYM_MODIFIER)
                for (size_t k = 0; k < sizeof arithmetic / sizeof *arithmetic; k++)
                    if (isl_word(a, i + 1, arithmetic[k])) {
                        isl_diag3(c, K_DIAG_C_TYPE_AS_ARGUMENT, isl_tok(a, i + 1), isl_tok(a, i), isl_tok(a, i + 1),
                              (keel_slice_char){0});
                        break;
                    }
        }

        bool stmt_start = i == n->body_first + 1 || isl_punct(a, i - 1, ";") || isl_punct(a, i - 1, "{") || isl_punct(a, i - 1, "}");
        /* `IDENT IDENT` or `IDENT * IDENT` opening a statement, the second a
           known keel symbol: a possible redeclaration, refused (spec §2.5) */
        if (!signature && stmt_start) {
            size_t second = isl_punct(a, i + 1, "*") ? i + 2 : i + 1;
            if (isl_ident(a, second) && (isl_punct(a, second + 1, ";") || isl_punct(a, second + 1, "=") ||
                                     isl_punct(a, second + 1, ",") || isl_punct(a, second + 1, "["))) {
                KToken nm = isl_tok(a, second);
                const Local *l = isl_find_local(c, nm);
                const KSymbol *sym = l ? NULL : k_symbol_resolve(a->symbols, nm);
                bool known = (l && l->spec.kind != K_SPEC_NONE) ||
                             (sym && sym->origin && (sym->kind == K_SYM_VARIABLE || sym->kind == K_SYM_FUNCTION ||
                                                     sym->kind == K_SYM_CONSTANT));
                if (known) isl_diag3(c, K_DIAG_SYMBOL_REDECLARATION, nm, nm, isl_tok(a, i), nm);
            }
        }
        /* `a = b;` between two `byref` instances by value: one storage, two
           names (spec §4.3, rule 17) */
        if (!signature && stmt_start && isl_punct(a, i + 1, "=") && isl_ident(a, i + 2) && isl_punct(a, i + 3, ";")) {
            const Local *l = isl_find_local(c, isl_tok(a, i)), *r = isl_find_local(c, isl_tok(a, i + 2));
            if (l && r && l->pointers == 0 && r->pointers == 0 && l->spec.kind != K_SPEC_NONE &&
                r->spec.kind != K_SPEC_NONE && isl_spec_byref(a, &l->spec) && isl_spec_byref(a, &r->spec))
                isl_diag3(c, K_DIAG_BYREF_ASSIGNMENT, isl_tok(a, i), isl_tok(a, i), isl_tok(a, i + 2), (keel_slice_char){0});
        }

        /* `alias.verb(`, `verb(x, ...)` over a container, and a function of the file */
        if (isl_call_at(c, i)) continue;
        if (isl_punct(a, i + 1, ".") && isl_ident(a, i + 2) && !isl_punct(a, i + 3, "(")) {
            const KModule *module = isl_module_alias(a->symbols, t);
            if (module) { isl_qualified_name(c, i, module); i += 2; continue; }
        }
        if (isl_punct(a, i + 1, "(")) continue;             /* a C function: not ours */
        /* a label is not a use */
        if (isl_punct(a, i + 1, ":") && (i == n->first || isl_punct(a, i - 1, "{") || isl_punct(a, i - 1, ";") ||
                                     isl_punct(a, i - 1, "}") || isl_punct(a, i - 1, ":")))
            continue;
        /* `s.len` on an instance of a modifier: the fields belong to its module
           (instance-field-access, a warning) */
        if (!signature && (isl_punct(a, i + 1, ".") || isl_punct(a, i + 1, "->")) && isl_ident(a, i + 2) && !isl_punct(a, i + 3, "(")) {
            const Local *l = isl_find_local(c, t);
            if (l && l->spec.kind == K_SPEC_MODIFIER && !l->rank)
                isl_diag3(c, K_DIAG_INSTANCE_FIELD_ACCESS, isl_tok(a, i + 2), isl_tok(a, i + 2), t, l->spec.text);
        }
        if ((isl_punct(a, i + 1, "[") || isl_punct(a, i + 1, ".") || isl_punct(a, i + 1, "->")) && isl_path_forms(c, i)) continue;
        isl_name_island(c, i);
    }
}
