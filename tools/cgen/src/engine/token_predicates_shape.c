#include <stdbool.h>
#include <string.h>
#include "keel/keel_slice_char.type.h"

typedef keel_slice_char KToken;

bool k_token_is_number(KToken t) {
    if (t.len == 0) return false;
    
    char first = t.ptr[0];
    if (first >= '0' && first <= '9') {
        return true;
    }
    
    if (first == '.') {
        if (t.len > 1) {
            char second = t.ptr[1];
            if (second >= '0' && second <= '9') {
                return true;
            }
        }
    }
    
    return false;
}

bool k_token_is_string(KToken t) {
    if (t.len == 0) return false;
    
    const char *ptr = t.ptr;
    size_t len = t.len;
    
    // Check for optional prefix
    if (len >= 2 && ptr[0] == 'u' && ptr[1] == '8') {
        ptr += 2;
        len -= 2;
    } else if (len >= 1) {
        if (ptr[0] == 'u' || ptr[0] == 'U' || ptr[0] == 'L') {
            ptr += 1;
            len -= 1;
        }
    }
    
    // Must have a quote after prefix
    if (len == 0) return false;
    
    if (*ptr == '"') {
        return true;
    }
    
    return false;
}

bool k_token_is_char(KToken t) {
    if (t.len == 0) return false;
    
    const char *ptr = t.ptr;
    size_t len = t.len;
    
    // Check for optional prefix
    if (len >= 1) {
        if (ptr[0] == 'u' || ptr[0] == 'U' || ptr[0] == 'L') {
            ptr += 1;
            len -= 1;
        }
    }
    
    // Must have a quote after prefix
    if (len == 0) return false;
    
    if (*ptr == '\'') {
        return true;
    }
    
    return false;
}

bool k_token_is_punct(KToken t, const char *spelling) {
    if (t.len == 0 || spelling == NULL) return false;
    
    size_t spell_len = strlen(spelling);
    if (spell_len != t.len) return false;
    
    return memcmp(t.ptr, spelling, t.len) == 0;
}