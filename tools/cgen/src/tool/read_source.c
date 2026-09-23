#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "keel/keel_buffer_char.type.h"

bool cgen_read_source(const char *path, keel_buffer_char *out) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "cgen: error: cannot open file '%s' [source-not-found]\n", path);
        return false;
    }

    int fd = fileno(file);
    if (fd == -1) {
        fclose(file);
        fprintf(stderr, "cgen: error: cannot get file descriptor for '%s' [source-not-found]\n", path);
        return false;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        fclose(file);
        fprintf(stderr, "cgen: error: cannot stat file '%s' [source-not-found]\n", path);
        return false;
    }

    if (st.st_size < 0) {
        fclose(file);
        fprintf(stderr, "cgen: error: invalid file size for '%s' [source-not-found]\n", path);
        return false;
    }

    size_t size = (size_t)st.st_size;

    char *buffer = malloc(size);
    if (!buffer) {
        fclose(file);
        fprintf(stderr, "cgen: error: cannot allocate memory for file '%s' [source-not-found]\n", path);
        return false;
    }

    size_t bytes_read = fread(buffer, 1, size, file);
    fclose(file);

    if (bytes_read != size) {
        free(buffer);
        fprintf(stderr, "cgen: error: short read from file '%s' [source-not-found]\n", path);
        return false;
    }

    out->ptr = buffer;
    out->len = size;
    out->cap = size;
    return true;
}