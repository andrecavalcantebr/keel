module app.pl;
import keel.pool    as pool    types;
import keel.entity  as entity  types;
import keel.arena   as arena   types;
import keel.outcome as outcome types;
import_c <stdlib.h>;

typedef struct { i32 x; } Particle;

/* The whole lifecycle of `pool`/`entity` (spec §5.9): fixed capacity, the
   outcome of a full pool, a slot's own generation reused after `free`, `at`
   checked in every build, `reset` invalidating everything at once, and the
   arena a `pool` came from moving on underneath it (rule 9). `which`
   (golden DEBUG) takes the one check only the debug build verifies (backend
   §5.17); release only runs past it. */
pub int main(int argc, char **argv) {
    int which = argc > 1 ? atoi(argv[1]) : 0;

    arena a;
    array u8 storage[4096];
    arena.from_array(a, storage);
    pool struct Particle p;
    if (!pool.from_arena(p, a, 3)) return 1;
    if (pool.capacity(p) != 3 || pool.length(p) != 0) return 2;

    outcome entity r0 = pool.alloc(p);
    outcome entity r1 = pool.alloc(p);
    outcome entity r2 = pool.alloc(p);
    if (outcome.failed(r0) || outcome.failed(r1) || outcome.failed(r2)) return 3;
    entity e0 = outcome.value(r0);
    entity e1 = outcome.value(r1);
    entity e2 = outcome.value(r2);
    if (pool.length(p) != 3) return 4;

    outcome entity full = pool.alloc(p);
    if (!outcome.failed(full)) return 5;      // capacity is fixed: no realloc (spec §5.9, rule 1)

    struct Particle *ref particle0 = pool.ptr(p, e0);
    particle0->x = 10;
    struct Particle *ref particle1 = pool.ptr(p, e1);
    particle1->x = 20;
    struct Particle *ref particle2 = pool.ptr(p, e2);
    particle2->x = 30;
    if (particle0->x != 10 || particle1->x != 20 || particle2->x != 30) return 6;

    entity stale0 = e0;
    pool.free(p, e0);
    if (pool.length(p) != 2) return 7;
    outcome struct Particle hit = pool.at(p, stale0);
    if (!outcome.failed(hit)) return 8;       // at() checks the generation in every build (rule 4)

    if (which == 1) { (void)pool.ptr(p, stale0); return 0; }   // DEBUG: entity-generation-mismatch

    outcome entity r3 = pool.alloc(p);
    if (outcome.failed(r3)) return 9;
    entity e3 = outcome.value(r3);
    if (e3.index != stale0.index || e3.gen != stale0.gen + 1) return 10;   // the freed slot, one generation later

    pool.reset(p);
    if (pool.length(p) != 0 || pool.capacity(p) != 3) return 11;
    outcome struct Particle after_reset = pool.at(p, e1);
    if (!outcome.failed(after_reset)) return 12;   // reset invalidates every entity distributed before it (rule 5)

    outcome entity r4 = pool.alloc(p);
    if (outcome.failed(r4)) return 13;

    /* a copy of `p`, taken before the arena moves on, is not followed by the
       lexical analysis (spec §4.12, "Casos especiais") — the only way left
       to reach the runtime side of rule 9, same idea as `stale0` above. */
    entity e5 = outcome.value(pool.alloc(p));
    pool struct Particle stale_p = p;
    arena.from_array(a, storage);   // same memory, new generation (spec §5.2, rule 12)

    outcome struct Particle after_region = pool.at(stale_p, e5);
    if (!outcome.failed(after_region)) return 14;   // at() is safe in every build (rule 9)

    if (which == 2) { outcome entity r5 = pool.alloc(stale_p); (void)r5; return 0; }   // DEBUG: pool-region-invalidated

    return 0;
}
