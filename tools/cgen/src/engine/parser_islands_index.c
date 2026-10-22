/* Pass 3: the `array`, `array-index`, `index` and `range-index` islands. Split from parser_islands.c by T3; the code is unchanged. */

#include "engine/parser_islands_internal.h"

/* ---- stage 4b: array, array-index, index, range-index ------------------ */

/* decl-array (spec §2.2, §4.2) at the `array` word `i`: one island per declared
   name, with the dimensions as written, and the name known as an array from
   there on. Returns the last token of the element type, which is not a
   declaration of its own: the caller skips to it. */
size_t array_decl(Ctx *c, size_t i, bool signature) {
    KAst *a = c->ast;
    if (i + 1 >= a->tokens.len) return i;
    KLexer lexer;
    TKPpKind pp;
    KToken next = {0}, first = enter(a, i + 1, &lexer, &pp);
    keel_slice_char element;
    if (!k_scan_argument(&lexer, first, a->symbols, &element, &next, &pp)) return i;
    size_t last = index_at(a, i + 1, next.len ? next.ptr : a->source.ptr + a->source.len) - 1;

    /* a keel element type is a use like any other; the declarators follow it */
    KLexer look;
    KSpecifier spec;
    KToken after_type = {0}, head = enter(a, i + 1, &look, &pp);
    if (k_token_is_ident(head) && k_scan_known_type(&look, head, a->symbols, &spec, &after_type, &pp) &&
        spec.kind != K_SPEC_NONE)
        declaration(c, i + 1, &spec, &look, after_type, signature, false);

    const char *source_end = a->source.ptr + a->source.len;
    for (KToken d = next; d.len; ) {
        KDeclarator decl;
        KToken after;
        if (!(k_token_is_punct(d, "*") || k_token_is_ident(d))) break;
        if (!k_scan_declarator(&lexer, d, &decl, &after, &pp) || !decl.name.len ||
            !decl.has_array_suffix || decl.parenthesized) break;
        const char *dims = decl.name.ptr + decl.name.len, *end = after.len ? after.ptr : source_end;
        Text t = { .ok = true };
        put(&t, decl.name.ptr, decl.name.len);
        put_str(&t, " ");
        put_source(&t, dims, end);
        emit(c, K_ISLAND_ARRAY, i, &t);
        if (signature) {
            /* a parameter names its dimension 0 with a binder, or is not one-dimensional (spec §4.2, item 19) */
            const char *p = dims, *q = end;
            while (p < q && (*p == ' ' || *p == '[')) p++;
            bool empty = p < q && *p == ']';
            bool binder_written = q - p >= 6 && !memcmp(p, "size_t", 6);
            if (empty)
                diag3(c, K_DIAG_ARRAY_PARAMETER_WITHOUT_DIMENSION, decl.name, decl.name, (keel_slice_char){0},
                      (keel_slice_char){0});
            else if (rank_of(dims, end) == 1 && !binder_written)
                diag3(c, K_DIAG_ARRAY_1D_AS_PARAMETER, decl.name, decl.name, (keel_slice_char){ (size_t)(end - dims), (char *)dims },
                      (keel_slice_char){0});
        }
        add_local(c, (Local){ .name = decl.name, .pointers = decl.pointer_depth + 1,
                              .decl_at = index_at(a, i, decl.name.ptr), .rank = rank_of(dims, end),
                              .element = element, .dims = { (size_t)(end - dims), (char *)dims },
                              /* an automatic `array` is a local origin (spec §4.12, rule 3) */
                              .local_origin = !signature && c->depth > 0 && !(i > 0 && word(a, i - 1, "static")) });
        if (k_token_is_punct(after, "=")) {
            static const char *const stops[] = { ",", ";" };
            size_t which;
            KToken value = k_lexer_next(&lexer, &pp);
            k_scan_opaque_until(&lexer, value, stops, 2, &which, &after, &pp);
        }
        if (signature || !k_token_is_punct(after, ",")) break;
        d = k_lexer_next(&lexer, &pp);
    }
    return last;
}

