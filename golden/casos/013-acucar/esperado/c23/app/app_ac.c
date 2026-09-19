/* app/app_ac.c — gerado de app/ac.k, perfil C23.
   §5.3: o açúcar baixa por função `static inline`, nunca por macro — é o que
   garante avaliação única —, e o par `&*` colapsa na geração. */

#include "keel.type.h"
#include "app/app_ac.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_slice_i32.h"
#line 8 "app/ac.k"
i32 app_ac_troca(keel_buffer_i32 *b, size_t i) {
    *keel_buffer_i32_ptr1(b, i) = *keel_buffer_i32_ptr1(b, i) + 1;
    return *keel_buffer_i32_ptr1(b, i);
}

#line 13 "app/ac.k"
i32 app_ac_uma_vez(keel_buffer_i32 *b, size_t *i) {
    *keel_buffer_i32_ptr1(b, (*i)++) = 9;
    return (i32)*i;
}

#line 19 "app/ac.k"
i32 app_ac_cubo(void) {
    i32 v[2][3][4];
    v[1][2][3] = 7;
    return v[1][2][3];
}

#line 26 "app/ac.k"
size_t app_ac_recorta(keel_buffer_i32 *b) {
    keel_slice_i32 meio = keel_buffer_i32_as_slice2(b, 2, 7);
    keel_slice_i32 fim  = keel_buffer_i32_as_slice2(b, 5, keel_buffer_i32_length(b));
    keel_slice_i32 ini  = keel_buffer_i32_as_slice2(b, 0, 3);
    keel_slice_i32 todo = keel_buffer_i32_as_slice(b);
    return keel_slice_i32_length(meio) + keel_slice_i32_length(fim)
         + keel_slice_i32_length(ini)  + keel_slice_i32_length(todo);
}
