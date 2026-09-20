/* proof.c — harness, and it is ONE file for BOTH profiles: it cannot use a
   spelling that §9.1 swaps. Hence `_Alignof` and not `alignof` — the latter
   only exists under C23, and it is the generated code that gets the right
   form per profile.

   Asserts §4.4: the allocation aligns the address, and the backing array's
   `alignas` is not what makes that correct. */

#include "keel.type.h"
#include "app/app_pool.h"
#include "keel/keel_arena.h"
#include <stdio.h>
#include <stdint.h>
int main(void) {
    keel_arena a = {0};
    if (!app_pool_start(&a)) return 1;
    if (keel_arena_capacity(&a) != 65536) return 2;

    i32 *p = (i32 *)keel_arena_alloc(&a, 4, sizeof(i32), _Alignof(i32));
    if (!p || (uintptr_t)p % _Alignof(i32)) return 3;

    /* keel_arena explicitamente zerada: capacidade zero e todo alloc falha limpo */
    keel_arena z = {0};
    if (keel_arena_capacity(&z) != 0) return 4;
    if (keel_arena_alloc(&z, 1, 1, 1) != NULL) return 5;
    puts("ok");
    return 0;
}
