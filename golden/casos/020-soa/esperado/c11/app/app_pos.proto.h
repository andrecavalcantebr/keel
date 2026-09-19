/* gen/app/pos.h — gerado de app/pos.k, perfil C11.
   Só o que é `pub`: o tipo e os três protótipos (backend §4.1). */
#ifndef APP_APP_POS_H
#define APP_APP_POS_H
#include "app/app_pos.type.h"

typedef struct keel_arena keel_arena;
void app_pos_ocupar(app_pos_position *p, keel_arena *a, size_t n);
bool app_pos_linha(app_pos_position *p, f32 vx, f32 vy);
f32  app_pos_somar_x(app_pos_position *p);
#endif /* APP_APP_POS_H */
