/* app/app_chk.c — generated from app/chk.k by cgen, C23 profile. */
#include "app/app_chk.h"
#include "keel/keel_bitbuffer_3_u8.h"
#include "keel/keel_bitslice_3_u8.h"
#line 1 "app/chk.k"







int app_chk_main(int argc, char **argv) {
    u8 store[4] = {0};
    keel_bitbuffer_3_u8 g = keel_bitbuffer_3_u8_of(&store[0], 8);
    keel_bitslice_3_u8 v = keel_bitbuffer_3_u8_as_slice2(&g, 2, 6);
    size_t i = argc > 2 ? (size_t)atoi(argv[2]) : 8;
    int which = argc > 1 ? atoi(argv[1]) : 0;
    if (which == 1) return keel_bitbuffer_3_u8_get(&g, i);
    if (which == 2) { keel_bitbuffer_3_u8_set(&g, i, 0); return 0; }
    if (which == 3) { keel_bitbuffer_3_u8_set(&g, 0, 8); return 0; }
    if (which == 4) { keel_bitbuffer_3_u8 h = keel_bitbuffer_3_u8_from(&store[0], 8); keel_bitbuffer_3_u8_push(&h, 9); return 0; }
    if (which == 5) return keel_bitslice_3_u8_get(v, i);
    if (which == 6) { keel_bitslice_3_u8_set(v, i, 0); return 0; }
    if (which == 7) { keel_bitslice_3_u8_set(v, 0, 8); return 0; }
    return 0;
}
