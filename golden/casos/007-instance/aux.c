/* gen/instancias.c — gerado de instancias.k: os corpos extern da instância. */
#include "pilha_stack_i32.h"

bool pilha_stack_i32_empurra(pilha_stack_i32 *s, i32 v) {
    if (s->len == s->cap) return false;
    s->ptr[s->len++] = v;
    return true;
}
