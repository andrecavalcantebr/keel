/* keel/keel_arena.type.h — gerado de keel/arena.k pelo cgen, perfil C23. */
#ifndef KEEL_KEEL_ARENA_TYPE_H
#define KEEL_KEEL_ARENA_TYPE_H
#include "keel.type.h"

typedef struct keel_arena {
    size_t         cap;
    size_t         top;
    unsigned char *buf;
} keel_arena;
#endif /* KEEL_KEEL_ARENA_TYPE_H */
