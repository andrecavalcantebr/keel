/* app/app_el.c — gerado de app/el.k pelo cgen, perfil C11. */
#include "app/app_el.h"
#include "keel/keel_outcome_i32.h"
#include "keel/keel_outcome_keel_slice_i32.h"
#include "keel/keel_slice_i32.h"
#line 1 "app/el.k"





static keel_outcome_i32 app_el_read(int n) {
    keel_outcome_i32 r = {0};
    if (n < 0) return keel_outcome_i32_fail(&r, 7);
    if (n == 0) return keel_outcome_i32_none(&r);
    return keel_outcome_i32_win1(&r, n * 2);
}
static keel_outcome_keel_slice_i32 app_el_view(int n, i32 *base) {
    keel_outcome_keel_slice_i32 r = {0};
    if (n <= 0) return keel_outcome_keel_slice_i32_none(&r);
    return keel_outcome_keel_slice_i32_win1(&r, keel_slice_i32_from(base, (size_t)n));
}


int app_el_twice(int n) {
    keel_outcome_i32 v = app_el_read(n); if (keel_outcome_i32_failed(v)) return -1;
    return keel_outcome_i32_value(v);
}


int app_el_twice_log(int n) {
    keel_outcome_i32 v = app_el_read(n); if (keel_outcome_i32_failed(v)) { puts("is_fault"); return -1; }
    return keel_outcome_i32_value(v);
}


size_t app_el_size(int n, i32 *base) {
    keel_outcome_keel_slice_i32 s = app_el_view(n, base); if (keel_outcome_keel_slice_i32_failed(s)) keel_outcome_keel_slice_i32_win1(&s, keel_slice_i32_from(base, 0));
    return keel_slice_i32_length(keel_outcome_keel_slice_i32_value(s));
}
