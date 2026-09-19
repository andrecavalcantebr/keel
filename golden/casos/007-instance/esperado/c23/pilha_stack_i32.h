/* pilha_stack_i32.h — gerado de pilha.k pelo cgen, perfil C23. */
#ifndef PILHA_STACK_I32_H
#define PILHA_STACK_I32_H
#include "pilha_stack_i32.proto.h"

/* menciona `stack`, embora não escreva `T` → pertence à instância (§4.9) */
static inline size_t pilha_stack_i32_length(pilha_stack_i32 *s) { return s->len; }
#endif /* PILHA_STACK_I32_H */
