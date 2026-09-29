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
    [K_DIAG_NO_RANGE_INDEX_VERB] =
        "`%s` declares no range-index verb for `%s`: the memory side of a memory/view pair (`buffer`, `slice`) does",
    [K_DIAG_DEFINE_OVER_KEEL_NAME] = "cannot `#%s` `%s`: %s",
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
