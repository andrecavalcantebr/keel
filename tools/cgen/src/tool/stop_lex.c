/* tool/stop_lex.c — `--stop-after=lex` (cgen design §2.6, §5.1): lexes only
 * the given .k, prints one token per line on stdout, and the lexical
 * diagnostics on stderr; with an `error`, the exit code is 1 (cgen design §4). */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "tool/memory.h"
#include "engine/lexer.h"

bool cgen_read_source(keel_arena *arena, const char *path, keel_buffer_char *out);
void cgen_report(const char *path, keel_slice_char source, const KDiagnosticSink *sink);

int cgen_write_output(const char *path, const char *data, size_t n);

/* `out` is the file of `-o`, or NULL for stdout (cgen-tool-spec §4.2). The
   file is written only without an `error`, as §6 asks; stdout always gets the
   tokens, so a lexical error can be read next to the stream. */
int cgen_stop_after_lex(keel_arena *arena, const char *path, const char *out) {
    keel_buffer_char source;
    if (!cgen_read_source(arena,path, &source)) return 2;

    keel_slice_char input = { source.len, source.ptr };
    keel_slice_char none = { 0, NULL };

    /* the first pass only sizes: the dump, and how many diagnostics */
    KDiagnosticSink counter;
    k_diag_init(&counter, NULL, 0);
    size_t need = k_parser_keel(input, none, &counter);
    size_t ndiag = counter.count[K_INFO] + counter.count[K_WARNING] + counter.count[K_ERROR];

    char *dump = cgen_alloc(arena,need ? need : 1,1,1,false,"token dump");
    KDiagnostic *items = CGEN_NEW(arena,KDiagnostic,ndiag ? ndiag : 1);
    if (dump == NULL || items == NULL) {
        return 2;
    }
    KDiagnosticSink sink;
    k_diag_init(&sink, items, ndiag);
    keel_slice_char output = { need, dump };
    k_parser_keel(input, output, &sink);

    /* the engine does not know the file name: each line gets it here */
    size_t lines = 0, plen = strlen(path);
    for (size_t i = 0; i < need; ++i) if (dump[i] == '\n') lines++;
    size_t total = need + lines * (plen + 1);
    char *text = cgen_alloc(arena, total ? total : 1, 1, 1, false, "token dump");
    if (text == NULL) return 2;
    size_t start = 0, at = 0;
    for (size_t i = 0; i < need; ++i) {
        if (dump[i] == '\n') {
            memcpy(text + at, path, plen); at += plen;
            text[at++] = ':';
            memcpy(text + at, dump + start, i + 1 - start); at += i + 1 - start;
            start = i + 1;
        }
    }

    int status = k_diag_count(&sink, K_ERROR) > 0 ? 1 : 0;
    if (out == NULL || status == 0) {
        int w = cgen_write_output(out, text, at);
        if (w) status = w;
    }
    cgen_report(path, input, &sink);
    return status;
}
