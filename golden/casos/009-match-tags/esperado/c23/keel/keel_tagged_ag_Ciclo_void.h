/* keel/keel_tagged_ag_Ciclo_void.h — instância de `tagged Ciclo void`.
   `void` omite o campo do valor: resta a etiqueta (backend §5.2, §5.6).
   A etiqueta é `i32`, e não o `enum`: largura estável, e o header de instância
   não passa a depender do header do módulo que declarou o conjunto. */
#ifndef KEEL_KEEL_TAGGED_AG_CICLO_VOID_H
#define KEEL_KEEL_TAGGED_AG_CICLO_VOID_H
#include "keel/keel_tagged_ag_Ciclo_void.proto.h"

static inline i32  keel_tagged_ag_Ciclo_void_tag (const keel_tagged_ag_Ciclo_void *t) { return t->tag; }
static inline void keel_tagged_ag_Ciclo_void_mark(keel_tagged_ag_Ciclo_void *t, i32 e) { t->tag = e; }
#endif /* KEEL_KEEL_TAGGED_AG_CICLO_VOID_H */
