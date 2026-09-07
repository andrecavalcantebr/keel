/* gen/app/dl.c — perfil C11.
   Sem `[now]` o corpo referencia as variáveis direto — nenhuma struct de
   captura é gerada (backend §5.5) — e o valor lido é o da SAÍDA. Toda saída
   aqui é o fim natural, então não há salto a emitir. */
#include "keel/prelude.h"
#include <stdio.h>

void app_dl_anota(int v);

void app_dl_tarde(void) {
    int x = 1;
    x = 2;
    x = 3;
    app_dl_anota(x);
}

void app_dl_cedo(void) {
    int x = 1;
    struct { int x; } keel__c0 = { x };
    x = 2;
    x = 3;
    app_dl_anota(keel__c0.x);
}

void app_dl_tarde_escrito(void) {
    int x = 1;
    x = 7;
    app_dl_anota(x);
}

/* ---- prova ---- */
static int visto = 0;
void app_dl_anota(int v) { visto = v; }

int main(void) {
    app_dl_tarde();          if (visto != 3) return 1;   /* valor na saída */
    app_dl_cedo();           if (visto != 1) return 2;   /* valor no registro */
    app_dl_tarde_escrito();  if (visto != 7) return 3;   /* `later` == padrão */
    puts("ok");
    return 0;
}
