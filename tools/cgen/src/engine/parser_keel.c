/* engine/parser_keel.c — the engine's entry point. For now, the token dump
 * of `--stop-after=lex` (cgen design §5.1). */
#include <string.h>
#include "engine/lexer.h"

typedef struct {
    keel_slice_char out;
    size_t n;                            /* bytes the dump needs so far */
} KOut;

static void put_c(KOut *o, char c) {
    if (o->n < o->out.len) o->out.ptr[o->n] = c;
    o->n++;
}

static void put_s(KOut *o, const char *s) {
    while (*s) put_c(o, *s++);
}

static void put_uint(KOut *o, size_t v) {
    char digits[24];
    size_t k = 0;
    do { digits[k++] = (char)('0' + v % 10); v /= 10; } while (v > 0);
    while (k > 0) put_c(o, digits[--k]);
}

/* the physical spelling, escaped as a C literal (cgen design §5.1) */
static void put_escaped(KOut *o, KToken t) {
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < t.len; ++i) {
        unsigned char c = (unsigned char)t.ptr[i];
        switch (c) {
            case '\\': put_s(o, "\\\\"); break;
            case '"':  put_s(o, "\\\""); break;
            case '\n': put_s(o, "\\n");  break;
            case '\r': put_s(o, "\\r");  break;
            case '\t': put_s(o, "\\t");  break;
            default:
                if (c < 0x20 || c == 0x7f) {
                    put_s(o, "\\x");
                    put_c(o, hex[c >> 4]);
                    put_c(o, hex[c & 15]);
                } else {
                    put_c(o, (char)c);
                }
        }
    }
}

static bool is_punct_byte(int c) {
    return c > 0 && strchr("[](){}.&*+-~!/%<>^|?:;=,#", c) != NULL;
}

/* The class is computed on the logical spelling [D7], in the order of cgen
   design §5.1. */
static const char *token_class(KToken t, bool directive, TKPpKind kind) {
    if (directive) {
        switch (kind) {
            case TK_PP_IF:    return "pp-if";
            case TK_PP_ELSE:  return "pp-else";
            case TK_PP_ENDIF: return "pp-endif";
            default:          return "pp-other";
        }
    }
    if (k_token_is_c_word(t)) return "cword";
    if (k_token_is_ident(t))  return "ident";
    if (k_token_is_number(t)) return "number";
    if (k_token_is_string(t)) return "string";
    if (k_token_is_char(t))   return "char";
    if (is_punct_byte(k_lexer_peek_at(t, 0, NULL))) return "punct";
    return "other";
}

size_t k_parser_keel(keel_slice_char input, keel_slice_char output,
                     KDiagnosticSink *diagnostics) {
    KOut o = { output, 0 };
    KLexer lexer;
    k_lexer_init(&lexer, input, diagnostics);

    size_t line = 1, col = 1, at = 0;
    for (;;) {
        TKPpKind kind;
        KToken t = k_lexer_next(&lexer, &kind);
        size_t off = input.len == 0 ? 0 : (size_t)(t.ptr - input.ptr);

        /* physical position: CRLF and a lone CR are one line break */
        for (; at < off; ++at) {
            char c = input.ptr[at];
            if (c == '\n' || (c == '\r' && !(at + 1 < input.len && input.ptr[at + 1] == '\n'))) {
                line++;
                col = 1;
            } else if (c != '\r') {
                col++;
            }
        }

        put_uint(&o, line);
        put_c(&o, ':');
        put_uint(&o, col);
        put_s(&o, ": ");
        if (t.len == 0) {
            put_s(&o, "eof\n");
            break;
        }
        put_s(&o, token_class(t, lexer.directive, kind));
        put_s(&o, " \"");
        put_escaped(&o, t);
        put_s(&o, "\"\n");
    }
    return o.n;
}
