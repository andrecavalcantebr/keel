/* Pass 3: the `array`, `array-index`, `index` and `range-index` islands. Split from parser_islands.c by T3; the code is unchanged. */

#include "engine/parser_islands_internal.h"

/* ---- stage 4b: array, array-index, index, range-index ------------------ */

/* decl-array (spec §2.2, §4.2) at the `array` word `i`: one island per declared
   name, with the dimensions as written, and the name known as an array from
   there on. Returns the last token of the element type, which is not a
   declaration of its own: the caller skips to it. */
size_t isl_array_decl(Ctx *c, size_t i, bool signature) {
    KAst *a = c->ast;
    if (i + 1 >= a->tokens.len) return i;
    KLexer lexer;
    TKPpKind pp;
    KToken next = {0}, first = isl_enter(a, i + 1, &lexer, &pp);
    keel_slice_char element;
    if (!k_scan_argument(&lexer, first, a->symbols, &element, &next, &pp)) return i;
    size_t last = isl_index_at(a, i + 1, next.len ? next.ptr : a->source.ptr + a->source.len) - 1;

    /* a keel element type is a use like any other; the declarators follow it */
    KLexer look;
    KSpecifier spec;
    KToken after_type = {0}, head = isl_enter(a, i + 1, &look, &pp);
    if (k_token_is_ident(head) && k_scan_known_type(&look, head, a->symbols, &spec, &after_type, &pp) &&
        spec.kind != K_SPEC_NONE)
        isl_declaration(c, i + 1, &spec, &look, after_type, signature, false);

    const char *source_end = a->source.ptr + a->source.len;
    for (KToken d = next; d.len; ) {
        KDeclarator decl;
        KToken after;
        if (!(k_token_is_punct(d, "*") || k_token_is_ident(d))) break;
        if (!k_scan_declarator(&lexer, d, &decl, &after, &pp) || !decl.name.len ||
            !decl.has_array_suffix || decl.parenthesized) break;
        const char *dims = decl.name.ptr + decl.name.len, *end = after.len ? after.ptr : source_end;
        Text t = { .ok = true };
        isl_put(&t, decl.name.ptr, decl.name.len);
        isl_put_str(&t, " ");
        isl_put_source(&t, dims, end);
        isl_emit(c, K_ISLAND_ARRAY, i, &t);
        if (signature) {
            /* a parameter names its dimension 0 with a binder, or is not one-dimensional (spec §4.2, item 19) */
            const char *p = dims, *q = end;
            while (p < q && (*p == ' ' || *p == '[')) p++;
            bool empty = p < q && *p == ']';
            bool binder_written = q - p >= 6 && !memcmp(p, "size_t", 6);
            if (empty)
                isl_diag3(c, K_DIAG_ARRAY_PARAMETER_WITHOUT_DIMENSION, decl.name, decl.name, (keel_slice_char){0},
                      (keel_slice_char){0});
            else if (isl_rank_of(dims, end) == 1 && !binder_written)
                isl_diag3(c, K_DIAG_ARRAY_1D_AS_PARAMETER, decl.name, decl.name, (keel_slice_char){ (size_t)(end - dims), (char *)dims },
                      (keel_slice_char){0});
        }
        isl_add_local(c, (Local){ .name = decl.name, .pointers = decl.pointer_depth + 1,
                              .decl_at = isl_index_at(a, i, decl.name.ptr), .rank = isl_rank_of(dims, end),
                              .element = element, .dims = { (size_t)(end - dims), (char *)dims },
                              /* an automatic `array` is a local origin (spec §4.12, rule 3) */
                              .local_origin = !signature && c->depth > 0 && !(i > 0 && isl_word(a, i - 1, "static")) });
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
        isl_decimal_of(c, (Range){ open + 1, range }, &low) && isl_decimal_of(c, (Range){ range + 1, close }, &high) &&
        low > high)
        isl_diag3(c, K_DIAG_INVERTED_RANGE_INDEX, name,
              (keel_slice_char){ (size_t)(isl_tok(a, close).ptr - isl_tok(a, open).ptr) - 1, isl_tok(a, open).ptr + 1 },
              (keel_slice_char){0}, (keel_slice_char){0});
}

/* `<type symbol>_<verb><suffix>` for a container and a call of `arity` arguments,
   object included; `adapt` says the object takes an `&`. */
