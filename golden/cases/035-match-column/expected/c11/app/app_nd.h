/* app/app_nd.h — generated from app/nd.k by cgen, C11 profile. */
#ifndef APP_APP_ND_H
#define APP_APP_ND_H

#include "app/app_nd.type.h"

#line 9 "app/nd.k"
static inline app_nd_Kind *app_nd_nodes_kind_ptr(struct app_nd_nodes *p, size_t i0);
#line 13 "app/nd.k"
i32 app_nd_weight(struct app_nd_nodes *t, size_t i);

#line 9 "app/nd.k"
static inline app_nd_Kind *app_nd_nodes_kind_ptr(struct app_nd_nodes *p, size_t i0) {
#line 9 "app/nd.k"
    KEEL_CHECK(i0 < p->n && p->n <= p->cap, "extent-index-out-of-bounds");
    return &p->kind[i0];
}

#endif /* APP_APP_ND_H */
