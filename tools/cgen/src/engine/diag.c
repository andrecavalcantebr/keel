/* engine/diag.c — the diagnostics sink and its table (diag-design §2-§4). */
#include <string.h>
#include "engine/diag.h"

/* The messages say what was found and, when there is one, the correct form
   [G2]. Sorted by name. */
const KDiagEntry k_diags[K_DIAG_COUNT] = {
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
