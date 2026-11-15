/* proof.c — harness. `start` builds an arena over `memo[1<<10]`: the dimension is an expression and
   reaches `from_array` as its length argument, so the arena has 1024 bytes and can allocate. */
#include "keel.type.h"
#include "adim.h"
#include "keel/keel_arena.h"

int main(void) {
    keel_arena a = {0};
    if (!adim_start(&a)) return 1;
    return keel_arena_alloc3(&a, 512, sizeof(u8), 1) ? 0 : 2;
}
