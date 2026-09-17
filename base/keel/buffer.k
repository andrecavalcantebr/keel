/*
 * module keel.buffer type T
 *
 * DESC: keel buffer — sequência de comprimento variável (spec §5.3, backend §5.2, §5.11, §5.13)
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — ver /LICENSE.md
 */

module keel.buffer type T;

import keel.slice   as slice   types;
import keel.outcome as outcome types;
import keel.arena   as arena   types;

#include <string.h>

// `byref` recusa parâmetro por valor e diagnostica cópia entre instâncias
// (spec §4.3) — o contrário de `slice`.
pub modifier buffer byref { size_t cap, len; T *ptr; }

// declaração que não menciona `T` nem `buffer`: emitida uma vez no módulo,
// não por instância (backend §5.11) — `keel_buffer_cursor`.
pub typedef struct { size_t i; } cursor;

// `buffer.from(p,cap)`: ponteiro cru — nasce vazio (spec §5.3 §4).
pub inline buffer as(T *p, size_t n) {
    return (buffer){ p ? n : 0, 0, p };
}

// `buffer.of(v)`: sobre `array` — nasce cheio, `len == cap`.
pub inline buffer of(T *p, size_t n) {
    return (buffer){ p ? n : 0, p ? n : 0, p };
}

pub inline size_t length(buffer *b) { return b->len; }
pub inline size_t capacity(buffer *b) { return b->cap; }
pub inline T      get(buffer *b, size_t i) { return b->ptr[i]; }
pub inline void   set(buffer *b, size_t i, T v) { b->ptr[i] = v; }
pub inline T     *ptr(buffer *b) { return b->ptr; }
pub inline T     *ptr(buffer *b, size_t i) { return &b->ptr[i]; }

pub inline T *push(buffer *b) {
    if (b->len == b->cap) return NULL;
    return &b->ptr[b->len++];
}

pub inline T *push(buffer *b, T v) {
    if (b->len == b->cap) return NULL;
    b->ptr[b->len] = v;
    return &b->ptr[b->len++];
}

pub inline T *pop(buffer *b) {
    if (b->len == 0) return NULL;
    return &b->ptr[--b->len];
}

pub inline void clear(buffer *b) { b->len = 0; }

// único acessor com verificação em release (spec §5.3, backend §5.13).
pub inline outcome T at(buffer *b, size_t i) {
    outcome T r = {0};
    return i < b->len ? outcome.win(&r, b->ptr[i]) : outcome.none(&r);
}

// o recorte é verbo do buffer, e não do slice: é o buffer que conhece os
// dois módulos, e não o contrário — quebra o ciclo de layout (backend
// §4.3.1). `slice.of(x,a,b)` da spec §4.5 desce por aqui quando a origem é
// `buffer`.
pub inline slice as_slice(buffer *b) {
    return (slice){ b->len, b->ptr };
}

pub inline slice as_slice(buffer *b, size_t a, size_t c) {
    if (a > c || c > b->len) return (slice){0, b->ptr};
    return (slice){ c - a, b->ptr + a };
}

// aloca em `a`, copia o comprimento da origem (spec §5.3).
pub inline outcome buffer clone(arena *a, buffer *b) {
    outcome buffer r = {0};
    T *dados = arena.alloc(a, T, b->cap);
    if (!dados) return outcome.none(&r);
    if (b->len > 0) memcpy(dados, b->ptr, b->len * sizeof(T));
    return outcome.win(&r, (buffer){ b->cap, b->len, dados });
}

// cursor: `walk` pede `begin`, `has_next` e `next`; `next` devolve o
// endereço do elemento, então o binder é por ponteiro (spec §5.3).
pub inline cursor begin(buffer *b) { (void)b; return (cursor){0}; }
pub inline bool   has_next(buffer *b, cursor *c) { return c->i < b->len; }
pub inline T     *next(buffer *b, cursor *c) { return &b->ptr[c->i++]; }

// `k` partes disjuntas cobrindo o contêiner; a de índice alto pode sair
// vazia. O produto é `slice T` (spec §4.8, backend §5.11).
pub inline slice partition(buffer *b, size_t k, size_t w) {
    if (k == 0) return (slice){0, b->ptr};
    size_t passo = b->len / k + (b->len % k ? 1 : 0);
    size_t lo = w * passo;
    if (lo >= b->len) return (slice){0, b->ptr};
    size_t hi = lo + passo;
    if (hi > b->len) hi = b->len;
    return (slice){ hi - lo, b->ptr + lo };
}
