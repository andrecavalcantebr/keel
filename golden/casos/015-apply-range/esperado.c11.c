/* gen/app/ap.c — perfil C11.
   `apply` é o mesmo laço do `foreach` com corpo fixo, e os índices geram a
   família `keel__i<N>` porque não há binder escrito (backend §5.7, regra 2). */
#include "keel/prelude.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_range.h"
#include <stdio.h>

static void app_ap_dobra(i32 v, size_t i);
static void app_ap_escala(i32 v, size_t i, i32 k);

void app_ap_percorre(keel_buffer_i32 *xs) {
    { keel_buffer_i32 *keel__c0 = xs; size_t keel__n0 = keel_buffer_i32_length(keel__c0); for (size_t keel__i0 = 0; keel__i0 < keel__n0; keel__i0++) { i32 keel__v0 = keel_buffer_i32_get(keel__c0, keel__i0);
        app_ap_dobra(keel__v0, keel__i0);
    } }
    { keel_buffer_i32 *keel__c1 = xs; size_t keel__n1 = keel_buffer_i32_length(keel__c1); for (size_t keel__i1 = 0; keel__i1 < keel__n1; keel__i1++) { i32 keel__v1 = keel_buffer_i32_get(keel__c1, keel__i1);
        app_ap_escala(keel__v1, keel__i1, 3);
    } }
}

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

/* ---- prova ---- */
static i32 acc = 0;
static void app_ap_dobra (i32 v, size_t i) { acc += v * 2 + (i32)i; }
static void app_ap_escala(i32 v, size_t i, i32 k) { acc += v * k + (i32)i; }

int main(void) {
    i32 v[4] = {1,2,3,4};
    keel_buffer_i32 b = keel_buffer_i32_of(v, 4);
    app_ap_percorre(&b);
    /* dobra: (2+0)+(4+1)+(6+2)+(8+3) = 26 ; escala: (3+0)+(6+1)+(9+2)+(12+3) = 36 */
    if (acc != 26 + 36) return 1;

    keel_range r = keel_range_of(2, 6);          /* [2,6) → 2,3,4,5 */
    /* soma = 14 ; soma ponderada = 2*0+3*1+4*2+5*3 = 26 */
    if (app_ap_soma_range(r) != 14 + 26) return 2;
    puts("ok");
    return 0;
}
