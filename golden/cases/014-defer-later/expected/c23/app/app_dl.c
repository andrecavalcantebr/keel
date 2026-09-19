/* app/app_dl.c — gerado de app/dl.k pelo cgen, perfil C23. */
#include "app/app_dl.h"
#line 1 "app/dl.k"


static int app_dl_seen = 0;
static void app_dl_note(int v) { app_dl_seen = v; }
int  app_dl_last(void) { return app_dl_seen; }



void app_dl_late(void) {
    int x = 1;

    x = 2;
    x = 3;
    app_dl_note(x); }

void app_dl_early(void) {
    int x = 1;
    struct { int x; } keel__c0 = { x };
    x = 2;
    x = 3;
    app_dl_note(keel__c0.x); }


void app_dl_late_written(void) {
    int x = 1;

    x = 7;
    app_dl_note(x); }
