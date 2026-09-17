/* gen/app/dl.c — gerado de app/dl.k, perfil C23.
   Sem `[now]` o corpo referencia as variáveis direto e NENHUMA struct de
   captura é gerada (backend §5.5); o valor lido é o da saída. */

#include "keel/keel.type.h"
#include "app/app_dl.impl.h"
#line 3 "app/dl.k"
static int app_dl_visto = 0;
static void app_dl_anota(int v) { app_dl_visto = v; }
int  app_dl_ultimo(void) { return app_dl_visto; }

#line 9 "app/dl.k"
void app_dl_tarde(void) {
    int x = 1;
    x = 2;
    x = 3;
    app_dl_anota(x);
}

#line 16 "app/dl.k"
void app_dl_cedo(void) {
    int x = 1;
    struct { int x; } keel__c0 = { x };
    x = 2;
    x = 3;
    app_dl_anota(keel__c0.x);
}

#line 24 "app/dl.k"
void app_dl_tarde_escrito(void) {
    int x = 1;
    x = 7;
    app_dl_anota(x);
}
