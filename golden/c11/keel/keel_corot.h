/* keel/keel_corot.h — generated from keel/corot.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_COROT_H
#define KEEL_KEEL_COROT_H
#include "keel/keel_corot.type.h"

#line 20 "keel/corot.k"
static inline bool keel_corot_ok     (keel_corot r);
static inline bool keel_corot_ongoing(keel_corot r);
static inline bool keel_corot_faulted(keel_corot r);
static inline i32  keel_corot_code   (keel_corot r);
#line 27 "keel/corot.k"
static inline keel_corot_Status keel_corot_tag (keel_corot r);
#line 37 "keel/corot.k"
static inline keel_corot keel_corot_win  (keel_corot *r);
static inline keel_corot keel_corot_again(keel_corot *r);
static inline keel_corot keel_corot_fault(keel_corot *r, i32 c);

#line 20 "keel/corot.k"
static inline bool keel_corot_ok     (keel_corot r) { return r.code <  0; }
static inline bool keel_corot_ongoing(keel_corot r) { return r.code == 0; }
static inline bool keel_corot_faulted(keel_corot r) { return r.code >  0; }
static inline i32  keel_corot_code   (keel_corot r) { return r.code; }
#line 27 "keel/corot.k"
static inline keel_corot_Status keel_corot_tag (keel_corot r) {
    return r.code < 0 ? keel_corot_Status_SUCCESS
         : r.code > 0 ? keel_corot_Status_FAILED
                      : keel_corot_Status_ONGOING;
}
#line 37 "keel/corot.k"
static inline keel_corot keel_corot_win  (keel_corot *r) { r->code = -1; return *r; }
static inline keel_corot keel_corot_again(keel_corot *r) { r->code =  0; return *r; }
static inline keel_corot keel_corot_fault(keel_corot *r, i32 c) { r->code = c; return *r; }
#endif /* KEEL_KEEL_COROT_H */
