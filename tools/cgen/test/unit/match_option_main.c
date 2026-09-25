/* Unit test of tool/, first written as the oracle of the harness task m0-match-long-option.md. Not
 * model-generated. */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool cgen_match_long_option(const char *arg, const char *name, bool *has_value, const char **value);

static int failures = 0;

static void expect_no_match(const char *arg, const char *name, const char *label) {
    bool has_value = false;
    const char *value = NULL;
    bool got = cgen_match_long_option(arg, name, &has_value, &value);
    if (got) {
        fprintf(stderr, "FAIL: %s — match('%s', '%s') = true, want false\n", label, arg, name);
        failures++;
    }
}

static void expect_match_no_value(const char *arg, const char *name, const char *label) {
    bool has_value = true; /* deliberately wrong default, to catch a function that forgets to set it */
    const char *value = "not-null"; /* same idea */
    bool got = cgen_match_long_option(arg, name, &has_value, &value);
    if (!got) {
        fprintf(stderr, "FAIL: %s — match('%s', '%s') = false, want true\n", label, arg, name);
        failures++;
        return;
    }
    if (has_value) {
        fprintf(stderr, "FAIL: %s — has_value = true, want false (separate form)\n", label);
        failures++;
    }
}

static void expect_match_value(const char *arg, const char *name, const char *want_value, const char *label) {
    bool has_value = false;
    const char *value = NULL;
    bool got = cgen_match_long_option(arg, name, &has_value, &value);
    if (!got) {
        fprintf(stderr, "FAIL: %s — match('%s', '%s') = false, want true\n", label, arg, name);
        failures++;
        return;
    }
    if (!has_value) {
        fprintf(stderr, "FAIL: %s — has_value = false, want true (glued form)\n", label);
        failures++;
        return;
    }
    if (value == NULL || strcmp(value, want_value) != 0) {
        fprintf(stderr, "FAIL: %s — value = '%s', want '%s'\n", label, value ? value : "(null)", want_value);
        failures++;
    }
}

int main(void) {
    expect_match_no_value("--cc", "cc", "separate form, no value yet");
    expect_match_value("--cc=gcc", "cc", "gcc", "glued form with a value");
    expect_match_value("--cc=", "cc", "", "glued form with an empty value");
    expect_match_value("--dest-dir=/tmp/gen", "dest-dir", "/tmp/gen", "a hyphenated option name");
    expect_match_value("--profile=c23", "profile", "c23", "another option, sanity check");

    expect_no_match("--ccache", "cc", "must not prefix-match a longer option");
    expect_no_match("--ccx", "cc", "must not prefix-match without = or end");
    expect_no_match("--dest-dir", "dest", "must not match a truncated name");
    expect_no_match("cc", "cc", "bare word, no dashes at all");
    expect_no_match("-cc", "cc", "single dash is not a long option");
    expect_no_match("--profile", "cc", "different option entirely");
    expect_no_match("--cc2", "cc", "extra trailing character, no separator");

    if (failures == 0) {
        puts("ok");
        return 0;
    }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
