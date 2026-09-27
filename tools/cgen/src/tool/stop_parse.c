/* --stop-after=parse: own storage and I/O stay on the tool side. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "keel/keel_buffer_char.type.h"
#include "engine/ast.h"

bool cgen_read_source(const char *path, keel_buffer_char *out);
void cgen_report(const char *path, keel_slice_char source, const KDiagnosticSink *sink);

int cgen_stop_after_parse(const char *path) {
    keel_buffer_char source;
    if (!cgen_read_source(path, &source)) return 2;
    keel_slice_char input = { source.len, source.ptr };
    KDiagnosticSink counter;
    k_diag_init(&counter, NULL, 0);
    size_t count = k_lexemes(input, NULL, 0, &counter);
    size_t ndiag = counter.count[K_INFO] + counter.count[K_WARNING] + counter.count[K_ERROR];
    if (count > SIZE_MAX / sizeof(KLexeme) - 1 ||
        count > SIZE_MAX / sizeof(KAstNode) - 1) {
        free(source.ptr);
        return 2;
    }
    KLexeme *tokens = calloc(count + 1, sizeof *tokens);
    KAstNode *nodes = calloc(count + 1, sizeof *nodes);
    KDiagnostic *items = calloc(ndiag ? ndiag : 1, sizeof *items);
    if (!tokens || !nodes || !items) {
        fprintf(stderr, "cgen: error: cannot allocate parse tree [out-of-memory]\n");
        free(items); free(tokens); free(nodes); free(source.ptr);
        return 2;
    }
    KDiagnosticSink sink;
    k_diag_init(&sink, items, ndiag);
    k_lexemes(input, tokens, count, &sink);
    cgen_report(path, input, &sink);
    if (k_diag_count(&sink, K_ERROR)) {
        free(items); free(tokens); free(nodes); free(source.ptr);
        return 1;
    }
    KAst ast = { .source = input, .tokens = tokens, .token_count = count };
    if (!k_parse_ast(&ast, nodes, count + 1)) {
        fprintf(stderr, "%s:1:1: error: cannot parse module [unexpected-token]\n", path);
        free(items); free(tokens); free(nodes); free(source.ptr);
        return 1;
    }
    size_t need = k_dump_ast(&ast, path, (keel_slice_char){0});
    char *dump = malloc(need ? need : 1);
    if (!dump) {
        fprintf(stderr, "cgen: error: cannot allocate parse dump [out-of-memory]\n");
        free(items); free(tokens); free(nodes); free(source.ptr);
        return 2;
    }
    k_dump_ast(&ast, path, (keel_slice_char){ need, dump });
    fwrite(dump, 1, need, stdout);
    free(dump); free(items); free(tokens); free(nodes); free(source.ptr);
    return 0;
}
