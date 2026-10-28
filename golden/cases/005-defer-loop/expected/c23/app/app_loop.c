/* app/app_loop.c — generated from app/loop.k by cgen, C23 profile. */
#include "app/app_loop.h"
#line 1 "app/loop.k"


static int app_loop_live  = 0;
static int app_loop_released = 0;

static int  app_loop_acquire(int i) { app_loop_live++; return i == 3 ? 0 : (i == 7 ? 95 : i + 1); }
static void app_loop_release(int h)   { (void)h; app_loop_live--; app_loop_released++; }

int  app_loop_live_now(void)  { return app_loop_live; }
int  app_loop_released_now(void) { return app_loop_released; }
void app_loop_reset(void)         { app_loop_live = 0; app_loop_released = 0; }




int app_loop_run(int n) {
    int total = 0;
    for (int i = 0; i < n; i++) {
        int h = app_loop_acquire(i);
        struct { int h; } keel__c0 = { h };
        if (h == 0) {
#line 20 "app/loop.k"
        app_loop_release(keel__c0.h);
        continue; }
        if (h < 0)  {
#line 20 "app/loop.k"
        app_loop_release(keel__c0.h);
#line 22 "app/loop.k"
        break; }
        if (h > 90) { int keel__rv0 = -1;
#line 20 "app/loop.k"
        app_loop_release(keel__c0.h);
#line 23 "app/loop.k"
        return keel__rv0; }
        total += h;
#line 20 "app/loop.k"
        app_loop_release(keel__c0.h);
#line 25 "app/loop.k"
    }
    return total;
}
