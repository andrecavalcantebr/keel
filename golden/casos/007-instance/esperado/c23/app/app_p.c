/* app/app_p.c — gerado de app/p.k pelo cgen, perfil C23. */
#include "app/app_p.h"
#include "pilha.h"
#include "pilha_stack_i32.h"
#line 5 "app/p.k"
int app_p_main(int argc, char **argv) {
    i32 v[4];
    pilha_stack_i32 s;
    s.cap = 4; s.len = 0; s.ptr = v;

    if (pilha_stack_i32_length(&s) != 0) return 1;
    if (!pilha_stack_i32_empurra(&s, 7)) return 2;              /* corpo vive em instancias.c */
    if (pilha_stack_i32_length(&s) != 1) return 3;
    if (pilha_VAZIA != -1) return 4;           /* nome nu, sem o argumento */
    puts("ok");
    return 0;
}
