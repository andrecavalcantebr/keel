/* app/app_pl.c — generated from app/pl.k by cgen, C23 profile. */
#include "app/app_pl.h"
#include "keel/keel_arena.h"
#include "keel/keel_entity.h"
#include "keel/keel_outcome_app_pl_Particle.h"
#include "keel/keel_outcome_keel_entity.h"
#include "keel/keel_pool_app_pl_Particle.h"
#line 1 "app/pl.k"















int app_pl_main(int argc, char **argv) {
    int which = argc > 1 ? atoi(argv[1]) : 0;

    keel_arena a = {0};
    u8 storage[4096];
    keel_arena_from_array(&a, storage, 4096);
    keel_pool_app_pl_Particle p;
    if (!keel_pool_app_pl_Particle_from_arena(&p, &a, 3)) return 1;
    if (keel_pool_app_pl_Particle_capacity(&p) != 3 || keel_pool_app_pl_Particle_length(&p) != 0) return 2;

    keel_outcome_keel_entity r0 = keel_pool_app_pl_Particle_alloc(&p);
    keel_outcome_keel_entity r1 = keel_pool_app_pl_Particle_alloc(&p);
    keel_outcome_keel_entity r2 = keel_pool_app_pl_Particle_alloc(&p);
    if (keel_outcome_keel_entity_failed(r0) || keel_outcome_keel_entity_failed(r1) || keel_outcome_keel_entity_failed(r2)) return 3;
    keel_entity e0 = keel_outcome_keel_entity_value(r0);
    keel_entity e1 = keel_outcome_keel_entity_value(r1);
    keel_entity e2 = keel_outcome_keel_entity_value(r2);
    if (keel_pool_app_pl_Particle_length(&p) != 3) return 4;

    keel_outcome_keel_entity full = keel_pool_app_pl_Particle_alloc(&p);
    if (!keel_outcome_keel_entity_failed(full)) return 5;

    struct app_pl_Particle *particle0 = keel_pool_app_pl_Particle_ptr(&p, e0);
    particle0->x = 10;
    struct app_pl_Particle *particle1 = keel_pool_app_pl_Particle_ptr(&p, e1);
    particle1->x = 20;
    struct app_pl_Particle *particle2 = keel_pool_app_pl_Particle_ptr(&p, e2);
    particle2->x = 30;
    if (particle0->x != 10 || particle1->x != 20 || particle2->x != 30) return 6;

    keel_entity stale0 = e0;
    keel_pool_app_pl_Particle_free(&p, e0);
    if (keel_pool_app_pl_Particle_length(&p) != 2) return 7;
    keel_outcome_app_pl_Particle hit = keel_pool_app_pl_Particle_at(&p, stale0);
    if (!keel_outcome_app_pl_Particle_failed(hit)) return 8;

    if (which == 1) { (void)keel_pool_app_pl_Particle_ptr(&p, stale0); return 0; }

    keel_outcome_keel_entity r3 = keel_pool_app_pl_Particle_alloc(&p);
    if (keel_outcome_keel_entity_failed(r3)) return 9;
    keel_entity e3 = keel_outcome_keel_entity_value(r3);
    if (e3.index != stale0.index || e3.gen != stale0.gen + 1) return 10;

    keel_pool_app_pl_Particle_reset(&p);
    if (keel_pool_app_pl_Particle_length(&p) != 0 || keel_pool_app_pl_Particle_capacity(&p) != 3) return 11;
    keel_outcome_app_pl_Particle after_reset = keel_pool_app_pl_Particle_at(&p, e1);
    if (!keel_outcome_app_pl_Particle_failed(after_reset)) return 12;

    keel_outcome_keel_entity r4 = keel_pool_app_pl_Particle_alloc(&p);
    if (keel_outcome_keel_entity_failed(r4)) return 13;




    keel_entity e5 = keel_outcome_keel_entity_value(keel_pool_app_pl_Particle_alloc(&p));
    keel_pool_app_pl_Particle stale_p = p;
    keel_arena_from_array(&a, storage, 4096);

    keel_outcome_app_pl_Particle after_region = keel_pool_app_pl_Particle_at(&stale_p, e5);
    if (!keel_outcome_app_pl_Particle_failed(after_region)) return 14;

    if (which == 2) { keel_outcome_keel_entity r5 = keel_pool_app_pl_Particle_alloc(&stale_p); (void)r5; return 0; }

    return 0;
}
