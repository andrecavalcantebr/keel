/* app/app_dj.c — generated from app/dj.k by cgen, C11 profile. */
#include "app/app_dj.h"
#line 1 "app/dj.k"




static char app_dj_trail[8] = {0};
static size_t app_dj_walked = 0;

static void app_dj_note(char c) { if (app_dj_walked < 8) app_dj_trail[app_dj_walked++] = c; }

void app_dj_reset(void) { app_dj_walked = 0; }
size_t app_dj_walked_now(void) { return app_dj_walked; }
char app_dj_step(size_t i) { return i < app_dj_walked ? app_dj_trail[i] : 0; }



int app_dj_jumped(int k) {
    {

        if (k == 0) {
#line 18 "app/dj.k"
        app_dj_note('g');
        goto out; }
        app_dj_note('b');
#line 18 "app/dj.k"
        app_dj_note('g');
#line 21 "app/dj.k"
    }
    return 0;
out:
    return -1;
}




int app_dj_nested(int k) {
    int keel__rv0;
    {

        if (k == 0) { keel__rv0 = 1;
#line 33 "app/dj.k"
        app_dj_note('2');
        goto keel__e0; }
        if (k == 1) { keel__rv0 = 2;
#line 33 "app/dj.k"
        app_dj_note('2');
#line 35 "app/dj.k"
        goto keel__e0; }

#line 36 "app/dj.k"
        app_dj_note('3');
#line 33 "app/dj.k"
        app_dj_note('2');
#line 37 "app/dj.k"
    }
    keel__rv0 = 3; goto keel__e0;
#line 31 "app/dj.k"
keel__e0: app_dj_note('1');
#line 39 "app/dj.k"
    return keel__rv0; }
