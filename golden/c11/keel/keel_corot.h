/* keel/keel_corot.h — generated from keel/corot.k by cgen, C11 profile. */
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

static inline bool keel_corot_ok     (keel_corot r) { return r.code <  0; }
static inline bool keel_corot_ongoing(keel_corot r) { return r.code == 0; }
static inline bool keel_corot_faulted(keel_corot r) { return r.code >  0; }
static inline i32  keel_corot_code   (keel_corot r) { return r.code; }
static inline i32  keel_corot_tag    (keel_corot r) {
    return r.code < 0 ? (i32)keel_corot_Status_SUCCESS
         : r.code > 0 ? (i32)keel_corot_Status_FAILED
                      : (i32)keel_corot_Status_ONGOING;
}
/* Produtores são verbos: ajustam o objeto e devolvem a cópia ajustada.
   Sair da função é `return corot.win(r);` escrito no fonte. */
static inline keel_corot keel_corot_win  (keel_corot *r) { r->code = -1; return *r; }
static inline keel_corot keel_corot_again(keel_corot *r) { r->code =  0; return *r; }
static inline keel_corot keel_corot_fault(keel_corot *r, i32 c) { r->code = c; return *r; }
#endif /* KEEL_KEEL_COROT_H */
