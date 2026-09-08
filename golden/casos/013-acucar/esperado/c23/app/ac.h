/* gen/app/ac.h — gerado de app/ac.k, perfil C23. */
#ifndef APP_AC_H
#define APP_AC_H
#include "keel/prelude.h"
#include "keel/keel_buffer_i32.h"

i32    app_ac_troca(keel_buffer_i32 *b, size_t i);
i32    app_ac_uma_vez(keel_buffer_i32 *b, size_t *i);
i32    app_ac_cubo(void);
size_t app_ac_recorta(keel_buffer_i32 *b);
#endif
