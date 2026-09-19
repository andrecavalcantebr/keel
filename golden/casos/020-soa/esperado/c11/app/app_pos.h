/* app/app_pos.h — gerado de app/pos.k pelo cgen, perfil C11. */
#ifndef APP_APP_POS_H
#define APP_APP_POS_H
#include "app/app_pos.type.h"

typedef struct keel_arena keel_arena;
#line 19 "app/pos.k"
void app_pos_ocupar(app_pos_position *p, keel_arena *a, size_t n);
#line 26 "app/pos.k"
bool app_pos_linha(app_pos_position *p, f32 vx, f32 vy);
#line 37 "app/pos.k"
f32  app_pos_somar_x(app_pos_position *p);
#endif
