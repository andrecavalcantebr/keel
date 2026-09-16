/* gen/app/ap.c — gerado de app/ap.k, perfil C23.
   `apply` é o mesmo laço do `foreach` com corpo fixo, e os índices saem na
   família `keel__i<N>` porque não há binder escrito (backend §5.7, regra 2). */

#include "keel/prelude.h"
#include "app/app_ap.impl.h"
#include "keel/keel_buffer_i32.impl.h"
#include "keel/keel_range.impl.h"
#line 5 "app/ap.k"
static i32 app_ap_acc = 0;
static void app_ap_dobra (i32 v, size_t i)        { app_ap_acc += v * 2 + (i32)i; }
static void app_ap_escala(i32 v, size_t i, i32 k) { app_ap_acc += v * k + (i32)i; }
i32 app_ap_soma_acc(void) { return app_ap_acc; }

#line 12 "app/ap.k"
void app_ap_percorre(keel_buffer_i32 *xs) {
    { keel_buffer_i32 *keel__c0 = xs; size_t keel__n0 = keel_buffer_i32_length(keel__c0); for (size_t keel__i0 = 0; keel__i0 < keel__n0; keel__i0++) { i32 keel__v0 = keel_buffer_i32_get(keel__c0, keel__i0);
        app_ap_dobra(keel__v0, keel__i0);
    } }
    { keel_buffer_i32 *keel__c1 = xs; size_t keel__n1 = keel_buffer_i32_length(keel__c1); for (size_t keel__i1 = 0; keel__i1 < keel__n1; keel__i1++) { i32 keel__v1 = keel_buffer_i32_get(keel__c1, keel__i1);
        app_ap_escala(keel__v1, keel__i1, 3);
    } }
}

#line 19 "app/ap.k"
size_t app_ap_soma_range(keel_range r) {
    size_t t = 0;
    { keel_range keel__c2 = r; size_t keel__n2 = keel_range_length(keel__c2); for (size_t keel__i2 = 0; keel__i2 < keel__n2; keel__i2++) { size_t v = keel_range_get(keel__c2, keel__i2);
        t += v;
    } }
    { keel_range keel__c3 = r; size_t keel__n3 = keel_range_length(keel__c3); for (size_t i = 0; i < keel__n3; i++) { size_t v = keel_range_get(keel__c3, i);
        t += v * i;
    } }
    return t;
}
