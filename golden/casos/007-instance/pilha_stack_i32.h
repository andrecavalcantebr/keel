/* gen/pilha/pilha_stack_i32.h — instância de `stack i32`.
   Inclui o `.h` do genérico, e não o contrário: a constante é dele, a struct é
   da instância (backend §4.4.1). */
#ifndef PILHA_STACK_I32_H
#define PILHA_STACK_I32_H
#include "keel/prelude.h"
#include "pilha.h"

typedef struct pilha_stack_i32 { size_t cap, len; i32 *ptr; } pilha_stack_i32;

static inline size_t pilha_stack_i32_length(pilha_stack_i32 *s) { return s->len; }

/* `pub` sem `inline`: aqui só a declaração. O corpo é de quem escreveu
   `instance pilha.stack i32;` — instancias.c. */
bool pilha_stack_i32_empurra(pilha_stack_i32 *s, i32 v);
#endif
