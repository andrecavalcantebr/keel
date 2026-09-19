/* app/app_pos.c — gerado de app/pos.k, perfil C23.
   `p[i].campo` (spec §4.11) reescreve para `p->campo[i]` — é o que aparece
   em `app_pos_linha`, saindo de `p[p->len].x = vx;` no fonte. Nenhum verbo
   sintetizado: `ocupar` aloca à mão, `somar_x` usa `slice.from`, já
   existente (§5.3), pra entrar em `foreach`. */

#include "keel.type.h"
#include "app/app_pos.h"
#include "keel/keel_arena.h"
#include "keel/keel_slice_f32.h"

#line 19 "app/pos.k"
void app_pos_ocupar(app_pos_position *p, keel_arena *a, size_t n) {
    p->x = (f32 *)keel_arena_alloc_n(a, n, sizeof(f32), alignof(f32));
    p->y = (f32 *)keel_arena_alloc_n(a, n, sizeof(f32), alignof(f32));
    p->cap = n;
    p->len = 0;
}

#line 26 "app/pos.k"
bool app_pos_linha(app_pos_position *p, f32 vx, f32 vy) {
    if (p->len == p->cap) return false;
    p->x[p->len] = vx;
    p->y[p->len] = vy;
    p->len++;
    return true;
}

#line 37 "app/pos.k"
f32 app_pos_somar_x(app_pos_position *p) {
    keel_slice_f32 xs = keel_slice_f32_from(p->x, p->len);
    f32 total = 0;
    { keel_slice_f32 keel__c0 = xs; size_t keel__n0 = keel_slice_f32_length(keel__c0); for (size_t i = 0; i < keel__n0; i++) { f32 v = keel_slice_f32_get(keel__c0, i);
        total += v;
    } }
    return total;
}
