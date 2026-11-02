/* pt.c — generated from pt.k by cgen, C23 profile. */
#include "pt.h"
#include "keel/keel_tagged_pt_Mode_void.h"
#line 1 "pt.k"







i32 pt_trail(void) {
    keel_tagged_pt_Mode_void m = {0};
    keel_tagged_pt_Mode_void_mark(&m, pt_Mode_OFF);
    i32 n = 0;
    bool done = false;

    while (!done) {
        switch (m.tag) {
        case pt_Mode_OFF:  goto keel__m0_OFF;
        case pt_Mode_ON:   goto keel__m0_ON;
        case pt_Mode_AUTO: goto keel__m0_AUTO;
        default:           KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
        }
#line 16 "pt.k"
        keel__m0_OFF: {
            n += 1;
            keel_tagged_pt_Mode_void_mark(&m, pt_Mode_ON);
        }
        goto keel__m0_end;
#line 19 "pt.k"
        keel__m0_ON: {
            n += 10;
            keel_tagged_pt_Mode_void_mark(&m, pt_Mode_AUTO);
        }
        goto keel__m0_end;
#line 22 "pt.k"
        keel__m0_AUTO: {
            n += 100;
            done = true;
        }
        keel__m0_end: ;
#line 26 "pt.k"
    }
    return n;
}
