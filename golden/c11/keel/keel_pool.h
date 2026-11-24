/* keel/keel_pool.h — generated from keel/pool.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_POOL_H
#define KEEL_KEEL_POOL_H

#include "keel/keel_pool.type.h"

#line 26 "keel/pool.k"
static inline void keel_pool_bump(u32 *gen, size_t i);

#line 26 "keel/pool.k"
static inline void keel_pool_bump(u32 *gen, size_t i) {
    KEEL_CHECK(gen[i] != UINT32_MAX, "pool-generation-overflow");
    if (gen[i] != UINT32_MAX) gen[i]++;
}

#endif /* KEEL_KEEL_POOL_H */
