#include <stddef.h>
#include <stdbool.h>
#include "keel/keel_slice_char.type.h"

extern int k_lexer_peek_at(keel_slice_char source, size_t pos, size_t *width_out);

size_t k_lexer_scan_quoted(keel_slice_char source, size_t pos, bool *unterminated_out) {
    size_t current = pos;
    size_t width = 0;
    int ch = k_lexer_peek_at(source, pos, &width);
    char opener = 0;

    // Step 1: Skip prefix and find opener
    if (ch == '"' || ch == '\'') {
        opener = ch;
        current += width;
    } else if (ch == 'u' || ch == 'U' || ch == 'L') {
        size_t next_pos = current + width;
        int next_ch = k_lexer_peek_at(source, next_pos, &width);
        if (ch == 'u' && next_ch == '8') {
            // Skip u8 prefix
            current = next_pos + width;
            next_pos = current;
            next_ch = k_lexer_peek_at(source, next_pos, &width);
            if (next_ch != '"') {
                return current;
            }
            opener = '"';
            current = next_pos + width;
        } else {
            // Skip u, U, or L prefix
            current = next_pos + width;
            opener = next_ch;
        }
    } else {
        // No valid opener found
        return current;
    }

    // Step 2: Scan until end of literal
    while (1) {
        ch = k_lexer_peek_at(source, current, &width);
        if (ch == -1) {
            // End of input
            if (unterminated_out != NULL)
                *unterminated_out = true;
            return current;
        }

        if (ch == opener) {
            // Found closing quote
            current += width;
            if (unterminated_out != NULL)
                *unterminated_out = false;
            return current;
        } else if (ch == '\\') {
            // Escape sequence
            current += width;
            size_t next_pos = current;
            int next_ch = k_lexer_peek_at(source, current, &next_pos);
            if (next_ch == -1) {
                // Trailing backslash at end of input
                if (unterminated_out != NULL)
                    *unterminated_out = true;
                return current;
            }
            current += next_pos;
        } else if (ch == '\n' || ch == '\r') {
            // Unescaped newline
            if (unterminated_out != NULL)
                *unterminated_out = true;
            return current;
        } else {
            // Regular character
            current += width;
        }
    }

    // Should never reach here
    return current;
}