/* tool/report.c — prints the engine's diagnostics on stderr, in the format of
 * the tool spec §7 (diag-design §6):
 *   <file>:<line>:<column>: <severity>: <message> [<name>]
 * Line and column start at 1; the column counts bytes from the start of the
 * physical line (cgen design §4). */
#include <stdio.h>
#include "engine/diag.h"
#include "engine/lexer.h"

static const char *const severity_name[K_SEVERITY_COUNT] = {
    [K_INFO] = "info", [K_WARNING] = "warning", [K_ERROR] = "error",
};

/* an argument, by its logical spelling: splices are not printed */
static void put_arg(keel_slice_char a) {
    size_t pos = 0, w;
    int c;
    while ((c = k_lexer_peek_at(a, pos, &w)) >= 0) {
        fputc(c, stderr);
        pos += w;
    }
}

static void put_message(const char *fmt, const KDiagArgs *args) {
    int next = 0;
    for (const char *p = fmt; *p; ++p) {
        if (p[0] == '%' && p[1] == 's') {
            if (next < K_DIAG_MAX_ARGS) put_arg(args->s[next++]);
            ++p;
        } else {
            fputc(*p, stderr);
        }
    }
}

/* the physical line and column of `at` in `source`: CRLF and a lone CR are
   one line break, as in the token dump */
static void position(keel_slice_char source, const char *at, size_t *line, size_t *col) {
    size_t off = (size_t)(at - source.ptr);
    *line = 1;
    *col = 1;
    for (size_t i = 0; i < off && i < source.len; ++i) {
        char c = source.ptr[i];
        if (c == '\n' || (c == '\r' && !(i + 1 < source.len && source.ptr[i + 1] == '\n'))) {
            ++*line;
            *col = 1;
        } else if (c != '\r') {
            ++*col;
        }
    }
}

/* The engine keeps the diagnostics in source order, which is the order of
   §4 for one module. */
void cgen_report(const char *path, keel_slice_char source, const KDiagnosticSink *sink) {
    for (size_t i = 0; i < sink->len; ++i) {
        const KDiagnostic *d = &sink->items[i];
        size_t line, col;
        position(source, d->at.ptr, &line, &col);
        fprintf(stderr, "%s:%zu:%zu: %s: ", path, line, col, severity_name[d->severity]);
        put_message(k_diags[d->id].fmt, &d->args);
        fprintf(stderr, " [%s]\n", k_diags[d->id].name);
    }
}
