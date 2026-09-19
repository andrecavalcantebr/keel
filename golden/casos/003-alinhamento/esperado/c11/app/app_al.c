/* app/app_al.c — gerado de app/al.k, perfil C11.
   `main` do keel é função comum e sai manglada como qualquer outra (§5.8). */

#include "keel.type.h"
#include "app/app_al.h"
#include "keel/keel_arena.h"
#include <stdio.h>
#include <stdint.h>
#line 10 "app/al.k"
int app_al_main(int argc, char **argv) {
    static u8 app_al_cru[8192];
    for (size_t d = 0; d < 8; d++) {
        keel_arena a;
        if (!keel_arena_from_array(&a, app_al_cru + d, sizeof app_al_cru - 8)) return 1;
        i32         *p = (i32 *)        keel_arena_alloc_n(&a, 3, sizeof(i32),         _Alignof(i32));
        f64         *q = (f64 *)        keel_arena_alloc_n(&a, 2, sizeof(f64),         _Alignof(f64));
        app_al_Vec8 *v = (app_al_Vec8 *)keel_arena_alloc_n(&a, 1, sizeof(app_al_Vec8), _Alignof(app_al_Vec8));
        if (!p || !q || !v) return 2;
        if ((uintptr_t)p % _Alignof(i32) || (uintptr_t)q % _Alignof(f64)
            || (uintptr_t)v % 64) return 3;
    }
    puts("ok");
    return 0;
}
