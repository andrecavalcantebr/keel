/* app/app_wa.type.h — generated from app/wa.k by cgen, C23 profile. */
#ifndef APP_APP_WA_TYPE_H
#define APP_APP_WA_TYPE_H

#include "keel.type.h"

#line 4 "app/wa.k"
typedef struct app_wa_Node { i32 v; struct app_wa_Node *link; } app_wa_Node;
typedef struct app_wa_List { app_wa_Node *head; } app_wa_List;
typedef struct app_wa_cursor { app_wa_Node *current; } app_wa_cursor;

#endif /* APP_APP_WA_TYPE_H */