/* `x[a..b]` with both limits known decimals and a start above the end */
static void check_inverted(Ctx *c, KToken name, size_t open, size_t range, size_t close) {
    const KAst *a = c->ast;
    long low, high;
    if (range > open + 1 && range + 1 < close &&
        decimal_of(c, (Range){ open + 1, range }, &low) && decimal_of(c, (Range){ range + 1, close }, &high) &&
        low > high)
        diag3(c, K_DIAG_INVERTED_RANGE_INDEX, name,
              (keel_slice_char){ (size_t)(tok(a, close).ptr - tok(a, open).ptr) - 1, tok(a, open).ptr + 1 },
              (keel_slice_char){0}, (keel_slice_char){0});
}

/* `<type symbol>_<verb><suffix>` for a container and a call of `arity` arguments,
   object included; `adapt` says the object takes an `&`. */
bool container_verb(const KType *k, const KAst *home, keel_slice_char verb, size_t arity, Text *tx,
                           bool *adapt) {
    Sig sig = find_verb(home, verb, arity);
    if (!sig.found || !*k->symbol) return false;
    put_str(tx, k->symbol);
    put_str(tx, "_");
    put(tx, verb.ptr, verb.len);
    if (sig.overloaded && arity > 1) put_uint(tx, arity - 1);
    if (sig.n && sig.p[0].pointer && k->pointers == 0 && k->lvalue) *adapt = true;
    return true;
}

/* One bracket group after the path [i, open): the islands of `x[…]`, where the
   container is any expression this pass can type (spec §2.2). An `array` takes
   its indices as they come, in one bracket or several; a modifier's container
   goes through `ptr`, or the verb of range-index. Returns the token after what
   it consumed. */
