/* gen/app/dr.h — gerado de app/dr.k, perfil C23. */
#ifndef APP_DR_H
#define APP_DR_H
#include "keel/prelude.h"

typedef struct { int v; } app_dr_Rec;

int  app_dr_consome(app_dr_Rec *r, int modo);
int  app_dr_dois(app_dr_Rec *r, int cedo);
void app_dr_nada(app_dr_Rec *r);
#endif
