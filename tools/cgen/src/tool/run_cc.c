/* tool/run_cc.c — the C compiler phase (cgen-tool-spec §4, §7): the driver hands the
 * generated unit to the compiler with the options it was given, verbatim and in order,
 * plus the `-I` of the destination (§4.1) and `-DKEEL_CHECKS=0` under --checks=off
 * (backend §5.17). The compiler's exit code is the tool's (§7). */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "tool/cli_limits.h"
#include "tool/tool.h"

/* ---- the depfile (cgen-tool-spec §4.10) ----
 * The trigger is the gcc's: -MMD or -MD on the line. The file is -MF's, else the output's name
 * with `.d`, else the unit's. What the compiler reported stays, and the closure of .k files joins
 * its one rule, sorted, so that two runs write the same bytes (§6.1). */

static int by_path(const void *a, const void *b) { return strcmp(*(const char *const *)a, *(const char *const *)b); }

/* `out` names the dependency the way make reads it: a space, `#` and `$` are escaped */
static size_t make_escape(char *out, size_t cap, const char *s) {
    size_t n = 0;
    for (; *s && n + 3 < cap; s++) {
        if (*s == ' ' || *s == '#') out[n++] = '\\';
        else if (*s == '$') out[n++] = '$';
        out[n++] = *s;
    }
    out[n] = '\0';
    return n;
}

static bool wants_depfile(const char *const *pass, int pass_n) {
    for (int i = 0; i < pass_n; i++)
        if (strcmp(pass[i], "-MMD") == 0 || strcmp(pass[i], "-MD") == 0) return true;
    return false;
}

/* the depfile's path (gcc's rule), into buf */
static void depfile_path(char *buf, size_t cap, const char *const *pass, int pass_n, const char *unit) {
    const char *mf = NULL, *out = NULL;
    for (int i = 0; i < pass_n; i++) {
        if (strcmp(pass[i], "-MF") == 0 && i + 1 < pass_n) mf = pass[i + 1];
        else if (strcmp(pass[i], "-o") == 0 && i + 1 < pass_n) out = pass[i + 1];
    }
    if (mf) { snprintf(buf, cap, "%s", mf); return; }
    const char *base = out ? out : strrchr(unit, '/') ? strrchr(unit, '/') + 1 : unit;
    snprintf(buf, cap, "%s", base);
    char *dot = strrchr(buf, '.'), *slash = strrchr(buf, '/');
    if (dot && (!slash || dot > slash)) *dot = '\0';
    size_t n = strlen(buf);
    if (n + 3 <= cap) memcpy(buf + n, ".d", 3);
}

/* the target of the rule: -MT, else -MQ (escaped), else `fallback` */
static void depfile_target(char *buf, size_t cap, const char *const *pass, int pass_n, const char *fallback) {
    for (int i = 0; i + 1 < pass_n; i++) {
        if (strcmp(pass[i], "-MT") == 0) { snprintf(buf, cap, "%s", pass[i + 1]); return; }
        if (strcmp(pass[i], "-MQ") == 0) { make_escape(buf, cap, pass[i + 1]); return; }
    }
    make_escape(buf, cap, fallback);
}

/* Appends the closure of .k files to the first rule of the compiler's depfile, or writes the
 * whole file when the compiler wrote none (`target` then names the rule). */
static int fuse_depfile(const char *path, const char *target, bool compiler_wrote, const CgenUnits *units) {
    char *text = NULL;
    size_t len = 0;
    if (compiler_wrote) {
        FILE *f = fopen(path, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            text = malloc((size_t)sz + 1);
            if (!text || fread(text, 1, (size_t)sz, f) != (size_t)sz) { fclose(f); free(text); return 2; }
            fclose(f);
            len = (size_t)sz;
            text[len] = '\0';
        }
    }
    const char *sorted[CGEN_SOURCES_CAP];
    size_t n = units->source_count;
    memcpy(sorted, units->sources, n * sizeof sorted[0]);
    qsort(sorted, n, sizeof sorted[0], by_path);

    /* the end of the first rule: the first newline that no backslash continues */
    size_t cut = len;
    for (size_t i = 0; i < len; i++)
        if (text[i] == '\n' && (i == 0 || text[i - 1] != '\\')) { cut = i; break; }
    FILE *o = fopen(path, "wb");
    if (!o) { free(text); fprintf(stderr, "cgen: error: cannot write '%s' [write-failure]\n", path); return 2; }
    if (len) fwrite(text, 1, cut, o);
    else fprintf(o, "%s:", target);
    for (size_t i = 0; i < n; i++) {
        char esc[8192];
        make_escape(esc, sizeof esc, sorted[i]);
        fprintf(o, " \\\n  %s", esc);
    }
    if (len) fwrite(text + cut, 1, len - cut, o);
    else fputc('\n', o);
    free(text);
    return fclose(o) == 0 ? 0 : 2;
}

