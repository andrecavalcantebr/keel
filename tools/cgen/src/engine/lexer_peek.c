#include "keel/keel_slice_char.type.h"
#include <stddef.h>
#include <stdbool.h>

size_t k_lexer_splice_width(keel_slice_char source, size_t pos) {
    if (pos >= source.len) return 0;
    if (source.ptr[pos] != '\\') return 0;

    size_t next = pos + 1;
    if (next >= source.len) return 0;

    if (source.ptr[next] == '\n') return 2;
    if (source.ptr[next] == '\r') {
        size_t after = next + 1;
        if (after < source.len && source.ptr[after] == '\n') return 3;
        return 2;
    }

    return 0;
}

int k_lexer_peek_at(keel_slice_char source, size_t pos, size_t *width_out) {
    size_t current = pos;
    size_t width = 0;

    while (current < source.len) {
        size_t splice_width = k_lexer_splice_width(source, current);
        if (splice_width == 0) break;
        current += splice_width;
    }

    if (current >= source.len) {
        if (width_out != NULL) *width_out = 0;
        return -1;
    }

    int byte = (unsigned char)source.ptr[current];
    if (width_out != NULL) *width_out = current - pos + 1;
    return byte;
}