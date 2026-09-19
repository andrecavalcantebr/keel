/* lst.h — gerado de lst.k pelo cgen, perfil C11. */
#ifndef LST_H
#define LST_H
#include "lst.proto.h"

static inline lst_cursor lst_begin(lst_Lista *l) { lst_cursor c = { l->cabeca }; return c; }
static inline bool lst_has_next(lst_Lista *l, lst_cursor *c) { (void)l; return c->atual != NULL; }
static inline i32 *lst_next(lst_Lista *l, lst_cursor *c) { (void)l; i32 *p = &c->atual->v; c->atual = c->atual->prox; return p; }
#endif /* LST_H */
