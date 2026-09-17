/* keel/keel_arena.type.h — perfil C11. O `.h` do módulo keel.arena; C comum.
   Normativo: keel-c-backend.md §5.4 e §5.4.1. */
#ifndef KEEL_KEEL_ARENA_TYPE_H
#define KEEL_KEEL_ARENA_TYPE_H
#include "keel/keel.type.h"

typedef struct keel_arena {
    size_t         cap;
    size_t         top;
    unsigned char *buf;
} keel_arena;
#endif /* KEEL_KEEL_ARENA_TYPE_H */
