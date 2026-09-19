/* app/app_p.c — gerado de app/p.k pelo cgen, perfil C11. */
#include "app/app_p.h"
#include "coll.h"
#include "coll_stack_i32.h"
#line 1 "app/p.k"




int app_p_main(int argc, char **argv) {
    i32 v[4];
    coll_stack_i32 s;
    s.cap = 4; s.len = 0; s.ptr = v;

    if (coll_stack_i32_length(&s) != 0) return 1;
    if (!coll_stack_i32_push(&s, 7)) return 2;
    if (coll_stack_i32_length(&s) != 1) return 3;
    if (coll_EMPTY != -1) return 4;
    puts("ok");
    return 0;
}
