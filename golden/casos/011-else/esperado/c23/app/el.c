/* gen/app/el.c — gerado de app/el.k, perfil C23.
   §5.12: a declaração sai como estava, seguida de um `if` cujo teste é o
   `failed` da instância — nunca `if (!x)`. Tudo numa linha só (regra 2 do §6). */
#include "app/el.h"
#include <stdio.h>

#line 6 "app/el.k"
static keel_outcome_i32 app_el_le(int n) {
    keel_outcome_i32 r = {0};
    if (n < 0) return keel_outcome_i32_fail(&r, 7);
    if (n == 0) return keel_outcome_i32_none(&r);
    return keel_outcome_i32_win1(&r, n * 2);
}
static keel_outcome_keel_slice_i32 app_el_vista(int n, i32 *base) {
    keel_outcome_keel_slice_i32 r = {0};
    if (n <= 0) return keel_outcome_keel_slice_i32_none(&r);
    return keel_outcome_keel_slice_i32_win1(&r, keel_slice_i32_from(base, (size_t)n));
}

#line 19 "app/el.k"
int app_el_dobro(int n) {
    keel_outcome_i32 v = app_el_le(n); if (keel_outcome_i32_failed(v)) return -1;
    return keel_outcome_i32_value(v);
}

#line 25 "app/el.k"
int app_el_dobro_log(int n) {
    keel_outcome_i32 v = app_el_le(n); if (keel_outcome_i32_failed(v)) { puts("falhou"); return -1; }
    return keel_outcome_i32_value(v);
}

#line 31 "app/el.k"
size_t app_el_tamanho(int n, i32 *base) {
    keel_outcome_keel_slice_i32 s = app_el_vista(n, base); if (keel_outcome_keel_slice_i32_failed(s)) keel_outcome_keel_slice_i32_win1(&s, keel_slice_i32_from(base, 0));
    return keel_slice_i32_length(keel_outcome_keel_slice_i32_value(s));
}
