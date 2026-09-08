/* prova.c — arnês. Afirma o que o §4.7 promete e nada além. */
#include "app/bs.h"
#include <stdio.h>

int main(void) {
    i32 v[64]; size_t o[4] = {0,0,0,0};
    for (size_t i = 0; i < 64; i++) v[i] = (i32)i;
    keel_buffer_i32    b  = keel_buffer_i32_of(v, 64);
    keel_buffer_size_t ob = keel_buffer_size_t_of(o, 4);

    /* alvo na faixa 2 = [32,48). Alguém vence, logo `ok` é falso — quem venceu
       não terminou naturalmente. E `failed` é falso: ninguém executou `fail`. */
    if (!app_bs_achou_alguem(&b, 40, &ob)) return 1;
    if (o[2] != 40) return 2;

    /* alvo ausente: ninguém vence, ninguém falha, todas terminam natural */
    size_t o2[4] = {0,0,0,0};
    keel_buffer_size_t ob2 = keel_buffer_size_t_of(o2, 4);
    if (app_bs_achou_alguem(&b, 999, &ob2)) return 3;
    puts("ok");
    return 0;
}
