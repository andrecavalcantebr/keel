/* gen/lst/lst.impl.h — gerado de lst/lst.k, perfil C23.
   `pub inline` sai `static inline` no header, com corpo (backend §4.1). */
#ifndef LST_LST_IMPL_H
#define LST_LST_IMPL_H
#include "lst/lst.h"

static inline lst_cursor lst_begin(lst_Lista *l) { lst_cursor c = { l->cabeca }; return c; }
static inline bool lst_has_next(lst_Lista *l, lst_cursor *c) { (void)l; return c->atual != NULL; }
static inline i32 *lst_next(lst_Lista *l, lst_cursor *c) { (void)l; i32 *p = &c->atual->v; c->atual = c->atual->prox; return p; }
#endif /* LST_LST_IMPL_H */
