/* engine/diag.h — the diagnostics sink (diag-design §2, §3). The engine
 * accumulates; tool/report.c prints. engine/: no I/O, no allocation — the
 * caller gives the storage. */
#ifndef CGEN_ENGINE_DIAG_H
#define CGEN_ENGINE_DIAG_H

#include <stdbool.h>
#include <stddef.h>
#include "keel/keel_slice_char.type.h"

typedef enum { K_INFO, K_WARNING, K_ERROR, K_SEVERITY_COUNT } KSeverity;

/* Index into k_diags, sorted by name [G1]. The table grows with each phase;
   its generator from keel-spec §6.2 comes with the parser (M2). */
typedef enum {
    K_DIAG_DEFINE_OVER_KEEL_NAME,
    K_DIAG_LITERAL_WITH_NEWLINE,
    K_DIAG_COUNT
} KDiagId;

typedef struct {
    const char *name;       /* kebab-case, the catalog's */
    KSeverity   severity;
    const char *fmt;        /* the message; each %s takes the next argument */
} KDiagEntry;

extern const KDiagEntry k_diags[K_DIAG_COUNT];

#define K_DIAG_MAX_ARGS 3

/* the message's holes, as views: of the source, or of a static string */
typedef struct {
    keel_slice_char s[K_DIAG_MAX_ARGS];
} KDiagArgs;

typedef struct {
    KDiagId         id;
    KSeverity       severity;   /* the table's; -Werror is the tool's [G4] */
    keel_slice_char at;         /* a view of the source: position and extent */
    KDiagArgs       args;
} KDiagnostic;

typedef struct {
    KDiagnostic *items;         /* the caller's storage */
    size_t       cap;
    size_t       len;           /* kept, at most cap */
    size_t       count[K_SEVERITY_COUNT];   /* emitted, kept or not */
} KDiagnosticSink;

void   k_diag_init(KDiagnosticSink *s, KDiagnostic *storage, size_t cap);
void   k_diag_emit(KDiagnosticSink *s, KDiagId id, keel_slice_char at, KDiagArgs args);
size_t k_diag_count(const KDiagnosticSink *s, KSeverity sev);

/* a view of a NUL-terminated static string, for a message argument */
keel_slice_char k_diag_text(const char *s);

#endif /* CGEN_ENGINE_DIAG_H */
