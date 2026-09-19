/* lst.c — gerado de lst.k pelo cgen, perfil C11. */
#include "lst.h"
#include "keel/keel_buffer_i32.h"
#line 16 "lst.k"
i32 lst_soma(lst_Lista *l) {
    i32 t = 0;
    {   lst_Lista *keel__c0 = l; lst_cursor c = lst_begin(keel__c0); while (lst_has_next(keel__c0, &c)) { i32 *p = lst_next(keel__c0, &c);
        t += *p;
    } }
    return t;
}

#line 26 "lst.k"
i32 lst_soma_buffer(keel_buffer_i32 *xs) {
    i32 t = 0;
    {   keel_buffer_i32 *keel__c1 = xs; keel_buffer_cursor c = keel_buffer_i32_begin(keel__c1); while (keel_buffer_i32_has_next(keel__c1, &c)) { i32 *p = keel_buffer_i32_next(keel__c1, &c);
        t += *p;
    } }
    return t;
}

#line 35 "lst.k"
i32 lst_primeiro_par(lst_Lista *l) {
    {   lst_Lista *keel__c2 = l; lst_cursor c = lst_begin(keel__c2); while (lst_has_next(keel__c2, &c)) { i32 *p = lst_next(keel__c2, &c);
        if (*p % 2 == 0) return *p;
    } }
    return -1;
}
