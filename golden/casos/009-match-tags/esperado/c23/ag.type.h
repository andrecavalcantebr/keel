/* ag.type.h — gerado de ag.k pelo cgen, perfil C23. */
#ifndef AG_TYPE_H
#define AG_TYPE_H
#include "keel.type.h"
#include "keel/keel_corot.type.h"
#include "keel/keel_tagged_ag_Ciclo_void.type.h"

typedef enum ag_Ciclo {
    ag_Ciclo_ST1,            /* 0 — ordinal da posição escrita */
    ag_Ciclo_ST2,
    ag_Ciclo_ST3
} ag_Ciclo;
typedef struct { i32 n; } ag_Agente;
#endif /* AG_TYPE_H */
