/* gen/pilha_stack_i32.h — instância de `stack i32`.
   O `.h` do genérico chega a quem usa a constante dele — aqui ninguém usa, e
   o include segue a necessidade (backend §4.3.2, regra 2). */
#ifndef PILHA_STACK_I32_H
#define PILHA_STACK_I32_H
#include "pilha_stack_i32.type.h"

/* menciona `stack`, embora não escreva `T` → pertence à instância (§4.9) */
static inline size_t pilha_stack_i32_length(pilha_stack_i32 *s);
/* `pub` sem `inline`: aqui só a declaração. O corpo é de quem escreveu
   `instance pilha.stack i32;` — gen/instancias.c. */
bool pilha_stack_i32_empurra(pilha_stack_i32 *s, i32 v);
#endif /* PILHA_STACK_I32_H */
