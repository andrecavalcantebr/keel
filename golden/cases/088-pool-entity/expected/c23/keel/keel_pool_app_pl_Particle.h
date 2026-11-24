/* keel/keel_pool_app_pl_Particle.h — generated from keel/pool.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_POOL_APP_PL_PARTICLE_H
#define KEEL_KEEL_POOL_APP_PL_PARTICLE_H

#include "keel/keel_pool_app_pl_Particle.type.h"
#include "keel/keel_entity.type.h"
#include "keel/keel_outcome_keel_entity.type.h"
#include "keel/keel_outcome_app_pl_Particle.type.h"

typedef struct keel_arena keel_arena;
#line 34 "keel/pool.k"
static inline bool keel_pool_app_pl_Particle_from_arena(keel_pool_app_pl_Particle *p, keel_arena *a, size_t cap);
#line 46 "keel/pool.k"
static inline keel_outcome_keel_entity keel_pool_app_pl_Particle_alloc(keel_pool_app_pl_Particle *p);
#line 57 "keel/pool.k"
static inline void keel_pool_app_pl_Particle_free(keel_pool_app_pl_Particle *p, keel_entity e);
#line 62 "keel/pool.k"
static inline void keel_pool_app_pl_Particle_reset(keel_pool_app_pl_Particle *p);
#line 70 "keel/pool.k"
[[nodiscard]] static inline struct app_pl_Particle *keel_pool_app_pl_Particle_ptr(keel_pool_app_pl_Particle *p, keel_entity e);
#line 77 "keel/pool.k"
static inline keel_outcome_app_pl_Particle keel_pool_app_pl_Particle_at(keel_pool_app_pl_Particle *p, keel_entity e);
#line 83 "keel/pool.k"
static inline size_t keel_pool_app_pl_Particle_length  (const keel_pool_app_pl_Particle *p);
static inline size_t keel_pool_app_pl_Particle_capacity(const keel_pool_app_pl_Particle *p);

#include "keel/keel_arena.h"
#include "keel/keel_outcome_app_pl_Particle.h"
#include "keel/keel_outcome_keel_entity.h"
#include "keel/keel_pool.h"

#line 34 "keel/pool.k"
static inline bool keel_pool_app_pl_Particle_from_arena(keel_pool_app_pl_Particle *p, keel_arena *a, size_t cap) {
    *p = (keel_pool_app_pl_Particle){0};
    struct app_pl_Particle   *data  = (struct app_pl_Particle *)keel_arena_alloc2(a, sizeof(struct app_pl_Particle), alignof(struct app_pl_Particle),   cap);
    u32 *gen   = (u32 *)keel_arena_alloc2(a, sizeof(u32), alignof(u32), cap);
    u32 *slots = (u32 *)keel_arena_alloc2(a, sizeof(u32), alignof(u32), cap);
    if (!data || !gen || !slots) return false;
    for (size_t i = 0; i < cap; ++i) { gen[i] = 0; slots[i] = (u32)(cap - 1 - i); }
    p->data = data; p->gen = gen; p->free = slots; p->top = cap; p->cap = cap;
    return true;
}
#line 46 "keel/pool.k"
static inline keel_outcome_keel_entity keel_pool_app_pl_Particle_alloc(keel_pool_app_pl_Particle *p) {
    keel_outcome_keel_entity r = {0};
    if (p->top == 0) return keel_outcome_keel_entity_none(&r);
    u32 idx = p->free[--p->top];
    return keel_outcome_keel_entity_win1(&r, (keel_entity){ idx, p->gen[idx] });
}
#line 57 "keel/pool.k"
static inline void keel_pool_app_pl_Particle_free(keel_pool_app_pl_Particle *p, keel_entity e) {
    keel_pool_bump(p->gen, e.index);
    p->free[p->top++] = e.index;
}
#line 62 "keel/pool.k"
static inline void keel_pool_app_pl_Particle_reset(keel_pool_app_pl_Particle *p) {
    for (size_t i = 0; i < p->cap; ++i) { keel_pool_bump(p->gen, i); p->free[i] = (u32)(p->cap - 1 - i); }
    p->top = p->cap;
}
#line 70 "keel/pool.k"
[[nodiscard]] static inline struct app_pl_Particle *keel_pool_app_pl_Particle_ptr(keel_pool_app_pl_Particle *p, keel_entity e) {
    KEEL_CHECK(e.index < p->cap && e.gen == p->gen[e.index], "entity-generation-mismatch");
    return &p->data[e.index];
}
#line 77 "keel/pool.k"
static inline keel_outcome_app_pl_Particle keel_pool_app_pl_Particle_at(keel_pool_app_pl_Particle *p, keel_entity e) {
    keel_outcome_app_pl_Particle r = {0};
    if (e.index < p->cap && e.gen == p->gen[e.index]) return keel_outcome_app_pl_Particle_win1(&r, p->data[e.index]);
    return keel_outcome_app_pl_Particle_none(&r);
}
#line 83 "keel/pool.k"
static inline size_t keel_pool_app_pl_Particle_length  (const keel_pool_app_pl_Particle *p) { return p->cap - p->top; }
static inline size_t keel_pool_app_pl_Particle_capacity(const keel_pool_app_pl_Particle *p) { return p->cap; }

#endif /* KEEL_KEEL_POOL_APP_PL_PARTICLE_H */
