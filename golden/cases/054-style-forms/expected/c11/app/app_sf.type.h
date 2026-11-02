/* app/app_sf.type.h — generated from app/sf.k by cgen, C11 profile. */
#ifndef APP_APP_SF_TYPE_H
#define APP_APP_SF_TYPE_H

#include "keel.type.h"

#line 6 "app/sf.k"
#define app_sf_A ((size_t)2)
#line 6 "app/sf.k"
static const size_t app_sf_A__chk = 2; _Static_assert(sizeof(app_sf_A__chk) > 0, "constexpr");
#define app_sf_B ((size_t)(app_sf_A * app_sf_A))
#line 7 "app/sf.k"
static const size_t app_sf_B__chk = (app_sf_A * app_sf_A); _Static_assert(sizeof(app_sf_B__chk) > 0, "constexpr");
#define app_sf_C ((size_t)(app_sf_B + app_sf_A))
#line 8 "app/sf.k"
static const size_t app_sf_C__chk = (app_sf_B + app_sf_A); _Static_assert(sizeof(app_sf_C__chk) > 0, "constexpr");
#line 13 "app/sf.k"
typedef struct app_sf_Node { i32 v; struct app_sf_Node *link; } app_sf_Node;
typedef struct app_sf_List { app_sf_Node *head; } app_sf_List;
typedef struct app_sf_cursor { app_sf_Node *current; } app_sf_cursor;

#endif /* APP_APP_SF_TYPE_H */
