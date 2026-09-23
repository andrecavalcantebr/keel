/* Hand-written driver for tools/harness/tasks/m1-read-source.md. Not
 * model-generated. */

#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "keel/keel_buffer_char.type.h"

bool cgen_read_source(const char *path, keel_buffer_char *out);

static int failures = 0;

static char *write_temp(const char *content, size_t len) {
    char *path = strdup("/tmp/keel-harness-read-source-XXXXXX");
    int fd = mkstemp(path);
    if (fd < 0) { perror("mkstemp"); exit(1); }
    FILE *f = fdopen(fd, "wb");
    fwrite(content, 1, len, f);
    fclose(f);
    return path;
}

int main(void) {
    /* a normal file, including an embedded NUL and a newline — the read
       must be byte-exact, not text-mode, not stopping at NUL */
    char content[] = {'m', 'o', 'd', 'u', 'l', 'e', ' ', 'a', ';', '\n', '\0', 'x'};
    char *path = write_temp(content, sizeof content);

    keel_buffer_char b;
    bool ok = cgen_read_source(path, &b);
    if (!ok) {
        fprintf(stderr, "FAIL: normal file — expected success\n");
        failures++;
    } else {
        if (b.len != sizeof content) {
            fprintf(stderr, "FAIL: normal file — len = %zu, want %zu\n", b.len, sizeof content);
            failures++;
        } else if (memcmp(b.ptr, content, sizeof content) != 0) {
            fprintf(stderr, "FAIL: normal file — content mismatch\n");
            failures++;
        }
        free(b.ptr);
    }
    remove(path);
    free(path);

    /* empty file: not an error, len 0 */
    char *empty_path = write_temp("", 0);
    keel_buffer_char eb;
    bool eok = cgen_read_source(empty_path, &eb);
    if (!eok) {
        fprintf(stderr, "FAIL: empty file — expected success (true), got false\n");
        failures++;
    } else if (eb.len != 0) {
        fprintf(stderr, "FAIL: empty file — len = %zu, want 0\n", eb.len);
        failures++;
    }
    if (eok) free(eb.ptr);
    remove(empty_path);
    free(empty_path);

    /* nonexistent path: false, and the diagnostic tag on stderr (checked
       by the shell oracle around this binary, not here) */
    keel_buffer_char nb;
    bool nok = cgen_read_source("/nonexistent/path/that/does/not/exist.k", &nb);
    if (nok) {
        fprintf(stderr, "FAIL: nonexistent file — expected false, got true\n");
        failures++;
    }

    if (failures == 0) {
        puts("ok");
        return 0;
    }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
