/* engine/emit/writer.h — KWriter, the one place that knows the line counters
 * (codegen-design §8, decision E4; backend §6).
 *
 * The writer keeps two counters: the physical output, and the line the C
 * compiler believes the next output line has in the .k (what the last `#line`
 * set, plus the lines written since). `k_emit_at` writes `#line` exactly when
 * the line it is asked for is not the one the counter gives — never otherwise
 * (backend §6: the invariant holds both ways). No other code writes `#line`.
 *
 * Memory: the caller hands an output slice. Bytes past its length are counted
 * and dropped, like snprintf, so a first pass with an empty slice measures the
 * size and a second one fills it (the convention of `k_dump_ast`). */
#ifndef CGEN_ENGINE_EMIT_WRITER_H
#define CGEN_ENGINE_EMIT_WRITER_H
#include <stdbool.h>
#include <stddef.h>
#include "keel/keel_slice_char.type.h"

typedef struct {
    keel_slice_char out;      /* caller memory; may be { 0, NULL } to measure */
    size_t          n;        /* bytes produced so far (may exceed out.len) */
    size_t          mapped;   /* .k line of the next output line under the last
                                 #line; 0 while no #line has been written */
    keel_slice_char src_path; /* the module's normalized path, with .k (§6, rule 6) */
    bool            line_on;  /* false under --line=off: no #line at all */
    bool            at_bol;   /* the next byte starts a line */
} KWriter;

/* Starts an empty file: n = 0, mapped = 0, at_bol = true. */
void k_writer_init(KWriter *w, keel_slice_char out, keel_slice_char src_path, bool line_on);

/* Writes `text` that has no .k line of its own: synthetic includes, guards,
 * the header comment, injected code (backend §6, "sintético" and "código
 * injetado"). It never writes #line. Each '\n' in it still advances `mapped`
 * when `mapped` is not 0, because the compiler counts those lines too. */
void k_emit_raw(KWriter *w, keel_slice_char text);

/* Writes `text`, whose first line is line `src_line` of the .k, and whose
 * lines are consecutive in the .k (one source line may become several output
 * lines: then the counter runs ahead, and the next k_emit_at resyncs).
 * Precondition: at_bol, and `text` ends in '\n'; otherwise nothing is written
 * and the function returns false.
 * When line_on and `mapped != src_line`, it first writes
 *     #line <src_line> "<src_path>"\n
 * Then it writes `text`, and `mapped` becomes src_line + (number of '\n' in
 * text). Returns true. */
bool k_emit_at(KWriter *w, size_t src_line, keel_slice_char text);

/* Bytes produced: the size the file needs. */
size_t k_writer_len(const KWriter *w);

#endif /* CGEN_ENGINE_EMIT_WRITER_H */
