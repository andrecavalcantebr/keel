/* gen/app/lc.c — perfil C11.
   Alguma saída é `break`/`continue` → forma INLINE (backend §5.5.2). Um rótulo
   só não serviria às três: `continue` cai no incremento, `break` deixa o laço,
   `return` deixa a função. */
#include "keel/prelude.h"
#include <stdio.h>

int app_lc_adquire(int i);
void app_lc_solta(int h);

int app_lc_roda(int n) {
    int total = 0;
    for (int i = 0; i < n; i++) {
        int h = app_lc_adquire(i);
        struct { int h; } keel__c0 = { h };
        if (h == 0) { app_lc_solta(keel__c0.h); continue; }
        if (h < 0)  { app_lc_solta(keel__c0.h); break; }
        if (h > 90) { app_lc_solta(keel__c0.h); return -1; }
        total += h;
        app_lc_solta(keel__c0.h);
    }
    return total;
}

/* ---- prova de execução: o cleanup roda uma vez por iteração, em toda saída ---- */
static int vivos = 0, soltos = 0;
int app_lc_adquire(int i) { vivos++; return i == 3 ? 0 : (i == 7 ? 95 : i + 1); }
void app_lc_solta(int h) { (void)h; vivos--; soltos++; }

int main(void) {
    int r = app_lc_roda(20);
    if (vivos != 0) return 1;          /* nada vazou */
    if (soltos != 8) return 2;         /* i=0..7: sete normais + o `return` */
    if (r != -1)     return 3;
    vivos = soltos = 0;
    r = app_lc_roda(3);
    if (vivos != 0 || soltos != 3 || r != 1 + 2 + 3) return 4;
    puts("ok");
    return 0;
}
