/* gen/app/ac.c — perfil C11. §5.3: o açúcar baixa por função `static inline`,
   nunca por macro, e o par `&*` colapsa na geração. */
#include "keel/prelude.h"
#include "keel/keel_buffer_i32.h"
#include <stdio.h>

i32 app_ac_troca(keel_buffer_i32 *b, size_t i) {
    *keel_buffer_i32_ptr1(b, i) = *keel_buffer_i32_ptr1(b, i) + 1;
    return *keel_buffer_i32_ptr1(b, i);
}

i32 app_ac_uma_vez(keel_buffer_i32 *b, size_t *i) {
    *keel_buffer_i32_ptr1(b, (*i)++) = 9;
    return (i32)*i;
}

i32 app_ac_cubo(void) {
    i32 v[2][3][4];
    v[1][2][3] = 7;
    return v[1][2][3];
}

size_t app_ac_recorta(keel_buffer_i32 *b) {
    keel_slice_i32 meio = keel_buffer_i32_as_slice2(b, 2, 7);
    keel_slice_i32 fim  = keel_buffer_i32_as_slice2(b, 5, keel_buffer_i32_length(b));
    keel_slice_i32 ini  = keel_buffer_i32_as_slice2(b, 0, 3);
    keel_slice_i32 todo = keel_buffer_i32_as_slice(b);
    return keel_slice_i32_length(&meio) + keel_slice_i32_length(&fim)
         + keel_slice_i32_length(&ini)  + keel_slice_i32_length(&todo);
}

int main(void) {
    i32 v[10];
    for (size_t k = 0; k < 10; k++) v[k] = (i32)k;
    keel_buffer_i32 b = keel_buffer_i32_of(v, 10);

    if (app_ac_troca(&b, 3) != 4) return 1;         /* v[3] era 3, virou 4 */
    size_t i = 2;
    if (app_ac_uma_vez(&b, &i) != 3) return 2;      /* i incrementado UMA vez */
    if (v[2] != 9) return 3;
    if (app_ac_cubo() != 7) return 4;
    /* 5 + 5 + 3 + 10 = 23 */
    if (app_ac_recorta(&b) != 23) return 5;
    puts("ok");
    return 0;
}
