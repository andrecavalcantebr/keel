/* engine/emit/writer.c — KWriter (writer.h; backend §6, codegen-design §8 E4).
 * The only code that writes `#line`. No I/O: the output goes to the slice
 * the caller hands in, and what does not fit is counted and dropped. */
#include <string.h>
#include "engine/emit/writer.h"

typedef struct { keel_slice_char out; size_t n; } KOut;
static void put_c(KOut *o, char c) {
    if (o->n < o->out.len) o->out.ptr[o->n] = c;
    o->n++;
}
static void put_s(KOut *o, const char *s) { while (*s) put_c(o, *s++); }
/* the decimal of n, by hand: no snprintf in engine/ (parser_dump.c put_uint) */
static void put_uint(KOut *o, size_t n) {
    char digits[24]; size_t len = 0;
    do { digits[len++] = (char)('0' + n % 10); n /= 10; } while (n);
    while (len) put_c(o, digits[--len]);
}

/* Copies `text` into w and updates at_bol. When `count` is set, each '\n'
 * advances mapped (only if mapped is not 0: raw text before any #line does
 * not move the counter, which is still "unknown"). */
static void put_text(KWriter *w, keel_slice_char text, bool count) {
    KOut o = { w->out, w->n };
    for (size_t i = 0; i < text.len; i++) {
        put_c(&o, text.ptr[i]);
        if (count && text.ptr[i] == '\n' && w->mapped != 0) w->mapped++;
    }
    w->n = o.n;
    if (text.len != 0) w->at_bol = text.ptr[text.len - 1] == '\n';
}

void k_writer_init(KWriter *w, keel_slice_char out, keel_slice_char src_path, bool line_on) {
    w->out = out;
    w->n = 0;
    w->mapped = 0;
    w->src_path = src_path;
    w->line_on = line_on;
    w->at_bol = true;
}

void k_emit_raw(KWriter *w, keel_slice_char text) {
    put_text(w, text, true);
}

bool k_emit_at(KWriter *w, size_t src_line, keel_slice_char text) {
    /* refused before anything is written (writer.h: precondition) */
    if (!w->at_bol || text.len == 0 || text.ptr[text.len - 1] != '\n') return false;

    /* mapped == 0 means no #line yet, so the first one is always written */
    if (w->line_on && (w->mapped == 0 || w->mapped != src_line)) {
        KOut o = { w->out, w->n };
        put_s(&o, "#line ");
        put_uint(&o, src_line);
        put_s(&o, " \"");
        for (size_t i = 0; i < w->src_path.len; i++) put_c(&o, w->src_path.ptr[i]);
        put_s(&o, "\"\n");
        w->n = o.n;
    }

    /* the text's first line is src_line; the #line above does not advance
     * mapped, it is what fixes it. The newlines of the text do. */
    w->mapped = src_line;
    put_text(w, text, true);
    return true;
}

size_t k_writer_len(const KWriter *w) {
    return w->n;
}
