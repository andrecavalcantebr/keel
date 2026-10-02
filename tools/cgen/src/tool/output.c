/* tool/output.c — where a phase that stops before the C compiler writes
 * (cgen-tool-spec §4.2): `stdout`, or the file of `-o` as gcc reads it with
 * `-E`; `-o -` is `stdout`. The file follows the writing rules of §6: an
 * identical file is not touched, and the bytes go to a temporary in the same
 * directory that is then renamed over it. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stddef.h>

static bool same_content(const char *path, const char *data, size_t n) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    bool same = true;
    char buf[4096];
    size_t seen = 0, got;
    while (same && (got = fread(buf, 1, sizeof buf, f)) > 0) {
        if (seen + got > n || memcmp(buf, data + seen, got) != 0) same = false;
        seen += got;
    }
    fclose(f);
    return same && seen == n;
}

/* Returns 0, or 2 after reporting write-failure. */
int cgen_write_output(const char *path, const char *data, size_t n) {
    if (path == NULL || strcmp(path, "-") == 0) {
        if (n && fwrite(data, 1, n, stdout) != n) goto stdout_failed;
        if (fflush(stdout) != 0) goto stdout_failed;
        return 0;
    }
    if (same_content(path, data, n)) return 0;
    size_t len = strlen(path);
    char *tmp = malloc(len + 32);
    if (!tmp) goto failed;
    snprintf(tmp, len + 32, "%s.tmp.%ld", path, (long)getpid());
    FILE *f = fopen(tmp, "wb");
    if (!f) { free(tmp); goto failed; }
    bool ok = (n == 0 || fwrite(data, 1, n, f) == n);
    ok = (fclose(f) == 0) && ok;
    if (!ok || rename(tmp, path) != 0) { remove(tmp); free(tmp); goto failed; }
    free(tmp);
    return 0;
failed:
    fprintf(stderr, "cgen: error: cannot write '%s' [write-failure]\n", path);
    return 2;
stdout_failed:
    fprintf(stderr, "cgen: error: cannot write the standard output [write-failure]\n");
    return 2;
}
