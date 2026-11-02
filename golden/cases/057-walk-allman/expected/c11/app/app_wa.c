/* app/app_wa.c — generated from app/wa.k by cgen, C11 profile. */
#include "app/app_wa.h"
#line 1 "app/wa.k"











i32 app_wa_sum(app_wa_List *l)
{
    i32 t = 0;
    {   app_wa_List *keel__c0 = l; app_wa_cursor c = app_wa_begin(keel__c0); while (app_wa_has_next(keel__c0, &c))
    { i32 *p = app_wa_next(keel__c0, &c);
        t += *p;
    } }
    return t;
}
