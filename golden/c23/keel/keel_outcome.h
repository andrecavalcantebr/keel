/* keel/keel_outcome.h — perfil C23. O `.h` do módulo, uma vez: as duas constantes
   não mencionam parâmetro nem modificador (keel-c-backend.md §4.4.1). */
#ifndef KEEL_KEEL_OUTCOME_H
#define KEEL_KEEL_OUTCOME_H
#include "keel/keel_outcome.type.h"

constexpr i32 keel_outcome_OK   = 0;
constexpr i32 keel_outcome_NONE = (-2147483647 - 1);
#endif /* KEEL_KEEL_OUTCOME_H */
