/* app/app_tw.c — generated from app/tw.k by cgen, C11 profile. */
#include "app/app_tw.h"
#include "twos_out_i32.h"
#include "twos_tgt_i32.h"
#line 1 "app/tw.k"




int app_tw_main(int argc, char **argv) {
    i32 v[4] = {0};
    twos_out_i32 o = {4, v};
    twos_out_i32_fill(&o, 7);
    twos_tgt_i32 d = {v};
    if (twos_tgt_i32_run(d, 3) != 7) return 1;
    puts("ok");
    return 0;
}
