/* gen/app/pos.type.h — gerado de app/pos.k, perfil C11.
   `soa struct position` é `pub`: layout completo aqui, com `x`/`y` já
   invertidos para ponteiro (spec §4.11). */
#ifndef APP_APP_POS_TYPE_H
#define APP_APP_POS_TYPE_H
#include "keel/prelude.h"

typedef struct app_pos_position {
    f32 *x;
    f32 *y;
    bool ativo;
    size_t len, cap;
} app_pos_position;
#endif /* APP_APP_POS_TYPE_H */
