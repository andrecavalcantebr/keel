/* prova.c — arnês, e ele é UM arquivo para os DOIS perfis: não pode usar
   grafia que o §9.1 troca. Daí `_Alignof` e não `alignof` — a segunda só
   existe sob C23, e o gerado é que ganha a forma certa por perfil.

   Afirma o §4.4: a alocação alinha o endereço, e o `alignas`
   do vetor de respaldo não é o que torna isso correto. */

#include "keel.type.h"
#include "app/app_pool.h"
#include "keel/keel_arena.h"
#include <stdio.h>
#include <stdint.h>
int main(void) {
    keel_arena a = {0};
    if (!app_pool_inicia(&a)) return 1;
    if (keel_arena_capacity(&a) != 65536) return 2;

    i32 *p = (i32 *)keel_arena_alloc_n(&a, 4, sizeof(i32), _Alignof(i32));
    if (!p || (uintptr_t)p % _Alignof(i32)) return 3;

    /* keel_arena explicitamente zerada: capacidade zero e todo alloc falha limpo */
    keel_arena z = {0};
    if (keel_arena_capacity(&z) != 0) return 4;
    if (keel_arena_alloc_n(&z, 1, 1, 1) != NULL) return 5;
    puts("ok");
    return 0;
}
