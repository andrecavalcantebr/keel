#define _POSIX_C_SOURCE 200809L
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "tool/memory.h"

char *cgen_resolve_base_dir(keel_arena *arena, const char *explicit_base_dir) {
    char *candidate = NULL;
    char *result = NULL;

    if (explicit_base_dir != NULL) {
        candidate = (char *)explicit_base_dir;
    } else {
        char exe_path[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
        if (len == -1) {
            fprintf(stderr, "cgen: error: base directory not found (no keel.k under '<candidate>') [base-not-found]\n");
            return NULL;
        }
        exe_path[len] = '\0';

        char *last_slash = strrchr(exe_path, '/');
        if (last_slash == NULL) {
            fprintf(stderr, "cgen: error: base directory not found (no keel.k under '<candidate>') [base-not-found]\n");
            return NULL;
        }
        *last_slash = '\0';

        const char *lib_base = "/../lib/base";
        size_t exe_len = strlen(exe_path);
        size_t lib_len = strlen(lib_base);
        candidate = cgen_alloc(arena,exe_len+lib_len+1,1,1,false,"base path");
        if (candidate == NULL) {
            return NULL;
        }
        memcpy(candidate, exe_path, exe_len);
        memcpy(candidate + exe_len, lib_base, lib_len + 1);
    }

    char *full_path = cgen_alloc(arena,strlen(candidate)+strlen("/keel.k")+1,1,1,false,"base path");
    if (full_path == NULL) {
        return NULL;
    }

    strcpy(full_path, candidate);
    strcat(full_path, "/keel.k");

    struct stat st;
    if (stat(full_path, &st) == -1) {
        fprintf(stderr, "cgen: error: base directory not found (no keel.k under '%s') [base-not-found]\n", candidate);
        return NULL;
    }

    result = str_cstr(cgen_string_dup(arena,candidate,strlen(candidate)));
    return result;
}
