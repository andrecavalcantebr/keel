/* app/app_dl.c — gerado de app/dl.k pelo cgen, perfil C23. */
#include "app/app_dl.h"
#line 1 "app/dl.k"


static int app_dl_visto = 0;
static void app_dl_anota(int v) { app_dl_visto = v; }
int  app_dl_ultimo(void) { return app_dl_visto; }



void app_dl_tarde(void) {
    int x = 1;

    x = 2;
    x = 3;
    app_dl_anota(x); }

void app_dl_cedo(void) {
    int x = 1;
    struct { int x; } keel__c0 = { x };
    x = 2;
    x = 3;
    app_dl_anota(keel__c0.x); }


void app_dl_tarde_escrito(void) {
    int x = 1;

    x = 7;
    app_dl_anota(x); }
