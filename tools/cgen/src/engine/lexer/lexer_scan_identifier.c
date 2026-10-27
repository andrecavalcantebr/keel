#include <stddef.h>
#include <stdbool.h>
#include "keel/keel_slice_char.type.h"

extern int k_lexer_peek_at(keel_slice_char source, size_t pos, size_t *width_out);

size_t k_lexer_scan_identifier(keel_slice_char source, size_t pos) {
    while (true) {
        size_t w0;
        int ch = k_lexer_peek_at(source, pos, &w0);
        if (ch == -1) {
            break;
        }

        // Rule 1: universal character name
        if (ch == '\\') {
            size_t width;
            int next_ch = k_lexer_peek_at(source, pos + w0, &width);
            if (next_ch == 'u' || next_ch == 'U') {
                size_t hex_pos = pos + w0 + width;
                bool valid = true;
                int hex_count = (next_ch == 'u') ? 4 : 8;

                for (int i = 0; i < hex_count; ++i) {
                    int hex_ch = k_lexer_peek_at(source, hex_pos, &width);
                    if (hex_ch == -1) {
                        valid = false;
                        break;
                    }
                    if (!((hex_ch >= '0' && hex_ch <= '9') ||
                          (hex_ch >= 'a' && hex_ch <= 'f') ||
                          (hex_ch >= 'A' && hex_ch <= 'F'))) {
                        valid = false;
                        break;
                    }
                    hex_pos += width;
                }

                if (valid) {
                    pos = hex_pos;
                    continue;
                } else {
                    // Malformed universal character name, stop here
                    break;
                }
            }
        }

        // Rule 2: ASCII letter, digit, or underscore
        if ((ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '_') {
            pos += w0;
            continue;
        }

        // Rule 3: anything else stops the scan
        break;
    }

    return pos;
}