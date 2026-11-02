/* app/app_wa.h — generated from app/wa.k by cgen, C11 profile. */
#ifndef APP_APP_WA_H
#define APP_APP_WA_H

#include "app/app_wa.type.h"

#line 8 "app/wa.k"
static inline app_wa_cursor app_wa_begin(app_wa_List *l);
static inline bool app_wa_has_next(app_wa_List *l, app_wa_cursor *cur);
static inline i32 *app_wa_next(app_wa_List *l, app_wa_cursor *cur);
#line 12 "app/wa.k"
i32 app_wa_sum(app_wa_List *l);

#line 8 "app/wa.k"
static inline app_wa_cursor app_wa_begin(app_wa_List *l) { app_wa_cursor cur = { l->head }; return cur; }
static inline bool app_wa_has_next(app_wa_List *l, app_wa_cursor *cur) { return cur->current != NULL; }
static inline i32 *app_wa_next(app_wa_List *l, app_wa_cursor *cur) { i32 *p = &cur->current->v; cur->current = cur->current->link; return p; }

#endif /* APP_APP_WA_H */
