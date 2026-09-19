/* lst.h — gerado de lst.k pelo cgen, perfil C23. */
#ifndef LST_H
#define LST_H
#include "lst.type.h"

typedef struct keel_buffer_i32 keel_buffer_i32;
#line 12 "lst.k"
static inline lst_cursor lst_begin(lst_Lista *l);
#line 13 "lst.k"
static inline bool lst_has_next(lst_Lista *l, lst_cursor *c);
static inline i32 *lst_next(lst_Lista *l, lst_cursor *c);
#line 16 "lst.k"
i32 lst_soma(lst_Lista *l);
#line 26 "lst.k"
i32 lst_soma_buffer(keel_buffer_i32 *xs);
#line 35 "lst.k"
i32 lst_primeiro_par(lst_Lista *l);

#line 12 "lst.k"
static inline lst_cursor lst_begin(lst_Lista *l) { lst_cursor c = { l->cabeca }; return c; }
#line 13 "lst.k"
static inline bool lst_has_next(lst_Lista *l, lst_cursor *c) { (void)l; return c->atual != NULL; }
static inline i32 *lst_next(lst_Lista *l, lst_cursor *c) { (void)l; i32 *p = &c->atual->v; c->atual = c->atual->prox; return p; }
#endif
