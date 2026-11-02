/* pt.c — generated from pt.k by cgen, C11 profile. */
#include "pt.h"
#line 1 "pt.k"


typedef enum pt_Mode {pt_Mode_OFF = 1, pt_Mode_ON = 3, pt_Mode_AUTO = 7} pt_Mode;



i32 pt_trail(void) {
    pt_Mode m = pt_Mode_OFF;
    i32 n = 0;
    bool done = false;

    while (!done) {
        switch (m) {
        case pt_Mode_OFF:  goto keel__m0_OFF;
        case pt_Mode_ON:   goto keel__m0_ON;
        case pt_Mode_AUTO: goto keel__m0_AUTO;
        default:           KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
        }
#line 14 "pt.k"
        keel__m0_OFF: {
            n += 1;
            m = pt_Mode_ON;
        }
        goto keel__m0_end;
#line 17 "pt.k"
        keel__m0_ON: {
            n += 10;
            m = pt_Mode_AUTO;
        }
        goto keel__m0_end;
#line 20 "pt.k"
        keel__m0_AUTO: {
            n += 100;
            done = true;
        }
        keel__m0_end: ;
#line 24 "pt.k"
    }
    return n;
}
