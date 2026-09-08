/* gen/app/bs.c — gerado de app/bs.k, perfil C23.
   §5.9: sob política diferente de ALL entram as duas variáveis atômicas,
   locais do gestor e nunca `static`. `relaxed` basta em todas: a bandeira é
   dica, e quem sincroniza o que o worker escreveu é a barreira implícita no
   fim do `omp parallel for`. */
#include "app/bs.h"
#include "keel/par.h"
#include <stdatomic.h>

#line 7 "app/bs.k"
bool app_bs_achou_alguem(keel_buffer_i32 *xs, i32 alvo, keel_buffer_size_t *onde) {
    bool keel__r0;
    {   keel_buffer_i32 *keel__c0 = xs;
        size_t keel__n0 = keel_buffer_i32_length(keel__c0), keel__k0 = 4;
        size_t keel__s0 = (keel__n0 + keel__k0 - 1) / keel__k0;
        keel_parstatus keel__st0[4] = {0};
        _Atomic size_t keel__vit0 = 0;
        _Atomic bool   keel__intr0 = false;

        #pragma omp parallel for num_threads(4) default(none) \
                shared(keel__c0, keel__n0, keel__s0, keel__st0, keel__vit0, keel__intr0, onde) \
                firstprivate(alvo)
        for (size_t w = 0; w < 4; w++) {
            size_t keel__lo = w * keel__s0;
            size_t keel__hi = (w + 1) * keel__s0 < keel__n0 ? (w + 1) * keel__s0 : keel__n0;
            for (size_t i = keel__lo; i < keel__hi; i++) { i32 x = keel_buffer_i32_get(keel__c0, i);
                if (atomic_load_explicit(&keel__intr0, memory_order_relaxed)) {
                    keel__st0[w] = keel_parstatus_INTERRUPTED; goto keel__fim0;
                }
                if (x == alvo) { keel_buffer_size_t_set(onde, w, i);
                    if (atomic_fetch_add_explicit(&keel__vit0, 1, memory_order_relaxed) + 1 >= 1)
                        atomic_store_explicit(&keel__intr0, true, memory_order_relaxed);
                    keel__st0[w] = keel_parstatus_SUCCESS; goto keel__fim0;
                }
            }
            keel__st0[w] = keel_parstatus_SUCCESS;
            keel__fim0: ;
        }
        keel__r0 = keel_par_failed(keel__st0, 4) == false && keel_par_ok(keel__st0, 4) == false;
    }
    return keel__r0;
}
