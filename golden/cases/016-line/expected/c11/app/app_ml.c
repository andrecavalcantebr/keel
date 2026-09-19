/* app/app_ml.c — gerado de app/ml.k pelo cgen, perfil C11. */
#include "app/app_ml.h"
#include "keel/keel_buffer_i32.h"
#line 1 "app/ml.k"






static void app_ml_release(app_ml_R *r);

int app_ml_use(app_ml_R *r, keel_buffer_i32 *xs) {
    int keel__rv0;
    f32 *a = keel_buffer_i32_ptr(xs);
    i32 t = 0;
    { keel_buffer_i32 *keel__c0 = xs; size_t keel__n0 = keel_buffer_i32_length(keel__c0); for (size_t i = 0; i < keel__n0; i++) { i32 x = keel_buffer_i32_get(keel__c0, i); t += x; } }
    f32 *b = keel_buffer_i32_ptr(xs);
    (void)a; (void)b;
    keel__rv0 = t; goto keel__e0;
keel__e0: app_ml_release(r); return keel__rv0; }

static void app_ml_release(app_ml_R *r) {
    f32 *c = (i32 *)0;
    (void)c; r->v = 0;
}
