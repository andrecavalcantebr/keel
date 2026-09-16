/* keel/keel_corot.h — o tipo `corot` e seu conjunto de tags (backend §5.14).
   Tipo verdadeiro, não modificador: um campo, e três estados por sinal do
   código — <0 SUCCESS, 0 ONGOING, >0 FAILED. Sai uma vez no módulo, sem
   header de instância: não há valor associado a parametrizar. */
#ifndef KEEL_KEEL_COROT_H
#define KEEL_KEEL_COROT_H
#include "keel/keel_corot.type.h"

static inline bool keel_corot_ok     (keel_corot r);
static inline bool keel_corot_ongoing(keel_corot r);
static inline bool keel_corot_faulted(keel_corot r);
static inline i32  keel_corot_code   (keel_corot r);
static inline i32  keel_corot_tag    (keel_corot r);
/* Produtores são verbos: ajustam o objeto e devolvem a cópia ajustada.
   Sair da função é `return corot.win(r);` escrito no fonte. */
static inline keel_corot keel_corot_win  (keel_corot *r);
static inline keel_corot keel_corot_again(keel_corot *r);
static inline keel_corot keel_corot_fault(keel_corot *r, i32 c);
#endif /* KEEL_KEEL_COROT_H */
