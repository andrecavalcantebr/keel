/* app/app_dc.c — generated from app/dc.k by cgen, C11 profile. */
#include "app/app_dc.h"
#line 1 "app/dc.k"










int app_dc_x = 1, app_dc_y = 2;

int app_dc_total(int n) {
    struct app_dc_Box b = { n };
    union app_dc_Num u;
    u.i = b.a;
    struct app_dc_Local l = { 5 };
    app_dc_Node nd = { NULL, 3 };
    app_dc_P p = { 4 };
    return u.i + l.a + nd.v + p.a + app_dc_x + app_dc_y + (int)strlen("ab");
}
