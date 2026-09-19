/* keel/keel_corot.type.h — gerado de keel/corot.k pelo cgen, perfil C11. */
#ifndef KEEL_KEEL_COROT_TYPE_H
#define KEEL_KEEL_COROT_TYPE_H
#include "keel.type.h"

typedef struct keel_corot { i32 code; } keel_corot;
/* O conjunto nomeia as três regiões do código; `tag` é leitura, não
   armazenamento (linguagem §5.5). */
typedef enum keel_corot_Status {
    keel_corot_Status_SUCCESS = -1,
    keel_corot_Status_ONGOING =  0,
    keel_corot_Status_FAILED  =  1
} keel_corot_Status;
#endif /* KEEL_KEEL_COROT_TYPE_H */
