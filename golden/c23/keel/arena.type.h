/* keel/arena.type.h — perfil C23. O `.h` do módulo keel.arena; C comum.
   Normativo: keel-c-backend.md §5.4 e §5.4.1. */
#ifndef KEEL_ARENA_TYPE_H
#define KEEL_ARENA_TYPE_H
#include "keel/prelude.h"

typedef struct arena {
    size_t         cap;
    size_t         top;
    unsigned char *buf;
} arena;
#endif /* KEEL_ARENA_TYPE_H */
