/* app/app_nd.type.h — generated from app/nd.k by cgen, C11 profile. */
#ifndef APP_APP_ND_TYPE_H
#define APP_APP_ND_TYPE_H

#include "keel.type.h"

#line 3 "app/nd.k"
typedef enum app_nd_Kind {app_nd_Kind_LIT, app_nd_Kind_ADD} app_nd_Kind;
#line 8 "app/nd.k"
struct app_nd_nodes {
    app_nd_Kind *kind;
    size_t n, cap;
};

#endif /* APP_APP_ND_TYPE_H */
