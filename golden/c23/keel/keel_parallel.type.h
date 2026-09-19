/* keel/keel_parallel.type.h — gerado de keel/parallel.k pelo cgen, perfil C23. */
#ifndef KEEL_KEEL_PARALLEL_TYPE_H
#define KEEL_KEEL_PARALLEL_TYPE_H
#include "keel.type.h"
#include <stdatomic.h>

typedef struct keel_parallel_control {
    _Atomic u32  wins;
    _Atomic u32  fails;
    _Atomic bool flag;
    u32          workers;
    u32          target;      /* 0 = ALL: a política pede que ninguém falhe */
} keel_parallel_control;
#endif /* KEEL_KEEL_PARALLEL_TYPE_H */
