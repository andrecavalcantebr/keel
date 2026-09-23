/* Hand-written driver for tools/harness/tasks/m0-base-resolve.md. Not
 * model-generated. argv[1], if present, is passed through as the explicit
 * --base-dir; if absent, resolution must fall back to the real executable's
 * own path (/proc/self/exe on Linux), which is exactly why the oracle
 * script copies this binary into a synthetic install tree before running
 * the no-argument case. */

#include <stdio.h>

/* implemented by the file under test */
char *cgen_resolve_base_dir(const char *explicit_base_dir);

int main(int argc, char **argv) {
    const char *explicit_dir = argc > 1 ? argv[1] : NULL;
    char *resolved = cgen_resolve_base_dir(explicit_dir);
    if (!resolved) {
        return 1;
    }
    printf("%s\n", resolved);
    return 0;
}