/* --stop-after=gen: no compiler to consult, the depfile has the closure, and the generated .c as the target */
int cgen_gen_depfile(const char *const *pass, int pass_n, const CgenUnits *units) {
    if (!wants_depfile(pass, pass_n)) return 0;
    char path[8192], target[8192];
    depfile_path(path, sizeof path, pass, pass_n, units->unit);
    depfile_target(target, sizeof target, pass, pass_n, units->unit);
    return fuse_depfile(path, target, false, units);
}

/* one compiler process; its exit code, or 128+signal */
static int spawn(const char *cc, char *const *argv) {
    fflush(NULL);
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 2; }
    if (pid == 0) {
        execvp(cc, argv);
        fprintf(stderr, "cgen: error: cannot run the C compiler '%s' [cc-not-found]\n", cc);
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) { perror("waitpid"); return 2; }
    return WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? 128 + WTERMSIG(status) : 2;
}

static bool is_dep_option(const char *a) {
    return strcmp(a, "-MMD") == 0 || strcmp(a, "-MD") == 0 || strcmp(a, "-MP") == 0 || strcmp(a, "-MF") == 0 ||
           strcmp(a, "-MT") == 0 || strcmp(a, "-MQ") == 0;
}

/* The entry unit under `-c`, `-S` or `-E` (§4.7, §4.10): a unit, an output, so a second compiler
 * run, as a build would write it. The output goes beside the module's, named after the unit
 * (`main_app_dbg.o`); the dependency options are the module's, and stay out of this run. */
static int compile_entry(const char *cc, const char *const *pass, int pass_n, const char *dest_flag,
                         bool checks_off, const char *entry) {
    char *argv[CGEN_CLI_ARGS_CAP + 8];
    char out[8192];
    int argc = 0;
    argv[argc++] = (char *)cc;
    const char *module_out = NULL;
    bool asm_only = false, pre_only = false;
    for (int i = 0; i < pass_n; i++) {
        if (strcmp(pass[i], "-o") == 0 && i + 1 < pass_n) { module_out = pass[++i]; continue; }
        if (strcmp(pass[i], "-S") == 0) asm_only = true;
        if (strcmp(pass[i], "-E") == 0) pre_only = true;
        if (is_dep_option(pass[i])) { if (strcmp(pass[i], "-MF") == 0 || strcmp(pass[i], "-MT") == 0 || strcmp(pass[i], "-MQ") == 0) i++; continue; }
        argv[argc++] = (char *)pass[i];
    }
    if (module_out) {
        const char *base = strrchr(entry, '/') ? strrchr(entry, '/') + 1 : entry;
        const char *slash = strrchr(module_out, '/');
        size_t dir = slash ? (size_t)(slash - module_out) + 1 : 0;
        int w = snprintf(out, sizeof out, "%.*s%s", (int)dir, module_out, base);
        char *dot = strrchr(out, '.');
        if (w < 0 || (size_t)w >= sizeof out - 3 || !dot) return 2;
        strcpy(dot, asm_only ? ".s" : pre_only ? ".i" : ".o");
        argv[argc++] = (char *)"-o";
        argv[argc++] = out;
    }
    argv[argc++] = (char *)dest_flag;
    if (checks_off) argv[argc++] = (char *)"-DKEEL_CHECKS=0";
    argv[argc++] = (char *)entry;
    argv[argc] = NULL;
    return spawn(cc, argv);
}

int cgen_run_cc(const char *cc, const char *const *pass, int pass_n, const char *dest, bool checks_off, const CgenUnits *units) {
    char *argv[CGEN_CLI_ARGS_CAP + 8];
    int argc = 0;
    char dest_flag[8192 + 2];
    if (snprintf(dest_flag, sizeof dest_flag, "-I%s", dest) >= (int)sizeof dest_flag) {
        fprintf(stderr, "cgen: error: destination path too long [write-failure]\n");
        return 2;
    }
    argv[argc++] = (char *)cc;
    for (int i = 0; i < pass_n; i++) argv[argc++] = (char *)pass[i];
    argv[argc++] = dest_flag;
    if (checks_off) argv[argc++] = (char *)"-DKEEL_CHECKS=0";
    argv[argc++] = (char *)units->unit;
    /* `-c`, `-S` and `-E` are one unit, one output (§4.10): the entry unit is a run of its own;
     * otherwise both units go to one run, which links them */
    bool one_unit = false;
    for (int i = 0; i < pass_n; i++)
        if (strcmp(pass[i], "-c") == 0 || strcmp(pass[i], "-S") == 0 || strcmp(pass[i], "-E") == 0) one_unit = true;
    if (units->entry[0] && !one_unit) argv[argc++] = (char *)units->entry;
    argv[argc] = NULL;

    int code = spawn(cc, argv);
    if (code == 0 && units->entry[0] && one_unit)
        code = compile_entry(cc, pass, pass_n, dest_flag, checks_off, units->entry);
    if (code == 0 && wants_depfile(pass, pass_n)) {
        char path[8192], target[8192];
        depfile_path(path, sizeof path, pass, pass_n, units->unit);
        depfile_target(target, sizeof target, pass, pass_n, units->unit);
        return fuse_depfile(path, target, true, units);
    }
    return code;
}
