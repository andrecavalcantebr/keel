/* app/app_xc.c — generated from app/xc.k by cgen, C11 profile. */
#include "app/app_xc.h"
#line 1 "app/xc.k"






int app_xc_before(Raw *r) { return r->a; }





int app_xc_after(Raw *r) { return twice(r->a); }


    static int cache = 7;


int app_xc_hidden(void) { return cache; }
