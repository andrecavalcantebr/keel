/* app/app_pos.c — generated from app/pos.k by cgen, C23 profile. */
#include "app/app_pos.h"
#include "keel/keel_arena.h"
#include "keel/keel_slice_f32.h"
#line 1 "app/pos.k"






























bool app_pos_reserve(struct app_pos_position *p, keel_arena *a, size_t n) {
    p->x = (f32 *)keel_arena_alloc2(a, sizeof(f32), alignof(f32), n);
    p->y = (f32 *)keel_arena_alloc2(a, sizeof(f32), alignof(f32), n);
    p->len = 0;
    p->cap = (p->x && p->y) ? n : 0;
    return p->cap > 0;
}


bool app_pos_push_row(struct app_pos_position *p, f32 vx, f32 vy) {
    if (p->len == p->cap) return false;
    size_t i = p->len++;
    *app_pos_position_x_ptr(p, i) = vx;
    *app_pos_position_y_ptr(p, i) = vy;
    return true;
}


f32 app_pos_sum_x(struct app_pos_position *p) {
    keel_slice_f32 xs = keel_slice_f32_from(p->x, p->len);
    f32 total = 0;
    { keel_slice_f32 keel__c0 = xs; size_t keel__n0 = keel_slice_f32_length(keel__c0); for (size_t i = 0; i < keel__n0; i++) { f32 v = keel_slice_f32_get(keel__c0, i); total += v; } }
    return total;
}

bool app_pos_grid_init(struct app_pos_grid *g, keel_arena *a, size_t rcap, size_t ccap) {
    g->v = (i32 *)keel_arena_alloc2(a, sizeof(i32), alignof(i32), rcap * ccap);
    g->rows = 0;
    g->cols = 0;
    g->rcap = g->v ? rcap : 0;
    g->ccap = g->v ? ccap : 0;
    return g->v != NULL;
}

void app_pos_grid_fill(struct app_pos_grid *g, size_t rows, size_t cols) {
    g->rows = rows;
    g->cols = cols;
    for (size_t i = 0; i < rows; i++)
        for (size_t j = 0; j < cols; j++)
            *app_pos_grid_v_ptr(g, i, j) = (i32)(i * 10 + j);
}

i32 app_pos_grid_at(struct app_pos_grid *g, size_t i, size_t j) {
    return *app_pos_grid_v_ptr(g, i, j);
}

void app_pos_paint(struct app_pos_image *img) {
    img->h = app_pos_H;
    img->w = app_pos_W;
    for (size_t i = 0; i < app_pos_H; i++)
        for (size_t j = 0; j < app_pos_W; j++) {
            *app_pos_image_r_ptr(img, i, j) = (u8)(i * app_pos_W + j);
            *app_pos_image_g_ptr(img, i, j) = (u8)(255 - *app_pos_image_r_ptr(img, i, j));
        }
}
