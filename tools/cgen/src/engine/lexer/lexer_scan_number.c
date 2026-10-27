#include <stddef.h>
#include <stdbool.h>
#include "keel/keel_slice_char.type.h"

extern int k_lexer_peek_at(keel_slice_char source, size_t pos, size_t *width_out);

size_t k_lexer_scan_number(keel_slice_char source, size_t pos) {
    int last_byte = -1;
    size_t current_pos = pos;

    while (true) {
        size_t width;
        int byte = k_lexer_peek_at(source, current_pos, &width);
        if (byte == -1) {
            break;
        }

        // Rule 1: digit, letter, or underscore
        if ((byte >= '0' && byte <= '9') ||
            (byte >= 'a' && byte <= 'z') ||
            (byte >= 'A' && byte <= 'Z') ||
            byte == '_') {
            current_pos += width;
            last_byte = byte;
            continue;
        }

        // Rule 2: Check for '..' sequence
        if (byte == '.') {
            size_t next_pos = current_pos + width;
            size_t next_width;
            int next_byte = k_lexer_peek_at(source, next_pos, &next_width);
            if (next_byte == '.') {
                break; // Stop before the dot
            } else {
                // Consume the dot and continue
                current_pos += width;
                last_byte = byte;
                continue;
            }
        }

        // Rule 3: '+' or '-' only if preceded by 'e', 'E', 'p', or 'P'
        if (byte == '+' || byte == '-') {
            if (last_byte == 'e' || last_byte == 'E' ||
                last_byte == 'p' || last_byte == 'P') {
                current_pos += width;
                last_byte = byte;
                continue;
            } else {
                break;
            }
        }

        // Anything else stops the scan
        break;
    }

    return current_pos;
}