static size_t index_group(Ctx *c, size_t i, size_t open, bool *any) {
    KAst *a = c->ast;
    size_t close = close_bracket(a, open, a->tokens.len);
    if (close == SIZE_MAX) return a->tokens.len;
    KToken name = tok(a, i);
    int t = type_of(c, (Range){ i, open });
    if (t < 0) return close + 1;
    const KType *k = kt(c, t);
    keel_slice_char path = { (size_t)(tok(a, open - 1).ptr + tok(a, open - 1).len - name.ptr), name.ptr };
    keel_slice_char none = {0};

    size_t commas = 0, range = SIZE_MAX, depth = 0;
    for (size_t j = open + 1; j < close; j++) {
        if (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{")) depth++;
        else if (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}")) depth--;
        else if (depth == 0 && punct(a, j, ",")) commas++;
        else if (depth == 0 && punct(a, j, "..") && range == SIZE_MAX) range = j;
    }

    /* an open end reads the container twice (spec §4.5, item 9): the path before
       it may not hold an index — `x[i][a..]` — for the read would repeat it */
    for (size_t o = close + 1; range == SIZE_MAX && punct(a, o, "["); ) {
        size_t next = close_bracket(a, o, a->tokens.len);
        if (next == SIZE_MAX) break;
        for (size_t j = o + 1, d = 0; j < next; j++) {
            if (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{")) d++;
            else if (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}")) d--;
            else if (d == 0 && punct(a, j, "..") && j > o + 1 && j + 1 == next)
                diag3(c, K_DIAG_OPEN_RANGE_INDEX_ON_COMPLEX_PATH, name,
                      (keel_slice_char){ (size_t)(tok(a, next).ptr - tok(a, o).ptr) + 1, tok(a, o).ptr }, none, none);
        }
        o = next + 1;
    }

    Text tx = { .ok = true };
    if (k->kind == KT_ARRAY) {
        if (range != SIZE_MAX) {
            /* `x[a..b]` over an `array` is the `as_slice` of `keel.array`: the element
               gives the instance, and dimension 0 comes from the table (spec §4.5) */
            check_inverted(c, name, open, range, close);
            const char *elem = kt_symbol(c, k->elem);
            if (commas || !*elem) return close + 1;
            keel_slice_char group = { (size_t)(tok(a, close).ptr - tok(a, open).ptr) + 1, tok(a, open).ptr };
            if (k->rank > 1) {
                diag3(c, K_DIAG_FLAT_VIEW_OF_N_DIM_ARRAY, name, k_diag_text("as_slice"), group, k->dims);
                return close + 1;
            }
            const KModule *am = module_by_name(a->symbols, "keel.array");
            Sig sig = am && ast_of(c, am) ? find_verb(ast_of(c, am), k_diag_text("as_slice"), 3) : (Sig){0};
            if (!sig.found) {
                diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, name, path, k_diag_text("Sliceable"), k_diag_text("length, as_slice"));
                return close + 1;
            }
            bool high = range + 1 < close;
            put(&tx, path.ptr, path.len);
            put_str(&tx, " ");
            put_source(&tx, tok(a, open).ptr + 1, tok(a, close).ptr);
            put_str(&tx, " \xe2\x86\x92 ");
            put_module_prefix(&tx, ast_of(c, am));
            put_str(&tx, "_");
            put_str(&tx, elem);
            put_str(&tx, sig.overloaded ? "_as_slice2" : "_as_slice");
            if (!high) put_str(&tx, " core");         /* the limit is keel.length(x) */
            put_str(&tx, " dim:1");
            *any = emit(c, K_ISLAND_RANGE_INDEX, i, &tx) || *any;
            return close + 1;
        }
        Range idx[8];
        size_t indices = 0, last = close;
        for (size_t o = open; punct(a, o, "[") && indices < (size_t)k->rank; ) {
            size_t cl = close_bracket(a, o, a->tokens.len);
            if (cl == SIZE_MAX) break;
            size_t st = o + 1, d = 0;
            for (size_t m = o + 1; m <= cl; m++) {
                if (punct(a, m, "(") || punct(a, m, "[") || punct(a, m, "{")) d++;
                else if (m < cl && (punct(a, m, ")") || punct(a, m, "]") || punct(a, m, "}"))) d--;
                else if (m == cl || (d == 0 && punct(a, m, ","))) {
                    if (indices < 8) idx[indices] = (Range){ st, m };
                    indices++;
                    st = m + 1;
                }
            }
            last = cl;
            o = cl + 1;
        }
        put(&tx, path.ptr, path.len);
        put_str(&tx, " rank ");
        put_uint(&tx, indices);
        if (indices < (size_t)k->rank) diag3(c, K_DIAG_PARTIAL_ARRAY_INDEX, name, k->dims, none, none);
        long dims[8];
        size_t known = dims_of(c, k->dims, dims);
        for (size_t n = 0; n < indices && n < known && n < 8; n++) {
            long v;
            if (dims[n] >= 0 && decimal_of(c, idx[n], &v) && v >= dims[n])
                diag3(c, K_DIAG_ARRAY_INDEX_ABOVE_DIMENSION, tok(a, idx[n].first), range_text(a, idx[n]), k->dims, none);
        }
        *any = emit(c, K_ISLAND_ARRAY_INDEX, i, &tx) || *any;
        return last + 1;
    }

    if (k->kind != KT_MODIFIER || !k->module) return close + 1;
    const KAst *home = ast_of(c, k->module);
    if (!home || close == open + 1) return close + 1;
    if (module_node(home)->dim_first != module_node(home)->dim_end) return close + 1;   /* full-rank accessor: §5.3.1 */
    keel_slice_char text = { (size_t)(tok(a, close).ptr - tok(a, open).ptr) + 1, tok(a, open).ptr };
    bool adapt = false;
    put(&tx, path.ptr, path.len);
    if (range == SIZE_MAX) {
        /* x[i] is *ptr(x, i); x[i, j] is *ptr(x, idx) with a vector of indices, and a
           module with no `dim` declares none (spec §4.5, item 1) */
        put_str(&tx, " \xe2\x86\x92 ");
        if (commas || !container_verb(k, home, k_diag_text("ptr"), 2, &tx, &adapt)) {
            diag3(c, K_DIAG_NO_PTR_FOR_ARITY, name, path, text, none);
            return close + 1;
        }
    } else {
        /* x[a..b] is as_slice(x, a, b) of the module of the container's type, and the
           open ends read `length` (spec §4.5, items 5 and 6) */
        if (commas) return close + 1;
        check_inverted(c, name, open, range, close);
        bool high = range + 1 < close;
        put_str(&tx, " ");
        put_source(&tx, tok(a, open).ptr + 1, tok(a, close).ptr);
        put_str(&tx, " \xe2\x86\x92 ");
        bool ignored = false;
        if (!container_verb(k, home, k_diag_text("as_slice"), 3, &tx, &adapt) ||
            (!high && (put_str(&tx, " "), !container_verb(k, home, k_diag_text("length"), 1, &tx, &ignored)))) {
            diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, name, path, k_diag_text("Sliceable"), k_diag_text("length, as_slice"));
            return close + 1;
        }
    }
    if (adapt) put_str(&tx, " &1");
    *any = emit(c, range == SIZE_MAX ? K_ISLAND_INDEX : K_ISLAND_RANGE_INDEX, i, &tx) || *any;
    return close + 1;
}


/* `x[`, `x.f[`, `x->f[i][j]`: every index or range-index over a container this
   pass can type is an island, even when the emission is the written text (spec
   §2.3). True when one was printed. */
bool path_forms(Ctx *c, size_t i) {
    KAst *a = c->ast;
    const Local *head = find_local(c, tok(a, i));
    if (!head || head->decl_at == i) return false;      /* not ours, or the declarator's own brackets */
    bool any = false;
    size_t j = i + 1;
    for (;;) {
        if ((punct(a, j, ".") || punct(a, j, "->")) && ident(a, j + 1)) { j += 2; continue; }
        if (!punct(a, j, "[")) break;
        resolution_begin(c);
        size_t past = column_access(c, i, j, &any);
        j = past ? past : index_group(c, i, j, &any);
        resolution_end(c);
    }
    return any;
}

bool signature_of(const KAstNode *n, size_t i) {
    return n->body_first != SIZE_MAX ? i < n->body_first : n->kind == K_AST_FUNCTION;
}

/* `constexpr T NAME = value;` in a block: the name is the identifier just before
   the `=` (spec §4.2), and the value is what keel reads if it needs the number */
void block_constexpr(Ctx *c, size_t i) {
    const KAst *a = c->ast;
    for (size_t j = i + 1; j < a->tokens.len && !punct(a, j, ";"); j++) {
        if (!punct(a, j, "=") || !ident(a, j - 1)) continue;
        size_t end = j + 1;
        while (end < a->tokens.len && !punct(a, end, ";")) end++;
        add_local(c, (Local){ .name = tok(a, j - 1), .decl_at = j - 1, .is_constexpr = true,
                              .value = range_text(a, (Range){ j + 1, end }) });
        return;
    }
}

bool assignment_op(const KAst *a, size_t i) {
    static const char *const ops[] = { "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", "++", "--" };
    for (size_t k = 0; k < sizeof ops / sizeof *ops; k++)
        if (punct(a, i, ops[k])) return true;
    return false;
}

/* an operand ends before token `i`: a name, a number, or a closing bracket */
bool after_operand(const KAst *a, size_t i) {
    return i > 0 && (ident(a, i - 1) || k_token_is_number(tok(a, i - 1)) || punct(a, i - 1, ")") || punct(a, i - 1, "]"));
}

/* the uses a constant and a `ref` do not take (spec §4.2, items 13 and 15) */
void check_uses(Ctx *c, size_t i) {
    const KAst *a = c->ast;
    KToken name = tok(a, i);
    const Local *l = find_local(c, name);
    if (l && l->decl_at == i) return;
    bool constant = l ? l->is_constexpr : false;
    if (!l) {
        const KSymbol *sym = k_symbol_resolve(a->symbols, name);
        constant = sym && sym->kind == K_SYM_CONSTANT && sym->origin;
    }
    bool address = i > 0 && punct(a, i - 1, "&") && !after_operand(a, i - 1);
    if (constant && (address || assignment_op(a, i + 1) || (i > 0 && (punct(a, i - 1, "++") || punct(a, i - 1, "--")))))
        diag3(c, K_DIAG_CONSTEXPR_AS_LVALUE, name, name, (keel_slice_char){0}, (keel_slice_char){0});
    if (l && l->is_ref) {
        bool before = i > 0 && (punct(a, i - 1, "++") || punct(a, i - 1, "--") ||
                                ((punct(a, i - 1, "+") || punct(a, i - 1, "-")) && after_operand(a, i - 1)));
        bool after = punct(a, i + 1, "+") || punct(a, i + 1, "-") || punct(a, i + 1, "+=") || punct(a, i + 1, "-=") ||
                     punct(a, i + 1, "++") || punct(a, i + 1, "--") || punct(a, i + 1, "[");
        if (before || after) diag3(c, K_DIAG_REF_ARITHMETIC, name, name, (keel_slice_char){0}, (keel_slice_char){0});
    }
}
