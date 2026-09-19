/* app/app_cfg.c — gerado de app/cfg.k pelo cgen, perfil C23. */
#include "keel.type.h"
#include "app/app_cfg.h"
#include "keel/keel_arena.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_outcome_i32.h"
#include <stdio.h>
#line 11 "app/cfg.k"
keel_outcome_i32 app_cfg_soma(keel_arena *a, const char *caminho) {
    keel_outcome_i32 resultado = {0};
    keel_outcome_i32 keel__rv0;
    FILE *fp = fopen(caminho, "rb");
    if (!fp) return keel_outcome_i32_fail(&resultado, 1);
    struct { FILE *fp; } keel__c0 = { fp };

    keel_buffer_i32 xs = keel_buffer_i32_from(
        (i32 *)keel_arena_alloc_n(a, app_cfg_MAX, sizeof(i32), alignof(i32)), app_cfg_MAX);
    if (keel_buffer_i32_capacity(&xs) == 0) { keel__rv0 = keel_outcome_i32_fail(&resultado, 2); goto keel__e0; }

    i32 v;
    while (fscanf(fp, "%d", &v) == 1)
        if (keel_buffer_i32_push1(&xs, v) == NULL) { keel__rv0 = keel_outcome_i32_fail(&resultado, 3); goto keel__e0; }

    i32 total = 0;
    { keel_buffer_i32 *keel__c1 = &xs; size_t keel__n1 = keel_buffer_i32_length(keel__c1); for (size_t i = 0; i < keel__n1; i++) { i32 x = keel_buffer_i32_get(keel__c1, i);
        total += x;
    } }
    keel__rv0 = keel_outcome_i32_win1(&resultado, total);
keel__e0: fclose(keel__c0.fp);
    return keel__rv0;
}

#line 30 "app/cfg.k"
keel_outcome_i32 app_cfg_soma_scratch(const char *caminho) {
    keel_outcome_i32 resultado = {0};
    keel_arena t;  alignas(alignof(max_align_t)) unsigned char keel__st0[4096];
    if (!keel_arena_from_array(&t, keel__st0, sizeof keel__st0))
        return keel_outcome_i32_fail(&resultado, 4);
    return app_cfg_soma(&t, caminho);
}
