/* tool/stop_lex.c — `--stop-after=lex` (cgen design §2.6, §5.1): lexes only
 * the given .k, and prints one token per line on stdout. */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "keel/keel_buffer_char.type.h"
#include "engine/lexer.h"

bool cgen_read_source(const char *path, keel_buffer_char *out);

int cgen_stop_after_lex(const char *path) {
    keel_buffer_char source;
    if (!cgen_read_source(path, &source)) return 2;

    keel_slice_char input = { source.len, source.ptr };
    keel_slice_char none = { 0, NULL };
    size_t need = k_parser_keel(input, none);

    char *dump = malloc(need > 0 ? need : 1);
    if (dump == NULL) {
        fprintf(stderr, "cgen: error: cannot allocate the token dump [out-of-memory]\n");
        free(source.ptr);
        return 2;
    }
    keel_slice_char output = { need, dump };
    k_parser_keel(input, output);

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

    free(dump);
    free(source.ptr);
    return 0;
}
