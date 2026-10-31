/* lst.c — generated from lst.k by cgen, C11 profile. */
#include "lst.h"
#include "keel/keel_buffer.h"
#include "keel/keel_buffer_i32.h"
#line 1 "lst.k"















i32 lst_sum(lst_List *l) {
    i32 t = 0;
    {   lst_List *keel__c0 = l; lst_cursor c = lst_begin(keel__c0); while (lst_has_next(keel__c0, &c)) { i32 *p = lst_next(keel__c0, &c);
        t += *p;
    } }
    return t;
}



i32 lst_sum_buffer(keel_buffer_i32 *xs) {
    i32 t = 0;
    {   keel_buffer_i32 *keel__c0 = xs; keel_buffer_cursor c = keel_buffer_i32_begin(keel__c0); while (keel_buffer_i32_has_next(keel__c0, &c)) { i32 *p = keel_buffer_i32_next(keel__c0, &c);
        t += *p;
    } }
    return t;
}


i32 lst_first_even(lst_List *l) {
    {   lst_List *keel__c0 = l; lst_cursor c = lst_begin(keel__c0); while (lst_has_next(keel__c0, &c)) { i32 *p = lst_next(keel__c0, &c);
        if (*p % 2 == 0) return *p;
    } }
    return -1;
}
