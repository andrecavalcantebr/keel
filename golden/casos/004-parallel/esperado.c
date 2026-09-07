/* gen/app/val.c — perfil C23.
   O rótulo de término vem DEPOIS da gravação de SUCCESS (backend §5.9, regra 6):
   quem chega ao fim natural grava e cai no rótulo; quem saltou já gravou o
   próprio status. Invertido, `fail` seria apagado. */
#include "keel/prelude.h"
#include "keel/keel_buffer_i32.h"
#include "keel/par.h"
#include <stdio.h>

bool app_val_tem_negativo(keel_buffer_i32 *xs) {
    bool keel__r0;
    {   keel_buffer_i32 *keel__c0 = xs;
        size_t keel__n0 = keel_buffer_i32_length(keel__c0), keel__k0 = 4;
        size_t keel__s0 = (keel__n0 + keel__k0 - 1) / keel__k0;
        keel_parstatus keel__st0[4] = {0};

        #pragma omp parallel for num_threads(4) default(none) \
                shared(keel__c0, keel__n0, keel__s0, keel__st0)
        for (size_t w = 0; w < 4; w++) {
            size_t keel__lo = w * keel__s0;
            size_t keel__hi = (w + 1) * keel__s0 < keel__n0 ? (w + 1) * keel__s0 : keel__n0;
            for (size_t i = keel__lo; i < keel__hi; i++) { i32 x = keel_buffer_i32_get(keel__c0, i);
                if (x < 0) { keel__st0[w] = keel_parstatus_FAILURE; goto keel__fim0; }
            }
            keel__st0[w] = keel_parstatus_SUCCESS;
            keel__fim0: ;
        }
        keel__r0 = keel_par_failed(keel__st0, 4);
    }
    return keel__r0;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    i32 v[16];
    keel_buffer_i32 b = keel_buffer_i32_of(v, sizeof v / sizeof(i32));
    { keel_buffer_i32 *keel__c1 = &b; size_t keel__n1 = keel_buffer_i32_length(keel__c1); for (size_t i = 0; i < keel__n1; i++) { i32 e = keel_buffer_i32_get(keel__c1, i); (void)e;
        keel_buffer_i32_set(&b, i, 1);
    } }
    if (app_val_tem_negativo(&b)) return 1;
    keel_buffer_i32_set(&b, 13, -7);
    if (!app_val_tem_negativo(&b)) return 2;
    puts("ok");
    return 0;
}