bool isl_container_verb(const KType *k, const KAst *home, keel_slice_char verb, size_t arity, Text *tx,
                           bool *adapt) {
    Sig sig = isl_find_verb(home, verb, arity);
    if (!sig.found || !*k->symbol) return false;
    isl_put_str(tx, k->symbol);
    isl_put_str(tx, "_");
    isl_put(tx, verb.ptr, verb.len);
    if (sig.overloaded && arity > 1) isl_put_uint(tx, arity - 1);
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
    size_t close = isl_close_bracket(a, open, a->tokens.len);
    if (close == SIZE_MAX) return a->tokens.len;
    KToken name = isl_tok(a, i);
    int t = isl_type_of(c, (Range){ i, open });
    if (t < 0) return close + 1;
    const KType *k = isl_kt(c, t);
    keel_slice_char path = { (size_t)(isl_tok(a, open - 1).ptr + isl_tok(a, open - 1).len - name.ptr), name.ptr };
    keel_slice_char none = {0};

    size_t commas = 0, range = SIZE_MAX, depth = 0;
    for (size_t j = open + 1; j < close; j++) {
        if (isl_punct(a, j, "(") || isl_punct(a, j, "[") || isl_punct(a, j, "{")) depth++;
        else if (isl_punct(a, j, ")") || isl_punct(a, j, "]") || isl_punct(a, j, "}")) depth--;
        else if (depth == 0 && isl_punct(a, j, ",")) commas++;
        else if (depth == 0 && isl_punct(a, j, "..") && range == SIZE_MAX) range = j;
    }

    /* an open end reads the container twice (spec §4.5, item 9): the path before
       it may not hold an index — `x[i][a..]` — for the read would repeat it */
    for (size_t o = close + 1; range == SIZE_MAX && isl_punct(a, o, "["); ) {
        size_t next = isl_close_bracket(a, o, a->tokens.len);
        if (next == SIZE_MAX) break;
        for (size_t j = o + 1, d = 0; j < next; j++) {
            if (isl_punct(a, j, "(") || isl_punct(a, j, "[") || isl_punct(a, j, "{")) d++;
            else if (isl_punct(a, j, ")") || isl_punct(a, j, "]") || isl_punct(a, j, "}")) d--;
            else if (d == 0 && isl_punct(a, j, "..") && j > o + 1 && j + 1 == next)
                isl_diag3(c, K_DIAG_OPEN_RANGE_INDEX_ON_COMPLEX_PATH, name,
                      (keel_slice_char){ (size_t)(isl_tok(a, next).ptr - isl_tok(a, o).ptr) + 1, isl_tok(a, o).ptr }, none, none);
        }
        o = next + 1;
    }

    Text tx = { .ok = true };
    if (k->kind == KT_ARRAY) {
        if (range != SIZE_MAX) {
            /* `x[a..b]` over an `array` is the `as_slice` of `keel.array`: the element
               gives the instance, and dimension 0 comes from the table (spec §4.5) */
            check_inverted(c, name, open, range, close);
            const char *elem = isl_kt_symbol(c, k->elem);
            if (commas || !*elem) return close + 1;
            keel_slice_char group = { (size_t)(isl_tok(a, close).ptr - isl_tok(a, open).ptr) + 1, isl_tok(a, open).ptr };
            if (k->rank > 1) {
                isl_diag3(c, K_DIAG_FLAT_VIEW_OF_N_DIM_ARRAY, name, k_diag_text("as_slice"), group, k->dims);
                return close + 1;
            }
            const KModule *am = isl_module_by_name(a->symbols, "keel.array");
            Sig sig = am && isl_ast_of(c, am) ? isl_find_verb(isl_ast_of(c, am), k_diag_text("as_slice"), 3) : (Sig){0};
            if (!sig.found) {
                isl_diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, name, path, k_diag_text("Sliceable"), k_diag_text("length, as_slice"));
                return close + 1;
            }
            bool high = range + 1 < close;
            isl_put(&tx, path.ptr, path.len);
            isl_put_str(&tx, " ");
            isl_put_source(&tx, isl_tok(a, open).ptr + 1, isl_tok(a, close).ptr);
            isl_put_str(&tx, " \xe2\x86\x92 ");
            isl_put_module_prefix(&tx, isl_ast_of(c, am));
            isl_put_str(&tx, "_");
            isl_put_str(&tx, elem);
            isl_put_str(&tx, sig.overloaded ? "_as_slice2" : "_as_slice");
            if (!high) isl_put_str(&tx, " core");         /* the limit is keel.length(x) */
            isl_put_str(&tx, " dim:1");
            *any = isl_emit(c, K_ISLAND_RANGE_INDEX, i, &tx) || *any;
            return close + 1;
        }
        Range idx[8];
        size_t indices = 0, last = close;
        for (size_t o = open; isl_punct(a, o, "[") && indices < (size_t)k->rank; ) {
            size_t cl = isl_close_bracket(a, o, a->tokens.len);
            if (cl == SIZE_MAX) break;
            size_t st = o + 1, d = 0;
            for (size_t m = o + 1; m <= cl; m++) {
                if (isl_punct(a, m, "(") || isl_punct(a, m, "[") || isl_punct(a, m, "{")) d++;
                else if (m < cl && (isl_punct(a, m, ")") || isl_punct(a, m, "]") || isl_punct(a, m, "}"))) d--;
                else if (m == cl || (d == 0 && isl_punct(a, m, ","))) {
                    if (indices < 8) idx[indices] = (Range){ st, m };
                    indices++;
                    st = m + 1;
                }
            }
            last = cl;
            o = cl + 1;
        }
        isl_put(&tx, path.ptr, path.len);
        isl_put_str(&tx, " rank ");
        isl_put_uint(&tx, indices);
        if (indices < (size_t)k->rank) isl_diag3(c, K_DIAG_PARTIAL_ARRAY_INDEX, name, k->dims, none, none);
        long dims[8];
        size_t known = isl_dims_of(c, k->dims, dims);
        for (size_t n = 0; n < indices && n < known && n < 8; n++) {
            long v;
            if (dims[n] >= 0 && isl_decimal_of(c, idx[n], &v) && v >= dims[n])
                isl_diag3(c, K_DIAG_ARRAY_INDEX_ABOVE_DIMENSION, isl_tok(a, idx[n].first), isl_range_text(a, idx[n]), k->dims, none);
        }
        *any = isl_emit(c, K_ISLAND_ARRAY_INDEX, i, &tx) || *any;
        return last + 1;
    }

    if (k->kind != KT_MODIFIER || !k->module) return close + 1;
    const KAst *home = isl_ast_of(c, k->module);
    if (!home || close == open + 1) return close + 1;
    if (isl_module_node(home)->dim_first != isl_module_node(home)->dim_end) return close + 1;   /* full-rank accessor: §5.3.1 */
    keel_slice_char text = { (size_t)(isl_tok(a, close).ptr - isl_tok(a, open).ptr) + 1, isl_tok(a, open).ptr };
    bool adapt = false;
    isl_put(&tx, path.ptr, path.len);
    if (range == SIZE_MAX) {
        /* x[i] is *ptr(x, i); x[i, j] is *ptr(x, idx) with a vector of indices, and a
           module with no `dim` declares none (spec §4.5, item 1) */
        isl_put_str(&tx, " \xe2\x86\x92 ");
        if (commas || !isl_container_verb(k, home, k_diag_text("ptr"), 2, &tx, &adapt)) {
            isl_diag3(c, K_DIAG_NO_PTR_FOR_ARITY, name, path, text, none);
            return close + 1;
        }
    } else {
        /* x[a..b] is as_slice(x, a, b) of the module of the container's type, and the
           open ends read `length` (spec §4.5, items 5 and 6) */
        if (commas) return close + 1;
        check_inverted(c, name, open, range, close);
        bool high = range + 1 < close;
        isl_put_str(&tx, " ");
        isl_put_source(&tx, isl_tok(a, open).ptr + 1, isl_tok(a, close).ptr);
        isl_put_str(&tx, " \xe2\x86\x92 ");
        bool ignored = false;
        if (!isl_container_verb(k, home, k_diag_text("as_slice"), 3, &tx, &adapt) ||
            (!high && (isl_put_str(&tx, " "), !isl_container_verb(k, home, k_diag_text("length"), 1, &tx, &ignored)))) {
            isl_diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, name, path, k_diag_text("Sliceable"), k_diag_text("length, as_slice"));
            return close + 1;
        }
    }
    if (adapt) isl_put_str(&tx, " &1");
    *any = isl_emit(c, range == SIZE_MAX ? K_ISLAND_INDEX : K_ISLAND_RANGE_INDEX, i, &tx) || *any;
    return close + 1;
}


