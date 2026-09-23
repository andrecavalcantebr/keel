#include <stdbool.h>
#include <string.h>

bool cgen_match_long_option(const char *arg, const char *name, bool *has_value, const char **value) {
    // Check if arg starts with "--"
    if (strncmp(arg, "--", 2) != 0) {
        return false;
    }

    // Get the length of the option name
    size_t name_len = strlen(name);
    
    // Check if the argument matches "--name" exactly
    if (strcmp(arg + 2, name) == 0) {
        *has_value = false;
        *value = NULL;
        return true;
    }
    
    // Check if the argument matches "--name=" followed by anything
    // The length should be 2 + name_len + 1 ('=')
    if (arg[2 + name_len] == '=') {
        *has_value = true;
        *value = arg + 2 + name_len + 1;  // Point to the text after '='
        return true;
    }
    
    // No match
    return false;
}