/* gen/ag/ag.h — gerado de ag/ag.k, perfil C23.
   O `enum` sai da declaração `tags`, e `pub` o põe no `.h` (backend §5.6). */
#ifndef AG_AG_H
#define AG_AG_H
#include "keel/prelude.h"
#include "keel/corot.h"
#include "keel/keel_tagged_ag_Ciclo_void.h"

typedef enum ag_Ciclo {
    ag_Ciclo_ST1,            /* 0 — ordinal da posição escrita */
    ag_Ciclo_ST2,
    ag_Ciclo_ST3
} ag_Ciclo;

typedef struct { i32 n; } ag_Agente;

keel_corot ag_passo(i32 *a, keel_tagged_ag_Ciclo_void *st, ag_Agente *ag);
#endif