/* `x[`, `x.f[`, `x->f[i][j]`: every index or range-index over a container this
   pass can type is an island, even when the emission is the written text (spec
   §2.3). True when one was printed. */
bool isl_path_forms(Ctx *c, size_t i) {
    KAst *a = c->ast;
    const Local *head = isl_find_local(c, isl_tok(a, i));
    if (!head || head->decl_at == i) return false;      /* not ours, or the declarator's own brackets */
    bool any = false;
    size_t j = i + 1;
    for (;;) {
        if ((isl_punct(a, j, ".") || isl_punct(a, j, "->")) && isl_ident(a, j + 1)) { j += 2; continue; }
        if (!isl_punct(a, j, "[")) break;
        isl_resolution_begin(c);
        size_t past = isl_column_access(c, i, j, &any);
        j = past ? past : index_group(c, i, j, &any);
        isl_resolution_end(c);
    }
    return any;
}

bool isl_signature_of(const KAstNode *n, size_t i) {
    return n->body_first != SIZE_MAX ? i < n->body_first : n->kind == K_AST_FUNCTION;
}

/* `constexpr T NAME = value;` in a block: the name is the identifier just before
   the `=` (spec §4.2), and the value is what keel reads if it needs the number */
void isl_block_constexpr(Ctx *c, size_t i) {
    const KAst *a = c->ast;
    for (size_t j = i + 1; j < a->tokens.len && !isl_punct(a, j, ";"); j++) {
        if (!isl_punct(a, j, "=") || !isl_ident(a, j - 1)) continue;
        size_t end = j + 1;
        while (end < a->tokens.len && !isl_punct(a, end, ";")) end++;
        isl_add_local(c, (Local){ .name = isl_tok(a, j - 1), .decl_at = j - 1, .is_constexpr = true,
                              .value = isl_range_text(a, (Range){ j + 1, end }) });
        return;
    }
}

