/* engine/diag.c — the diagnostics sink and its table (diag-design §2-§4). */
#include <string.h>
#include "engine/diag.h"

/* The messages say what was found and, when there is one, the correct form
   [G2]. Sorted by name. */
const KDiagEntry k_diags[K_DIAG_COUNT] = {
    [K_DIAG_MODULE_NOT_FOUND] = { "module-not-found", K_ERROR, "module `%s` was not found in the source roots" },
    [K_DIAG_MODULE_PATH_MISMATCH] = { "module-path-mismatch", K_ERROR, "expected module `%s` for this path, found `%s`" },
    [K_DIAG_PARSE_FAILED] = { "unexpected-token", K_ERROR, "malformed or truncated declaration" },
    [K_DIAG_LOAD_FAILED] = { "module-load-failed", K_ERROR, "cannot load module `%s`" },
    [K_DIAG_CAPACITY] = { "implementation-limit", K_ERROR, "insufficient storage while collecting module `%s`" },
    [K_DIAG_CIRCULAR_IMPORT] = { "circular-import", K_ERROR,
        "module `%s` is already being loaded: importing it here closes a cycle" },
    [K_DIAG_DEFINE_OVER_KEEL_NAME] = { "define-over-keel-name", K_ERROR,
        "cannot `#%s` `%s`: %s" },
    [K_DIAG_LITERAL_WITH_NEWLINE] = { "literal-with-newline", K_ERROR,
        "%s literal has no closing `%s` before the end of the line; write `\\n` for a newline inside it" },
};

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
