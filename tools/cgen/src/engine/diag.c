/* engine/diag.c — the diagnostics sink and its table (diag-design §2-§4). */
#include <string.h>
#include "engine/diag.h"

/* Name and severity come from the catalog; nothing here can disagree with it. */
const KDiagEntry k_diags[K_DIAG_COUNT] = {
#define K_DIAG_ROW(NAME, ID, SEV) [K_DIAG_##NAME] = { ID, SEV },
    K_DIAG_TOOL(K_DIAG_ROW)
    K_DIAG_CATALOG(K_DIAG_ROW)
#undef K_DIAG_ROW
};

/* The messages say what was found and, when there is one, the correct form
   [G2]. Only these are written by hand; a diagnostic with no entry here is
   one the implementation does not emit yet. */
static const char *const messages[K_DIAG_COUNT] = {
    [K_DIAG_NAME_TOO_LONG] = "generated name exceeds the backend limit of 255 characters",
    [K_DIAG_MODULE_NOT_FOUND] = "module `%s` was not found in the source roots",
    [K_DIAG_MODULE_PATH_MISMATCH] = "expected module `%s` for this path, found `%s`",
    [K_DIAG_PARSE_FAILED] = "malformed or truncated declaration",
    [K_DIAG_LOAD_FAILED] = "cannot load module `%s`",
    [K_DIAG_CAPACITY] = "insufficient storage while collecting module `%s`",
    [K_DIAG_CIRCULAR_IMPORT] =
        "module `%s` is already being loaded: importing it here closes a cycle",
    [K_DIAG_NOT_A_CONTAINER_EXPRESSION] =
        "`%s` takes a container as its object, and `%s` is not one: name a symbol declared with a keel type (or an index, field or verb over one)",
    [K_DIAG_WRONG_QUALIFIER] =
        "the object of `%s` is a type of module `%s`, not of the module the qualifier names: write `%s.<verb>` with the module of the object",
    [K_DIAG_ADDRESS_IN_OBJECT_POSITION] =
        "`%s` takes its object without `&`: the object is written as itself and the address is inserted from the parameter (`%s`)",
    [K_DIAG_FLAT_VIEW_OF_N_DIM_ARRAY] =
        "`%s` needs a one-dimensional `array`, and `%s` has dimensions `%s`: index it down to one dimension first",
    [K_DIAG_NO_PTR_FOR_ARITY] =
        "`%s` has no `ptr` for the indices `%s`: a container is indexed with the arities its module declares `ptr` for",
    [K_DIAG_NOT_IN_V0] =
        "`%s` is a v1 construction: this implementation recognizes it and does not translate it",
    [K_DIAG_ROLE_POSITION] =
        "`%s` is a role: a role stands before the type of a parameter, and only `child` before the return type",
    [K_DIAG_PROTOCOL_ROLE_MISMATCH] =
        "`%s` declares roles that differ from the prototype in `%s`: write the same roles in the same positions, or none, and the prototype's apply",
    [K_DIAG_DEFER_WITHOUT_BRACES] =
        "`defer` is the whole body of `%s`: put it in braces, so the scope it cleans up is the block's",
    [K_DIAG_DEFER_IN_CONTROL_BLOCK] =
        "`defer` in the body of `%s` runs when that block ends, not when the function does",
    [K_DIAG_LATER_WITH_CAPTURE] =
        "`later` reads the values at the exit and takes no list (`%s`): write `[now ...]` to copy them",
    [K_DIAG_WALK_WITHOUT_CURSOR] =
        "`walk` needs two binders, the element and the cursor, and `%s` is one: write `walk (T *e, M.cursor c : x)`",
    [K_DIAG_FOREACH_TWO_BINDERS_ON_LITERAL] =
        "`foreach` over the literal `%s` takes one binder: name the range to have the position",
    [K_DIAG_POINTER_BINDER_ON_RANGE] =
        "`%s` is a pointer, and a range binder is the counter itself: write it by value",
    [K_DIAG_INDEX_NOT_SIZE_T] =
        "the index binder `%s` must be `size_t`",
    [K_DIAG_TAG_NOT_IN_SET] =
        "`%s` is not a tag of `%s`",
    [K_DIAG_DUPLICATE_TAG] =
        "`%s` labels a second arm of the same `match` over `%s`",
    [K_DIAG_TAG_WITHOUT_LABEL] =
        "`%s` of `%s` has no arm in this `match`: every tag of the set has a label",
    [K_DIAG_MATCH_WITHOUT_TAGS] =
        "the `tag` of `%s` returns no set of tags, so `match` has no labels to check",
    [K_DIAG_UNNAMED_PARALLEL] =
        "`parallel` has no name: write `parallel NAME POLICY (...)`; the name is the control symbol the queries read",
    [K_DIAG_FLOW_VERB_OUTSIDE_PARALLEL] =
        "`%s;` is a worker exit, and it stands outside the body of a `parallel`",
    [K_DIAG_NESTED_PARALLEL] =
        "`parallel %s` stands in the body of `parallel %s`: a worker does not start workers",
    [K_DIAG_DUPLICATE_PARALLEL_NAME] =
        "`%s` names a second `parallel` in the same function: each one needs its own name",
    [K_DIAG_NONCONSTANT_PARALLEL] =
        "`%s` is not known at translation time: the worker count and the policy of `parallel` are a decimal literal or a `constexpr`",
    [K_DIAG_CAPTURED_WRITE] =
        "`%s` is captured by `parallel %s`, and each worker writes its own copy: pass a pointer to give a result back",
    [K_DIAG_RETURN_IN_PARALLEL] =
        "`return` in the body of `parallel %s`: the worker does not leave the function; end it with `win;` or `fail;`",
    [K_DIAG_PARTITION_TYPE_MISMATCH] =
        "the partition binder is `%s`, and the `partition` of `%s` produces `%s`",
    [K_DIAG_MUTATION_DURING_TRAVERSAL] =
        "`%s` changes `%s` while the `%s` goes over it: change the container before or after",
    [K_DIAG_ELSE_WITHOUT_INITIALIZER] =
        "the `else` of a result tests the value just stored, and this declaration stores none: write `T x = e else ...`",
    [K_DIAG_ELSE_ON_COMPLEX_TARGET] =
        "the `else` of a result needs a name declared with a keel type as its target, and `%s` is not one: store the result in a local first",
    [K_DIAG_ELSE_MULTIPLE_DECLARATORS] =
        "the `else` of a result takes one declarator: declare the others in their own statement",
    [K_DIAG_EXTENT_COUNT_NOT_FIELD] =
        "the count `%s` of `extent struct %s` names no field of the struct",
    [K_DIAG_EXTENT_UNKNOWN_CAPACITY] =
        "the capacity `%s` of `extent struct %s` is not a field, a known `constexpr` or a decimal literal",
    [K_DIAG_EXTENT_WITHOUT_COLUMN] =
        "`extent struct %s` has no column: mark the columns with `array`",
    [K_DIAG_EXTENT_MIXED_COLUMNS] =
        "`extent struct %s` mixes embedded columns and columns by pointer: use one kind",
    [K_DIAG_EXTENT_EMBEDDED_FIELD_CAPACITY] =
        "the embedded column `%s` needs constant capacities, and `%s` is a field: use a `constexpr`, or a column by pointer",
    [K_DIAG_EXTENT_DIMENSION_MISMATCH] =
        "the embedded column `%s` declares `%s`, and the capacities of the groups are `%s`: one dimension per group, named as its capacity",
    [K_DIAG_EXTENT_INDEX_ARITY] =
        "`%s` is a column of rank %s, and the access writes %s indices",
    [K_DIAG_EXTENT_INDEX_ABOVE_CAPACITY] =
        "index `%s` is at or above the capacity `%s` of its group",
    [K_DIAG_EXTENT_PATH_WITH_CALL] =
        "the column access `%s` goes through a call: store the struct, or a pointer to it, in a local first",
    [K_DIAG_PROTOCOL_NOT_SATISFIED] =
        "`%s` does not meet `%s`: its module does not declare %s (an `array` takes the verbs from `keel.array`, which must be imported)",
    [K_DIAG_NONCONSTANT_DIM_INDEX] =
        "`keel.dim` needs a dimension index known at translation time, and `%s` is not one: write a decimal literal or a `constexpr`",
    [K_DIAG_PARTIAL_ARRAY_INDEX] =
        "the array has dimensions %s: index every one of them (`v[i,j]` or `v[i][j]`); a partial index has no meaning",
    [K_DIAG_ARRAY_INDEX_ABOVE_DIMENSION] =
        "index `%s` is at or above its dimension in %s",
    [K_DIAG_INVERTED_RANGE_INDEX] =
        "the range `[%s]` starts above where it ends; the empty range is `[a..a]`",
    [K_DIAG_OPEN_RANGE_INDEX_ON_COMPLEX_PATH] =
        "`%s` leaves the end open on a path that holds an index or a verb, and the path would be evaluated twice: name the length",
    [K_DIAG_ARRAY_PARAMETER_WITHOUT_DIMENSION] =
        "the parameter `%s` is an `array` without a dimension: write `array T name[size_t n]` to bind its length",
    [K_DIAG_ARRAY_1D_AS_PARAMETER] =
        "the one-dimensional `array` parameter `%s` has no binder: write `array T name[size_t n]` so the function receives its length",
    [K_DIAG_REF_WITHOUT_INITIALIZER] =
        "`ref` symbol `%s` has no initializer: a `ref` is initialized where it is declared",
    [K_DIAG_REF_ARITHMETIC] =
        "`%s` is a `ref`: it names one element and takes no arithmetic or index; use a plain pointer to walk",
    [K_DIAG_CONSTEXPR_AS_LVALUE] =
        "`%s` is a `constexpr`: it has no address and cannot be assigned; use `static const` for an object",
    [K_DIAG_BUFFER_OF_UNKNOWN_SIZE] =
        "`%s` needs a one-dimensional `array` to read the size from, and `%s` is not one: use `buffer.of(p, n)`",
    [K_DIAG_BUFFER_OVER_CONST] =
        "`%s` cannot make a `buffer` over the const elements `%s`: use `slice.of`, which gives a `slice const T`",
    [K_DIAG_C_TYPE_AS_ARGUMENT] =
        "`%s` takes keel types as arguments, and `%s` is a C keyword: write the keel spelling (`i32`, `u8`, `f64`, ...) or a named type",
    [K_DIAG_RESTRICT_ON_CONTAINER] =
        "`restrict` does not apply to the type `%s`: qualify a pointer to it (`T *restrict p`)",
    [K_DIAG_ELEMENT_COPY_IN_GET_SET] =
        "`%s` would copy an element that is itself `%s`; take its address with `ptr` instead",
    [K_DIAG_SLICE_FROM_REF] =
        "`slice.from` over `%s`, a `ref`: keel does not vouch for the extent behind a `ref`; pass a pointer",
    [K_DIAG_DEFINE_OVER_KEEL_NAME] = "cannot `#%s` `%s`: %s",
    [K_DIAG_DEFINE_OVER_KEEL_WORD] =
        "`#%s` of `%s`, a keel word: where it stands as a construction, keel reads the construction, not the macro",
    [K_DIAG_LITERAL_WITH_NEWLINE] =
        "%s literal has no closing `%s` before the end of the line; write `\\n` for a newline inside it",
};

const char *k_diag_message(KDiagId id) {
    return messages[id] != NULL ? messages[id] : "the tool reports this condition; its message is not written yet";
}

void k_diag_init(KDiagnosticSink *s, KDiagnostic *storage, size_t cap) {
    s->items = storage;
    s->cap = storage != NULL ? cap : 0;
    s->len = 0;
    for (int k = 0; k < K_SEVERITY_COUNT; ++k) s->count[k] = 0;
}

void k_diag_emit(KDiagnosticSink *s, KDiagId id, keel_slice_char at, KDiagArgs args) {
    if (s == NULL) return;
    KSeverity sev = k_diags[id].severity;
    s->count[sev]++;
    if (s->len < s->cap) {
        s->items[s->len++] = (KDiagnostic){ id, sev, at, args };
    }
}

size_t k_diag_count(const KDiagnosticSink *s, KSeverity sev) {
    return s != NULL ? s->count[sev] : 0;
}

keel_slice_char k_diag_text(const char *s) {
    return (keel_slice_char){ strlen(s), (char *)s };
}