bool isl_assignment_op(const KAst *a, size_t i) {
    static const char *const ops[] = { "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", "++", "--" };
    for (size_t k = 0; k < sizeof ops / sizeof *ops; k++)
        if (isl_punct(a, i, ops[k])) return true;
    return false;
}

/* an operand ends before token `i`: a name, a number, or a closing bracket */
bool isl_after_operand(const KAst *a, size_t i) {
    return i > 0 && (isl_ident(a, i - 1) || k_token_is_number(isl_tok(a, i - 1)) || isl_punct(a, i - 1, ")") || isl_punct(a, i - 1, "]"));
}

/* the uses a constant and a `ref` do not take (spec §4.2, items 13 and 15) */
void isl_check_uses(Ctx *c, size_t i) {
    const KAst *a = c->ast;
    KToken name = isl_tok(a, i);
    const Local *l = isl_find_local(c, name);
    if (l && l->decl_at == i) return;
    bool constant = l ? l->is_constexpr : false;
    if (!l) {
        const KSymbol *sym = k_symbol_resolve(a->symbols, name);
        constant = sym && sym->kind == K_SYM_CONSTANT && sym->origin;
    }
    bool address = i > 0 && isl_punct(a, i - 1, "&") && !isl_after_operand(a, i - 1);
    if (constant && (address || isl_assignment_op(a, i + 1) || (i > 0 && (isl_punct(a, i - 1, "++") || isl_punct(a, i - 1, "--")))))
        isl_diag3(c, K_DIAG_CONSTEXPR_AS_LVALUE, name, name, (keel_slice_char){0}, (keel_slice_char){0});
    if (l && l->is_ref) {
        bool before = i > 0 && (isl_punct(a, i - 1, "++") || isl_punct(a, i - 1, "--") ||
                                ((isl_punct(a, i - 1, "+") || isl_punct(a, i - 1, "-")) && isl_after_operand(a, i - 1)));
        bool after = isl_punct(a, i + 1, "+") || isl_punct(a, i + 1, "-") || isl_punct(a, i + 1, "+=") || isl_punct(a, i + 1, "-=") ||
                     isl_punct(a, i + 1, "++") || isl_punct(a, i + 1, "--") || isl_punct(a, i + 1, "[");
        if (before || after) isl_diag3(c, K_DIAG_REF_ARITHMETIC, name, name, (keel_slice_char){0}, (keel_slice_char){0});
    }
}
