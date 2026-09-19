/* app/app_val.c — gerado de app/val.k pelo cgen, perfil C23. */
#include "app/app_val.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_slice_i32.h"
#include "keel/keel_parallel.h"
#line 9 "app/val.k"
bool app_val_tem_negativo(keel_buffer_i32 *xs) {
    keel_parallel_control valida = { .workers = 4, .target = 0 };
    {   keel_buffer_i32 *keel__c0 = xs;

        #pragma omp parallel for num_threads(4) default(none) \
                shared(keel__c0, valida)
        for (size_t w = 0; w < 4; w++) {
            keel_slice_i32 parte = keel_buffer_i32_partition(keel__c0, 4, w);
            {   keel_slice_i32 keel__c1 = parte;
                size_t keel__n1 = keel_slice_i32_length(keel__c1);
                for (size_t i = 0; i < keel__n1; i++) { i32 x = keel_slice_i32_get(keel__c1, i);
                    if (x < 0) {
                        atomic_fetch_add_explicit(&valida.fails, 1, memory_order_relaxed);
                        goto keel__end0;
                    }
                }
            }
            keel__end0: ;
        }
    }
#line 15 "app/val.k"
    return keel_parallel_failed(&valida);
}
