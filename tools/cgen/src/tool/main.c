#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool cgen_match_long_option(const char *arg, const char *name, bool *has_value, const char **value);
bool cgen_source_under_root(const char *source, const char *root);
bool cgen_source_in_multiple_roots(const char *source, const char *const *roots, int root_count);
extern char *cgen_resolve_base_dir(const char *explicit_base_dir);
int cgen_stop_after_lex(const char *path);

static void fatal(const char *diagnostic_id, const char *message) {
    fprintf(stderr, "cgen: error: %s [%s]\n", message, diagnostic_id);
    exit(2);
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        execvp("cc", argv);
        perror("execvp");
        exit(1);
    }

    const char *cc = "cc";
    const char *dest_dir = NULL;
    const char *base_dir = NULL;
    const char *instance = NULL;
    const char *stop_after = NULL;
    const char *checks = NULL;
    const char *line = NULL;
    const char *main_module = NULL;
    const char *profile = NULL;
    const char *parallel_lowering = NULL;
    bool pedantic_names = false;
    bool f_flag = false;

    int root_count = 0;
    const char *roots[128]; // arbitrary limit
    int passthrough_count = 0;
    const char *passthrough[256]; // arbitrary limit

    int k_file_count = 0;
    const char *k_file = NULL;

    for (int i = 1; i < argc; ++i) {
        const char *w = argv[i];

        if (strcmp(w, "--cgen-version") == 0) {
            printf("cgen 0.1.0\n");
            exit(0);
        }

        if (strcmp(w, "-I") == 0) {
            roots[root_count++] = argv[++i];
            passthrough[passthrough_count++] = w;
            passthrough[passthrough_count++] = roots[root_count - 1];
        } else if (strncmp(w, "-I", 2) == 0 && w[2] != '\0') {
            roots[root_count++] = w + 2;
            passthrough[passthrough_count++] = w;
        } else if (strcmp(w, "-o") == 0 || strcmp(w, "-MF") == 0 || strcmp(w, "-MT") == 0 ||
                   strcmp(w, "-MQ") == 0 || strcmp(w, "-D") == 0 || strcmp(w, "-U") == 0 ||
                   strcmp(w, "-L") == 0 || strcmp(w, "-l") == 0 || strcmp(w, "-x") == 0 ||
                   strcmp(w, "-include") == 0 || strcmp(w, "-imacros") == 0 ||
                   strcmp(w, "-isystem") == 0 || strcmp(w, "-iquote") == 0 ||
                   strcmp(w, "-idirafter") == 0 || strcmp(w, "-iprefix") == 0 ||
                   strcmp(w, "-iwithprefix") == 0 || strcmp(w, "-isysroot") == 0 ||
                   strcmp(w, "-Xlinker") == 0 || strcmp(w, "-Xassembler") == 0 ||
                   strcmp(w, "-Xpreprocessor") == 0 || strcmp(w, "-u") == 0 ||
                   strcmp(w, "-T") == 0 || strcmp(w, "-z") == 0 || strcmp(w, "--param") == 0 ||
                   strcmp(w, "-aux-info") == 0) {
            if (i + 1 >= argc)
                fatal("invalid-option", "option requires an argument");
            passthrough[passthrough_count++] = w;
            passthrough[passthrough_count++] = argv[++i];
        } else {
            bool has_value;
            const char *value;

            if (cgen_match_long_option(w, "dest-dir", &has_value, &value)) {
                if (!has_value) {
                    if (i + 1 >= argc)
                        fatal("invalid-option", "option requires an argument");
                    dest_dir = argv[++i];
                } else {
                    dest_dir = value;
                }
            } else if (cgen_match_long_option(w, "base-dir", &has_value, &value)) {
                if (!has_value) {
                    if (i + 1 >= argc)
                        fatal("invalid-option", "option requires an argument");
                    base_dir = argv[++i];
                } else {
                    base_dir = value;
                }
            } else if (cgen_match_long_option(w, "instance", &has_value, &value)) {
                if (!has_value) {
                    if (i + 1 >= argc)
                        fatal("invalid-option", "option requires an argument");
                    instance = argv[++i];
                } else {
                    instance = value;
                }
            } else if (cgen_match_long_option(w, "stop-after", &has_value, &value)) {
                if (!has_value) {
                    if (i + 1 >= argc)
                        fatal("invalid-option", "option requires an argument");
                    stop_after = argv[++i];
                } else {
                    stop_after = value;
                }
            } else if (cgen_match_long_option(w, "checks", &has_value, &value)) {
                if (!has_value) {
                    if (i + 1 >= argc)
                        fatal("invalid-option", "option requires an argument");
                    checks = argv[++i];
                } else {
                    checks = value;
                }
            } else if (cgen_match_long_option(w, "line", &has_value, &value)) {
                if (!has_value) {
                    if (i + 1 >= argc)
                        fatal("invalid-option", "option requires an argument");
                    line = argv[++i];
                } else {
                    line = value;
                }
            } else if (cgen_match_long_option(w, "main", &has_value, &value)) {
                if (!has_value) {
                    if (i + 1 >= argc)
                        fatal("invalid-option", "option requires an argument");
                    main_module = argv[++i];
                } else {
                    main_module = value;
                }
            } else if (cgen_match_long_option(w, "cc", &has_value, &value)) {
                if (!has_value) {
                    if (i + 1 >= argc)
                        fatal("invalid-option", "option requires an argument");
                    cc = argv[++i];
                } else {
                    cc = value;
                }
            } else if (cgen_match_long_option(w, "profile", &has_value, &value)) {
                if (!has_value) {
                    if (i + 1 >= argc)
                        fatal("invalid-option", "option requires an argument");
                    profile = argv[++i];
                } else {
                    profile = value;
                }
            } else if (cgen_match_long_option(w, "parallel-lowering", &has_value, &value)) {
                if (!has_value) {
                    if (i + 1 >= argc)
                        fatal("invalid-option", "option requires an argument");
                    parallel_lowering = argv[++i];
                } else {
                    parallel_lowering = value;
                }
            } else if (strcmp(w, "--pedantic-names") == 0) {
                pedantic_names = true;
            } else if (strcmp(w, "-f") == 0) {
                f_flag = true;
            } else if (w[0] == '-' && w[1] != '\0') {
                passthrough[passthrough_count++] = w;
            } else if (strlen(w) > 2 && strcmp(w + strlen(w) - 2, ".k") == 0) {
                k_file_count++;
                k_file = w;
                if (k_file_count > 1)
                    fatal("multiple-sources", "only one source file allowed");
            } else {
                passthrough[passthrough_count++] = w;
            }
        }
    }

    if (checks && strcmp(checks, "on") != 0 && strcmp(checks, "off") != 0)
        fatal("invalid-option", "invalid value for --checks");
    if (line && strcmp(line, "on") != 0 && strcmp(line, "off") != 0)
        fatal("invalid-option", "invalid value for --line");
    if (profile && strcmp(profile, "auto") != 0 && strcmp(profile, "c11") != 0 && strcmp(profile, "c23") != 0)
        fatal("invalid-option", "invalid value for --profile");
    if (parallel_lowering && strcmp(parallel_lowering, "auto") != 0 && strcmp(parallel_lowering, "serie") != 0 && strcmp(parallel_lowering, "openmp") != 0)
        fatal("invalid-option", "invalid value for --parallel-lowering");
    if (stop_after && strcmp(stop_after, "lex") != 0 && strcmp(stop_after, "parse") != 0 && strcmp(stop_after, "gen") != 0)
        fatal("invalid-option", "invalid value for --stop-after");

    if (k_file_count == 0 && instance == NULL) {
        // Transparent link
        char *new_argv[256];
        int new_argc = 0;
        new_argv[new_argc++] = (char *)cc;
        for (int i = 0; i < passthrough_count; ++i)
            new_argv[new_argc++] = (char *)passthrough[i];
        new_argv[new_argc] = NULL;
        execvp(cc, new_argv);
        perror("execvp");
        exit(1);
    }

    if (k_file_count == 1) {
        // Check source in multiple roots
        if (root_count == 0)
            roots[root_count++] = ".";
        if (cgen_source_in_multiple_roots(k_file, roots, root_count))
            fatal("source-in-multiple-roots", "source file is under multiple -I roots");
    }

    /* --stop-after=lex lexes only the given .k: no import, no base
       (cgen design §2.6) */
    if (k_file_count == 1 && stop_after && strcmp(stop_after, "lex") == 0)
        return cgen_stop_after_lex(k_file);

    if (k_file_count == 1) {
        char *base = cgen_resolve_base_dir(base_dir);
        if (!base) {
            exit(2);
        }
        /* `base` is deliberately unused past this point — nothing reads
           the base directory yet, only validates that it resolves. Do not
           free it, do not do anything else with it: that is later work. */
    }

    fprintf(stderr, "cgen: not yet implemented\n");
    exit(1);
}