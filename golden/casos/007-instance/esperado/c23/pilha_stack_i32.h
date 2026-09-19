/* pilha_stack_i32.h — gerado de pilha.k pelo cgen, perfil C23. */
#ifndef PILHA_STACK_I32_H
#define PILHA_STACK_I32_H
#include "pilha_stack_i32.type.h"

/* menciona `stack`, embora não escreva `T` → pertence à instância (§4.9) */
static inline size_t pilha_stack_i32_length(pilha_stack_i32 *s);
/* `pub` sem `inline`: aqui só a declaração. O corpo é de quem escreveu
   `instance pilha.stack i32;` — gen/instancias.c. */
bool pilha_stack_i32_empurra(pilha_stack_i32 *s, i32 v);

/* menciona `stack`, embora não escreva `T` → pertence à instância (§4.9) */
static inline size_t pilha_stack_i32_length(pilha_stack_i32 *s) { return s->len; }
#endif /* PILHA_STACK_I32_H */
