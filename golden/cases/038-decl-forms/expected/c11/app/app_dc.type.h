/* app/app_dc.type.h — generated from app/dc.k by cgen, C11 profile. */
#ifndef APP_APP_DC_TYPE_H
#define APP_APP_DC_TYPE_H

#include "keel.type.h"

#line 2 "app/dc.k"
#include <string.h>
#line 4 "app/dc.k"
struct app_dc_Box { int a; };
union app_dc_Num { int i; float f; };
typedef struct app_dc_Node app_dc_Node;
struct app_dc_Node { app_dc_Node *next; int v; };
struct app_dc_Local { int a; };
typedef struct app_dc_P { int a; } app_dc_P, *app_dc_PP;

#endif /* APP_APP_DC_TYPE_H */
