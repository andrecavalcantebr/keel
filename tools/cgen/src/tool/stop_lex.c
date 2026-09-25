/* tool/stop_lex.c — `--stop-after=lex` (cgen design §2.6, §5.1): lexes only
 * the given .k, prints one token per line on stdout, and the lexical
 * diagnostics on stderr; with an `error`, the exit code is 1 (cgen design §4). */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "keel/keel_buffer_char.type.h"
#include "engine/lexer.h"

bool cgen_read_source(const char *path, keel_buffer_char *out);
void cgen_report(const char *path, keel_slice_char source, const KDiagnosticSink *sink);

int cgen_stop_after_lex(const char *path) {
    keel_buffer_char source;
    if (!cgen_read_source(path, &source)) return 2;

    keel_slice_char input = { source.len, source.ptr };
    keel_slice_char none = { 0, NULL };

    /* the first pass only sizes: the dump, and how many diagnostics */
    KDiagnosticSink counter;
    k_diag_init(&counter, NULL, 0);
    size_t need = k_parser_keel(input, none, &counter);
    size_t ndiag = counter.count[K_INFO] + counter.count[K_WARNING] + counter.count[K_ERROR];

    char *dump = malloc(need > 0 ? need : 1);
    KDiagnostic *items = malloc((ndiag > 0 ? ndiag : 1) * sizeof *items);
    if (dump == NULL || items == NULL) {
        fprintf(stderr, "cgen: error: cannot allocate the token dump [out-of-memory]\n");
        free(items);
        free(dump);
        free(source.ptr);
        return 2;
    }
    KDiagnosticSink sink;
    k_diag_init(&sink, items, ndiag);
    keel_slice_char output = { need, dump };
    k_parser_keel(input, output, &sink);

    /* the engine does not know the file name: each line gets it here */
    size_t start = 0;
    for (size_t i = 0; i < need; ++i) {
        if (dump[i] == '\n') {
            fputs(path, stdout);
            fputc(':', stdout);
            fwrite(dump + start, 1, i + 1 - start, stdout);
            start = i + 1;
        }
    }

    fflush(stdout);
    cgen_report(path, input, &sink);
    int status = k_diag_count(&sink, K_ERROR) > 0 ? 1 : 0;

    free(items);
    free(dump);
    free(source.ptr);
    return status;
}
