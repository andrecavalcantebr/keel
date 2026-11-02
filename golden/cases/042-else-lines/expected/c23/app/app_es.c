/* app/app_es.c — generated from app/es.k by cgen, C23 profile. */
#include "app/app_es.h"
#include "keel/keel_outcome_i32.h"
#line 1 "app/es.k"



static keel_outcome_i32 app_es_read(int n) {
    keel_outcome_i32 r = {0};
    if (n < 0) return keel_outcome_i32_fail(&r, 7);
    return keel_outcome_i32_win1(&r, n);
}

int app_es_split(int n) {
    keel_outcome_i32 v = app_es_read(n); if (keel_outcome_i32_failed(v))
        return -1;
    return keel_outcome_i32_value(v);
}

int app_es_split_block(int n) {
    keel_outcome_i32 v = app_es_read(n); if (keel_outcome_i32_failed(v))
    {
        return -2;
    }
    return keel_outcome_i32_value(v);
}
