/* gen/pilha_stack_i32.type.h — instância de `stack i32`.
   Inclui o `.h` do genérico, e não o contrário: a constante é dele, a struct é
   da instância (backend §4.4.1). */
#ifndef PILHA_STACK_I32_TYPE_H
#define PILHA_STACK_I32_TYPE_H
#include "keel/prelude.h"

typedef struct pilha_stack_i32 { size_t cap, len; i32 *ptr; } pilha_stack_i32;
#endif /* PILHA_STACK_I32_TYPE_H */
