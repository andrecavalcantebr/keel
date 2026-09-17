/*
 * module keel.slice type T
 *
 * DESC: keel slice — trecho de extensão fixa (spec §5.3, backend §5.2, §5.11, §5.13)
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — ver /LICENSE.md
 */

module keel.slice type T;

import keel.arena   as arena   types;
import keel.outcome as outcome types;

#include <string.h>

// NÃO é `byref` — passa por cópia (spec §4.3: "slice T pode passar por
// valor"). É o que faz `slice.length(slice.of(b))` aninhar sem temporário.
pub modifier slice { size_t len; T *ptr; }

// declaração que não menciona `T` nem `slice`: emitida uma vez no módulo,
// não por instância (backend §5.11) — é `keel_slice_cursor`, não
// `keel_slice_T_cursor`.
pub typedef struct { size_t i; } cursor;

pub inline slice from(T *p, size_t n) {
    return (slice){ p ? n : 0, p };
}

pub inline size_t length(slice s) { return s.len; }
pub inline T      get(slice s, size_t i) { return s.ptr[i]; }
pub inline void   set(slice s, size_t i, T v) { s.ptr[i] = v; }
pub inline T     *ptr(slice s) { return s.ptr; }
pub inline T     *ptr(slice s, size_t i) { return &s.ptr[i]; }

// único acessor com verificação em release (spec §5.3, backend §5.13).
pub inline outcome T at(slice s, size_t i) {
    outcome T r = {0};
    return i < s.len ? outcome.win(&r, s.ptr[i]) : outcome.none(&r);
}

// recorte de um recorte — o `of` da aridade de 2 (spec §4.5: "quem declara o
// `of` é o módulo do contêiner"; para origem `slice`, é este módulo).
pub inline slice of(slice s, size_t a, size_t b) {
    if (a > b || b > s.len) return (slice){0, s.ptr};
    return (slice){ b - a, s.ptr + a };
}

// aloca em `a`, copia o comprimento da origem (spec §5.3).
pub inline outcome slice clone(arena *a, slice s) {
    outcome slice r = {0};
    T *dados = arena.alloc(a, T, s.len);
    if (!dados) return outcome.none(&r);
    if (s.len > 0) memcpy(dados, s.ptr, s.len * sizeof(T));
    return outcome.win(&r, (slice){ s.len, dados });
}

// cursor e partição recebem a cópia, como todo verbo de `slice`.
pub inline cursor begin(slice s) { (void)s; return (cursor){0}; }
pub inline bool   has_next(slice s, cursor *c) { return c->i < s.len; }
pub inline T     *next(slice s, cursor *c) { return &s.ptr[c->i++]; }

// `k` partes disjuntas cobrindo o contêiner; a de índice alto pode sair
// vazia (spec §4.8, backend §5.11).
pub inline slice partition(slice s, size_t k, size_t w) {
    if (k == 0) return (slice){0, s.ptr};
    size_t passo = s.len / k + (s.len % k ? 1 : 0);
    size_t lo = w * passo;
    if (lo >= s.len) return (slice){0, s.ptr};
    size_t hi = lo + passo;
    if (hi > s.len) hi = s.len;
    return (slice){ hi - lo, s.ptr + lo };
}
