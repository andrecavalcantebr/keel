/* keel/keel_parallel.type.h — generated from keel/parallel.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_PARALLEL_TYPE_H
#define KEEL_KEEL_PARALLEL_TYPE_H
#include "keel.type.h"
#include <stdatomic.h>

#line 17 "keel/parallel.k"
typedef struct keel_parallel_control {
    _Atomic u32  wins;
    _Atomic u32  fails;
    _Atomic bool flag;
    u32          workers;
    u32          target;
} keel_parallel_control;
#endif /* KEEL_KEEL_PARALLEL_TYPE_H */
