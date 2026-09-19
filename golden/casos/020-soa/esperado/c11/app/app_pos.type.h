/* app/app_pos.type.h — gerado de app/pos.k pelo cgen, perfil C11. */
#ifndef APP_APP_POS_TYPE_H
#define APP_APP_POS_TYPE_H
#include "keel.type.h"

typedef struct app_pos_position {
    f32 *x;
    f32 *y;
    bool ativo;
    size_t len, cap;
} app_pos_position;
#endif /* APP_APP_POS_TYPE_H */
