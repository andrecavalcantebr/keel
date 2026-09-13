/* keel/parallel.h — o símbolo de controle de um bloco `parallel` (backend §5.9).
   O nome escrito no fonte declara um objeto deste tipo no escopo que contém o
   bloco; o gestor escreve nos campos, e o programa lê pelos verbos. */
#ifndef KEEL_PARALLEL_H
#define KEEL_PARALLEL_H
#include "keel/prelude.h"
#include <stdatomic.h>

typedef struct keel_parallel_control {
    _Atomic u32  wins;
    _Atomic u32  fails;
    _Atomic bool flag;
    u32          workers;
    u32          target;      /* 0 = ALL: a política pede que ninguém falhe */
} keel_parallel_control;

static inline bool keel_parallel_interrupted(keel_parallel_control *c) {
    return atomic_load_explicit(&c->flag, memory_order_relaxed);
}
static inline u32 keel_parallel_wins(keel_parallel_control *c) {
    return atomic_load_explicit(&c->wins, memory_order_relaxed);
}
static inline bool keel_parallel_failed(keel_parallel_control *c) {
    return atomic_load_explicit(&c->fails, memory_order_relaxed) > 0;
}
static inline bool keel_parallel_ok(keel_parallel_control *c) {
    return c->target ? keel_parallel_wins(c) >= c->target
                     : atomic_load_explicit(&c->fails, memory_order_relaxed) == 0;
}
#endif
