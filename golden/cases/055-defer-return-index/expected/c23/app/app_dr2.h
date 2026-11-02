/* app/app_dr2.h — generated from app/dr2.k by cgen, C23 profile. */
#ifndef APP_APP_DR2_H
#define APP_APP_DR2_H

#include "app/app_dr2.type.h"

typedef struct keel_buffer_i32 keel_buffer_i32;
#line 7 "app/dr2.k"
void app_dr2_reset(void);
int app_dr2_trail_now(void);
#line 12 "app/dr2.k"
int app_dr2_tail(void);
#line 20 "app/dr2.k"
int app_dr2_head(void);
#line 27 "app/dr2.k"
int app_dr2_pick(size_t i);
#line 35 "app/dr2.k"
int app_dr2_two(size_t i);
#line 44 "app/dr2.k"
size_t app_dr2_scan(keel_buffer_i32 *s);

#endif /* APP_APP_DR2_H */
