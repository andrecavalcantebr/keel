/* Oracle for tool/paths.c (cgen-tool §2.7, §3.3 items 4 and 6). Expected
 * values hand-derived from those rules. Not model-generated.
 *
 * Nothing here touches the filesystem: these are the string half of the
 * loader, and `a/b/../c` normalizes to `a/c` whether or not `a/b` exists.
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

bool cgen_path_normalize(const char *path, char *out, size_t cap);
bool cgen_module_name_of(const char *source, const char *root, char *out, size_t cap);
bool cgen_module_path(const char *module_name, size_t name_len,
                      const char *root, char *out, size_t cap);

static int failures = 0;

static void norm(const char *in, const char *want) {
    char got[256];
    if (!cgen_path_normalize(in, got, sizeof got)) {
        fprintf(stderr, "FAIL: normalize(\"%s\") — returned false\n", in);
        failures++;
        return;
    }
    if (strcmp(got, want) != 0) {
        fprintf(stderr, "FAIL: normalize(\"%s\") = \"%s\", want \"%s\"\n", in, got, want);
        failures++;
    }
}

static void name_of(const char *src, const char *root, const char *want) {
    char got[256];
    bool ok = cgen_module_name_of(src, root, got, sizeof got);
    if (want == NULL) {
        if (ok) {
            fprintf(stderr, "FAIL: name_of(\"%s\",\"%s\") = \"%s\", want refusal\n",
                    src, root, got);
            failures++;
        }
        return;
    }
    if (!ok) {
        fprintf(stderr, "FAIL: name_of(\"%s\",\"%s\") — returned false\n", src, root);
        failures++;
        return;
    }
    if (strcmp(got, want) != 0) {
        fprintf(stderr, "FAIL: name_of(\"%s\",\"%s\") = \"%s\", want \"%s\"\n",
                src, root, got, want);
        failures++;
    }
}

static void mod_path(const char *name, const char *root, const char *want) {
    char got[256];
    if (!cgen_module_path(name, strlen(name), root, got, sizeof got)) {
        fprintf(stderr, "FAIL: module_path(\"%s\",\"%s\") — returned false\n", name, root);
        failures++;
        return;
    }
    if (strcmp(got, want) != 0) {
        fprintf(stderr, "FAIL: module_path(\"%s\",\"%s\") = \"%s\", want \"%s\"\n",
                name, root, got, want);
        failures++;
    }
}

int main(void) {
    /* §3.3 item 4: "remove `./`, colapsa `//`, resolve `..` lexicalmente" */
    norm("a/b.k",            "a/b.k");
    norm("./a/b.k",          "a/b.k");
    norm("a//b.k",           "a/b.k");
    norm("a/./b.k",          "a/b.k");
    norm("a/b/../c.k",       "a/c.k");
    norm("a/b/c/../../d.k",  "a/d.k");
    norm("/a//b/./c.k",      "/a/b/c.k");
    norm("/a/../b.k",        "/b.k");
    norm("/../a.k",          "/a.k");   /* `/..` is `/` */
    norm("../a.k",           "../a.k"); /* relative: a real path, kept */
    norm("../../a.k",        "../../a.k");
    norm("a/../../b.k",      "../b.k");
    norm(".",                ".");
    norm("./",               ".");
    norm("a/b/",             "a/b");

    /* the expected module name is the path under the root, `/` → `.` */
    name_of("src/app/reg.k", "src",  "app.reg");
    name_of("app/reg.k",     ".",    "app.reg");
    name_of("base/keel.k",   "base", "keel");
    name_of("base/keel/slice.k", "base", "keel.slice");
    name_of("src/a.k",       "src",  "a");
    /* refusals */
    name_of("other/a.k",     "src",  NULL);   /* not under the root */
    name_of("src/a.c",       "src",  NULL);   /* not a .k */
    name_of("src.k",         "src",  NULL);   /* the root is not a prefix dir */
    name_of("src/",          "src",  NULL);

    /* §3.3 item 6: `<root>/<a>/<b>.k` */
    mod_path("keel.slice",  "base", "base/keel/slice.k");
    mod_path("keel",        "base", "base/keel.k");
    mod_path("app.reg",     ".",    "app/reg.k");
    mod_path("a.b.c",       "/x/y", "/x/y/a/b/c.k");

    /* round trip: the name of a path under a root builds that path back */
    {
        char name[256], back[256];
        if (cgen_module_name_of("base/keel/slice.k", "base", name, sizeof name) &&
            cgen_module_path(name, strlen(name), "base", back, sizeof back)) {
            if (strcmp(back, "base/keel/slice.k") != 0) {
                fprintf(stderr, "FAIL: round trip = \"%s\"\n", back);
                failures++;
            }
        } else {
            fprintf(stderr, "FAIL: round trip — a step returned false\n");
            failures++;
        }
    }

    /* the output buffer is respected */
    {
        char tiny[4];
        if (cgen_path_normalize("aaaa/bbbb.k", tiny, sizeof tiny)) {
            fprintf(stderr, "FAIL: normalize must refuse a buffer too small\n");
            failures++;
        }
        if (cgen_module_path("aaaa.bbbb", 9, "root", tiny, sizeof tiny)) {
            fprintf(stderr, "FAIL: module_path must refuse a buffer too small\n");
            failures++;
        }
    }

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
