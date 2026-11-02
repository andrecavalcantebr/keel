/* app/app_dr2.c — generated from app/dr2.k by cgen, C23 profile. */
#include "app/app_dr2.h"
#include "keel/keel_buffer_i32.h"
#line 1 "app/dr2.k"



static int app_dr2_trail = 0;
static void app_dr2_note(int d) { app_dr2_trail = app_dr2_trail * 10 + d; }

void app_dr2_reset(void) { app_dr2_trail = 0; }
int app_dr2_trail_now(void) { return app_dr2_trail; }



int app_dr2_tail(void) {
    i32 v[3] = {7, 8, 9};

    { int keel__rv0 = v[0];
#line 14 "app/dr2.k"
    v[1] = 0;
    return keel__rv0; }
}



int app_dr2_head(void) {
    i32 v[3] = {7, 8, 9};

    { int keel__rv0 = v[0];
#line 22 "app/dr2.k"
    v[0] = 0;
    return keel__rv0; }
}


int app_dr2_pick(size_t i) {
    i32 v[3] = {7, 8, 9};

    { int keel__rv0 = v[keel_index(i, 3)];
#line 29 "app/dr2.k"
    v[0] = 0;
    return keel__rv0; }
}



int app_dr2_two(size_t i) {
    i32 v[3] = {7, 8, 9};


    { int keel__rv0 = v[keel_index(i, 3)];
#line 38 "app/dr2.k"
    app_dr2_note(2);
#line 37 "app/dr2.k"
    app_dr2_note(1);
#line 39 "app/dr2.k"
    return keel__rv0; }
}



size_t app_dr2_scan(keel_buffer_i32 *s) {
    size_t count = 0;
    { keel_buffer_i32 *keel__c0 = s; size_t keel__n0 = keel_buffer_i32_length(keel__c0); for (size_t i = 0; i < keel__n0; i++) { i32 x = keel_buffer_i32_get(keel__c0, i);

        if (x == 0) {
#line 47 "app/dr2.k"
        count++;
        break; }
#line 47 "app/dr2.k"
        count++;
#line 49 "app/dr2.k"
    } }
    return count;
}
