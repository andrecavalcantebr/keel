/* lst.type.h — gerado de lst.k pelo cgen, perfil C23. */
#ifndef LST_TYPE_H
#define LST_TYPE_H
#include "keel.type.h"

#line 8 "lst.k"
typedef struct lst_No { i32 v; struct lst_No *prox; } lst_No;
#line 9 "lst.k"
typedef struct lst_Lista { lst_No *cabeca; } lst_Lista;
#line 10 "lst.k"
typedef struct lst_cursor { lst_No *atual; } lst_cursor;
#endif
