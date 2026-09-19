/* app/app_ac.h — generated from app/ac.k by cgen, C23 profile. */
#ifndef APP_APP_AC_H
#define APP_APP_AC_H
#include "app/app_ac.type.h"

typedef struct keel_buffer_i32 keel_buffer_i32;
#line 8 "app/ac.k"
i32    app_ac_bump(keel_buffer_i32 *b, size_t i);
#line 13 "app/ac.k"
i32    app_ac_once(keel_buffer_i32 *b, size_t *i);
#line 19 "app/ac.k"
i32    app_ac_cube(void);
#line 26 "app/ac.k"
size_t app_ac_cut(keel_buffer_i32 *b);
#endif
