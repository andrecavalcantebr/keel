/* keel/corot.h — o tipo `corot` e seu conjunto de tags (backend §5.14).
   Tipo verdadeiro, não modificador: um campo, e três estados por sinal do
   código — <0 SUCCESS, 0 ONGOING, >0 FAILED. Sai uma vez no módulo, sem
   header de instância: não há valor associado a parametrizar. */
#ifndef KEEL_COROT_H
#define KEEL_COROT_H
#include "keel/prelude.h"

typedef struct keel_corot { i32 code; } keel_corot;

/* O conjunto nomeia as três regiões do código; `tag` é leitura, não
   armazenamento (linguagem §5.5). */
typedef enum keel_corot_Status {
    keel_corot_Status_SUCCESS = -1,
    keel_corot_Status_ONGOING =  0,
    keel_corot_Status_FAILED  =  1
} keel_corot_Status;

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
#endif
