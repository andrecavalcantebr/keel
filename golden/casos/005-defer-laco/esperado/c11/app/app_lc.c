/* app/app_lc.c — gerado de app/lc.k pelo cgen, perfil C11. */
#include "app/app_lc.h"
#line 1 "app/lc.k"


static int app_lc_vivos  = 0;
static int app_lc_soltos = 0;

static int  app_lc_adquire(int i) { app_lc_vivos++; return i == 3 ? 0 : (i == 7 ? 95 : i + 1); }
static void app_lc_solta(int h)   { (void)h; app_lc_vivos--; app_lc_soltos++; }

int  app_lc_vivos_agora(void)  { return app_lc_vivos; }
int  app_lc_soltos_agora(void) { return app_lc_soltos; }
void app_lc_zera(void)         { app_lc_vivos = 0; app_lc_soltos = 0; }




int app_lc_roda(int n) {
    int total = 0;
    for (int i = 0; i < n; i++) {
        int h = app_lc_adquire(i);
        struct { int h; } keel__c0 = { h };
        if (h == 0) { app_lc_solta(keel__c0.h); continue; }
        if (h < 0)  { app_lc_solta(keel__c0.h); break; }
        if (h > 90) { app_lc_solta(keel__c0.h); return -1; }
        total += h;
    app_lc_solta(keel__c0.h); }
    return total;
}
