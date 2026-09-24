/* app/app_pos.h — generated from app/pos.k by cgen, C23 profile. */
#ifndef APP_APP_POS_H
#define APP_APP_POS_H
#include "app/app_pos.type.h"

typedef struct keel_arena keel_arena;
#line 12 "app/pos.k"
static inline f32 *app_pos_position_x_ptr(struct app_pos_position *p, size_t i0);
#line 13 "app/pos.k"
static inline f32 *app_pos_position_y_ptr(struct app_pos_position *p, size_t i0);
#line 20 "app/pos.k"
static inline i32 *app_pos_grid_v_ptr(struct app_pos_grid *p, size_t i0, size_t i1);
#line 26 "app/pos.k"
static inline u8 *app_pos_image_r_ptr(struct app_pos_image *p, size_t i0, size_t i1);
#line 27 "app/pos.k"
static inline u8 *app_pos_image_g_ptr(struct app_pos_image *p, size_t i0, size_t i1);
#line 31 "app/pos.k"
bool app_pos_reserve(struct app_pos_position *p, keel_arena *a, size_t n);
#line 40 "app/pos.k"
bool app_pos_push_row(struct app_pos_position *p, f32 vx, f32 vy);
#line 49 "app/pos.k"
f32 app_pos_sum_x(struct app_pos_position *p);
#line 56 "app/pos.k"
bool app_pos_grid_init(struct app_pos_grid *g, keel_arena *a, size_t rcap, size_t ccap);
#line 65 "app/pos.k"
void app_pos_grid_fill(struct app_pos_grid *g, size_t rows, size_t cols);
#line 73 "app/pos.k"
i32 app_pos_grid_at(struct app_pos_grid *g, size_t i, size_t j);
#line 77 "app/pos.k"
void app_pos_paint(struct app_pos_image *img);

#line 12 "app/pos.k"
static inline f32 *app_pos_position_x_ptr(struct app_pos_position *p, size_t i0) {
    KEEL_CHECK(i0 < p->len && p->len <= p->cap, "extent-index-out-of-bounds");
    return &p->x[i0];
}
#line 13 "app/pos.k"
static inline f32 *app_pos_position_y_ptr(struct app_pos_position *p, size_t i0) {
    KEEL_CHECK(i0 < p->len && p->len <= p->cap, "extent-index-out-of-bounds");
    return &p->y[i0];
}
#line 20 "app/pos.k"
static inline i32 *app_pos_grid_v_ptr(struct app_pos_grid *p, size_t i0, size_t i1) {
    KEEL_CHECK(i0 < p->rows && p->rows <= p->rcap, "extent-index-out-of-bounds");
    KEEL_CHECK(i1 < p->cols && p->cols <= p->ccap, "extent-index-out-of-bounds");
    return &p->v[i0 * p->ccap + i1];
}
#line 26 "app/pos.k"
static inline u8 *app_pos_image_r_ptr(struct app_pos_image *p, size_t i0, size_t i1) {
    KEEL_CHECK(i0 < p->h && p->h <= app_pos_H, "extent-index-out-of-bounds");
    KEEL_CHECK(i1 < p->w && p->w <= app_pos_W, "extent-index-out-of-bounds");
    return &p->r[i0][i1];
}
#line 27 "app/pos.k"
static inline u8 *app_pos_image_g_ptr(struct app_pos_image *p, size_t i0, size_t i1) {
    KEEL_CHECK(i0 < p->h && p->h <= app_pos_H, "extent-index-out-of-bounds");
    KEEL_CHECK(i1 < p->w && p->w <= app_pos_W, "extent-index-out-of-bounds");
    return &p->g[i0][i1];
}
#endif /* APP_APP_POS_H */
