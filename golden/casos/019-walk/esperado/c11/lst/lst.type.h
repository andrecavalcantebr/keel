/* gen/lst/lst.type.h — gerado de lst/lst.k, perfil C11.
   `pub inline` sai `static inline` no header, com corpo (backend §4.1). */
#ifndef LST_LST_TYPE_H
#define LST_LST_TYPE_H
#include "keel/prelude.h"
#include "keel/keel_buffer_i32.type.h"

typedef struct lst_No { i32 v; struct lst_No *prox; } lst_No;
typedef struct { lst_No *cabeca; } lst_Lista;
typedef struct { lst_No *atual; } lst_cursor;
#endif /* LST_LST_TYPE_H */
