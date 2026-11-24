/* app/app_ca.c — generated from app/ca.k by cgen, C23 profile. */
#include "app/app_ca.h"
#include "keel/keel_buffer_i64.h"
#include "keel/keel_slice_const_u8.h"
#include "keel/keel_slice_recs_Person.h"
#include "recs.h"
#line 1 "app/ca.k"






int app_ca_main(int argc, char **argv) {
    const u8 raw[3] = {1, 2, 3};
    recs_bytes b = {3, raw};
    if (keel_slice_const_u8_length(b) != 3) return 1;
    recs_Person ps[2] = {{40}, {41}};
    recs_people p = {2, ps};
    if (keel_slice_recs_Person_length(p) != 2) return 2;
    recs_wide w = {0};
    if (keel_buffer_i64_length(&w) != 0) return 3;
    puts("ok");
    return 0;
}
