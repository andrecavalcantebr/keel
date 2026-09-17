/* keel/keel_tagged_ag_Ciclo_void.type.h — instância de `tagged Ciclo void`.
   `void` omite o campo do valor: resta a etiqueta (backend §5.2, §5.6).
   A etiqueta é `i32`, e não o `enum`: largura estável, e o header de instância
   não passa a depender do header do módulo que declarou o conjunto. */
#ifndef KEEL_KEEL_TAGGED_AG_CICLO_VOID_TYPE_H
#define KEEL_KEEL_TAGGED_AG_CICLO_VOID_TYPE_H
#include "keel/keel.type.h"

typedef struct keel_tagged_ag_Ciclo_void { i32 tag; } keel_tagged_ag_Ciclo_void;
#endif /* KEEL_KEEL_TAGGED_AG_CICLO_VOID_TYPE_H */
