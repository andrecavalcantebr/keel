/* Hand-written driver for tools/harness/tasks/m0-roots-check.md. Not
 * model-generated. */

#include <stdbool.h>
#include <stdio.h>

bool cgen_source_under_root(const char *source, const char *root);
bool cgen_source_in_multiple_roots(const char *source, const char *const *roots, int root_count);

static int failures = 0;

static void check_under(const char *source, const char *root, bool want, const char *label) {
    bool got = cgen_source_under_root(source, root);
    if (got != want) {
        fprintf(stderr, "FAIL: %s — under('%s', '%s') = %d, want %d\n",
                label, source, root, got, want);
        failures++;
    }
}

static void check_multi(const char *source, const char *const *roots, int n, bool want, const char *label) {
    bool got = cgen_source_in_multiple_roots(source, roots, n);
    if (got != want) {
        fprintf(stderr, "FAIL: %s — in_multiple('%s', %d roots) = %d, want %d\n",
                label, source, n, got, want);
        failures++;
    }
}

int main(void) {
    /* cgen_source_under_root */
    check_under("a.k", ".", true, "dot contains a bare relative file");
    check_under("src/a.k", ".", true, "dot contains a nested relative path (the special case)");
    check_under("src/a.k", "src", true, "exact directory prefix");
    check_under("srcfoo/a.k", "src", false, "must not match a sibling dir sharing a prefix");
    check_under("src", "src", true, "source identical to root");
    check_under("a.k", "src", false, "unrelated root");
    check_under("src/sub/a.k", "src", true, "root contains a deeper nested path");
    check_under("s", "src", false, "root longer than source never contains it");

    /* cgen_source_in_multiple_roots */
    {
        const char *roots[] = {".", "src"};
        check_multi("src/a.k", roots, 2, true, "dot + src both contain src/a.k");
    }
    {
        const char *roots[] = {"src", "lib"};
        check_multi("src/a.k", roots, 2, false, "only src contains src/a.k");
    }
    {
        const char *roots[] = {"."};
        check_multi("a.k", roots, 1, false, "a single root is never \"multiple\"");
    }
    {
        const char *roots[] = {".", "."};
        check_multi("a.k", roots, 2, false, "the same root twice is not two roots");
    }
    {
        const char *roots[] = {"src", "src/sub"};
        check_multi("src/sub/a.k", roots, 2, true, "nested roots can both contain the same source");
    }

    if (failures == 0) {
        puts("ok");
        return 0;
    }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
