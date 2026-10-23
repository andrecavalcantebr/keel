#include <stdbool.h>
#include <string.h>

bool cgen_match_long_option(const char *arg, const char *name, bool *has_value, const char **value) {
    // Check if arg starts with "--"
    if (strncmp(arg, "--", 2) != 0) {
        return false;
    }

    // The argument has to begin with the name itself; this also keeps the
    // read below inside an argument shorter than the name
    size_t name_len = strlen(name);
    if (strncmp(arg + 2, name, name_len) != 0) {
        return false;
    }

    // Check if the argument matches "--name" exactly
    if (arg[2 + name_len] == '\0') {
        *has_value = false;
        *value = NULL;
        return true;
    }

    // Check if the argument matches "--name=" followed by anything
    if (arg[2 + name_len] == '=') {
        *has_value = true;
        *value = arg + 2 + name_len + 1;  // Point to the text after '='
        return true;
    }
    
    // No match
    return false;
}