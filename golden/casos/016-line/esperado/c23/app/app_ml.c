/* app/app_ml.c — gerado de app/ml.k pelo cgen, perfil C23. */
#include "keel.type.h"
#include "app/app_ml.h"
#include "keel/keel_buffer_i32.h"
static void app_ml_solta(app_ml_R *r);

#line 9 "app/ml.k"
int app_ml_usa(app_ml_R *r, keel_buffer_i32 *xs) {
    int keel__rv0;
    f32 *a = keel_buffer_i32_ptr(xs);
    i32 t = 0;
    { keel_buffer_i32 *keel__c0 = xs; size_t keel__n0 = keel_buffer_i32_length(keel__c0); for (size_t i = 0; i < keel__n0; i++) { i32 x = keel_buffer_i32_get(keel__c0, i); t += x; } }
    f32 *b = keel_buffer_i32_ptr(xs);
    (void)a; (void)b;
    keel__rv0 = t; goto keel__e0;
keel__e0: app_ml_solta(r);
    return keel__rv0;
#line 17 "app/ml.k"
}

static void app_ml_solta(app_ml_R *r) {
    f32 *c = (i32 *)0;
    (void)c; r->v = 0;
}
