/* app/app_dl.c — generated from app/dl.k by cgen, C23 profile. */
#include "app/app_dl.h"
#line 1 "app/dl.k"


static int app_dl_seen = 0;
static void app_dl_note(int v) { app_dl_seen = v; }
int  app_dl_last(void) { return app_dl_seen; }




void app_dl_late(void) {
    int x = 1;

    x = 2;
    x = 3;
#line 12 "app/dl.k"
    app_dl_note(x);
#line 15 "app/dl.k"
}

void app_dl_early(void) {
    int x = 1;
    struct { int x; } keel__c0 = { x };
    x = 2;
    x = 3;
#line 19 "app/dl.k"
    app_dl_note(keel__c0.x);
#line 22 "app/dl.k"
}


void app_dl_late_written(void) {
    int x = 1;

    x = 7;
#line 27 "app/dl.k"
    app_dl_note(x);
#line 29 "app/dl.k"
}
