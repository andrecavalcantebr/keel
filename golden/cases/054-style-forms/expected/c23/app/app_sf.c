/* app/app_sf.c — generated from app/sf.k by cgen, C23 profile. */
#include "app/app_sf.h"
#include "keel/keel_slice_i32.h"
#line 1 "app/sf.k"









size_t app_sf_cval(void) { return app_sf_C; }










i32 app_sf_sum(app_sf_List *l) {
    i32 t = 0;
    {   app_sf_List *keel__c0 = l; app_sf_cursor c = app_sf_begin(keel__c0); while (app_sf_has_next(keel__c0, &c)) { i32 *p = app_sf_next(keel__c0, &c);
        t += *p;
    } }
    return t;
}

i32 app_sf_allman(keel_slice_i32 s)
{
    i32 t = 0;
    { keel_slice_i32 keel__c0 = s; size_t keel__n0 = keel_slice_i32_length(keel__c0); for (size_t i = 0; i < keel__n0; i++)
    { i32 x = keel_slice_i32_get(keel__c0, i);
        t += x - (i32)i;
    } }
    return t;
}
