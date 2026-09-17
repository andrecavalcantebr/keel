/* gen/app/bs.c — gerado de app/bs.k, perfil C23.
   Sob política diferente de `ALL` a vitória arma a bandeira, e o alvo sai como
   literal (backend §5.9). `interrupted` é consulta: devolve `bool` onde foi
   escrita, não salta e não grava status. */

#include "keel/keel.type.h"
#include "app/app_bs.impl.h"
#include "keel/keel_buffer_i32.impl.h"
#include "keel/keel_buffer_size_t.impl.h"
#include "keel/keel_slice_i32.impl.h"
#include "keel/keel_parallel.impl.h"
#line 9 "app/bs.k"
bool app_bs_achou_alguem(keel_buffer_i32 *xs, i32 alvo, keel_buffer_size_t *onde) {
    keel_parallel_control busca = { .workers = 4, .target = 1 };
    {   keel_buffer_i32 *keel__c0 = xs;

        #pragma omp parallel for num_threads(4) default(none) \
                shared(keel__c0, busca, onde) firstprivate(alvo)
        for (size_t w = 0; w < 4; w++) {
            keel_slice_i32 parte = keel_buffer_i32_partition(keel__c0, 4, w);
            {   keel_slice_i32 keel__c1 = parte;
                size_t keel__n1 = keel_slice_i32_length(keel__c1);
                for (size_t i = 0; i < keel__n1; i++) { i32 x = keel_slice_i32_get(keel__c1, i);
                    if (atomic_load_explicit(&busca.flag, memory_order_relaxed)) break;
                    if (x == alvo) {
                        keel_buffer_size_t_set(onde, w, i + 1);
                        if (atomic_fetch_add_explicit(&busca.wins, 1, memory_order_relaxed) + 1 >= 1)
                            atomic_store_explicit(&busca.flag, true, memory_order_relaxed);
                        goto keel__end0;
                    }
                }
            }
            keel__end0: ;            /* fim natural: nada a contabilizar */
        }
    }
    return keel_parallel_ok(&busca);
}
