/* gen/instancias.c — gerado de instancias.k: os corpos extern da instância.
   É este `.k` do usuário que dá ao build o `.o` e a regra, e é o que mantém a
   invariante de nenhum alvo sem fonte (backend §4.3). */
#include "instancias.h"
#include "pilha_stack_i32.h"

bool pilha_stack_i32_empurra(pilha_stack_i32 *s, i32 v) {
    if (s->len == s->cap) return false;
    s->ptr[s->len++] = v;
    return true;
}
