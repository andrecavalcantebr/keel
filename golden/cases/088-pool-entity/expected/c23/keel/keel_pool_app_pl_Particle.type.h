/* keel/keel_pool_app_pl_Particle.type.h — generated from keel/pool.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_POOL_APP_PL_PARTICLE_TYPE_H
#define KEEL_KEEL_POOL_APP_PL_PARTICLE_TYPE_H

#include "keel.type.h"

#line 21 "keel/pool.k"
typedef struct keel_pool_app_pl_Particle { struct app_pl_Particle *data; u32 *gen, *free; size_t top, cap; keel_arena *owner; size_t stamp; } keel_pool_app_pl_Particle;

#endif /* KEEL_KEEL_POOL_APP_PL_PARTICLE_TYPE_H */
