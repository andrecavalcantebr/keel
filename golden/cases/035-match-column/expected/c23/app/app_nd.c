/* app/app_nd.c — generated from app/nd.k by cgen, C23 profile. */
#include "app/app_nd.h"
#line 1 "app/nd.k"












i32 app_nd_weight(struct app_nd_nodes *t, size_t i) {
    i32 r = 0;
    switch (*app_nd_nodes_kind_ptr(t, i)) {
    case app_nd_Kind_LIT: goto keel__m0_LIT;
    case app_nd_Kind_ADD: goto keel__m0_ADD;
    default:              KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
    }
#line 16 "app/nd.k"
    keel__m0_LIT: {
        r = 1;
    }
    goto keel__m0_end;
#line 18 "app/nd.k"
    keel__m0_ADD: {
        r = 2;
    }
    keel__m0_end: ;
#line 21 "app/nd.k"
    return r;
}
