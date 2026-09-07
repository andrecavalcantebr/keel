/* keel/par.h — incluído quando o módulo usa `parallel`.
   ACHADO: `keel_parstatus` é usado em keel-c-backend.md §5.9 e não é definido
   em lugar nenhum — nem entre os headers fixos do §4.2, nem no gerado.
   Materializado aqui; precisa entrar no backend. */
#ifndef KEEL_PAR_H
#define KEEL_PAR_H
#include "keel/prelude.h"

/* SUCCESS vale zero: é o que faz `keel__stN[k] = {0}` nascer coerente com
   faixa vazia, que termina em sucesso sem executar verbo nenhum. */
typedef enum keel_parstatus {
    keel_parstatus_SUCCESS     = 0,
    keel_parstatus_FAILURE     = 1,
    keel_parstatus_INTERRUPTED = 2
} keel_parstatus;

static inline bool keel_par_ok(const keel_parstatus *st, size_t k) {
    for (size_t w = 0; w < k; w++) if (st[w] != keel_parstatus_SUCCESS) return false;
    return true;
}
static inline bool keel_par_failed(const keel_parstatus *st, size_t k) {
    for (size_t w = 0; w < k; w++) if (st[w] == keel_parstatus_FAILURE) return true;
    return false;
}
static inline bool keel_par_interrupted(const keel_parstatus *st, size_t k) {
    for (size_t w = 0; w < k; w++) if (st[w] == keel_parstatus_INTERRUPTED) return true;
    return false;
}
#endif
