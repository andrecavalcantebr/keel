/* gen/app/p.c — gerado de app/p.k, perfil C11. */

#include "keel/prelude.h"
#include "app/app_p.impl.h"
#include "pilha.impl.h"
#include "pilha_stack_i32.impl.h"
#include <stdio.h>
#line 5 "app/p.k"
int app_p_main(int argc, char **argv) {
    (void)argc; (void)argv;
    i32 app_p_v[4];
    pilha_stack_i32 s = {0};
    s.cap = 4; s.len = 0; s.ptr = app_p_v;

    if (pilha_stack_i32_length(&s) != 0) return 1;
    if (!pilha_stack_i32_empurra(&s, 7)) return 2;   /* corpo vive em instancias.c */
    if (pilha_stack_i32_length(&s) != 1) return 3;
    if (pilha_VAZIA != -1) return 4;                 /* nome nu, sem o argumento */
    puts("ok");
    return 0;
}
