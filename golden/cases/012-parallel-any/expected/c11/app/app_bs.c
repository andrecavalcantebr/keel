/* app/app_bs.c — gerado de app/bs.k pelo cgen, perfil C11. */
#include "app/app_bs.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_buffer_size_t.h"
#include "keel/keel_slice_i32.h"
#include "keel/keel_parallel.h"
#line 1 "app/bs.k"








bool app_bs_found_any(keel_buffer_i32 *xs, i32 target, keel_buffer_size_t *where) {
    keel_parallel_control search = { .workers = 4, .target = 1 };
    {   keel_buffer_i32 *keel__c0 = xs;

        #pragma omp parallel for num_threads(4) default(none) \
                shared(keel__c0, search, where) firstprivate(target)
        for (size_t w = 0; w < 4; w++) {
            keel_slice_i32 part = keel_buffer_i32_partition(keel__c0, 4, w);
            {   keel_slice_i32 keel__c1 = part;
                size_t keel__n1 = keel_slice_i32_length(keel__c1);
                for (size_t i = 0; i < keel__n1; i++) { i32 x = keel_slice_i32_get(keel__c1, i);
                    if (atomic_load_explicit(&search.flag, memory_order_relaxed)) break;
                    if (x == target) {
                        keel_buffer_size_t_set(where, w, i + 1);
                        if (atomic_fetch_add_explicit(&search.wins, 1, memory_order_relaxed) + 1 >= 1)
                            atomic_store_explicit(&search.flag, true, memory_order_relaxed);
                        goto keel__end0;
                    }
                }
            }
            keel__end0: ;
        }
    }
#line 16 "app/bs.k"
    return keel_parallel_ok(&search);
}
