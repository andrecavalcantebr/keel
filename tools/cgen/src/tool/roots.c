#include <stdbool.h>
#include <string.h>

bool cgen_source_under_root(const char *source, const char *root) {
    if (strcmp(root, ".") == 0) {
        return source[0] != '/';
    }
    size_t root_len = strlen(root);
    if (strncmp(source, root, root_len) == 0 && (source[root_len] == '/' || source[root_len] == '\0')) {
        return true;
    }
    return false;
}

bool cgen_source_in_multiple_roots(const char *source, const char *const *roots, int root_count) {
    int count = 0;
    for (int i = 0; i < root_count; i++) {
        if (cgen_source_under_root(source, roots[i])) {
            // Check if this root was already counted
            bool already_counted = false;
            for (int j = 0; j < i; j++) {
                if (strcmp(roots[j], roots[i]) == 0) {
                    already_counted = true;
                    break;
                }
            }
            if (!already_counted) {
                count++;
                if (count > 1) {
                    return true;
                }
            }
        }
    }
    return false;
}