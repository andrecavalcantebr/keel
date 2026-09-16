/* keel/keel_parallel.type.h — o símbolo de controle de um bloco `parallel` (backend §5.9).
   O nome escrito no fonte declara um objeto deste tipo no escopo que contém o
   bloco; o gestor escreve nos campos, e o programa lê pelos verbos. */
#ifndef KEEL_KEEL_PARALLEL_TYPE_H
#define KEEL_KEEL_PARALLEL_TYPE_H
#include "keel/prelude.h"
#include <stdatomic.h>

typedef struct keel_parallel_control {
    _Atomic u32  wins;
    _Atomic u32  fails;
    _Atomic bool flag;
    u32          workers;
    u32          target;      /* 0 = ALL: a política pede que ninguém falhe */
} keel_parallel_control;
#endif /* KEEL_KEEL_PARALLEL_TYPE_H */
