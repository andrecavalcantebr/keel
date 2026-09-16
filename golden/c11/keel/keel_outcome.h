/* keel/keel_outcome.h — perfil C11. O `.h` do módulo, uma vez: as duas constantes
   não mencionam parâmetro nem modificador (keel-c-backend.md §4.4.1). */
#ifndef KEEL_KEEL_OUTCOME_H
#define KEEL_KEEL_OUTCOME_H
#include "keel/keel_outcome.type.h"

#define keel_outcome_OK ((i32)0)
static const i32 keel_outcome_OK__chk = 0;
#define keel_outcome_NONE ((i32)(-2147483647 - 1))
static const i32 keel_outcome_NONE__chk = (-2147483647 - 1);
#endif /* KEEL_KEEL_OUTCOME_H */
