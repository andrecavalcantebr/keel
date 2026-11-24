/* app/app_ti.c — generated from app/ti.k by cgen, C23 profile. */
#include "app/app_ti.h"
#include "keel/keel_buffer_u8.h"
#include "keel/keel_slice_const_u8.h"
#include "strs.h"
#line 1 "app/ti.k"






int app_ti_main(int argc, char **argv) {
    strs_text t = {0};
    strs_view v = {0};
    if (keel_buffer_u8_length(&t) != 0) return 1;
    if (keel_slice_const_u8_length(v) != 0) return 2;
    if (strs_size(&t) != 0) return 3;
    puts("ok");
    return 0;
}
