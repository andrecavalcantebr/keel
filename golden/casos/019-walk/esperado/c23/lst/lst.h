/* gen/lst/lst.h — gerado de lst/lst.k, perfil C23.
   `pub inline` sai `static inline` no header, com corpo (backend §4.1). */
#ifndef LST_LST_H
#define LST_LST_H
#include "keel/prelude.h"
#include "keel/keel_buffer_i32.h"

typedef struct lst_No { i32 v; struct lst_No *prox; } lst_No;
typedef struct { lst_No *cabeca; } lst_Lista;
typedef struct { lst_No *atual; } lst_cursor;

#line 12 "lst/lst.k"
static inline lst_cursor lst_begin(lst_Lista *l) { lst_cursor c = { l->cabeca }; return c; }
#line 13 "lst/lst.k"
static inline bool lst_has_next(lst_Lista *l, lst_cursor *c) { (void)l; return c->atual != NULL; }
#line 14 "lst/lst.k"
static inline i32 *lst_next(lst_Lista *l, lst_cursor *c) { (void)l; i32 *p = &c->atual->v; c->atual = c->atual->prox; return p; }

i32 lst_soma(lst_Lista *l);
i32 lst_soma_buffer(keel_buffer_i32 *xs);
i32 lst_primeiro_par(lst_Lista *l);
#endif
