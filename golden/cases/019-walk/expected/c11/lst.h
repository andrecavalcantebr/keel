/* lst.h — gerado de lst.k pelo cgen, perfil C11. */
#ifndef LST_H
#define LST_H
#include "lst.type.h"

typedef struct keel_buffer_i32 keel_buffer_i32;
#line 12 "lst.k"
static inline lst_cursor lst_begin(lst_List *l);
#line 13 "lst.k"
static inline bool lst_has_next(lst_List *l, lst_cursor *c);
static inline i32 *lst_next(lst_List *l, lst_cursor *c);
#line 16 "lst.k"
i32 lst_sum(lst_List *l);
#line 26 "lst.k"
i32 lst_sum_buffer(keel_buffer_i32 *xs);
#line 35 "lst.k"
i32 lst_first_even(lst_List *l);

#line 12 "lst.k"
static inline lst_cursor lst_begin(lst_List *l) { lst_cursor c = { l->head }; return c; }
#line 13 "lst.k"
static inline bool lst_has_next(lst_List *l, lst_cursor *c) { (void)l; return c->current != NULL; }
static inline i32 *lst_next(lst_List *l, lst_cursor *c) { (void)l; i32 *p = &c->current->v; c->current = c->current->link; return p; }
#endif
