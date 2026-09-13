/* prova.c — arnês. Afirma o que o §4.8 promete e nada além.
   O índice gravado é o da PARTE, não o do contêiner: quem parte recebe um
   recorte, e converter para índice global é conta do programa. */
#include "app/bs.h"
#include <stdio.h>

int main(void) {
    i32 v[64]; size_t o[4] = {0,0,0,0};
    for (size_t i = 0; i < 64; i++) v[i] = (i32)i;
    keel_buffer_i32    b  = keel_buffer_i32_of(v, 64);
    keel_buffer_size_t ob = keel_buffer_size_t_of(o, 4);

    /* alvo 40: parte 2 = [32,48), posição local 8 → grava 9 */
    if (!app_bs_achou_alguem(&b, 40, &ob)) return 1;
    if (o[2] != 9) return 2;
    if (o[0] || o[1] || o[3]) return 3;

    /* alvo ausente: ninguém vence, e sem vitória a política de `ANY` não é
       satisfeita — é o que distingue terminar de achar. */
    size_t o2[4] = {0,0,0,0};
    keel_buffer_size_t ob2 = keel_buffer_size_t_of(o2, 4);
    if (app_bs_achou_alguem(&b, 999, &ob2)) return 4;
    puts("ok");
    return 0;
}
