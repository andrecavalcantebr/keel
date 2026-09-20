/* app/app_cfg.c — generated from app/cfg.k by cgen, C11 profile. */
#include "app/app_cfg.h"
#include "keel/keel_arena.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_outcome_i32.h"
#line 1 "app/cfg.k"










keel_outcome_i32 app_cfg_sum(keel_arena *a, const char *path) { keel_outcome_i32 keel__rv0;
    keel_outcome_i32 result = {0};
    FILE *fp = fopen(path, "rb");
    if (!fp) return keel_outcome_i32_fail(&result, 1);
    struct { FILE *fp; } keel__c0 = { fp };

    keel_buffer_i32 xs = keel_buffer_i32_from((i32 *)keel_arena_alloc(a, app_cfg_MAX, sizeof(i32), _Alignof(i32)), app_cfg_MAX);
    if (keel_buffer_i32_capacity(&xs) == 0) { keel__rv0 = keel_outcome_i32_fail(&result, 2); goto keel__e0; }

    i32 v;
    while (fscanf(fp, "%d", &v) == 1)
        if (keel_buffer_i32_push1(&xs, v) == NULL) { keel__rv0 = keel_outcome_i32_fail(&result, 3); goto keel__e0; }

    i32 total = 0;
    { keel_buffer_i32 *keel__c1 = &xs; size_t keel__n1 = keel_buffer_i32_length(keel__c1); for (size_t i = 0; i < keel__n1; i++) { i32 x = keel_buffer_i32_get(keel__c1, i); total += x; } }
    keel__rv0 = keel_outcome_i32_win1(&result, total);
keel__e0: fclose(keel__c0.fp); return keel__rv0; }


keel_outcome_i32 app_cfg_sum_scratch(const char *path) {
    keel_outcome_i32 result = {0};
    keel_arena t = {0};  _Alignas(_Alignof(max_align_t)) unsigned char keel__st0[4096];
    if (!keel_arena_from_array(&t, keel__st0, sizeof keel__st0)) return keel_outcome_i32_fail(&result, 4);
    return app_cfg_sum(&t, path);
}
