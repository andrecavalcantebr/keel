/* app/app_sf.h — generated from app/sf.k by cgen, C11 profile. */
#ifndef APP_APP_SF_H
#define APP_APP_SF_H

#include "app/app_sf.type.h"
#include "keel/keel_slice_i32.type.h"

#line 10 "app/sf.k"
size_t app_sf_cval(void);
#line 17 "app/sf.k"
static inline app_sf_cursor app_sf_begin(app_sf_List *l);
static inline bool app_sf_has_next(app_sf_List *l, app_sf_cursor *cur);
static inline i32 *app_sf_next(app_sf_List *l, app_sf_cursor *cur);
#line 21 "app/sf.k"
i32 app_sf_sum(app_sf_List *l);
#line 29 "app/sf.k"
i32 app_sf_allman(keel_slice_i32 s);

#line 17 "app/sf.k"
static inline app_sf_cursor app_sf_begin(app_sf_List *l) { app_sf_cursor cur = { l->head }; return cur; }
static inline bool app_sf_has_next(app_sf_List *l, app_sf_cursor *cur) { return cur->current != NULL; }
static inline i32 *app_sf_next(app_sf_List *l, app_sf_cursor *cur) { i32 *p = &cur->current->v; cur->current = cur->current->link; return p; }

#endif /* APP_APP_SF_H */
