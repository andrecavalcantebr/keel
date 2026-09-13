/* gen/ag/ag.h — gerado de ag/ag.k, perfil C11.
   O `enum` da máquina sai do `decl-maquina`, e `pub` o põe no `.h` (§5.6). */
#ifndef AG_AG_H
#define AG_AG_H
#include "keel/prelude.h"
#include "keel/keel_corot_i32.h"

typedef struct { int s; int n; } ag_Agente;

typedef enum ag_ciclo {
    ag_ciclo_ST1,            /* 0 — estado inicial */
    ag_ciclo_ST2,
    ag_ciclo_ST3
} ag_ciclo;

keel_corot_i32 ag_passo(int *a, ag_Agente *ag);
#endif
