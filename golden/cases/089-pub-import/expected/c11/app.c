/* app.c — generated from app.k by cgen, C11 profile. */
#include "app.h"
#include "owner.h"
#line 1 "app.k"



i32 app_run(void) {
    owner_Count n = owner_value();
    return n + owner_BIAS;
}
