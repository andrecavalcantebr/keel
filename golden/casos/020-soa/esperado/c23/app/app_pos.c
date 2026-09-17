/* gen/app/pos.c — gerado de app/pos.k, perfil C23.
   `soa struct position` (spec §4.11): `x` e `y`, marcados `array`, viram
   coluna; `ativo` é único, compartilhado por toda a instância. Não é `pub`
   — nenhum artefato público o menciona —, então mora inteiro aqui, sem
   `.type.h`/`.h` próprios (backend §4.3.2: só `pub` atravessa a fronteira
   do arquivo). `keel.slice_of` desce para a conversão de coluna escrita à
   mão; `keel.push`/`keel.capacity` para os verbos sintetizados do §4.11. */

#include "keel/prelude.h"
#include "app/app_pos.impl.h"
#include "keel/keel_arena.impl.h"
#include "keel/keel_outcome_i32.impl.h"
#include "keel/keel_slice_f32.impl.h"

typedef enum { app_pos_position_x, app_pos_position_y } app_pos_position_fields;

typedef struct app_pos_position {
    f32 *x;
    f32 *y;
    bool ativo;
    size_t len, cap;
} app_pos_position;

static app_pos_position app_pos_position_from(keel_arena *a, size_t cap) {
    f32 *x = (f32 *)keel_arena_alloc_n(a, cap, sizeof(f32), alignof(f32));
    f32 *y = (f32 *)keel_arena_alloc_n(a, cap, sizeof(f32), alignof(f32));
    if (!x || !y) return (app_pos_position){0};
    return (app_pos_position){ x, y, false, 0, cap };
}

static size_t app_pos_position_capacity(app_pos_position *p) { return p->cap; }

static void app_pos_position_push(app_pos_position *p, f32 vx, f32 vy) {
    p->x[p->len] = vx;
    p->y[p->len] = vy;
    p->len++;
}

static keel_slice_f32 app_pos_position_slice_of(app_pos_position *p, app_pos_position_fields campo) {
    switch (campo) {
    case app_pos_position_x: return (keel_slice_f32){ p->len, p->x };
    case app_pos_position_y: return (keel_slice_f32){ p->len, p->y };
    }
    return (keel_slice_f32){0, NULL};
}

#line 16 "app/pos.k"
keel_outcome_i32 app_pos_somar_x(keel_arena *a, size_t n) {
    keel_outcome_i32 r = {0};
    app_pos_position p = app_pos_position_from(a, n);
    if (app_pos_position_capacity(&p) == 0) return keel_outcome_i32_fail(&r, 1);

    p.ativo = true;
    for (size_t i = 0; i < n; i++)
        app_pos_position_push(&p, (f32)i, (f32)i * 2.0f);

    f32 total = 0;
    keel_slice_f32 xs = app_pos_position_slice_of(&p, app_pos_position_x);
    { keel_slice_f32 keel__c0 = xs; size_t keel__n0 = keel_slice_f32_length(keel__c0); for (size_t i = 0; i < keel__n0; i++) { f32 v = keel_slice_f32_get(keel__c0, i);
        total += v;
    } }

    return keel_outcome_i32_win1(&r, (i32)total);
}
