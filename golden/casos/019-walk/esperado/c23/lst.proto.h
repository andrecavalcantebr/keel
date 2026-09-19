/* lst.proto.h — gerado de lst.k pelo cgen, perfil C23. */
#ifndef LST_PROTO_H
#define LST_PROTO_H
#include "lst.type.h"

#line 12 "lst/lst.k"
static inline lst_cursor lst_begin(lst_Lista *l);
#line 13 "lst/lst.k"
static inline bool lst_has_next(lst_Lista *l, lst_cursor *c);
#line 14 "lst/lst.k"
static inline i32 *lst_next(lst_Lista *l, lst_cursor *c);
i32 lst_soma(lst_Lista *l);
i32 lst_soma_buffer(keel_buffer_i32 *xs);
i32 lst_primeiro_par(lst_Lista *l);
#endif /* LST_PROTO_H */
