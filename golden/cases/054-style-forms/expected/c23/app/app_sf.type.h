/* app/app_sf.type.h — generated from app/sf.k by cgen, C23 profile. */
#ifndef APP_APP_SF_TYPE_H
#define APP_APP_SF_TYPE_H

#include "keel.type.h"

#line 6 "app/sf.k"
constexpr size_t app_sf_A = 2;
constexpr size_t app_sf_B = app_sf_A * app_sf_A;
constexpr size_t app_sf_C = app_sf_B + app_sf_A;
#line 13 "app/sf.k"
typedef struct app_sf_Node { i32 v; struct app_sf_Node *link; } app_sf_Node;
typedef struct app_sf_List { app_sf_Node *head; } app_sf_List;
typedef struct app_sf_cursor { app_sf_Node *current; } app_sf_cursor;

#endif /* APP_APP_SF_TYPE_H */
