/* app/app_ed.c — generated from app/ed.k by cgen, C23 profile. */
#include "app/app_ed.h"
#include "keel/keel_outcome_keel_slice_i32.h"
#include "keel/keel_slice_i32.h"
#line 1 "app/ed.k"




static keel_outcome_keel_slice_i32 app_ed_view(int n, i32 *base) {
    keel_outcome_keel_slice_i32 r = {0};
    if (n <= 0) return keel_outcome_keel_slice_i32_none(&r);
    return keel_outcome_keel_slice_i32_win1(&r, keel_slice_i32_from(base, (size_t)n));
}

size_t app_ed_size(int n, i32 *base) {
    keel_outcome_keel_slice_i32 s = app_ed_view(n, base); if (keel_outcome_keel_slice_i32_failed(s))
        keel_outcome_keel_slice_i32_win1(&s, keel_slice_i32_from(base, 0));
    return keel_slice_i32_length(keel_outcome_keel_slice_i32_value(s));
}
