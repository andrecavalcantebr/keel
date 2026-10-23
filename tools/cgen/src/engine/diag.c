/* engine/diag.c — the diagnostics sink and its table (diag-design §2-§4). */
#include <string.h>
#include "engine/diag.h"
#include "engine/diag_messages.def"

/* Name and severity come from the catalog; nothing here can disagree with it. */
const KDiagEntry k_diags[K_DIAG_COUNT] = {
#define K_DIAG_ROW(NAME, ID, SEV) [K_DIAG_##NAME] = { ID, SEV },
    K_DIAG_TOOL(K_DIAG_ROW)
    K_DIAG_CATALOG(K_DIAG_ROW)
#undef K_DIAG_ROW
};

/* The messages say what was found and, when there is one, the correct form
   [G2]. They are written in tools/cgen/diag-messages.md and generated into
   diag_messages.def; a diagnostic with no message there is one the
   implementation does not emit yet. */
static const char *const messages[K_DIAG_COUNT] = {
#define K_DIAG_MESSAGE_ROW(NAME, TEXT) [K_DIAG_##NAME] = TEXT,
    K_DIAG_MESSAGES(K_DIAG_MESSAGE_ROW)
#undef K_DIAG_MESSAGE_ROW
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
