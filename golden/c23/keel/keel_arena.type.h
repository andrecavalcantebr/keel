/* keel/keel_arena.type.h — generated from keel/arena.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_ARENA_TYPE_H
#define KEEL_KEEL_ARENA_TYPE_H
#include "keel.type.h"

typedef struct keel_arena {
    size_t         cap;
    size_t         top;
    unsigned char *buf;
} keel_arena;
#endif /* KEEL_KEEL_ARENA_TYPE_H */
