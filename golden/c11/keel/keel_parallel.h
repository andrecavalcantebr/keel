/* keel/keel_parallel.impl.h — o símbolo de controle de um bloco `parallel` (backend §5.9).
   O nome escrito no fonte declara um objeto deste tipo no escopo que contém o
   bloco; o gestor escreve nos campos, e o programa lê pelos verbos. */
#ifndef KEEL_KEEL_PARALLEL_IMPL_H
#define KEEL_KEEL_PARALLEL_IMPL_H
#include "keel/keel_parallel.h"

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
#endif /* KEEL_KEEL_PARALLEL_IMPL_H */
