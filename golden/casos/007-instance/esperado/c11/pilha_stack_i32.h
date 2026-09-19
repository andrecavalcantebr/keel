/* pilha_stack_i32.h — gerado de pilha.k pelo cgen, perfil C11. */
#ifndef PILHA_STACK_I32_H
#define PILHA_STACK_I32_H
#include "pilha_stack_i32.type.h"

#line 10 "pilha.k"
static inline size_t pilha_stack_i32_length(pilha_stack_i32 *s);
#line 13 "pilha.k"
bool pilha_stack_i32_empurra(pilha_stack_i32 *s, i32 v);

#line 10 "pilha.k"
static inline size_t pilha_stack_i32_length(pilha_stack_i32 *s) { return s->len; }
#endif /* PILHA_STACK_I32_H */