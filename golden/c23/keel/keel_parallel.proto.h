/* keel/keel_parallel.h — o símbolo de controle de um bloco `parallel` (backend §5.9).
   O nome escrito no fonte declara um objeto deste tipo no escopo que contém o
   bloco; o gestor escreve nos campos, e o programa lê pelos verbos. */
#ifndef KEEL_KEEL_PARALLEL_H
#define KEEL_KEEL_PARALLEL_H
#include "keel/keel_parallel.type.h"

static inline bool keel_parallel_interrupted(keel_parallel_control *c);
static inline u32 keel_parallel_wins(keel_parallel_control *c);
static inline bool keel_parallel_failed(keel_parallel_control *c);
static inline bool keel_parallel_ok(keel_parallel_control *c);
#endif /* KEEL_KEEL_PARALLEL_H */
