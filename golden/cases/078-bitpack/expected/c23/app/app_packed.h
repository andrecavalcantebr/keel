/* app/app_packed.h — generated from app/packed.k by cgen, C23 profile. */
#ifndef APP_APP_PACKED_H
#define APP_APP_PACKED_H

#include "app/app_packed.type.h"

typedef struct keel_arena keel_arena;
#line 11 "app/packed.k"
void app_packed_layout(u8 *store);
#line 18 "app/packed.k"
i32 app_packed_widths(keel_arena *a);
#line 45 "app/packed.k"
i32 app_packed_views(keel_arena *a);
#line 63 "app/packed.k"
u32 app_packed_total(keel_arena *a);
#line 74 "app/packed.k"
i32 app_packed_stack(keel_arena *a);
#line 91 "app/packed.k"
i32 app_packed_cloned(keel_arena *a);

#endif /* APP_APP_PACKED_H */
